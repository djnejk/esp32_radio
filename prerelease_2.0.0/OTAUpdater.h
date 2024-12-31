#ifndef OTA_UPDATER_H
#define OTA_UPDATER_H

#include <Arduino.h>

namespace OTAUpdater {
  void checkForUpdates(const String& currentVersion);
  void performOTAUpdate(const char* downloadURL);
  bool upToDate(const String& currentVersion);
}

#endif
