#include <ArduinoJson.h>
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

String urlEncode(const char* value) {
  const char hex[] = "0123456789ABCDEF";
  String encoded;
  while (*value) {
    const uint8_t character = static_cast<uint8_t>(*value++);
    if ((character >= 'a' && character <= 'z') ||
        (character >= 'A' && character <= 'Z') ||
        (character >= '0' && character <= '9') ||
        character == '-' || character == '_' || character == '.' || character == '~') {
      encoded += static_cast<char>(character);
    } else {
      encoded += '%';
      encoded += hex[character >> 4];
      encoded += hex[character & 0x0F];
    }
  }
  return encoded;
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
    if (!document["command"].is<const char*>() || !document["payload"].is<JsonObjectConst>()) {
      Serial.println("[WS] Erro ao processar comando: envelope invalido.");
      publishStatus("command", "error", "invalid_envelope");
      return;
    }

    JsonDocument commandDocument;
    for (JsonPairConst field : document["payload"].as<JsonObjectConst>()) {
      commandDocument[field.key()] = field.value();
    }
    commandDocument["comando"] = document["command"];
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
      Serial.printf("[WS] Conectado a ws://%s:%u/ws/cofres\n", deviceConfig.backendHost, BACKEND_PORT);
      publishStatus("connection", "connected");
      break;
    case WStype_DISCONNECTED:
      Serial.println("[WS] Desconectado. Reconexao automatica aguardando.");
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
  if (detail.length() > 0) {
    status["detail"] = detail;
  }

  String payload;
  serializeJson(document, payload);
  if (!webSocket.sendTXT(payload)) {
    Serial.println("[WS] Erro ao enviar status.");
  }
}

void initializeWebSocket() {
  if (deviceConfig.backendHost[0] == '\0' || deviceConfig.deviceToken[0] == '\0' || deviceConfig.deviceId[0] == '\0') {
    Serial.println("[WS] Backend host/IP, token ou identificador do cofrinho nao configurado.");
    return;
  }

  const String path = "/ws/cofres?deviceId=" + urlEncode(deviceConfig.deviceId) +
      "&token=" + urlEncode(deviceConfig.deviceToken);
  Serial.printf("[WS] Conectando a ws://%s:%u%s\n", deviceConfig.backendHost, BACKEND_PORT, path.c_str());
  webSocket.begin(deviceConfig.backendHost, BACKEND_PORT, path.c_str());
  webSocket.onEvent(onWebSocketEvent);
  webSocket.setReconnectInterval(WEBSOCKET_RECONNECT_INTERVAL_MS);
}

void runWebSocket() {
  webSocket.loop();
}
