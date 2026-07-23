#include "SpotifyClient.h"

#include <atomic>
#include <cstdio>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>

#include "BellDSP.h"
#include "BellLogger.h"
#include "BellTask.h"
#include "BellUtils.h"
#include "CSpotContext.h"
#include "CentralAudioBuffer.h"
#include "LoginBlob.h"
#include "SpircHandler.h"
#include "StreamInfo.h"
#include "SwitchAudioSink.h"
#include "TrackPlayer.h"
#include "TrackQueue.h"

namespace {

constexpr const char* kCredentialsPath = "sdmc:/switch/mangospot/login.txt";

std::mutex gNowPlayingMutex;
SpotifyNowPlaying gNowPlaying = {};

void setNowPlayingTrack(const cspot::TrackInfo& track) {
  std::scoped_lock lock(gNowPlayingMutex);
  snprintf(gNowPlaying.title, sizeof(gNowPlaying.title), "%s",
           track.name.c_str());
  snprintf(gNowPlaying.artist, sizeof(gNowPlaying.artist), "%s",
           track.artist.c_str());
  snprintf(gNowPlaying.album, sizeof(gNowPlaying.album), "%s",
           track.album.c_str());
}

// Pumps the raw network connection: reads + decrypts packets off the wire.
// Mirrors upstream cspot's CLI target, just moved onto its own thread instead
// of blocking the caller (we can't block the main render/input loop).
class SessionPumpTask : public bell::Task {
 public:
  explicit SessionPumpTask(std::shared_ptr<cspot::MercurySession> session)
      : bell::Task("spotify_session", 1024 * 16, 0, 0),
        session(std::move(session)) {
    startTask();
  }

  void runTask() override {
    while (running) {
      session->handlePacket();
    }
  }

  std::atomic<bool> running = true;

 private:
  std::shared_ptr<cspot::MercurySession> session;
};

// Pumps decoded PCM from cspot's central audio buffer into our AudioSink.
// Mirrors upstream cspot's CliPlayer.
class PlayerPumpTask : public bell::Task {
 public:
  PlayerPumpTask(std::unique_ptr<AudioSink> sink,
                 std::shared_ptr<cspot::SpircHandler> handlerIn)
      : bell::Task("spotify_player", 1024 * 16, 0, 1),
        handler(std::move(handlerIn)),
        audioSink(std::move(sink)) {
    centralAudioBuffer =
        std::make_shared<bell::CentralAudioBuffer>(128 * 1024);
    dsp = std::make_shared<bell::BellDSP>(centralAudioBuffer);

    handler->getTrackPlayer()->setDataCallback(
        [this](uint8_t* data, size_t bytes, std::string_view trackId) {
          auto hash = std::hash<std::string_view>()(trackId);
          return centralAudioBuffer->writePCM(data, bytes, hash);
        });

    handler->setEventHandler(
        [this](std::unique_ptr<cspot::SpircHandler::Event> event) {
          switch (event->eventType) {
            case cspot::SpircHandler::EventType::PLAY_PAUSE: {
              bool paused = std::get<bool>(event->data);
              isPaused = paused;
              std::scoped_lock lock(gNowPlayingMutex);
              gNowPlaying.is_playing = paused ? 0 : 1;
              break;
            }
            case cspot::SpircHandler::EventType::TRACK_INFO:
              setNowPlayingTrack(std::get<cspot::TrackInfo>(event->data));
              break;
            case cspot::SpircHandler::EventType::FLUSH:
            case cspot::SpircHandler::EventType::DISC:
            case cspot::SpircHandler::EventType::SEEK:
              centralAudioBuffer->clearBuffer();
              break;
            case cspot::SpircHandler::EventType::PLAYBACK_START:
              isPaused = false;
              playlistEnd = false;
              centralAudioBuffer->clearBuffer();
              {
                std::scoped_lock lock(gNowPlayingMutex);
                gNowPlaying.is_playing = 1;
                gNowPlaying.is_connected = 1;
              }
              break;
            case cspot::SpircHandler::EventType::DEPLETED:
              playlistEnd = true;
              break;
            default:
              break;
          }
        });

    startTask();
  }

  void runTask() override {
    while (running) {
      if (isPaused) {
        BELL_SLEEP_MS(10);
        continue;
      }

      auto chunk = centralAudioBuffer->readChunk();
      if (!chunk || chunk->pcmSize == 0) {
        if (playlistEnd) {
          handler->notifyAudioEnded();
          playlistEnd = false;
        }
        BELL_SLEEP_MS(10);
        continue;
      }

      if (lastHash != chunk->trackHash) {
        lastHash = chunk->trackHash;
        handler->notifyAudioReachedPlayback();
      }

      dsp->process(chunk->pcmData, chunk->pcmSize, 2, 44100,
                   bell::BitWidth::BW_16);
      audioSink->feedPCMFrames(chunk->pcmData, chunk->pcmSize);
    }
  }

  std::atomic<bool> running = true;

 private:
  std::shared_ptr<cspot::SpircHandler> handler;
  std::unique_ptr<AudioSink> audioSink;
  std::shared_ptr<bell::CentralAudioBuffer> centralAudioBuffer;
  std::shared_ptr<bell::BellDSP> dsp;
  std::atomic<bool> isPaused = true;
  std::atomic<bool> playlistEnd = false;
  size_t lastHash = 0;
};

std::shared_ptr<cspot::Context> gContext;
std::shared_ptr<cspot::SpircHandler> gHandler;
std::unique_ptr<SessionPumpTask> gSessionPump;
std::unique_ptr<PlayerPumpTask> gPlayerPump;

bool readCredentials(std::string& username, std::string& password) {
  std::ifstream file(kCredentialsPath);
  if (!file.is_open()) {
    printf("SpotifyClient: could not open %s\n", kCredentialsPath);
    printf(
        "SpotifyClient: create it on the SD card with your Spotify username "
        "on line 1 and password on line 2\n");
    return false;
  }

  if (!std::getline(file, username) || !std::getline(file, password) ||
      username.empty() || password.empty()) {
    printf(
        "SpotifyClient: %s must have username on line 1, password on line "
        "2\n",
        kCredentialsPath);
    return false;
  }

  return true;
}

}  // namespace

int spotify_client_start(void) {
  bell::setDefaultLogger();

  std::string username, password;
  if (!readCredentials(username, password)) {
    return 1;
  }

  auto loginBlob = std::make_shared<cspot::LoginBlob>("MangoSpot");
  loginBlob->loadUserPass(username, password);

  gContext = cspot::Context::createFromBlob(loginBlob);

  printf("SpotifyClient: connecting to a Spotify access point...\n");
  gContext->session->connectWithRandomAp();
  gContext->config.authData = gContext->session->authenticate(loginBlob);

  if (gContext->config.authData.empty()) {
    printf("SpotifyClient: authentication failed (check credentials)\n");
    return 2;
  }

  printf("SpotifyClient: authenticated OK, starting session + player\n");
  gHandler = std::make_shared<cspot::SpircHandler>(gContext);
  gContext->session->startTask();
  gSessionPump = std::make_unique<SessionPumpTask>(gContext->session);

  auto sink = std::make_unique<SwitchAudioSink>();
  sink->setParams(44100, 2, 16);
  gPlayerPump = std::make_unique<PlayerPumpTask>(std::move(sink), gHandler);

  {
    std::scoped_lock lock(gNowPlayingMutex);
    gNowPlaying.is_connected = 1;
  }

  printf(
      "SpotifyClient: ready - open Spotify on your phone/PC and pick "
      "'%s' from the Connect device list\n",
      loginBlob->getDeviceName().c_str());

  return 0;
}

void spotify_client_get_now_playing(SpotifyNowPlaying* out) {
  std::scoped_lock lock(gNowPlayingMutex);
  *out = gNowPlaying;
}
