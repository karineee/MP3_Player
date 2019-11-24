#pragma once
#include "FreeRTOS.h"

#include "queue.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef char name[32];

extern QueueHandle_t Song_Q;

typedef struct {
  bool inPlay, inPause, noSong
} song_status;

void playback_status_init(void);

bool playback__is_paused(void);
bool playback__is_playing(void);
bool playback__no_song(void);

void playback__set_playing();
void playback__clear_playing();

void playback__set_pause();
void playback__clear_pause();

void playback__toggle_pause();