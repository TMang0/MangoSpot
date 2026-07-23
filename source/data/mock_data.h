#pragma once

#define MAX_TRACKS 10

typedef struct {
    const char* title;
    const char* artist;
    const char* album;
    int         duration_secs;
    const char* file_path;
} Song;

typedef struct {
    const char* name;
    const char* artist;
    int         year;
    Song        tracks[MAX_TRACKS];
    int         track_count;
} Album;

extern Album library[];
extern int   library_count;