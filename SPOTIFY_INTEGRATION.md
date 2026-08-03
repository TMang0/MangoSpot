# Integración de Spotify en MangoSpot

Plan de trabajo para agregar un cliente/receptor de Spotify Connect real a la
app, reemplazando la librería mock (`source/data/mock_data.c`).

## Decisión de arquitectura

Se evaluaron 3 opciones iniciales (librespot-golang, TinyGo `nintendoswitch`,
librespot en Rust) y las 3 se descartaron: el ABI/runtime de devkitPro-libnx-C,
el backend custom de TinyGo, y el `std` de Rust son entornos mutuamente
incompatibles — ninguno se puede linkear dentro del NRO actual.

**Elegido: [cspot](https://github.com/feelfreelinux/cspot)**, cliente de
Spotify Connect en C++ pensado para embebidos (ESP32) pero portable. Encaja
con el toolchain devkitA64/libnx que ya usa este proyecto.

Arquitectura resultante:

```mermaid
flowchart LR
    subgraph Switch["MangoSpot (NRO)"]
        UI[ui/ existente] --> Player[player.c]
        Player --> Sink[SwitchAudioSink nuevo]
        Sink --> cspot[libcspot.a + libbell.a]
    end
    cspot <--> |TCP/TLS mbedTLS| Spotify[Servidores de Spotify]
    Phone[App oficial de Spotify] -. controla vía Spotify Connect .-> cspot
```

- **cspot/bell** resuelven todo el protocolo (ApResolve, handshake Shannon +
  Diffie-Hellman, sesión, Mercury, Spirc/Connect state, decodificación de
  Vorbis/Opus/AAC/MP3 a PCM).
- Nuestra app solo implementa un `AudioSink` (recibe PCM 44.1kHz/16-bit
  estéreo) y engancha los controles de Joy-Con al estado de reproducción.
- Fase de autenticación: login usuario/contraseña (cachear el auth blob),
  zeroconf/mDNS queda para después (ver research en memoria de repo).
- Fase futura (no empezada): cliente de la Web API de Spotify (búsqueda/
  navegación) usando OAuth Authorization Code + PKCE — ver notas de research.

## Fases

1. **[EN CURSO] Motor de reproducción (cspot)** — hacer que cspot/bell
   compilen para el target Switch, escribir el shim de plataforma que falta,
   escribir el `AudioSink`, linkear todo dentro del Makefile existente y
   lograr reproducir un track de prueba.
2. **Autenticación** — login usuario/contraseña con teclado en pantalla
   (`swkbd`), cachear el auth blob en la SD.
3. **(Futuro) Cliente Web API** — búsqueda/navegación + control de la propia
   sesión Connect vía `PUT /v1/me/player/play?device_id=...`.

## Estado actual — Fase 1

### Hecho

- Repo inicializado con git (no lo tenía) y commit inicial del estado previo.
- `cspot` vendorizado como submódulo git en `external/cspot` (con sus propios
  submódulos: `bell`, `mdnssvc`, `nlohmann_json`, `nanopb`, `opus`,
  `opencore-aacdec`, `tremor`, `cJSON`, `civetweb`, `mqtt`, `portaudio`,
  `alac`, `fmt`).
- Entorno de build preparado en esta Mac:
  - `cmake` instalado vía Homebrew (no estaba).
  - `switch-mbedtls` instalado vía `dkp-pacman` (portlib requerido por bell).
  - Venv de Python en `.venv-tools/` con `protobuf` + `grpcio-tools` +
    `setuptools<81` (necesario para que `pkg_resources`, deprecado, siga
    existiendo — lo usa el generador de nanopb) para la generación de código
    de protobuf/nanopb (esto corre solo en la Mac, nunca en la Switch).
- **`libcspot.a` y `libbell.a` compilan y linkean con éxito para
  aarch64/devkitA64 usando el toolchain `/opt/devkitpro/cmake/Switch.cmake`.**
  Esto incluye TODO lo difícil: sesión Spotify, cifrado Shannon, TLS
  (mbedTLS), sockets, HTTP client, y los decoders de Vorbis/Opus/AAC/MP3.
- 4 parches mínimos necesarios (documentados también en la memoria de repo):
  1. `external/cspot/cspot/bell/external/libhelix-mp3/mp3dec.h` — agregar
     `#elif defined(__aarch64__)` al chequeo de plataforma (no-op, solo
     desbloquea la compilación).
  2. `external/cspot/cspot/bell/external/libhelix-mp3/assembly.h` — agregar
     `defined(__aarch64__)` a la rama de fallback en C portable (sin
     assembly) que ya usan Apple/ESP32/amd64.
  3. `external/cspot/cspot/bell/external/nanopb/generator/nanopb_generator.py`
     — `reflection.MakeClass` fue removido en protobuf moderno; se agregó un
     fallback a `message_factory.GetMessageClass`.
  4. `external/cspot/cspot/src/PlainConnection.cpp` — faltaba
     `#include <arpa/inet.h>` para `htonl`/`ntohl`.

  Nota: los parches 1-3 son a submódulos vendorizados (no a nuestro código) —
  viven como cambios locales sin commitear en `external/cspot` por ahora.
  Habría que evaluar más adelante si forkeamos `cspot`/`bell` para
  persistirlos limpiamente.

### Cómo reproducir el build

```sh
source .venv-tools/bin/activate   # provee protobuf/grpcio-tools al build
cmake -S external/cspot/cspot -B build-cspot-switch \
  -DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/cmake/Switch.cmake \
  -DBELL_DISABLE_MQTT=ON -DBELL_DISABLE_WEBSERVER=ON \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build build-cspot-switch -j8
```

Produce `build-cspot-switch/libcspot.a` y `build-cspot-switch/bell/libbell.a`.

- **Shim de plataforma `switch` para bell escrito y funcionando**:
  `WrappedSemaphore` (libnx NO implementa `sem_init`/`sem_wait` de POSIX pese
  a tener el header — hubo que escribirlo a mano sobre `Mutex`+`CondVar`
  nativos de libnx). `MDNSService` confirmado innecesario por ahora (solo lo
  usa el flujo de Zeroconf pairing, que no vamos a implementar todavía).
- **Prueba de linkeo end-to-end exitosa**: proyecto descartable en
  `linktest/` arma una sesión real (`LoginBlob → Context → SpircHandler`) y
  **linkea sin errores** para aarch64/devkitA64. Este era el riesgo técnico
  más grande de todo el proyecto y ya está confirmado:
  ```sh
  source .venv-tools/bin/activate
  cmake -S linktest -B build-linktest -DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/cmake/Switch.cmake -DCMAKE_POLICY_VERSION_MINIMUM=3.5
  cmake --build build-linktest -j8
  ```

### Falta (próximos pasos, en orden)

1. ~~Escribir el shim de plataforma "switch" para bell~~ **HECHO** —
   `WrappedSemaphore` implementado sobre `Mutex`+`CondVar` nativos de libnx
   (libnx NO tiene una implementación real de `sem_init`/`sem_wait` de POSIX
   pese a que el header existe — hubo que escribir la sincronización a mano).
   `MDNSService` se confirmó opcional: solo lo usa `ZeroconfAuthenticator` en
   el target CLI de cspot, no el núcleo (`SpircHandler`/`Session`/etc.), así
   que no hace falta todavía (login usuario/contraseña no lo requiere).
2. ~~Prueba de linkeo end-to-end~~ **HECHO** — proyecto CMake descartable en
   `linktest/` que arma `LoginBlob → Context → SpircHandler` (esto arrastra
   TrackPlayer/TrackQueue/CDNAudioFile/MercurySession/Shannon/mbedTLS/
   sockets) y **linkea sin errores** (`linktest.elf`). Este es el riesgo
   técnico más grande del proyecto y ya está confirmado que funciona.
3. ~~Escribir `SwitchAudioSink` real~~ **HECHO** — `source/spotify/SwitchAudioSink.{h,cpp}`,
   usa un `SDL_AudioDeviceID` propio (independiente del que abre
   `Mix_OpenAudio`) con `SDL_QueueAudio`, con backpressure simple (~2s de
   buffer máximo).
4. ~~Mover la integración al Makefile real~~ **HECHO** — `source/spotify/SpotifyClient.{h,cpp}`
   (login usuario/contraseña, sesión, hilos de red y de audio, todo
   modelado 1:1 sobre el target CLI oficial de cspot) + cambios en el
   `Makefile` raíz (ver más abajo) + `main.c` llama a `spotify_client_start()`
   al arrancar. **`make` genera un `mangospot.nro` real con todo
   incluido.**
5. Implementar login con `swkbd` (teclado en pantalla) + cache del auth blob
   en la SD — hoy el login lee usuario/contraseña de un archivo de texto
   plano en la SD (ver sección "Probarlo" abajo). Es un atajo deliberado para
   tener un prototipo cuanto antes; `swkbd` queda como mejora de UX.
6. Enganchar los controles existentes (Joy-Con / mini-player) al estado real
   de reproducción en vez del mock — hoy el mock (`mock_data.c`) y Spotify
   corren en paralelo, sin tocarse.

## Estado actual — Fase 1: COMPLETA (prototipo listo para probar)

### Cambios en el Makefile

- `SOURCES` ahora incluye `source/spotify`.
- Se agregó globbing de `.cpp` (`CPPFILES`), antes solo compilaba `.c`.
- `CXXFLAGS` ya no tiene `-fno-rtti -fno-exceptions` (cspot los necesita:
  `LoginBlob`, `nlohmann::json`, etc. usan excepciones) y suma `-std=gnu++20`
  + los `-I` de cspot/bell (nanopb, fmt, tremor, nlohmann_json, y los headers
  de protobuf generados en `build-cspot-switch/`).
- `LIBS` suma, al principio: `-lcspot -lbell -lopencore-aacdec -lopus
  -lmbedtls -lmbedx509 -lmbedcrypto`.
- `LIBPATHS` suma los 4 directorios de `build-cspot-switch/` (raíz,
  `bell/`, `bell/external/opus/`, `bell/external/opencore-aacdec/`).
- `LDFLAGS` suma `-Wl,--allow-multiple-definition`: bell vendoriza su propia
  copia de `libogg` (via tremor, para Vorbis de punto fijo) que duplica
  símbolos (`ogg_page_version`, etc.) con el portlib `-logg` que ya usaba el
  proyecto para reproducción local. Ambas copias son ABI-compatibles; el
  flag solo le dice al linker que se quede con la primera que encuentre.

**Importante**: antes de correr `make` en este proyecto hace falta tener
`build-cspot-switch/` ya compilado (ver comando más arriba) — el Makefile
NO dispara ese build por sí solo todavía (posible mejora futura: un target
`cspot-libs` que lo automatice).

### Arquitectura del código nuevo (`source/spotify/`)

- `SwitchAudioSink.{h,cpp}`: implementa `AudioSink::feedPCMFrames` con
  `SDL_QueueAudio` sobre un `SDL_AudioDeviceID` propio.
- `SpotifyClient.{h,cpp}`: API en C (`spotify_client_start()`,
  `spotify_client_get_now_playing()`) que por dentro arma, en espejo casi
  exacto del target CLI oficial de cspot (`targets/cli/main.cpp` +
  `CliPlayer.cpp`):
  - `LoginBlob` (usuario/contraseña) → `Context::createFromBlob` →
    `session->connectWithRandomAp()` → `session->authenticate()`.
  - `SpircHandler` + dos hilos propios (clases `bell::Task`):
    - `SessionPumpTask`: bombea `session->handlePacket()` en loop (el CLI
      oficial hace esto en el hilo principal con un `while` bloqueante; acá
      no podemos bloquear el hilo principal de SDL, así que va en su propio
      hilo).
    - `PlayerPumpTask`: bombea `CentralAudioBuffer` → `BellDSP` →
      `AudioSink`, réplica de `CliPlayer::runTask()`.
- `main.c` llama a `socketInitializeDefault()` + `nxlinkStdio()` (para poder
  ver los `printf` con `nxlink -s`) y luego `spotify_client_start()` justo
  después de abrir el audio, antes de crear la ventana. Si falla (por
  ejemplo no existe el archivo de credenciales), solo imprime un mensaje y
  el resto de la app sigue funcionando normal con la library mock.

### Cómo probarlo (falta hacer esto en hardware real)

1. En la SD de la Switch, crear `sdmc:/switch/mangospot/login.txt` con:
   ```
   tu_usuario_de_spotify
   tu_contraseña
   ```
   (dos líneas, sin comillas). Ojo: si tu cuenta usa login de
   Facebook/Google/Apple, hay que setear una contraseña "normal" desde la
   configuración de la cuenta de Spotify primero.
2. Copiar `mangospot.nro` a la SD (por ej. `sdmc:/switch/mangospot.nro`)
   y correrlo desde el homebrew menu, o con `nxlink -s mangospot.nro` para
   además ver los logs por red.
3. Si conecta bien, la Switch debería aparecer en el selector "Conectar a un
   dispositivo" de la app de Spotify en el celular/PC — elegirla y darle
   play a algo.
4. Cosas sin probar todavía / posibles puntos de falla en hardware real (no
   se pueden validar sin una Switch física):
   - Comportamiento real de `pthread_create`/hilos de libnx bajo carga
     (linkea bien, pero nunca se ejecutó).
   - Abrir dos `SDL_AudioDeviceID` simultáneos (uno de `Mix_OpenAudio`, otro
     del `SwitchAudioSink`) — no hay garantía de que el driver de audio de
     Switch en SDL2 soporte esto sin probarlo.
   - Permisos de red del homebrew (debería andar igual que cualquier NRO que
     hace requests HTTP, pero no está confirmado en este proyecto puntual).

## ACTUALIZACIÓN (2026-07-24): el login usuario/contraseña de arriba ya NO se usa

Probado en hardware real: el crash de antes se solucionó (el fix de
excepciones aguantó), pero Spotify respondió **"Authorization declined"**
después de un handshake Shannon exitoso. Confirmado con la wiki oficial de
`librespot`: **el login clásico usuario/contraseña está deprecado y
bloqueado por Spotify del lado del servidor**, para cualquier cliente tipo
librespot/cspot, en cualquier cuenta. No es arreglable desde nuestro lado.

`cspot` no tiene un flujo de OAuth propio (a diferencia del librespot
moderno en Rust). La única alternativa real que sigue funcionando es
**Zeroconf / "tocar para vincular"**, y ya está implementada:

- `bell/main/platform/switch/MDNSService.cpp` (nuevo) — anuncia la Switch
  por mDNS usando la librería `mdnssvc` ya vendorizada, obteniendo la IP
  local vía `nifmGetCurrentIpAddress` de libnx (no vía `getifaddrs`, que no
  existe en libnx).
- `ZeroconfHttpServer` (dentro de `SpotifyClient.cpp`) — un servidor HTTP
  mínimo escrito a mano (sockets crudos), NO usamos `BellHTTPServer`/civetweb
  de bell porque civetweb asume un entorno POSIX mucho más completo
  (`grp.h`, `pwd.h`, `sys/wait.h`) que no vale la pena portar para el único
  endpoint que necesitamos (`GET`/`POST /spotify_info`).
- `LoginCompletionTask` — espera a que el celular complete el pairing y
  recién ahí hace el `connectWithRandomAp`/`authenticate`/arranca la sesión
  (en su propio hilo, para no demorar la respuesta HTTP al celular).

### Cómo probarlo ahora
1. Copiar `mangospot.nro` a la SD y correrlo (con `nxlink -a <ip-de-la-switch> -s mangospot.nro`
   para ver los logs — el descubrimiento por broadcast de `nxlink` puede
   fallar por aislamiento de clientes en el router; usar `-a` con la IP que
   muestra la pantalla de Netloader de hbmenu).
2. En el celular/PC, abrir la app de Spotify — la Switch debería aparecer
   sola como "MangoSpot" en el selector de dispositivos (sin escribir
   usuario ni contraseña en ningún lado).
3. Elegirla y darle play a algo.

Ya no hace falta el archivo `login.txt` en la SD — ese flujo fue eliminado
por completo.

---

## Investigación: token de Web API disponible vía Connect (2026-08-03)

Durante la implementación de "Agregar a favoritos" se descubrió que
`cspot` ya obtiene un **access token de Spotify asociado a la cuenta del
usuario** durante el flujo de Connect. El token se pide en
`AccessKeyFetcher::updateAccessKey()` (login5.spotify.com) con estos
scopes:

```cpp
"streaming,user-library-read,user-library-modify,user-top-read,user-read-recently-played"
```

El mismo token que `TrackQueue` usa para pedir la URL del CDN
(`api.spotify.com/v1/storage-resolve/files/audio/interactive/...`) sirve
para llamar a la **Web API de Spotify**, porque los scopes incluyen
`user-library-read` y `user-library-modify`. Esto permitió implementar
`PUT/DELETE https://api.spotify.com/v1/me/tracks?ids=<track_id>` para
agregar/quitar la canción actual de favoritos sin ningún flujo OAuth
adicional.

### Implicaciones a futuro (sin cambios planeados aún)

Dado que el token pertenece a la cuenta del usuario y tiene scopes de
biblioteca y reproducción, Spotify Connect deja de ser la única forma de
usar MangoSpot. Sería posible construir un **cliente nativo completo** que:

- Busque tracks, álbumes, artistas y playlists (`/v1/search`).
- Liste las listas guardadas del usuario (`/v1/me/playlists`).
- Muestre las canciones favoritas (`/v1/me/tracks`).
- Reproduzca directamente desde la biblioteca del usuario sin depender del
celular/PC.
- Use Connect como un "modo adicional" (elegir MangoSpot como dispositivo
desde otra app).

Esto es solo investigación por ahora; no hay cambios de arquitectura
planificados todavía. Spotify Connect sigue siendo el modo principal y el
Web API se usa hoy solo para favoritos.

---

## Fixes de conexión y arranque (2026-08-03)

Sesión dedicada a una regresión ("Spotify reconoce la Switch pero se queda
en *Conectando a MangoSpot*") y, una vez resuelta, a por qué tardaba ~20s en
empezar a sonar.

### Bug raíz: `HTTPClient` de bell devolvía bodies vacíos

Síntoma: tras el pairing, la sesión conectaba y los controles
(play/pausa/next) funcionaban, pero la canción nunca arrancaba. El log
mostraba `apresolve status=200 body_len=0`, `login5 ... body_len=0` →
`Failed to fetch access token` → `Track failed to load, skipping it`.

Clave del diagnóstico: los controles viajan por el canal Mercury/Shannon
(TCP crudo), que funcionaba bien; **solo** las llamadas HTTPS REST
(ApResolve, token de acceso, URL del CDN) volvían con el cuerpo vacío.

Causa: una edición local previa a
`external/cspot/cspot/bell/main/io/HTTPClient.cpp` había borrado, en
`readResponseHeaders()`, el bucle que copia los headers parseados por
picohttpparser al vector `responseHeaders`. Como `header()` solo lee ese
vector, `header("content-length")` siempre devolvía `""`, `contentSize`
quedaba en 0 y `readRawBody()` no leía nada → todos los bodies HTTPS
vacíos. Se encontró con
`git -C external/cspot/cspot/bell diff HEAD -- main/io/HTTPClient.cpp`.

Fix: se restauró el bucle de copia de headers y, además, `readRawBody()`
ahora lee en loop hasta completar el `Content-Length` (un solo record TLS
puede ser más chico que el body).

### Fixes secundarios de pairing/descubrimiento

- **`Content-Length` en minúscula**: algunos clientes de Spotify mandan
  `content-length:` en minúsculas en el POST de Zeroconf. El parser HTTP
  hecho a mano solo buscaba `Content-Length:`, leía un body de largo 0 y
  `createFromBlob` tiraba un error de JSON. Ahora se busca sin distinguir
  mayúsculas.
- **IP aún no lista al arrancar**: `nifmGetCurrentIpAddress()` puede
  devolver 0 justo al bootear (Wi-Fi todavía sin dirección), con lo que el
  responder mDNS no registraba nada y MangoSpot no aparecía. Ahora se
  reintenta la obtención de IP por unos segundos.
- **Anuncios proactivos con meta-PTR**: se incluye el PTR de enumeración
  DNS-SD (`_services._dns-sd._udp.local`) en una ráfaga inicial de anuncios,
  para que los browsers pasivos descubran el dispositivo aunque no consulten
  directamente el tipo de servicio.
- **Fallback de ApResolve**: si `apresolve.spotify.com` falla o responde
  vacío, se usa un access point conocido (`ap.spotify.com:4070`) en vez de
  quedar colgado.

### Arranque lento (~20s hasta el primer audio): era contención, no crypto

Se instrumentó con timestamps cada handshake TLS (`TLSSocket::open`) y cada
request HTTPS (ApResolve / AccessKeyFetcher / storage-resolve). Los datos de
hardware **descartaron** la hipótesis de "el ECDHE por software es lento":

- Handshakes sin contención: `apresolve` 269ms, `login5` 246ms — rápidos.
- La **misma** operación bajo carga: `api.spotify.com` 2688ms, CDN
  `audio-ak` 3881ms, e incluso el `tcp_connect` (red pura, sin crypto)
  llegó a 2042ms.

Mismo servidor, misma CPU, 246ms vs 2688ms ⇒ no es CPU-bound, es
**contención** de Wi-Fi + CPU. Mecanismo: apenas la pista 0 quedaba lista,
cspot arrancaba a precargar hasta 5 pistas (`MAX_TRACKS_PRELOAD`), cada una
con su handshake TLS a `api.spotify.com`, justo mientras la pista 0 abría su
propio stream de audio. Todo competía por el enlace y la CPU.

Nota sobre "hardware acceleration": libnx solo expone AES/SHA por hardware
(simétrico); no hay RSA/ECC, y mbedTLS ya usa ensamblador aarch64 para el
bignum. Es decir, acelerar crypto no ayudaría al handshake (que es
asimétrico) — el problema era la contención, no la velocidad del cifrado.

Fixes aplicados:

- **Precarga en dos fases (ventana rodante)** en
  `external/cspot/cspot/src/TrackQueue.cpp` + `include/TrackQueue.h`:
  durante un periodo de gracia tras quedar lista la primera pista, la
  ventana se limita a `INITIAL_TRACKS_PRELOAD = 2` (la pista 0 arranca sin
  competencia); pasado ese periodo crece a `MAX_TRACKS_PRELOAD = 4` (antes
  5) y se rellena **de a una pista por vez** a medida que se consumen
  durante la reproducción (solo pide la siguiente cuando la anterior ya
  cargó, evitando ráfagas).
- **Timeout de carga de pista** en `TrackPlayer.cpp` de 5s → 20s, para que
  la primera pista cargue al primer intento en vez de descartarse y
  reintentar (un handshake TLS frío en la Switch puede pasar de 5s
  legítimamente).
- Se dejaron los logs de timing (TLS/ApResolve/login5/storage-resolve) como
  instrumentación útil para futuras mediciones.

Detalle interno completo (con los números de handshake y el análisis) en la
memoria de repo `cspot-build-notes.md`.



