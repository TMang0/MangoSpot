#include "SwitchAudioSink.h"

#include <cstdio>

SwitchAudioSink::SwitchAudioSink() {
  openDevice();
}

SwitchAudioSink::~SwitchAudioSink() {
  if (deviceId) {
    SDL_CloseAudioDevice(deviceId);
    deviceId = 0;
  }
}

void SwitchAudioSink::openDevice() {
  if (deviceId) {
    SDL_CloseAudioDevice(deviceId);
    deviceId = 0;
  }

  SDL_AudioSpec want, have;
  SDL_zero(want);
  want.freq = sampleRate;
  want.format = AUDIO_S16LSB;
  want.channels = channelCount;
  want.samples = 4096;
  want.callback = nullptr;  // push model: we call SDL_QueueAudio ourselves

  deviceId = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
  if (deviceId == 0) {
    printf("SwitchAudioSink: SDL_OpenAudioDevice failed: %s\n",
           SDL_GetError());
    return;
  }

  // Start the device immediately; SDL_QueueAudio silently no-ops until then.
  SDL_PauseAudioDevice(deviceId, 0);
}

bool SwitchAudioSink::setParams(uint32_t newSampleRate,
                                uint8_t newChannelCount,
                                uint8_t newBitDepth) {
  if (newBitDepth != 16) {
    // Only 16-bit PCM is supported for now.
    return false;
  }

  if (newSampleRate == sampleRate && newChannelCount == channelCount &&
      deviceId != 0) {
    return true;
  }

  sampleRate = newSampleRate;
  channelCount = newChannelCount;
  bitDepth = newBitDepth;
  openDevice();
  return deviceId != 0;
}

void SwitchAudioSink::feedPCMFrames(const uint8_t* buffer, size_t bytes) {
  if (!deviceId) return;

  // Basic backpressure so a stalled/slow consumer doesn't grow the queue
  // without bound: cap it at ~2 seconds of audio.
  const Uint32 maxQueuedBytes = sampleRate * channelCount * 2 /* bytes/sample */ * 2 /* seconds */;
  while (SDL_GetQueuedAudioSize(deviceId) > maxQueuedBytes) {
    SDL_Delay(5);
  }

  SDL_QueueAudio(deviceId, buffer, (Uint32)bytes);
}
