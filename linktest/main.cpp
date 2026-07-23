// Minimal link-test: not the real app, just proves that cspot/bell (with the
// new Switch platform shim) resolve all symbols when something actually
// constructs a session (LoginBlob -> Context -> SpircHandler), which pulls in
// TrackPlayer/TrackQueue/CDNAudioFile (WrappedSemaphore users), MercurySession,
// the Shannon handshake, etc.
#include <memory>

#include "AudioSink.h"
#include "CSpotContext.h"
#include "LoginBlob.h"
#include "SpircHandler.h"

class SwitchAudioSinkStub : public AudioSink {
 public:
  void feedPCMFrames(const uint8_t* buffer, size_t bytes) override {
    // Real SDL2 output comes later; this stub only exists to prove linkage.
  }
};

int main() {
  auto blob = std::make_shared<cspot::LoginBlob>("spotiswitch");
  blob->loadUserPass("user", "pass");

  auto ctx = cspot::Context::createFromBlob(blob);
  auto handler = std::make_shared<cspot::SpircHandler>(ctx);

  SwitchAudioSinkStub sink;
  (void)handler;
  (void)sink;
  return 0;
}
