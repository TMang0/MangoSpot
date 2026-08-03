# MangoSpot

Nintendo Switch homebrew that turns the console into a music player with a
local library and, in active development, a real **Spotify Connect**
receiver.

> Status: active development prototype, built from source.

## Libraries and projects used

### Toolchain and platform
- **[devkitPro](https://devkitpro.org/)** (`devkitA64` + `libnx`) — cross-
  compilation toolchain and homebrew SDK for Nintendo Switch.
- **[SDL2](https://www.libsdl.org/)** (`SDL2_mixer`, `SDL2_ttf`,
  `SDL2_image`) — rendering, Joy-Con/Pro Controller input, and audio.

### Spotify Connect client
- **[cspot](https://github.com/feelfreelinux/cspot)** (included as a
  submodule in `external/cspot`) — C++ Spotify Connect client, originally
  designed for embedded devices (ESP32) but portable. Handles the full
  Spotify protocol: access-point resolution, encrypted handshake
  (Diffie-Hellman + Shannon cipher), Mercury/Spirc session, and audio
  decoding.
  - **[bell](https://github.com/feelfreelinux/bell)** — cspot's support
    library (sockets, TLS, threads, audio buffers). It also bundles:
    - **mbedTLS** (devkitPro portlib `switch-mbedtls`) for peripheral HTTPS
      calls (the main Spotify Connect channel uses its own encryption, not
      TLS).
    - **[nanopb](https://github.com/nanopb/nanopb)** — Protocol Buffers
      runtime/generator for the Spotify protocol.
    - **tremor** — fixed-point Vorbis decoder (the format Spotify serves).
    - **Opus** / **opencore-aacdec** — additional codec support.
    - **[nlohmann/json](https://github.com/nlohmann/json)** — JSON.

### Why cspot (and not librespot-golang, TinyGo, or Rust librespot)
Those three alternatives were evaluated before choosing cspot:
- `librespot-golang` is practically abandoned and the project itself marks
  it as experimental/incomplete.
- A TinyGo `nintendoswitch` target exists, but it requires an unofficial
  TinyGo fork several years old plus an abandoned packaging tool
  (`linkle`) — it doesn't fit the devkitPro pipeline.
- `librespot` (Rust, the original project) is very active, but it depends
  on Rust's `std` for a "hosted" OS (tokio, TLS, sockets), and there is no
  official Rust target for Horizon OS/libnx.

`cspot` is C++ and designed for resource-constrained targets, which fits
straight into the `devkitA64` toolchain this project already uses.

## Project description

MangoSpot is a Switch homebrew app with two parts:

1. **Local player**: browse an album/song library (today with test data in
   `source/data/mock_data.c`) with a Joy-Con-controlled UI, playing local
   files via `SDL2_mixer`.
2. **Spotify Connect client**: the Switch authenticates with Spotify and
   appears as a device in the official app's "Connect to a device" picker
   (just like a Sonos or Chromecast) — you choose what plays from your
   phone/PC, and audio is decoded and played directly on the console.

Both parts currently coexist independently; they haven't been fully unified
into a single UI yet.

## Project structure

```
Makefile                  # main build (devkitA64)
source/
  main.c                  # entry point, SDL/audio/Spotify init
  render/                 # rendering layer (SDL2 + EGL/deko)
  player/                 # local playback state and control
  ui/                     # screens (library, album, now playing)
  data/                   # test library (mock)
  spotify/                # Spotify Connect client (SwitchAudioSink, SpotifyClient)
external/cspot/           # submodule: Spotify Connect client (C++)
linktest/                 # disposable CMake project used to validate
                          # that cspot links for the Switch target
romfs/                    # test fonts/audio bundled into the .nro
SPOTIFY_INTEGRATION.md    # plan and technical log for the Spotify integration
```

## Build

Requirements: [devkitPro](https://devkitpro.org/wiki/Getting_Started) with
packages `switch-dev`, `switch-sdl2`, `switch-sdl2_mixer`, `switch-sdl2_ttf`,
`switch-sdl2_image`, and `switch-mbedtls`; `cmake`; Python 3 (for the
nanopb code generator).

The Spotify part (`cspot`/`bell`) is built separately with CMake before
compiling the main app — full details (exact commands, required patches,
and why) are in [SPOTIFY_INTEGRATION.md](SPOTIFY_INTEGRATION.md). Once
that is built, `make` at the project root generates the `.nro`.

## Status / roadmap

See [SPOTIFY_INTEGRATION.md](SPOTIFY_INTEGRATION.md) for the full detail.
Summary:

- [x] cspot/bell compiling and linking for devkitA64/Switch
- [x] Real audio sink (SDL2) and Spotify session integrated into the app
- [x] Pairing via **Zeroconf/Spotify Connect** ("MangoSpot" appears in the
      official app's device picker, no username/password)
- [x] Real end-to-end playback confirmed on hardware (streaming,
      decoding, and audio of real Spotify tracks)
- [x] "Now Playing" shows real title/artist/album/progress from Spotify
      when a session is connected
- [x] Mini-player (bottom bar in library/album) reflects real Spotify state
- [x] Controls (A/L/R) control the real Spotify Connect session, not just
      the local mock player
- [x] Settings screen with language switch (English / Spanish / Portuguese)
- [x] Credits screen
- [x] Add/remove current track from favorites via Spotify Web API
- [ ] Fully unify the local player UI with real Spotify state
- [ ] Standalone Spotify client: search/browse/playlists using the Web API

## Integration log: problems found and solved

Condensed log of the most important issues solved during Spotify Connect
client development (full technical detail in
[SPOTIFY_INTEGRATION.md](SPOTIFY_INTEGRATION.md)):

1. **Username/password login doesn't work: blocked by Spotify.**
   First tried `LoginBlob::loadUserPass` (username/password in a file). On
   real hardware, after a successful handshake, Spotify answered
   `Authorization declined`. Confirmed that Spotify deprecated and blocked
   password login for unofficial clients (librespot/cspot).
   **Solution**: pivoted to **Zeroconf/"tap to pair"** (the same mechanism
   Sonos/Chromecast uses): the Switch advertises itself over mDNS and
   exposes a tiny HTTP server (`/spotify_info`) that receives already-
   authenticated credentials from the official app.

2. **No `getifaddrs()` or embedded HTTP server in libnx.** There is no
   standard way to list network interfaces, and `civetweb` (the HTTP
   server bundled with `bell`) requires `sys/utsname.h`/`grp.h`/`pwd.h`,
   which are unavailable. **Solution**: local IP via
   `nifmGetCurrentIpAddress()` (libnx-specific API) and a minimal hand-
   written HTTP server (~150 lines, raw BSD sockets) only for the two
   routes we need.

3. **Immediate crash when registering the mDNS service.** `registerService()`
   returned a `unique_ptr` that was discarded, destroying the responder
   thread right after creation (use-after-free). **Solution**: store the
   handle in a global variable that lives as long as the app runs.

4. **Hard crash (no logs) using third-party `tinysvcmdns` for mDNS.**
   Suspected unaligned memory access on aarch64 parsing real DNS packets
   (casting structs over raw bytes). **Solution**: dropped that library for
   Switch and wrote a minimal custom mDNS responder that parses everything
   byte by byte without struct casts.

5. **Multicast `setsockopt` (`IP_ADD_MEMBERSHIP`) failed on real hardware.**
   **Solution**: made it non-fatal (log and continue) and added the missing
   `IP_MULTICAST_IF` before joining the group — libnx, unlike Linux, does
   not infer the interface by itself.

6. **Device never appeared in Spotify Connect list, even though `dns-sd -B`
   /`curl` always worked.** Several spec fixes (RFC 6762/6763) were applied
   in order to the custom mDNS responder: "cache-flush" bit on SRV/TXT/A
   records, correcting which records go in ANSWER vs ADDITIONAL, and
   support for the "QU" bit (unicast vs multicast replies). Each was
   spec-correct but none fixed the symptom alone.
   **Real cause found**: the hand-written HTTP server compared the request
   path with exact equality (`/spotify_info`), but Spotify appends a query
   string (`/spotify_info?action=getInfo`), so the comparison failed and it
   returned an empty response. **Solution**: cut the path at the first `?`
   before comparing.

7. **Startup crash in `render_init()` (black screen).** Two layers: (a)
   fonts and test songs were loaded from SD paths
   (`sdmc:/switch/mangospot/...`) instead of `romfs:/...`, and even after
   fixing that, `romfs` was **never embedded in the `.nro`** because the
   `Makefile` never defined `NROFLAGS` (the `switch_rules` rule that adds
   `--romfsdir` stayed empty); (b) once packaging was fixed, the included
   `.ttf` fonts turned out to be HTML pages (a failed download saved with
   `.ttf` extension), so `SDL_ttf` couldn't open them.
   **Solution**: stopped bundling our own fonts and switched to the
   **system shared font** via libnx's `pl` service
   (`plGetSharedFontByType`), the standard approach in homebrew (no
   licensing issues, always available).

8. **No way to debug hard crashes on hardware (black screen, no logs).**
   `nxlink` loses connection as soon as the app crashes. **Solution**:
   `applog_init()` redirects stdout/stderr to an SD file
   (`sdmc:/switch/mangospot/mangospot.log`), unbuffered, with a thread that
   `fflush`+`fsdevCommitDevice` every 100ms so the log survives even a
   fatal crash.

9. **Audio device conflict when connecting to real Spotify.** The local
   mock player opened audio with `Mix_OpenAudio()` at startup and never
   closed it; when authenticating with Spotify, `SwitchAudioSink` failed to
   open its own device (the Switch has only one physical audio output).
   **Solution**: close the mock player's audio (`Mix_HaltMusic()`+
   `Mix_CloseAudio()`) right before opening Spotify's real sink.

10. **"Now Playing" showed nothing from the real Spotify session.** The
    information (title/artist/album) was received and saved correctly from
    Spotify events, but no screen read it — the UI only knew the mock
    library. **Solution**: the "Now Playing" screen now checks the Spotify
    state first and only falls back to mock data if no session is
    connected; playback progress tracking (duration/position) was also
    added because Spotify does not emit a periodic progress event.

11. **Paired but stuck on "Connecting to MangoSpot"; the song never
    played.** After pairing, the session connected and the transport
    controls (play/pause/next) worked, but audio never started. Logs showed
    `apresolve status=200 body_len=0` and repeated `login5 ... body_len=0`
    → `Failed to fetch access token` → `Track failed to load`. The controls
    worked because they travel over the raw Mercury/Shannon channel; only
    the HTTPS REST calls (access-point resolve, access token, CDN URL) came
    back with an empty body. **Real cause**: an earlier local edit to
    `bell`'s `HTTPClient::readResponseHeaders()` had removed the loop that
    copies the parsed headers into the lookup vector, so
    `header("content-length")` always returned empty, the content length
    stayed 0, and every HTTPS response body was read as empty. Found by
    diffing the vendored `bell` submodule against its last commit.
    **Solution**: restored the header-population loop, and made the body
    reader loop until the full `Content-Length` has been received (a single
    TLS record can be smaller than the body).

12. **Pairing sometimes delivered an empty credentials blob / MangoSpot
    intermittently missing from the device list.** Two smaller issues found
    along the way: some Spotify clients send the `content-length` header in
    lowercase, which the hand-written POST parser didn't match (it read a
    zero-length body and `createFromBlob` threw a JSON parse error); and
    `nifmGetCurrentIpAddress()` can return 0 right at boot before Wi-Fi has
    an address, so the mDNS responder registered nothing.
    **Solution**: match the header case-insensitively, retry the IP lookup
    for a few seconds, and include the DNS-SD meta-PTR in an initial burst
    of proactive announcements so passive browsers discover the device.

13. **~20 seconds from pairing to first audio.** Each TLS handshake and
    HTTPS request was instrumented with timestamps. The data disproved the
    "software crypto is slow" hypothesis: an uncontended handshake was
    ~250 ms, but the *same* operation ballooned to 2–4 s during startup
    because cspot preloaded 5 upcoming tracks at once, saturating Wi-Fi and
    CPU while the first track was still opening its own stream (even plain
    TCP connect hit 2 s). libnx only exposes hardware AES/SHA (symmetric),
    not RSA/ECC, so crypto acceleration wouldn't help the handshake anyway.
    **Solution**: a two-phase rolling preload — only 2 tracks while the
    first one starts, then growing to 4 and refilling one at a time as
    tracks play — plus raising the track-load timeout so the first track
    loads on the first attempt instead of a wasteful skip+retry.

## Disclaimer

This project uses Spotify with Premium accounts for personal use only, in
the same spirit as `librespot`, `raspotify`, or `spotifyd`. It is not
affiliated with Spotify.
