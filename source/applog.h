#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Redirects stdout/stderr to sdmc:/mangospot.log (unbuffered) and starts a
// background thread that commits the file to the SD card every ~100 ms, so the
// log survives even a hard crash. Call this as the very first thing in main().
void applog_init(void);

// Force-flush + commit the log to the SD card right now.
void applog_flush(void);

#ifdef __cplusplus
}
#endif
