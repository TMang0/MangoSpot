#pragma once

#include <cstddef>
#include <cstdint>

#include "AudioSink.h"
#include <SDL2/SDL.h>

// Feeds PCM audio coming from cspot into its own SDL2 audio device (kept
// separate from SDL2_mixer's device used for local mock playback).
class SwitchAudioSink : public AudioSink {
 public:
  SwitchAudioSink();
  ~SwitchAudioSink() override;

  void feedPCMFrames(const uint8_t* buffer, size_t bytes) override;
  bool setParams(uint32_t sampleRate, uint8_t channelCount,
                 uint8_t bitDepth) override;

 private:
  void openDevice();

  SDL_AudioDeviceID deviceId = 0;
  uint32_t sampleRate = 44100;
  uint8_t channelCount = 2;
  uint8_t bitDepth = 16;
};
