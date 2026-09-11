# Firmware Cofrinho (ESP32)

Firmware do cofrinho para ESP32, baseado no comportamento do Cofre FIAP V2, comunicando-se com o backend por WebSocket. Organizado para PlatformIO.

## Estrutura

- `src/main.cpp`: inicialização e loop principal.
- `src/settings.cpp`: Wi-Fi/backend via portal WiFiManager + persistência na flash (`Preferences`).
- `src/hardware.cpp`: servos, fita NeoPixel e efeitos (etapas, abertura/fechamento).
- `src/websocket_service.cpp`: conexão/reconexão WebSocket, heartbeat e tradução de envelopes.
- `src/command_handler.cpp`: validação e execução dos comandos (`abrir`, `fechar`, `luz`, `apagar`, `tranca_direita`, `tranca_esquerda`, `correta`, `erro`, objeto `{tentando, etapa, cor}`, `firmware_update`).
- `src/ota.cpp` + `include/ota.h`: atualização OTA via manifesto (mesmo fluxo do Cofre FIAP V2).
- `src/state.cpp` + `include/state.h`: `DeviceConfig` (host, porta, token, id, ângulos, cor da equipe).
- `include/config.h`: GPIOs, LEDs e defaults.

## Hardware

| Componente | GPIO | Observação |
| --- | --- | --- |
| Servo da tranca | 18 | Aberto em 30° (`LOCK_OPEN_ANGLE`), fechado em 90° (`LOCK_CLOSED_ANGLE`). |
| Servo da porta | 19 | Abertura `doorOpenAngle` (padrão 0°), fechamento `doorCloseAngle` (padrão 60°), configuráveis pelo gestor e salvos na flash. |
| Fita NeoPixel | 23 | 18 LEDs: 0–1 luz interna (`luz` alterna liga/desliga) e 2–17 progresso das etapas. |
| Botão BOOT | 0 | Segurar nos primeiros 3s (`WIFI_RESET_HOLD_MS`) apaga só rede/host/porta/token/id e reabre o portal. Ajustes mecânicos (ângulos, cor, nº de senhas) são preservados. |

LEDs de progresso: distribuídos proporcionalmente ao nº de etapas; cada etapa guarda a cor da última equipe que a acertou (`stageColors`); o acerto final (`correta`) não sobrescreve tudo — restaura por etapa. `apagar` limpa todos os LEDs.

## Configuração inicial (portal `Cofrinho_Config`)

Portal Wi-Fi **aberto** (sem senha). Valores iniciais (`include/config.h`):

```text
Backend host/IP: https://backend-cofrinho.onrender.com
Porta backend:   443
Device token:    cofrinho-device-token  (igual ao DEVICE_TOKEN do backend)
Identificador:   cofre01
```

- O firmware remove `http(s)://` e caminho do host automaticamente antes de conectar.
- **TLS é automático na porta 443** (`wss`); nas demais portas usa `ws` sem TLS.
- Para backend local, informe o **IP LAN do PC** (nunca `localhost`) e porta `3000`.
- DNS público (`1.1.1.1` e `8.8.8.8`) é aplicado antes e reaplicado depois da conexão, pois o DHCP de alguns hotspots sobrescreve o DNS.

Para o Render, os campos do portal devem ficar assim:

```text
Backend host/IP: https://backend-cofrinho.onrender.com
Porta backend: 443
Device token: <mesmo DEVICE_TOKEN do .env do backend>
Identificador do cofrinho: cofre01
```

## Protocolo WebSocket

```text
wss://<host>:443/ws/cofres/<id>/<mac-sem-dois-pontos>
ws://<ip-lan>:3000/ws/cofres/<id>/<mac-sem-dois-pontos>
```

O backend aceita IDs de `cofre01` a `cofre30`. A conexão bem-sucedida recebe:

```json
{"type":"connected","deviceId":"cofre01"}
```

O token **não** vai na URL: o firmware o envia no handshake como `Authorization: Bearer <device-token>` (protegido em trânsito com TLS). O MAC do ESP32 vai no caminho e identifica a instância: o backend mantém **uma conexão por cofre** e rejeita outro ESP32 com o mesmo identificador (código `1008`) em vez de derrubar o conectado. Heartbeat a cada 30s mantém a sessão; reconexão automática a cada `WEBSOCKET_RECONNECT_INTERVAL_MS` (5s).

## Mensagens recebidas

Comandos de texto (exigem `payload` objeto, pode ser `{}`):

```json
{"type":"command","command":"abrir","payload":{}}
{"type":"command","command":"erro","payload":{}}
{"type":"command","command":"correta","payload":{"cor":{"R":0,"G":255,"B":0}}}
```

Comando-objeto de etapa (aceito **com ou sem** `payload`; é assim que o backend informa quem acertou):

```json
{"type":"command","command":{"tentando":true,"etapa":1,"cor":{"R":255,"G":0,"B":0}}}
```

São convertidos para o formato legado `{ "comando": … }` e processados pelo `command_handler`. A cor da etapa acende os LEDs da tampa com a cor da equipe atacante.

Config (`stages` vira `num_senhas`, 1–16; `reset: true` reinicia o visual):

```json
{"type":"config","config":{"ownerTeamId":"equipe01","stages":3,"doorOpenAngle":0,"doorCloseAngle":60,"teamColor":{"R":255,"G":0,"B":0},"reset":true}}
```

## Retorno do cofre

Após comando/config, o ESP32 envia `status` (visível no detalhe do cofre no `/admin`):

```json
{"type":"status","status":{"event":"command","result":"ok","detail":"abrir"}}
{"type":"status","status":{"event":"command","result":"error","detail":"unknown_command"}}
{"type":"status","status":{"event":"configuration","result":"ok","detail":"saved"}}
```

## OTA (over-the-air)

Mesmo fluxo do Cofre FIAP V2, adaptado ao loop único do cofrinho (atualização
síncrona — o loop fica bloqueado durante o download):

1. O admin dispara no backend, que envia ao cofre:
```json
{"type":"command","command":"firmware_update","payload":{"manifestUrl":"https://.../api/firmware/manifest/1.1.0","targetVersion":"1.1.0","serverTime":1730000000000}}
```
2. O cofre baixa o manifesto `{version, downloadUrl, sha256, size}` com
   `X-Device-Token`, exige HTTPS (exceto LAN) e sincroniza o relógio via NTP.
3. Baixa o `.bin`, grava via `Update`, verifica o SHA-256 e reinicia.
4. LEDs: laranja durante o download, 3 piscadas verdes no sucesso (vermelhas
   em erro) e restaura as etapas. Progresso a cada 5%:
```json
{"type":"status","status":{"event":"ota","result":"downloading","progress":45,"targetVersion":"1.1.0","firmwareVersion":"1.0.0","detail":"Baixando e gravando firmware"}}
```
5. Versão atual em `FIRMWARE_VERSION` (`platformio.ini`, padrão `1.0.0`) e em
   todo `status` (`firmwareVersion`), visível no detalhe do cofre no `/admin`.
6. Requer partições OTA (`partitions_ota.csv`: `app0`/`app1` de ~1,9 MB) —
   **regrave com upload serial ao trocar o particionamento pela primeira vez**.

```text
[OTA] Iniciando atualização
[OTA] Firmware validado; reiniciando
```

## Serial Monitor (115200)

```
[WiFi] DNS ativo: 1.1.1.1 e 8.8.8.8
[WS] Conectando a wss://backend-cofrinho.onrender.com:443/ws/cofres/cofre01/B8D61AB32480
[WS] Conectado a wss://backend-cofrinho.onrender.com:443/ws/cofres
[WS] Mensagem recebida: {"type":"connected","deviceId":"cofre01"}
[WS] Mensagem recebida: {"type":"command","command":{"tentando":true,"etapa":1,...}}
[WS] Desconectado: <motivo>. Reconexao automatica aguardando.
```

O motivo da desconexão é exibido (ex.: `Replaced by reconnecting device`, `Device ID already in use`).

## Compilar e gravar (PlatformIO)

```powershell
cd firmware_cofrinho
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run                                        # compilar
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -t upload --upload-port COM4          # gravar
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" device monitor --port COM4 --baud 115200  # monitor
```

Dependências (`platformio.ini`, `env:esp32dev`): ArduinoJson, WebSockets (links2004), WiFiManager (tzapu), Adafruit NeoPixel, ESP32Servo.
