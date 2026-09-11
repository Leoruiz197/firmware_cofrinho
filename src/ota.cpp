#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <mbedtls/sha256.h>
#include <time.h>
#include <sys/time.h>
#include "config.h"
#include "ota.h"
#include "state.h"
#include "websocket_service.h"
#include "hardware.h"

// Mesmo fluxo OTA do Cofre FIAP V2, adaptado ao cofrinho (loop único, sem
// tarefa FreeRTOS dedicada): a atualização é síncrona e bloqueia o loop
// durante o download. Progresso vai por publishOtaStatus (evento "ota").
static volatile bool otaInProgress = false;

bool isOtaUpdateInProgress() {
  return otaInProgress;
}

// Mesma cadeia de CAs do V2 (raízes Google Trust Services).
static const char OTA_ROOT_CA[] = R"pem(-----BEGIN CERTIFICATE-----
MIICjjCCAjOgAwIBAgIQf/NXaJvCTjAtkOGKQb0OHzAKBggqhkjOPQQDAjBQMSQw
IgYDVQQLExtHbG9iYWxTaWduIEVDQyBSb290IENBIC0gUjQxEzARBgNVBAoTCkds
b2JhbFNpZ24xEzARBgNVBAMTCkdsb2JhbFNpZ24wHhcNMjMxMjEzMDkwMDAwWhcN
MjkwMjIwMTQwMDAwWjA7MQswCQYDVQQGEwJVUzEeMBwGA1UEChMVR29vZ2xlIFRy
dXN0IFNlcnZpY2VzMQwwCgYDVQQDEwNXRTEwWTATBgcqhkjOPQIBBggqhkjOPQMB
BwNCAARvzTr+Z1dHTCEDhUDCR127WEcPQMFcF4XGGTfn1XzthkubgdnXGhOlCgP4
mMTG6J7/EFmPLCaY9eYmJbsPAvpWo4IBAjCB/zAOBgNVHQ8BAf8EBAMCAYYwHQYD
VR0lBBYwFAYIKwBBQUHAwEGCCsGAQUFBwMCMBIGA1UdEwEB/wQIMAYBAf8CAQAw
HQYDVR0OBBYEFJB3kjVnxP+ozKnme9mAeXvMk/k4MB8GA1UdIwQYMBaAFFSwe61F
uOJAf/sKbvu+M8k8o4TVMDYGCCsGAQUFBwEBBCowKDAmBggrBgEFBQcwAoYaaHR0
cDovL2kucGtpLmdvb2cvZ3NyNC5jcnQwLQYDVR0fBCYwJDAioCCgHoYcaHR0cDov
L2MucGtpLmdvb2cvci9nc3I0LmNybDATBgNVHSAEDDAKMAgGBmeBDAECATAKBggq
hkjOPQQDAgNJADBGAiEAokJL0LgR6SOLR02WWxccAq3ndXp4EMRveXMUVUxMWSMC
IQDspFWa3fj7nLgouSdkcPy1SdOR2AGm9OQWs7veyXsBwA==
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIB3DCCAYOgAwIBAgINAgPlfvU/k/2lCSGypjAKBggqhkjOPQQDAjBQMSQwIgYD
VQQLExtHbG9iYWxTaWduIEVDQyBSb290IENBIC0gUjQxEzARBgNVBAoTCkdsb2Jh
bFNpZ24xEzARBgNVBAMTCkdsb2JhbFNpZ24wHhcNMTIxMTEzMDAwMDAwWhcNMzgw
MTE5MDMxNDA3WjBQMSQwIgYDVQQLExtHbG9iYWxTaWduIEVDQyBSb290IENBIC0g
UjQxEzARBgNVBAoTCkdsb2JhbFNpZ24xEzARBgNVBAMTCkdsb2JhbFNpZ24wWTAT
BgcqhkjOPQIBBggqhkjOPQMBBwNCAAS4xnnTj2wlDp8uORkcA6SumuU5BwkWymOx
uYb4ilfBV85C+nOh92VC/x7BALJucw7/xyHlKGSq2XE/qNS5zowdo0IwQDAOBgNV
HQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUVLB7rUW44kB/
+wpu+74zyTyjhNUwCgYIKoZIzj0EAwIDRwAwRAIgIk90crlgr/HmnKAWBVBfw147
bmF0774BxL4YSFlhgjICICadVGNA3jdgUM/I2O2dgq43mLyjj0xMqTQrbO/7lZsm
-----END CERTIFICATE-----
)pem";

static bool syncClock() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  time_t now = time(nullptr);

  for (int i = 0; now < 1700000000 && i < 20; i++) {
    delay(500);
    now = time(nullptr);
  }

  return now >= 1700000000;
}

static bool isLocalOtaUrl(const String& url) {
  return url.startsWith("http://localhost") ||
    url.startsWith("http://192.168.") ||
    url.startsWith("http://10.") ||
    url.startsWith("http://172.16.") ||
    url.startsWith("http://172.17.") ||
    url.startsWith("http://172.18.") ||
    url.startsWith("http://172.19.") ||
    url.startsWith("http://172.2") ||
    url.startsWith("http://172.30.") ||
    url.startsWith("http://172.31.");
}

static void otaFail(int progress, const String& targetVersion, const String& message) {
  publishOtaStatus("error", progress, targetVersion, message);
  otaLedResult(false);
}

static void performOtaUpdate(const String& manifestUrl, uint64_t serverTimeMs, String targetVersion) {
  Serial.println("[OTA] Iniciando atualização");
  if (!manifestUrl.startsWith("https://") && !isLocalOtaUrl(manifestUrl)) {
    Serial.println("[OTA] URL OTA pública precisa usar HTTPS");
    publishOtaStatus("error", 0, targetVersion, "URL OTA não segura");
    return;
  }
  publishOtaStatus("downloading", 0, targetVersion, "Buscando manifesto");

  // Sinaliza OTA em laranja (padrão do V2).
  otaLedProgress();
  if (serverTimeMs >= 1700000000000ULL) {
    timeval timeValue = { static_cast<time_t>(serverTimeMs / 1000), 0 };
    settimeofday(&timeValue, nullptr);
  }

  if (manifestUrl.startsWith("https://") && time(nullptr) < 1700000000 && !syncClock()) {
    Serial.println("[OTA] NTP indisponível; atualização cancelada");
    otaFail(0, targetVersion, "Não foi possível sincronizar o relógio");
    return;
  }

  WiFiClient manifestPlainClient;
  WiFiClientSecure manifestSecureClient;
  WiFiClient* manifestClient = &manifestPlainClient;
  if (manifestUrl.startsWith("https://")) {
    manifestSecureClient.setCACert(OTA_ROOT_CA);
    manifestClient = &manifestSecureClient;
  }
  HTTPClient manifestHttp;
  manifestHttp.begin(*manifestClient, manifestUrl);
  manifestHttp.addHeader("X-Device-Token", deviceConfig.deviceToken);
  const int manifestStatus = manifestHttp.GET();
  if (manifestStatus != HTTP_CODE_OK) {
    Serial.printf("[OTA] Manifesto falhou: %d %s\n", manifestStatus, manifestHttp.errorToString(manifestStatus).c_str());
    manifestHttp.end();
    otaFail(0, targetVersion, "Falha ao baixar o manifesto");
    return;
  }
  JsonDocument manifest;
  if (deserializeJson(manifest, manifestHttp.getString())) {
    Serial.println("[OTA] Manifesto JSON inválido");
    manifestHttp.end();
    otaFail(0, targetVersion, "Manifesto inválido");
    return;
  }
  manifestHttp.end();
  manifestClient->stop();

  const String url = manifest["downloadUrl"] | "";
  const String expectedHash = manifest["sha256"] | "";
  const int expectedSize = manifest["size"] | 0;
  const String manifestVersion = manifest["version"] | "";
  if (manifestVersion.length() > 0) targetVersion = manifestVersion;
  if (url.length() == 0 || expectedHash.length() != 64 || expectedSize <= 0) {
    Serial.println("[OTA] Manifesto incompleto");
    otaFail(0, targetVersion, "Manifesto incompleto");
    return;
  }

  WiFiClient downloadPlainClient;
  WiFiClientSecure downloadSecureClient;
  WiFiClient* downloadClient = &downloadPlainClient;
  if (url.startsWith("https://")) {
    downloadSecureClient.setCACert(OTA_ROOT_CA);
    downloadClient = &downloadSecureClient;
  }
  HTTPClient downloadHttp;
  downloadHttp.begin(*downloadClient, url);
  downloadHttp.addHeader("X-Device-Token", deviceConfig.deviceToken);
  const int downloadStatus = downloadHttp.GET();
  if (downloadStatus != HTTP_CODE_OK) {
    Serial.printf("[OTA] Download falhou: %d %s\n", downloadStatus, downloadHttp.errorToString(downloadStatus).c_str());
    downloadHttp.end();
    otaFail(0, targetVersion, "Falha ao baixar o firmware");
    return;
  }
  if (!Update.begin(expectedSize)) {
    Serial.printf("[OTA] Sem espaço OTA: %s\n", Update.errorString());
    downloadHttp.end();
    otaFail(0, targetVersion, "Sem espaço para gravar o firmware");
    return;
  }

  mbedtls_sha256_context hash;
  mbedtls_sha256_init(&hash);
  mbedtls_sha256_starts_ret(&hash, 0);
  uint8_t buffer[1024];
  int total = 0;
  int lastReportedProgress = 0;
  WiFiClient* stream = downloadHttp.getStreamPtr();
  while (total < expectedSize) {
    const int available = stream->available();
    if (available <= 0) {
      if (!downloadHttp.connected()) break;
      delay(1);
      continue;
    }
    const size_t read = stream->readBytes(buffer, min(min((int)sizeof(buffer), expectedSize - total), available));
    if (!read) continue;
    if (Update.write(buffer, read) != read) { Update.abort(); break; }
    mbedtls_sha256_update_ret(&hash, buffer, read);
    total += read;
    const int progress = (total * 100LL) / expectedSize;
    if (progress >= lastReportedProgress + 5 || progress == 100) {
      lastReportedProgress = progress;
      publishOtaStatus("downloading", progress, targetVersion, "Baixando e gravando firmware");
    }
  }
  uint8_t digest[32];
  mbedtls_sha256_finish_ret(&hash, digest);
  mbedtls_sha256_free(&hash);
  downloadHttp.end();
  char actualHash[65];
  for (int i = 0; i < 32; i++) sprintf(actualHash + i * 2, "%02x", digest[i]);
  actualHash[64] = '\0';
  if (total != expectedSize || expectedHash != actualHash) {
    Serial.printf("[OTA] Integridade inválida: recebido %d de %d bytes\n", total, expectedSize);
    Serial.printf("[OTA] SHA esperado: %s\n", expectedHash.c_str());
    Serial.printf("[OTA] SHA recebido: %s\n", actualHash);
    Update.abort();
    otaFail(lastReportedProgress, targetVersion, "Falha na integridade do firmware");
    return;
  }
  if (!Update.end(true)) {
    Serial.printf("[OTA] Gravação falhou: %s\n", Update.errorString());
    otaFail(lastReportedProgress, targetVersion, "Falha ao finalizar a gravação");
    return;
  }
  Serial.println("[OTA] Firmware validado; reiniciando");
  publishOtaStatus("restarting", 100, targetVersion, "Firmware gravado; reiniciando para validar");
  otaLedResult(true);
  delay(500);
  ESP.restart();
}

void startOtaUpdate(const String& manifestUrl, uint64_t serverTimeMs, const String& targetVersion) {
  if (otaInProgress) {
    publishOtaStatus("error", 0, targetVersion, "Já existe uma atualização em andamento");
    return;
  }
  otaInProgress = true;
  performOtaUpdate(manifestUrl, serverTimeMs, targetVersion);
  otaInProgress = false;
}
