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

O host inicia preenchido como `https://backend-cofrinho.onrender.com`, o token
inicia como `cofrinho-device-token` e o identificador inicia como `cofre01`.
Para o Render, use a porta `443`; o firmware remove o `https://`
automaticamente antes de abrir a conexao e usa TLS nessa porta.

Para apagar a rede salva e reabrir o portal, mantenha o botao **BOOT** do ESP32
(GPIO 0) pressionado durante os primeiros tres segundos da inicializacao. O
reset preserva os ajustes mecanicos e visuais do cofrinho, apagando somente Wi-Fi,
host do backend, token e identificador do dispositivo.

O backend deve estar acessivel pela rede local: no portal informe o IP LAN da
maquina que executa o backend, e nao `localhost` ou `127.0.0.1`. Em
`backend_cofrinho/.env`, configure `DEVICE_TOKEN` com o mesmo valor informado no
ESP32.

## Protocolo WebSocket

O dispositivo conecta por WebSocket. Localmente use `ws` na porta `3000`; no
Render use `wss` na porta `443`:

```text
ws://<backend-host-ou-ip>:3000/ws/cofres/<id>/<mac-sem-dois-pontos>
```

Para o Render, os campos do portal devem ficar assim:

```text
Backend host/IP: https://backend-cofrinho.onrender.com
Porta backend: 443
```

O backend aceita IDs de `cofre01` a `cofre30`. A conexao bem-sucedida recebe:

```json
{"type":"connected","deviceId":"cofre01"}
```

O token nao e enviado pela URL. O firmware o inclui no handshake WebSocket como
`Authorization: Bearer <device-token>`. Com TLS ativado, esse cabecalho e
protegido em transito.

O firmware inclui o MAC do ESP32 no caminho WebSocket, sem os dois-pontos. O
backend mantem somente uma conexao por cofre e rejeita outro ESP32 que tente
usar o mesmo identificador, em vez de derrubar o dispositivo ja conectado.

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
