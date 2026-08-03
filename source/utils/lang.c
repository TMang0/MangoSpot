#include <stddef.h>
#include "lang.h"

static Language current = LANG_EN;

static const char* lang_en[LK_COUNT] = {
    [LK_WELCOME_TO]                            = "Welcome to",
    [LK_CONNECT_FROM_SPOTIFY]                  = "Connect from Spotify",
    [LK_TO_START_PLAYING]                      = "to start playing",
    [LK_OPEN_DEVICES_MENU]                     = "Look for \"Connect to a device\"",
    [LK_PRESS_DOWN_TO_FOCUS_MINI_PLAYER]       = "Press [Down] to focus the mini player",
    [LK_AND_USE_A_PAUSE_L_R_SKIP]              = "and use [A] Pause / [L][R] Skip",
    [LK_OPEN_PLAYER]                           = "[Y] Open player",
    [LK_EXIT]                                  = "[+] Exit",
    [LK_NO_ACTIVE_PLAYBACK]                    = "No active playback",
    [LK_CONNECT_FROM_SPOTIFY_AND_SELECT_DEVICE]= "Connect from Spotify and select this device",
    [LK_BACK]                                  = "[B] Back",
    [LK_BACK_TO_LIBRARY]                       = "[B] Back to library",
    [LK_PREV_TRACK]                            = "<< [L/ZL]",
    [LK_NEXT_TRACK]                            = "[R/ZR] >>",
    [LK_PAUSE_ICON]                            = "||",
    [LK_PLAY_ICON]                             = ">",
    [LK_NOT_CONNECTED]                         = "Not connected",
    [LK_OPEN_SPOTIFY_CHOOSE_MANGOSPOT]         = "Open Spotify and choose MangoSpot",
    [LK_MINI_PLAYER_HINT]                      = "[Y] Open | [A] Play/Pause | [L/R] Skip",
    [LK_CHANGING_SONG]                         = "Changing song...",
    [LK_SETTINGS]                              = "Settings",
    [LK_LANGUAGE]                              = "Language",
    [LK_CREDITS]                               = "Credits",
    [LK_CREDITS_MADE_BY]                       = "Made by TMang0",
    [LK_CREDITS_WITH_LOVE]                     = "Made with lots of love :3",
    [LK_CREDITS_GITHUB]                        = "GitHub: github.com/tmang0",
    [LK_CREDITS_REPOS]                         = "Powered by cspot, SDL2, libnx and devkitPro",
    [LK_CREDITS_INSPIRATION]                   = "github.com/librespot-org/librespot (inspiration)",
    [LK_ENGLISH]                               = "English",
    [LK_SPANISH]                               = "Spanish",
    [LK_PORTUGUESE]                            = "Portuguese",
    [LK_ADD_TO_FAVORITES]                      = "[X] Add to favorites",
    [LK_REMOVE_FROM_FAVORITES]                 = "[X] Remove from favorites",
};

static const char* lang_es[LK_COUNT] = {
    [LK_WELCOME_TO]                            = "Bienvenido a",
    [LK_CONNECT_FROM_SPOTIFY]                  = "Con\u00e9ctate desde Spotify",
    [LK_TO_START_PLAYING]                      = "para empezar a reproducir",
    [LK_OPEN_DEVICES_MENU]                     = "Busca \"Conectar a un dispositivo\"",
    [LK_PRESS_DOWN_TO_FOCUS_MINI_PLAYER]       = "Presiona [Abajo] para enfocar el mini reproductor",
    [LK_AND_USE_A_PAUSE_L_R_SKIP]              = "y usar [A] Pausa / [L][R] Saltar",
    [LK_OPEN_PLAYER]                           = "[Y] Abrir reproductor",
    [LK_EXIT]                                  = "[+] Salir",
    [LK_NO_ACTIVE_PLAYBACK]                    = "No hay reproducci\u00f3n activa",
    [LK_CONNECT_FROM_SPOTIFY_AND_SELECT_DEVICE]= "Con\u00e9ctate desde Spotify y selecciona este dispositivo",
    [LK_BACK]                                  = "[B] Volver",
    [LK_BACK_TO_LIBRARY]                       = "[B] Volver a biblioteca",
    [LK_PREV_TRACK]                            = "<< [L/ZL]",
    [LK_NEXT_TRACK]                            = "[R/ZR] >>",
    [LK_PAUSE_ICON]                            = "||",
    [LK_PLAY_ICON]                             = ">",
    [LK_NOT_CONNECTED]                         = "No conectado",
    [LK_OPEN_SPOTIFY_CHOOSE_MANGOSPOT]         = "Abre Spotify y elige MangoSpot",
    [LK_MINI_PLAYER_HINT]                      = "[Y] Abrir | [A] Play/Pausa | [L/R] Saltar",
    [LK_CHANGING_SONG]                         = "Cambiando de canci\u00f3n...",
    [LK_SETTINGS]                              = "Ajustes",
    [LK_LANGUAGE]                              = "Idioma",
    [LK_CREDITS]                               = "Cr\u00e9ditos",
    [LK_CREDITS_MADE_BY]                       = "Hecho por TMang0",
    [LK_CREDITS_WITH_LOVE]                     = "Hecho con mucho amor :3",
    [LK_CREDITS_GITHUB]                        = "GitHub: github.com/tmang0",
    [LK_CREDITS_REPOS]                         = "Impulsado por cspot, SDL2, libnx y devkitPro",
    [LK_CREDITS_INSPIRATION]                   = "github.com/librespot-org/librespot (inspiraci\u00f3n)",
    [LK_ENGLISH]                               = "Ingl\u00e9s",
    [LK_SPANISH]                               = "Espa\u00f1ol",
    [LK_PORTUGUESE]                            = "Portugu\u00e9s",
    [LK_ADD_TO_FAVORITES]                      = "[X] Agregar a favoritos",
    [LK_REMOVE_FROM_FAVORITES]                 = "[X] Quitar de favoritos",
};

static const char* lang_pt[LK_COUNT] = {
    [LK_WELCOME_TO]                            = "Bem-vindo ao",
    [LK_CONNECT_FROM_SPOTIFY]                  = "Conecte-se pelo Spotify",
    [LK_TO_START_PLAYING]                      = "para come\u00e7ar a reproduzir",
    [LK_OPEN_DEVICES_MENU]                     = "Procure \"Conectar a um dispositivo\"",
    [LK_PRESS_DOWN_TO_FOCUS_MINI_PLAYER]       = "Pressione [Baixo] para focar o mini player",
    [LK_AND_USE_A_PAUSE_L_R_SKIP]              = "e use [A] Pausa / [L][R] Pular",
    [LK_OPEN_PLAYER]                           = "[Y] Abrir player",
    [LK_EXIT]                                  = "[+] Sair",
    [LK_NO_ACTIVE_PLAYBACK]                    = "Nenhuma reprodu\u00e7\u00e3o ativa",
    [LK_CONNECT_FROM_SPOTIFY_AND_SELECT_DEVICE]= "Conecte-se pelo Spotify e selecione este dispositivo",
    [LK_BACK]                                  = "[B] Voltar",
    [LK_BACK_TO_LIBRARY]                       = "[B] Voltar \u00e0 biblioteca",
    [LK_PREV_TRACK]                            = "<< [L/ZL]",
    [LK_NEXT_TRACK]                            = "[R/ZR] >>",
    [LK_PAUSE_ICON]                            = "||",
    [LK_PLAY_ICON]                             = ">",
    [LK_NOT_CONNECTED]                         = "N\u00e3o conectado",
    [LK_OPEN_SPOTIFY_CHOOSE_MANGOSPOT]         = "Abra o Spotify e escolha MangoSpot",
    [LK_MINI_PLAYER_HINT]                      = "[Y] Abrir | [A] Tocar/Pausar | [L/R] Pular",
    [LK_CHANGING_SONG]                         = "Mudando de m\u00fasica...",
    [LK_SETTINGS]                              = "Configura\u00e7\u00f5es",
    [LK_LANGUAGE]                              = "Idioma",
    [LK_CREDITS]                               = "Cr\u00e9ditos",
    [LK_CREDITS_MADE_BY]                       = "Feito por TMang0",
    [LK_CREDITS_WITH_LOVE]                     = "Feito com muito amor :3",
    [LK_CREDITS_GITHUB]                        = "GitHub: github.com/tmang0",
    [LK_CREDITS_REPOS]                         = "Desenvolvido com cspot, SDL2, libnx e devkitPro",
    [LK_CREDITS_INSPIRATION]                   = "github.com/librespot-org/librespot (inspira\u00e7\u00e3o)",
    [LK_ENGLISH]                               = "Ingl\u00eas",
    [LK_SPANISH]                               = "Espanhol",
    [LK_PORTUGUESE]                            = "Portugu\u00eas",
    [LK_ADD_TO_FAVORITES]                      = "[X] Adicionar aos favoritos",
    [LK_REMOVE_FROM_FAVORITES]                 = "[X] Remover dos favoritos",
};

void lang_set_language(Language lang) {
    if (lang >= 0 && lang < LANG_COUNT) {
        current = lang;
    }
}

Language lang_get_language(void) {
    return current;
}

const char* lang_get(LangKey key) {
    if (key < 0 || key >= LK_COUNT) {
        return "";
    }
    const char* s = NULL;
    switch (current) {
        case LANG_EN: s = lang_en[key]; break;
        case LANG_ES: s = lang_es[key]; break;
        case LANG_PT: s = lang_pt[key]; break;
        default:      s = lang_en[key]; break;
    }
    if (!s || !s[0]) {
        s = lang_en[key];
    }
    return s ? s : "";
}

const char* lang_name(Language lang) {
    switch (lang) {
        case LANG_EN: return lang_en[LK_ENGLISH];
        case LANG_ES: return lang_es[LK_SPANISH];
        case LANG_PT: return lang_pt[LK_PORTUGUESE];
        default:      return lang_en[LK_ENGLISH];
    }
}
