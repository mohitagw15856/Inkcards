// SD-card path helpers and deck/state load-save glue for the device. Uses the
// FreeInk SDK Storage API. Paths mirror docs/FORMAT.md.
#pragma once

#include <HalStorage.h>

#include <string>
#include <vector>

#include "DeckReader.h"
#include "Fnv.h"
#include "ReviewState.h"
#include "SdByteStream.h"

namespace inkcards {

constexpr char kDecksDir[] = "/inkcards/decks";
constexpr char kStateDir[] = "/inkcards/state";

inline std::string statePathForDeckName(const std::string& deckName) {
  return std::string(kStateDir) + "/" + deckIdHex(deckName) + ".rev";
}

// Ensure the state directory exists (decks dir is created by the user copying
// decks across). Safe to call repeatedly.
inline void ensureStateDir() {
  if (!Storage.exists("/inkcards")) Storage.mkdir("/inkcards");
  if (!Storage.exists(kStateDir)) Storage.mkdir(kStateDir);
}

// List *.deck files under the decks directory.
//
// TODO(hardware-test): confirm the directory-listing API of the installed
// freeink-sdk. CrossPoint iterates directories via HalStorage; the concrete
// call is filled in here against that surface and should be verified on device.
std::vector<std::string> listDeckFiles();

// Load a deck's review state from SD, or start fresh if no file exists yet.
inline void loadReviewState(const std::string& deckName, ReviewStore& store) {
  store.reset();
  std::string path = statePathForDeckName(deckName);
  if (!Storage.exists(path.c_str())) return;
  HalFile file;
  if (!Storage.openFileForRead("INK", path.c_str(), file)) return;
  SdFileReader reader(file);
  store.load(reader);  // on failure the store stays reset
  file.close();
}

// Persist a deck's review state to SD.
inline bool saveReviewState(const std::string& deckName, const ReviewStore& store) {
  ensureStateDir();
  std::string path = statePathForDeckName(deckName);
  HalFile file;
  if (!Storage.openFileForWrite("INK", path.c_str(), file)) return false;
  SdFileWriter writer(file);
  bool ok = store.save(writer);
  file.close();
  return ok;
}

}  // namespace inkcards
