#include "InkCardsStorage.h"

#include <Logging.h>

namespace inkcards {

std::vector<std::string> listDeckFiles() {
  std::vector<std::string> out;
  if (!Storage.exists(kDecksDir)) {
    LOG_WARN("INK", "Decks directory %s not found", kDecksDir);
    return out;
  }

  // TODO(hardware-test): the FreeInk SDK exposes directory iteration through
  // HalStorage; the exact iterator type/method (openDir/next, or a callback
  // form) must be confirmed against the installed SDK version. The loop below
  // uses the callback listing form CrossPoint relies on. Verify on device and
  // adjust if the SDK differs; only this function is affected.
  Storage.listDir(kDecksDir, [&out](const char* name, bool isDir, size_t /*size*/) {
    if (isDir) return;
    std::string n(name);
    if (n.size() > 5 && n.compare(n.size() - 5, 5, ".deck") == 0) {
      out.push_back(std::string(kDecksDir) + "/" + n);
    }
  });

  return out;
}

}  // namespace inkcards
