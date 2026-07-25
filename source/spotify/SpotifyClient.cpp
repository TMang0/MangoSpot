#include "SpotifyClient.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <atomic>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>

#include <SDL2/SDL_mixer.h>

#include "BellDSP.h"
#include "BellLogger.h"
#include "BellTask.h"
#include "BellUtils.h"
#include "CSpotContext.h"
#include "CentralAudioBuffer.h"
#include "LoginBlob.h"
#include "MDNSService.h"
#include "SpircHandler.h"
#include "StreamInfo.h"
#include "SwitchAudioSink.h"
#include "TrackPlayer.h"
#include "TrackQueue.h"

namespace {

// Spotify's official apps discover Connect receivers via mDNS
// (_spotify-connect._tcp) and then hit this HTTP endpoint on the
// advertised port to hand over already-authenticated credentials. Plain
// username/password login is deprecated/blocked by Spotify's servers (see
// librespot's own docs), so this Zeroconf "tap to pair" flow is the only
// login method cspot actually supports that still works.
constexpr int kZeroconfPort = 7864;

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
  gNowPlaying.duration_ms = track.duration;
  gNowPlaying.position_secs = 0.0f;
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
      try {
        session->handlePacket();
      } catch (const std::exception& e) {
        printf("SpotifyClient: session pump exception: %s\n", e.what());
        BELL_SLEEP_MS(50);
      } catch (...) {
        printf("SpotifyClient: session pump unknown exception\n");
        BELL_SLEEP_MS(50);
      }
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
            case cspot::SpircHandler::EventType::SEEK: {
              int positionMs = std::get<int>(event->data);
              {
                std::scoped_lock lock(gNowPlayingMutex);
                gNowPlaying.position_secs = positionMs / 1000.0f;
              }
              centralAudioBuffer->clearBuffer();
              break;
            }
            case cspot::SpircHandler::EventType::FLUSH:
            case cspot::SpircHandler::EventType::DISC:
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
                gNowPlaying.position_secs = 0.0f;
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
      try {
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
      } catch (const std::exception& e) {
        printf("SpotifyClient: player pump exception: %s\n", e.what());
        BELL_SLEEP_MS(50);
      } catch (...) {
        printf("SpotifyClient: player pump unknown exception\n");
        BELL_SLEEP_MS(50);
      }
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

// Minimal single-endpoint HTTP server for Spotify's Zeroconf pairing
// handshake (GET returns device info, POST delivers the credentials blob).
// We deliberately don't use bell's own BellHTTPServer/civetweb: civetweb
// assumes a much heavier POSIX environment (grp.h, pwd.h, sys/wait.h, ...)
// that devkitA64/libnx doesn't provide, and porting it isn't worth it for
// the one tiny endpoint we actually need.
class ZeroconfHttpServer : public bell::Task {
 public:
  ZeroconfHttpServer(std::shared_ptr<cspot::LoginBlob> blobIn, int portIn)
      : bell::Task("spotify_zeroconf_http", 1024 * 16, 0, 0),
        blob(std::move(blobIn)),
        port(portIn) {
    startTask();
  }

  std::atomic<bool> paired = false;
  std::atomic<bool> running = true;

  void runTask() override {
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
      printf("SpotifyClient: zeroconf socket() failed\n");
      return;
    }

    int yes = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(serverFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
      printf("SpotifyClient: zeroconf bind() failed on port %d\n", port);
      close(serverFd);
      return;
    }

    if (listen(serverFd, 4) < 0) {
      printf("SpotifyClient: zeroconf listen() failed\n");
      close(serverFd);
      return;
    }

    printf("SpotifyClient: zeroconf HTTP server listening on port %d\n",
           port);

    while (running) {
      struct sockaddr_in clientAddr {};
      socklen_t clientLen = sizeof(clientAddr);
      int clientFd =
          accept(serverFd, (struct sockaddr*)&clientAddr, &clientLen);
      if (clientFd < 0) continue;

      try {
        handleConnection(clientFd);
      } catch (const std::exception& e) {
        printf("SpotifyClient: zeroconf request exception: %s\n", e.what());
      } catch (...) {
        printf("SpotifyClient: zeroconf request unknown exception\n");
      }
      close(clientFd);
    }

    close(serverFd);
  }

 private:
  std::shared_ptr<cspot::LoginBlob> blob;
  int port;

  static std::string urlDecode(const std::string& in) {
    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); i++) {
      if (in[i] == '+') {
        out += ' ';
      } else if (in[i] == '%' && i + 2 < in.size()) {
        int value = 0;
        std::sscanf(in.substr(i + 1, 2).c_str(), "%x", &value);
        out += static_cast<char>(value);
        i += 2;
      } else {
        out += in[i];
      }
    }
    return out;
  }

  static std::map<std::string, std::string> parseFormBody(
      const std::string& body) {
    std::map<std::string, std::string> result;
    size_t pos = 0;
    while (pos < body.size()) {
      size_t amp = body.find('&', pos);
      std::string pair = body.substr(
          pos, amp == std::string::npos ? std::string::npos : amp - pos);
      size_t eq = pair.find('=');
      if (eq != std::string::npos) {
        result[urlDecode(pair.substr(0, eq))] =
            urlDecode(pair.substr(eq + 1));
      }
      if (amp == std::string::npos) break;
      pos = amp + 1;
    }
    return result;
  }

  void sendJson(int fd, const std::string& body) {
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: application/json\r\n";
    response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    response += "Connection: close\r\n\r\n";
    response += body;
    send(fd, response.data(), response.size(), 0);
  }

  void handleConnection(int fd) {
    std::string request;
    char buf[4096];
    size_t headerEnd = std::string::npos;

    while (headerEnd == std::string::npos) {
      ssize_t n = recv(fd, buf, sizeof(buf), 0);
      if (n <= 0) return;
      request.append(buf, n);
      headerEnd = request.find("\r\n\r\n");
      if (request.size() > 32768) return;  // safety cap
    }

    size_t lineEnd = request.find("\r\n");
    std::string requestLine = request.substr(0, lineEnd);
    size_t methodEnd = requestLine.find(' ');
    size_t pathEnd =
        methodEnd == std::string::npos
            ? std::string::npos
            : requestLine.find(' ', methodEnd + 1);
    if (methodEnd == std::string::npos || pathEnd == std::string::npos)
      return;

    std::string method = requestLine.substr(0, methodEnd);
    std::string fullPath =
        requestLine.substr(methodEnd + 1, pathEnd - methodEnd - 1);
    // Real HTTP clients are allowed (RFC 7230 origin-form) to append a query
    // string to the request target, e.g. "/spotify_info?action=getInfo".
    // cspot's reference Linux/ESP32 targets never hit this because their
    // BellHTTPServer is backed by civetweb/mongoose, which separates the
    // path from the query string before route matching. Our hand-rolled
    // router compared the raw request-target verbatim, so any query string
    // made the exact match fail and silently fell through to the empty "{}"
    // stub - indistinguishable from "the client never asked" in our own
    // logs, and never caught by manual curl tests that happened to omit a
    // query string.
    std::string path = fullPath.substr(0, fullPath.find('?'));

    if (path != "/spotify_info") {
      sendJson(fd, "{}");
      return;
    }

    if (method == "GET") {
      sendJson(fd, blob->buildZeroconfInfo());
      return;
    }

    if (method == "POST") {
      size_t clPos = request.find("Content-Length:");
      size_t contentLength = 0;
      if (clPos != std::string::npos) {
        contentLength = std::strtoul(request.c_str() + clPos + 15, nullptr, 10);
      }

      std::string body = request.substr(headerEnd + 4);
      while (body.size() < contentLength) {
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n <= 0) break;
        body.append(buf, n);
      }

      auto queryMap = parseFormBody(body);
      blob->loadZeroconfQuery(queryMap);
      paired = true;

      sendJson(fd,
               "{\"status\":101,\"spotifyError\":0,\"statusString\":\"ERROR-"
               "OK\"}");
      return;
    }

    sendJson(fd, "{}");
  }
};

// Once ZeroconfHttpServer::paired flips (the phone posted valid
// credentials), do the actual (blocking, potentially slow) connect+auth on
// its own thread rather than inside the HTTP request handler, so the HTTP
// response back to the phone stays fast.
class LoginCompletionTask : public bell::Task {
 public:
  LoginCompletionTask(std::shared_ptr<cspot::LoginBlob> blobIn,
                      ZeroconfHttpServer* serverIn)
      : bell::Task("spotify_login_wait", 1024 * 16, 0, 0),
        blob(std::move(blobIn)),
        server(serverIn) {
    startTask();
  }

  void runTask() override {
    while (!server->paired) {
      BELL_SLEEP_MS(200);
    }

    try {
      gContext = cspot::Context::createFromBlob(blob);

      printf("SpotifyClient: paired! connecting to a Spotify access point...\n");
      gContext->session->connectWithRandomAp();
      gContext->config.authData = gContext->session->authenticate(blob);

      if (gContext->config.authData.empty()) {
        printf("SpotifyClient: authentication failed after pairing\n");
        return;
      }

      printf("SpotifyClient: authenticated OK, starting session + player\n");
      gHandler = std::make_shared<cspot::SpircHandler>(gContext);
      gContext->session->startTask();
      gSessionPump = std::make_unique<SessionPumpTask>(gContext->session);

      // The Switch only has one physical audio output stream. SDL_mixer's
      // Mix_OpenAudio (opened at startup in main.c, used by the local
      // mock-library demo player) already holds it, so SwitchAudioSink's
      // own SDL_OpenAudioDevice() would otherwise fail with "Audio device
      // already open" right here - real Spotify playback wins once we're
      // actually paired and authenticated.
      Mix_HaltMusic();
      Mix_CloseAudio();

      auto sink = std::make_unique<SwitchAudioSink>();
      sink->setParams(44100, 2, 16);
      gPlayerPump = std::make_unique<PlayerPumpTask>(std::move(sink), gHandler);

      std::scoped_lock lock(gNowPlayingMutex);
      gNowPlaying.is_connected = 1;
    } catch (const std::exception& e) {
      printf("SpotifyClient: exception after pairing: %s\n", e.what());
    } catch (...) {
      printf("SpotifyClient: unknown exception after pairing\n");
    }
  }

 private:
  std::shared_ptr<cspot::LoginBlob> blob;
  ZeroconfHttpServer* server;
};

std::unique_ptr<ZeroconfHttpServer> gZeroconfServer;
std::unique_ptr<LoginCompletionTask> gLoginCompletionTask;
// Must outlive the function: registerService() returns a handle that owns
// the mDNS responder thread. If discarded, the thread is destroyed
// immediately on startup and the app hard-crashes with no logs.
std::unique_ptr<bell::MDNSService> gMdnsService;

}  // namespace

int spotify_client_start(void) {
  // Everything here can throw (LoginBlob/JSON/crypto/networking all use
  // exceptions, which we need enabled for cspot). This function is called
  // from plain C (main.c) through an `extern "C"` boundary, and letting a
  // C++ exception escape across that boundary is undefined behavior - it's
  // the most likely cause of a hard crash instead of a clean error message.
  try {
    bell::setDefaultLogger();

    auto loginBlob = std::make_shared<cspot::LoginBlob>("MangoSpot");

    gZeroconfServer =
        std::make_unique<ZeroconfHttpServer>(loginBlob, kZeroconfPort);

    gMdnsService = bell::MDNSService::registerService(
        loginBlob->getDeviceName(), "_spotify-connect", "_tcp", "",
        kZeroconfPort,
        {{"VERSION", "1.0"}, {"CPath", "/spotify_info"}, {"Stack", "SP"}});

    gLoginCompletionTask = std::make_unique<LoginCompletionTask>(
        loginBlob, gZeroconfServer.get());

    printf(
        "SpotifyClient: waiting for pairing - open Spotify on your phone/PC "
        "and pick '%s' from the Connect device list\n",
        loginBlob->getDeviceName().c_str());

    return 0;
  } catch (const std::exception& e) {
    printf("SpotifyClient: fatal exception during startup: %s\n", e.what());
    return 3;
  } catch (...) {
    printf("SpotifyClient: unknown fatal exception during startup\n");
    return 3;
  }
}

void spotify_client_get_now_playing(SpotifyNowPlaying* out) {
  std::scoped_lock lock(gNowPlayingMutex);
  *out = gNowPlaying;
}

void spotify_client_advance_playback(float delta) {
  std::scoped_lock lock(gNowPlayingMutex);
  if (!gNowPlaying.is_connected || !gNowPlaying.is_playing) return;
  gNowPlaying.position_secs += delta;
  float durationSecs = gNowPlaying.duration_ms / 1000.0f;
  if (durationSecs > 0 && gNowPlaying.position_secs > durationSecs) {
    gNowPlaying.position_secs = durationSecs;
  }
}

void spotify_client_toggle_pause(void) {
  if (!gHandler) return;
  bool currentlyPlaying;
  {
    std::scoped_lock lock(gNowPlayingMutex);
    if (!gNowPlaying.is_connected) return;
    currentlyPlaying = gNowPlaying.is_playing != 0;
  }
  // setPause(true) pauses, setPause(false) resumes - so pass whether we're
  // currently playing to flip it. This synchronously fires the PLAY_PAUSE
  // event back through our handler, which updates gNowPlaying.is_playing.
  gHandler->setPause(currentlyPlaying);
}

void spotify_client_next(void) {
  if (gHandler) gHandler->nextSong();
}

void spotify_client_prev(void) {
  if (gHandler) gHandler->previousSong();
}
