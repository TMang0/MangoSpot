#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  char title[256];
  char artist[256];
  char album[256];
  int is_playing;   // 1 if actively playing, 0 if paused/stopped
  int is_connected; // 1 once login + session are up
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

#ifdef __cplusplus
}
#endif
