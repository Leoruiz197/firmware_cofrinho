#pragma once

#include <Arduino.h>

void initializeWebSocket();
void runWebSocket();
void publishStatus(const char* event, const char* result, const String& detail = "");
void publishOtaStatus(const char* status, int progress, const String& targetVersion, const String& message);
