# Firmware Cofrinho (ESP32)

Firmware do cofrinho para ESP32, baseado no comportamento do Cofre FIAP V2, comunicando-se com o backend por WebSocket. Organizado para PlatformIO.

## Estrutura

- `src/main.cpp`: inicialização e loop principal.
- `src/settings.cpp`: Wi-Fi/backend via portal WiFiManager + persistência na flash (`Preferences`).
- `src/hardware.cpp`: servos, fita NeoPixel e efeitos (etapas, abertura/fechamento).
- `src/websocket_service.cpp`: conexão/reconexão WebSocket e tradução de envelopes.
- `src/command_handler.cpp`: validação e execução dos comandos (`abrir`, `fechar`, `luz`, `apagar`, `tranca_direita`, `tranca_esquerda`, `correta`, `erro`).
- `src/state.cpp` + `include/state.h`: `DeviceConfig` (host, porta, token, id, ângulos, cor da equipe).
- `include/config.h`: GPIOs, LEDs e defaults.

## Hardware

| Componente | GPIO | Observação |
| --- | --- | --- |
| Servo da tranca | 18 | Aberto em 30° (`LOCK_OPEN_ANGLE`), fechado em 90° (`LOCK_CLOSED_ANGLE`). |
| Servo da porta | 19 | Abertura `doorOpenAngle` (padrão 0°), fechamento `doorCloseAngle` (padrão 60°), configuráveis pelo gestor e salvos na flash. |
| Fita NeoPixel | 23 | 18 LEDs: 0–1 luz interna (`luz` alterna) e 2–17 progresso das etapas. |
| Botão BOOT | 0 | Segurar nos primeiros 3s (`WIFI_RESET_HOLD_MS`) apaga só rede/host/token/id e reabre o portal. |

LEDs de progresso: distribuídos proporcionalmente ao nº de etapas; cada etapa guarda a cor da última equipe que a acertou; o acerto final não sobrescreve tudo (restaura por etapa).

## Configuração inicial (portal `Cofrinho_Config`)

<<<<<<< HEAD
O host inicia preenchido como `https://backend-cofrinho.onrender.com`, o token
inicia como `cofrinho-device-token` e o identificador inicia como `cofre01`.
Para o Render, use a porta `443`; o firmware remove o `https://`
automaticamente antes de abrir a conexao e usa TLS nessa porta.
=======
Sem rede configurada, o ESP32 abre o portal Wi-Fi aberto `Cofrinho_Config` com campos separados:
>>>>>>> 0061d9280d5d7fc53cc2d63004cb959bdc004a27

- Rede Wi-Fi (SSID/senha)
- **Backend host/IP** (só host, sem `:porta` — o firmware remove se digitado)
- **Backend port** (padrão `3000`, salvo na flash)
- **Device token** (global, igual ao `DEVICE_TOKEN` do `backend_cofrinho/.env`)
- **Identificador do cofrinho** (`deviceId`: `cofre01`, `cofre02`, … — diferencia os cofres)

<<<<<<< HEAD
O backend deve estar acessivel pela rede local: no portal informe o IP LAN da
maquina que executa o backend, e nao `localhost` ou `127.0.0.1`. Em
`backend_cofrinho/.env`, configure `DEVICE_TOKEN` com o mesmo valor informado no
ESP32.

## Protocolo WebSocket

O dispositivo conecta por WebSocket. Localmente use `ws` na porta `3000`; no
Render use `wss` na porta `443`:
=======
Defaults (`include/config.h`): host vazio, porta `3000`, token `cofrinho-device-token`, id `cofre01`.

O backend deve estar acessível na LAN: informe o **IP LAN do PC** (nunca `localhost`). Confira `PORT=3000` no backend ou ajuste a porta no portal.

## Compilar e gravar (PlatformIO)

```powershell
cd firmware_cofrinho
pio run        # compilar
pio run -t upload       # gravar no ESP32
pio device monitor      # monitor serial 115200
```

Dependências (`platformio.ini`, `env:esp32dev`): ArduinoJson, WebSockets (links2004), WiFiManager (tzapu), Adafruit NeoPixel, ESP32Servo.

## Protocolo WebSocket

Conexão sem TLS:
>>>>>>> 0061d9280d5d7fc53cc2d63004cb959bdc004a27

```text
ws://<backend-host-ou-ip>:3000/ws/cofres/<id>/<mac-sem-dois-pontos>
```

<<<<<<< HEAD
Para o Render, os campos do portal devem ficar assim:

```text
Backend host/IP: https://backend-cofrinho.onrender.com
Porta backend: 443
```

O backend aceita IDs de `cofre01` a `cofre30`. A conexao bem-sucedida recebe:
=======
Backend aceita `cofre01`…`cofre30` (limite atual de 30 equipes). Sucesso:
>>>>>>> 0061d9280d5d7fc53cc2d63004cb959bdc004a27

```json
{"type":"connected","deviceId":"cofre01"}
```

<<<<<<< HEAD
O token nao e enviado pela URL. O firmware o inclui no handshake WebSocket como
`Authorization: Bearer <device-token>`. Com TLS ativado, esse cabecalho e
protegido em transito.

O firmware inclui o MAC do ESP32 no caminho WebSocket, sem os dois-pontos. O
backend mantem somente uma conexao por cofre e rejeita outro ESP32 que tente
usar o mesmo identificador, em vez de derrubar o dispositivo ja conectado.

## Mensagens recebidas

Comandos recebidos sao convertidos para o formato legado `{ "comando": command }`
com os campos de `payload` no mesmo objeto:
=======
Comandos (convertidos para o formato legado `{ "comando": … }` com `payload` junto):
>>>>>>> 0061d9280d5d7fc53cc2d63004cb959bdc004a27

```json
{"type":"command","command":"abrir","payload":{}}
{"type":"command","command":"correta","payload":{"cor":{"R":0,"G":255,"B":0}}}
{"type":"command","command":{"tentando":true,"etapa":1,"cor":{"R":255,"G":0,"B":0}}}
```

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

Reconecta a cada `WEBSOCKET_RECONNECT_INTERVAL_MS` (5s) se cair. Sem `pio` no PATH, compile em máquina com PlatformIO instalado.
