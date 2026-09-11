#pragma once
#include <Arduino.h>

// Atualização de firmware over-the-air (OTA) via manifesto JSON + Update.
// Mesmo fluxo do Cofre FIAP V2: manifesto {downloadUrl, sha256, size, version},
// download com X-Device-Token, verificação SHA-256 e reinício após gravar.
// Execução síncrona (o loop principal fica bloqueado durante o download).
bool isOtaUpdateInProgress();
void startOtaUpdate(const String& manifestUrl, uint64_t serverTimeMs, const String& targetVersion);
