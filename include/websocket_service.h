#pragma once

#include <Arduino.h>

void initializeWebSocket();
void runWebSocket();
void publishStatus(const char* event, const char* result, const String& detail = "");
