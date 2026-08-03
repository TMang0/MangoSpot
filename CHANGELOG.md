# Changelog de MangoSpot

Formato basado en [Keep a Changelog](https://keepachangelog.com/es-ES/1.1.0/) y versionado en [VERSIONING.md](VERSIONING.md).

## [0.1.0-beta.1] - 2026-08-03

### Arreglado
- Conexión Spotify: reparado bug crítico en `bell` (`HTTPClient.cpp`) que dejaba vacíos todos los cuerpos de respuestas HTTPS.
- Zeroconf: acepta `Content-Length` sin distinción de mayúsculas/minúsculas en el POST de emparejamiento.
- mDNS: reintentos de obtención de IP y anuncios meta-PTR proactivos para que el Switch aparezca en la red.
- ApResolve: fallback a `ap.spotify.com:4070` cuando la respuesta de apresolve llega vacía.

### Cambiado
- Arranque: implementada precarga escalonada (2 pistas iniciales → 4 en estado estable) para reducir la latencia hasta el primer audio.
- Timeout de carga de pista en `TrackPlayer` ampliado de 5 s a 20 s para redes lentas.
- Logs de tiempo añadidos en TLS, apresolve y login5 para diagnóstico futuro.

### Añadido
- Pantalla de ajustes (`source/ui/settings.c/h`).
- Pantalla de créditos (`source/ui/credits.c/h`).
- Internacionalización base (`source/utils/lang.c/h`).
- Icono de configuración (`romfs/images/gear.png`).

### Notas
- Los submódulos `cspot` y `bell` contienen commits locales que no se han empujado a sus orígenes upstream (`feelfreelinux`). El repositorio principal apunta a esos commits mediante el hash del submódulo; para clonar y compilar desde cero será necesario que esos commits estén disponibles en este repo principal.

## [0.0.0] - 2026-07-xx

### Añadido
- Estructura inicial del proyecto MangoSpot para Nintendo Switch.
- Integración con cspot como cliente Spotify Connect.
