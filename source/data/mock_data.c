#include "mock_data.h"
#include <stddef.h>

Album library[] = {
    {
        .name        = "Random Access Memories",
        .artist      = "Daft Punk",
        .year        = 2013,
        .track_count = 5,
        .tracks = {
            { "Give Life Back to Music", "Daft Punk", "Random Access Memories", 274,
              "romfs:/songs/give_life_back_to_music.mp3" },
            { "The Game of Love",        "Daft Punk", "Random Access Memories", 279,
              "romfs:/songs/the_game_of_love.mp3" },
            { "Giorgio by Moroder",      "Daft Punk", "Random Access Memories", 544, NULL },
            { "Within",                  "Daft Punk", "Random Access Memories", 230, NULL },
            { "Instant Crush",           "Daft Punk", "Random Access Memories", 337, NULL },
        }
    },
    {
        .name        = "Discovery",
        .artist      = "Daft Punk",
        .year        = 2001,
        .track_count = 5,
        .tracks = {
            { "One More Time",   "Daft Punk", "Discovery", 320, NULL },
            { "Aerodynamic",     "Daft Punk", "Discovery", 212, NULL },
            { "Digital Love",    "Daft Punk", "Discovery", 301, NULL },
            { "Harder Better",   "Daft Punk", "Discovery", 224, NULL },
            { "Something About", "Daft Punk", "Discovery", 230, NULL },
        }
    },
    {
        .name        = "Currents",
        .artist      = "Tame Impala",
        .year        = 2015,
        .track_count = 5,
        .tracks = {
            { "Let It Happen",    "Tame Impala", "Currents", 467, NULL },
            { "Nangs",            "Tame Impala", "Currents", 114, NULL },
            { "The Moment",       "Tame Impala", "Currents", 268, NULL },
            { "Yes I'm Changing", "Tame Impala", "Currents", 302, NULL },
            { "Eventually",       "Tame Impala", "Currents", 319, NULL },
        }
    },
};

int library_count = 3;