#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebSocketsClient.h>

#include "command_handler.h"
#include "config.h"
#include "state.h"
#include "websocket_service.h"

namespace {
WebSocketsClient webSocket;

String payloadToString(const uint8_t* payload, size_t length) {
  String message;
  message.reserve(length);
  for (size_t index = 0; index < length; ++index) {
    message += static_cast<char>(payload[index]);
  }
  return message;
}

void handleWebSocketMessage(const String& message) {
  Serial.printf("[WS] Mensagem recebida: %s\n", message.c_str());

  JsonDocument document;
  const DeserializationError error = deserializeJson(document, message);
  if (error) {
    Serial.printf("[WS] Erro de parse: %s\n", error.c_str());
    return;
  }

  const char* type = document["type"] | "";
  if (strcmp(type, "command") == 0) {
    if (document["command"].isNull()) {
      Serial.println("[WS] Erro ao processar comando: envelope invalido.");
      publishStatus("command", "error", "invalid_envelope");
      return;
    }
    // Comando-objeto (ex: {tentando, etapa, cor}) pode vir sem payload.
    if (document["command"].is<JsonObjectConst>() && !document["payload"].is<JsonObjectConst>()) {
      JsonDocument commandDocument;
      commandDocument["comando"].set(document["command"]);
      String commandPayload;
      serializeJson(commandDocument, commandPayload);
      handleCommand(commandPayload);
      return;
    }
    if (!document["payload"].is<JsonObjectConst>()) {
      Serial.println("[WS] Erro ao processar comando: envelope invalido.");
      publishStatus("command", "error", "invalid_envelope");
      return;
    }

    JsonDocument commandDocument;
    for (JsonPairConst field : document["payload"].as<JsonObjectConst>()) {
      commandDocument[field.key()] = field.value();
    }
    commandDocument["comando"].set(document["command"]);
    String commandPayload;
    serializeJson(commandDocument, commandPayload);
    handleCommand(commandPayload);
    return;
  }

  if (strcmp(type, "config") == 0) {
    JsonObjectConst config = document["config"].as<JsonObjectConst>();
    if (config.isNull() || !config["stages"].is<int>()) {
      Serial.println("[WS] Erro ao processar configuracao: envelope invalido.");
      publishStatus("configuration", "error", "invalid_envelope");
      return;
    }

    JsonDocument configurationDocument;
    configurationDocument["num_senhas"] = config["stages"];
    if (config["doorOpenAngle"].is<int>()) {
      configurationDocument["angulo-min"] = config["doorOpenAngle"];
    }
    if (config["doorCloseAngle"].is<int>()) {
      configurationDocument["angulo-max"] = config["doorCloseAngle"];
    }
    if (config["teamColor"].is<JsonObjectConst>()) {
      JsonObject color = configurationDocument["cor_equipe"]["cor"].to<JsonObject>();
      JsonObjectConst teamColor = config["teamColor"].as<JsonObjectConst>();
      color["R"] = teamColor["R"] | 255;
      color["G"] = teamColor["G"] | 255;
      color["B"] = teamColor["B"] | 255;
    }
    String configurationPayload;
    serializeJson(configurationDocument, configurationPayload);
    handleConfiguration(configurationPayload);
    if (config["reset"] | false) {
      resetStages();
    }
    return;
  }

  if (strcmp(type, "connected") != 0) {
    Serial.printf("[WS] Erro ao processar mensagem: tipo desconhecido (%s).\n", type);
  }
}

void onWebSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.printf("[WS] Conectado a %s://%s:%u/ws/cofres\n", deviceConfig.backendPort == 443 ? "wss" : "ws", deviceConfig.backendHost, deviceConfig.backendPort);
      publishStatus("connection", "connected");
      break;
    case WStype_DISCONNECTED:
      Serial.printf("[WS] Desconectado: %s. Reconexao automatica aguardando.\n", payloadToString(payload, length).c_str());
      break;
    case WStype_TEXT:
      handleWebSocketMessage(payloadToString(payload, length));
      break;
    case WStype_ERROR:
      Serial.printf("[WS] Erro: %s\n", payloadToString(payload, length).c_str());
      break;
    default:
      break;
  }
}
}

void publishStatus(const char* event, const char* result, const String& detail) {
  if (!webSocket.isConnected()) {
    return;
  }

  JsonDocument document;
  JsonObject status = document["status"].to<JsonObject>();
  document["type"] = "status";
  status["event"] = event;
  status["result"] = result;
  status["firmwareVersion"] = FIRMWARE_VERSION;
  if (detail.length() > 0) {
    status["detail"] = detail;
  }

  String payload;
  serializeJson(document, payload);
  if (!webSocket.sendTXT(payload)) {
    Serial.println("[WS] Erro ao enviar status.");
  }
}

void publishOtaStatus(const char* status, int progress, const String& targetVersion, const String& message) {
  if (!webSocket.isConnected()) {
    return;
  }

  JsonDocument document;
  JsonObject statusObj = document["status"].to<JsonObject>();
  document["type"] = "status";
  statusObj["event"] = "ota";
  statusObj["result"] = status;
  statusObj["progress"] = constrain(progress, 0, 100);
  statusObj["targetVersion"] = targetVersion;
  statusObj["firmwareVersion"] = FIRMWARE_VERSION;
  statusObj["detail"] = message;

  String payload;
  serializeJson(document, payload);
  if (!webSocket.sendTXT(payload)) {
    Serial.println("[WS] Erro ao enviar status OTA.");
  }
}

void initializeWebSocket() {
  if (deviceConfig.backendHost[0] == '\0' || deviceConfig.deviceToken[0] == '\0' || deviceConfig.deviceId[0] == '\0') {
    Serial.println("[WS] Backend host/IP, token ou identificador do cofrinho nao configurado.");
    return;
  }

  String instanceId = WiFi.macAddress();
  instanceId.replace(":", "");
  const String path = "/ws/cofres/" + String(deviceConfig.deviceId) + "/" + instanceId;
  Serial.printf("[WS] Conectando a %s://%s:%u%s\n", deviceConfig.backendPort == 443 ? "wss" : "ws", deviceConfig.backendHost, deviceConfig.backendPort, path.c_str());
  if (deviceConfig.backendPort == 443) {
    webSocket.beginSSL(deviceConfig.backendHost, deviceConfig.backendPort, path.c_str());
  } else {
    webSocket.begin(deviceConfig.backendHost, deviceConfig.backendPort, path.c_str());
  }
  // begin() limpa a autorizacao. Configurar depois, sem CRLF: a biblioteca
  // adiciona o cabecalho e as quebras HTTP na ordem correta.
  const String authorization = "Bearer " + String(deviceConfig.deviceToken);
  webSocket.setAuthorization(authorization.c_str());
  webSocket.onEvent(onWebSocketEvent);
  webSocket.setReconnectInterval(WEBSOCKET_RECONNECT_INTERVAL_MS);
  webSocket.enableHeartbeat(30000, 5000, 2);
}

void runWebSocket() {
  webSocket.loop();
}
