#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  char title[256];
  char artist[256];
  char album[256];
  int is_playing;   // 1 if actively playing, 0 if paused/stopped
  int is_connected; // 1 once login + session are up
  uint32_t duration_ms;  // current track duration, from cspot's TrackInfo
  float position_secs;   // locally-tracked elapsed playback position
} SpotifyNowPlaying;

// Starts Spotify Connect via Zeroconf ("tap to pair") discovery: advertises
// this device over mDNS and runs a tiny local HTTP endpoint that the
// official Spotify app posts credentials to once you pick this device from
// its Connect device list. (Plain username/password login is deprecated and
// blocked by Spotify's servers, so this is the only login method cspot
// supports that still works.) Spins up background threads for pairing,
// networking, and audio playback, and returns immediately (0 on success -
// meaning "listening for pairing", not "already connected").
int spotify_client_start(void);

// Fills `out` with the most recently known playback state. Safe to call
// every frame; does not block.
void spotify_client_get_now_playing(SpotifyNowPlaying* out);

// Advances the locally-tracked playback position by `delta` seconds while a
// track is actively playing (cspot only emits track-change/seek/pause
// events, never periodic position updates, so the UI has to extrapolate
// elapsed time itself). Call once per frame from the main loop. No-op when
// not connected or not playing.
void spotify_client_advance_playback(float delta);

// Real playback control for the actual connected Spotify Connect session
// (as opposed to the local mock-library demo player). These send SPIRC
// commands upstream so the official Spotify app's UI also stays in sync.
// All are no-ops if not yet connected/authenticated.
void spotify_client_toggle_pause(void);
void spotify_client_next(void);
void spotify_client_prev(void);

#ifdef __cplusplus
}
#endif
