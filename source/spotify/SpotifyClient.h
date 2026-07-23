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

// Reads credentials from sdmc:/switch/spotiswitch/login.txt (line 1:
// username, line 2: password), logs into Spotify, and spins up background
// threads for the network session and audio playback. This is a blocking
// call (it performs the network handshake before returning) meant to be
// called once at startup.
//
// Returns 0 on success, non-zero on failure (missing/invalid credentials
// file, auth failure, network error).
int spotify_client_start(void);

// Fills `out` with the most recently known playback state. Safe to call
// every frame; does not block.
void spotify_client_get_now_playing(SpotifyNowPlaying* out);

#ifdef __cplusplus
}
#endif
