#pragma once

constexpr bool DEBUG_ENABLED = true;

constexpr uint8_t LOCK_SERVO_PIN = 18;
constexpr uint8_t DOOR_SERVO_PIN = 19;
constexpr uint8_t LED_PIN = 23;
constexpr uint8_t BOOT_BUTTON_PIN = 0;
constexpr unsigned long WIFI_RESET_HOLD_MS = 3000;

constexpr uint8_t LED_COUNT = 18;
constexpr uint8_t INTERNAL_LED_START = 0;
constexpr uint8_t INTERNAL_LED_END = 1;
constexpr uint8_t PROGRESS_LED_START = 2;
constexpr uint8_t PROGRESS_LED_END = 17;

constexpr int DOOR_OPEN_ANGLE = 0;
constexpr int LOCK_OPEN_ANGLE = 30;
constexpr int LOCK_CLOSED_ANGLE = 90;
constexpr uint8_t MAX_STAGES = PROGRESS_LED_END - PROGRESS_LED_START + 1;

constexpr unsigned long WEBSOCKET_RECONNECT_INTERVAL_MS = 5000;
constexpr uint16_t BACKEND_PORT = 3000;
constexpr char DEFAULT_BACKEND_HOST[] = "";
constexpr char DEFAULT_DEVICE_TOKEN[] = "cofrinho-device-token";
constexpr char DEFAULT_DEVICE_ID[] = "cofre01";
