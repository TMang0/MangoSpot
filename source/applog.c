#include "applog.h"

#include <switch.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

#define LOG_DIR  "sdmc:/switch/mangospot"
#define LOG_PATH LOG_DIR "/mangospot.log"

static int g_ready = 0;
static Thread g_flusher;
static volatile bool g_run = false;

static void flusher_thread(void* arg) {
    (void)arg;
    while (g_run) {
        fflush(stdout);
        fsdevCommitDevice("sdmc");
        svcSleepThread(100000000ULL);  // 100 ms
    }
}

void applog_init(void) {
    if (g_ready) return;

    // Make sure the app folder exists (ignore "already exists" errors).
    mkdir("sdmc:/switch", 0777);
    mkdir(LOG_DIR, 0777);

    // Redirect stdout to the log file. Everything that uses printf (our code,
    // bell's BELL_LOG, cspot) then lands in the file.
    if (freopen(LOG_PATH, "w", stdout) == NULL) {
        return;
    }
    setvbuf(stdout, NULL, _IONBF, 0);  // no buffering: write each byte through

    // Point stderr at the same file descriptor.
    dup2(fileno(stdout), fileno(stderr));
    setvbuf(stderr, NULL, _IONBF, 0);

    g_ready = 1;
    g_run = true;

    if (R_SUCCEEDED(threadCreate(&g_flusher, flusher_thread, NULL, NULL,
                                 0x2000, 0x3B, -2))) {
        threadStart(&g_flusher);
    }

    printf("=== mangospot log start ===\n");
    applog_flush();
}

void applog_flush(void) {
    if (!g_ready) return;
    fflush(stdout);
    fsdevCommitDevice("sdmc");
}
