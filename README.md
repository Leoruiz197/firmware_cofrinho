# Firmware Cofrinho

Firmware para ESP32 do cofrinho, baseado no comportamento do Cofre FIAP V2,
comunicando-se com o backend por WebSocket e organizado para uso com PlatformIO.

## Estrutura

- `src/main.cpp`: inicializacao do dispositivo e ciclo principal.
- `src/settings.cpp`: configuracao Wi-Fi/backend e persistencia na flash.
- `src/hardware.cpp`: servos, fita NeoPixel e efeitos visuais.
- `src/websocket_service.cpp`: conexao, reconexao e roteamento WebSocket.
- `src/command_handler.cpp`: validacao e execucao dos comandos recebidos.
- `src/state.cpp`: estado compartilhado do cofrinho e das etapas.
- `include/`: contratos publicos e constantes de hardware.

## Hardware

| Componente | GPIO | Observacao |
| --- | --- | --- |
| Servo da tranca | 18 | Aberto em 30 graus e fechado em 90 graus. |
| Servo da porta | 19 | Aberto em 0 grau; fechamento configuravel. |
| Fita NeoPixel | 23 | 18 LEDs: 0-1 luz interna e 2-17 progresso. |

## Configuracao inicial

Ao iniciar sem rede configurada, o ESP32 abre o portal Wi-Fi aberto
`Cofrinho_Config`. Nele devem ser informados a rede Wi-Fi, o **Backend host/IP**,
o **Device token** e o identificador do cofrinho. Essas configuracoes ficam salvas
na flash do ESP32.

O host inicia vazio, o token inicia como `cofrinho-device-token` e o identificador
inicia como `cofre01`. O backend usa a porta `3000` (`PORT=3000` no `.env`).

Para apagar a rede salva e reabrir o portal, mantenha o botao **BOOT** do ESP32
(GPIO 0) pressionado durante os primeiros tres segundos da inicializacao. O
reset preserva os ajustes mecanicos e visuais do cofrinho, apagando somente Wi-Fi,
host do backend, token e identificador do dispositivo.

O backend deve estar acessivel pela rede local: no portal informe o IP LAN da
maquina que executa o backend, e nao `localhost` ou `127.0.0.1`. Em
`backend_cofrinho/.env`, configure `DEVICE_TOKEN` com o mesmo valor informado no
ESP32 e mantenha `PORT=3000` (ou atualize `BACKEND_PORT` em `include/config.h`).

## Protocolo WebSocket

O dispositivo conecta sem TLS a:

```text
ws://<backend-host-ou-ip>:3000/ws/cofres?deviceId=<id>&token=<token>
```

O backend aceita IDs de `cofre01` a `cofre50`. A conexao bem-sucedida recebe:

```json
{"type":"connected","deviceId":"cofre01"}
```

## Mensagens recebidas

Comandos recebidos sao convertidos para o formato legado `{ "comando": command }`
com os campos de `payload` no mesmo objeto:

```json
{"type":"command","command":"abrir","payload":{}}
{"type":"command","command":"correta","payload":{"cor":{"R":0,"G":255,"B":0}}}
```

Configuracoes recebidas usam `stages` como `num_senhas`; quando `reset` e `true`,
as etapas visuais tambem sao reiniciadas:

```json
{
  "type": "config",
  "config": {"ownerTeamId": "equipe01", "stages": 3, "reset": true}
}
```

`num_senhas` aceita de 1 a 16, que corresponde ao numero de LEDs de progresso.

## Retorno do Cofre

Depois de processar um comando ou configuracao, o ESP32 envia o status:

```json
{"type":"status","status":{"event":"command","result":"ok","detail":"abrir"}}
{"type":"status","status":{"event":"command","result":"error","detail":"unknown_command"}}
{"type":"status","status":{"event":"configuration","result":"ok","detail":"saved"}}
```
