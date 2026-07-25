# MangoSpot

Homebrew para Nintendo Switch que convierte la consola en un reproductor de
música con una librería local y, en desarrollo activo, un receptor real de
**Spotify Connect**.

> Estado: prototipo en desarrollo activo, se compila desde el código fuente.

## Librerías y proyectos utilizados

### Toolchain y plataforma
- **[devkitPro](https://devkitpro.org/)** (`devkitA64` + `libnx`) — toolchain
  de compilación cruzada y SDK de homebrew para Nintendo Switch.
- **[SDL2](https://www.libsdl.org/)** (+ `SDL2_mixer`, `SDL2_ttf`,
  `SDL2_image`) — renderizado, input de los Joy-Con/Pro Controller, y audio.

### Cliente de Spotify Connect
- **[cspot](https://github.com/feelfreelinux/cspot)** (incluido como
  submódulo en `external/cspot`) — cliente de Spotify Connect en C++,
  pensado originalmente para embebidos (ESP32) pero portable. Resuelve todo
  el protocolo de Spotify: resolución de access point, el handshake cifrado
  (Diffie-Hellman + cifrado Shannon), la sesión Mercury/Spirc, y la
  decodificación de audio.
  - **[bell](https://github.com/feelfreelinux/bell)** — la librería de
    soporte de cspot (sockets, TLS, hilos, buffers de audio). A su vez trae:
    - **mbedTLS** (portlib `switch-mbedtls` de devkitPro) para las llamadas
      HTTPS periféricas (el canal principal de Spotify Connect usa su propio
      cifrado, no TLS).
    - **[nanopb](https://github.com/nanopb/nanopb)** — generación/runtime de
      Protocol Buffers para el protocolo de Spotify.
    - **tremor** — decodificador Vorbis de punto fijo (el formato en el que
      Spotify entrega el audio).
    - **Opus** / **opencore-aacdec** — soporte de códecs adicionales.
    - **[nlohmann/json](https://github.com/nlohmann/json)** — JSON.

### Por qué cspot (y no librespot-golang, TinyGo, o librespot en Rust)
Se evaluaron esas tres alternativas antes de elegir cspot:
- `librespot-golang` está prácticamente abandonado y el propio proyecto lo
  marca como experimental/incompleto.
- Un target `nintendoswitch` de TinyGo existe, pero requiere un fork no
  oficial de TinyGo de varios años de antigüedad y una herramienta de
  empaquetado (`linkle`) también abandonada — no encaja con el pipeline de
  devkitPro.
- `librespot` (Rust, el proyecto original) está muy activo, pero depende del
  `std` de Rust para un SO "hosted" (tokio, TLS, sockets), y no existe un
  target oficial de Rust para Horizon OS/libnx.

`cspot` es C++ y está pensado para targets con recursos limitados, lo que
encaja directamente con el toolchain `devkitA64` que ya usa este proyecto.

## Descripción del proyecto

MangoSpot es una app de homebrew para Switch con dos partes:

1. **Reproductor local**: navegación de una librería de álbumes/canciones
   (hoy con datos de prueba en `source/data/mock_data.c`) con una UI
   controlada por Joy-Con, reproduciendo archivos locales vía
   `SDL2_mixer`.
2. **Cliente de Spotify Connect**: la Switch se autentica contra Spotify y
   aparece como un dispositivo más en el selector "Conectar a un
   dispositivo" de la app oficial (igual que un Sonos o un Chromecast) — se
   elige qué suena desde el celular/PC, y el audio se decodifica y reproduce
   directamente en la consola.

Ambas partes conviven de forma independiente por ahora; todavía no están
unificadas en una sola UI.

## Estructura del proyecto

```
Makefile                  # build principal (devkitA64)
source/
  main.c                  # entry point, init de SDL/audio/Spotify
  render/                 # capa de renderizado (SDL2 + EGL/deko)
  player/                 # estado y control de reproducción local
  ui/                      # pantallas (biblioteca, álbum, now playing)
  data/                    # librería de prueba (mock)
  spotify/                # cliente de Spotify Connect (SwitchAudioSink, SpotifyClient)
external/cspot/            # submódulo: cliente de Spotify Connect (C++)
linktest/                  # proyecto CMake descartable usado para validar
                            # que cspot linkea para el target Switch
romfs/                     # fuentes y audio de prueba empaquetados en el .nro
SPOTIFY_INTEGRATION.md     # plan y bitácora técnica de la integración de Spotify
```

## Compilar

Requisitos: [devkitPro](https://devkitpro.org/wiki/Getting_Started) con los
paquetes `switch-dev`, `switch-sdl2`, `switch-sdl2_mixer`, `switch-sdl2_ttf`,
`switch-sdl2_image` y `switch-mbedtls`; `cmake`; Python 3 (para el generador
de código de nanopb).

La parte de Spotify (`cspot`/`bell`) se compila aparte con CMake antes de
compilar la app principal — el detalle completo (comandos exactos, parches
necesarios, y por qué) está en [SPOTIFY_INTEGRATION.md](SPOTIFY_INTEGRATION.md).
Una vez compilado eso, `make` en la raíz del proyecto genera el `.nro`.

## Estado / roadmap

Ver [SPOTIFY_INTEGRATION.md](SPOTIFY_INTEGRATION.md) para el detalle
completo. Resumen:

- [x] cspot/bell compilando y linkeando para devkitA64/Switch
- [x] Sink de audio real (SDL2) y sesión de Spotify integrados en la app
- [x] Emparejamiento vía **Zeroconf/Spotify Connect** ("MangoSpot" aparece en
      el selector de dispositivos de la app oficial, sin usuario/contraseña)
- [x] Reproducción real end-to-end confirmada en hardware (streaming,
      decodificación y audio de pistas reales de Spotify)
- [x] "Now Playing" muestra título/artista/álbum/progreso real de Spotify
      cuando hay una sesión conectada
- [ ] Mini-player (barra inferior en biblioteca/álbum) todavía muestra
      siempre datos de la librería mock, no el estado real de Spotify
- [ ] Controles (A/L/R) todavía solo afectan al reproductor local mock,
      no controlan remotamente la sesión real de Spotify
- [ ] Unificar por completo la UI del reproductor local con el estado real
      de Spotify
- [ ] Cliente de búsqueda/navegación vía Web API de Spotify

## Historial de la integración: problemas encontrados y solución

Bitácora resumida de los problemas más importantes resueltos durante el
desarrollo del cliente de Spotify Connect (detalle técnico completo en
[SPOTIFY_INTEGRATION.md](SPOTIFY_INTEGRATION.md)):

1. **Login con usuario/contraseña no funciona: bloqueado por Spotify.**
   Se probó primero con `LoginBlob::loadUserPass` (usuario/contraseña en un
   archivo). En hardware real, tras un handshake exitoso, Spotify respondía
   `Authorization declined`. Confirmado que Spotify deprecó y bloqueó el
   login por contraseña para clientes no oficiales (librespot/cspot).
   **Solución**: se pivoteó a **Zeroconf/"tap to pair"** (el mismo mecanismo
   de "conectar dispositivo" de Sonos/Chromecast): la Switch se anuncia por
   mDNS y expone un pequeño servidor HTTP (`/spotify_info`) que recibe las
   credenciales ya autenticadas desde la app oficial.

2. **Sin `getifaddrs()` ni servidor HTTP embebido en libnx.** No hay forma
   estándar de listar interfaces de red ni de usar `civetweb` (el servidor
   HTTP que trae `bell`, requiere `sys/utsname.h`/`grp.h`/`pwd.h`, no
   disponibles). **Solución**: IP local vía `nifmGetCurrentIpAddress()`
   (API propia de libnx) y un servidor HTTP mínimo hecho a mano (~150
   líneas, sockets BSD crudos) sólo para las dos rutas que hacen falta.

3. **Crash inmediato al registrar el servicio mDNS.** `registerService()`
   devolvía un `unique_ptr` que se descartaba, destruyendo el hilo del
   responder justo después de crearlo (use-after-free). **Solución**:
   guardar el handle en una variable global que vive todo el tiempo que la
   app está corriendo.

4. **Crash duro (sin logs) usando `tinysvcmdns` de terceros para mDNS.**
   Se sospechó fallo de acceso a memoria no alineada en aarch64 al parsear
   paquetes DNS reales (cast de structs sobre bytes crudos). **Solución**:
   se descartó esa librería para Switch y se escribió un responder mDNS
   propio, mínimo, que parsea todo byte a byte sin castear structs.

5. **`setsockopt` de multicast (`IP_ADD_MEMBERSHIP`) fallaba en hardware
   real.** **Solución**: se volvió no-fatal (log y continúa) y se agregó el
   `IP_MULTICAST_IF` que faltaba antes de unirse al grupo — libnx, a
   diferencia de Linux, no infiere solo la interfaz.

6. **El dispositivo nunca aparecía en la lista de Spotify Connect, pese a
   responder siempre bien a `dns-sd -B`/`curl`.** Se probaron y aplicaron,
   en orden, varias correcciones de spec (RFC 6762/6763) sobre el
   respondedor mDNS propio: bit de "cache-flush" en los registros SRV/TXT/A,
   corrección de qué registros van en la sección ANSWER vs ADDITIONAL, y
   soporte del bit "QU" (respuesta unicast vs multicast). Cada una se
   verificó como spec-correcta pero ninguna arregló el síntoma por sí sola.
   **Causa real encontrada**: el servidor HTTP hecho a mano comparaba el
   path de la petición con igualdad exacta (`/spotify_info`), pero Spotify
   agrega un query string (`/spotify_info?action=getInfo`), así que la
   comparación fallaba y devolvía una respuesta vacía. **Solución**: cortar
   el path en el primer `?` antes de comparar.

7. **Crash de arranque en `render_init()` (pantalla en negro).** Tenía dos
   capas: (a) las fuentes y canciones de prueba se cargaban desde rutas de
   la SD (`sdmc:/switch/mangospot/...`) en vez de `romfs:/...`, y aun tras
   corregir eso el `romfs` **nunca se empaquetaba en el `.nro`** porque el
   `Makefile` nunca definía `NROFLAGS` (la regla de `switch_rules` que
   agrega `--romfsdir` quedaba vacía); (b) una vez arreglado el empaquetado,
   las fuentes `.ttf` incluidas resultaron ser páginas HTML (una descarga
   fallida guardada con extensión `.ttf`), por lo que `SDL_ttf` no podía
   abrirlas. **Solución**: se dejó de empaquetar fuentes propias y se pasó a
   usar la **fuente compartida del sistema** vía el servicio `pl` de libnx
   (`plGetSharedFontByType`), el enfoque estándar en homebrew (sin licencias
   que resolver, siempre disponible).

8. **Sin forma de depurar crashes duros en hardware (pantalla negra, sin
   logs).** `nxlink` pierde la conexión en cuanto la app crashea.
   **Solución**: `applog_init()` redirige stdout/stderr a un archivo en la
   SD (`sdmc:/switch/mangospot/mangospot.log`), sin buffer, con un hilo que
   hace `fflush`+`fsdevCommitDevice` cada 100ms para que el log sobreviva
   incluso a un crash fatal.

9. **Conflicto de dispositivo de audio al conectar con Spotify real.** El
   reproductor local mock abre el audio con `Mix_OpenAudio()` al arrancar y
   nunca lo cierra; al autenticar con Spotify, `SwitchAudioSink` fallaba al
   abrir su propio dispositivo (la Switch solo tiene una salida de audio
   física). **Solución**: cerrar el audio del reproductor mock
   (`Mix_HaltMusic()`+`Mix_CloseAudio()`) justo antes de abrir el sink real
   de Spotify.

10. **"Now Playing" no mostraba nada de la sesión real de Spotify.** La
    información (título/artista/álbum) sí se recibía y guardaba
    correctamente desde los eventos de Spotify, pero ninguna pantalla la
    leía — la UI solo conocía la librería mock. **Solución**: la pantalla
    "Now Playing" ahora consulta primero el estado de Spotify y solo cae de
    vuelta a los datos mock si no hay sesión conectada; se agregó también el
    seguimiento de progreso de reproducción (duración/posición) ya que
    Spotify no emite un evento periódico de progreso.

## Aviso

Este proyecto usa Spotify solo con cuentas Premium y para uso personal,
en la misma línea que proyectos como `librespot`, `raspotify` o `spotifyd`.
No está afiliado a Spotify.
