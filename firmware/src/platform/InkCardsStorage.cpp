#include "InkCardsStorage.h"

#include <Logging.h>

#include <inkkit/Storage.h>

namespace inkcards {

std::vector<std::string> listDeckFiles() {
  std::vector<std::string> out;
  if (!inkkit::sd::exists(kDecksDir)) {
    LOG_WARN("INK", "Decks directory %s not found", kDecksDir);
    return out;
  }
  // Directory iteration and the ".deck" filter now live in inkkit::sd::listFiles.
  inkkit::sd::listFiles(kDecksDir, ".deck",
                        [&out](const std::string& path) { out.push_back(path); });
  return out;
}

}  // namespace inkcards
