# spotiswitch

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

spotiswitch es una app de homebrew para Switch con dos partes:

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
- [ ] Login con teclado en pantalla (`swkbd`) + credenciales cacheadas de
      forma segura (hoy usa un archivo de texto plano como atajo de
      desarrollo)
- [ ] Unificar la UI del reproductor local con el estado real de Spotify
- [ ] Cliente de búsqueda/navegación vía Web API de Spotify

## Aviso

Este proyecto usa Spotify solo con cuentas Premium y para uso personal,
en la misma línea que proyectos como `librespot`, `raspotify` o `spotifyd`.
No está afiliado a Spotify.
