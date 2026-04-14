#include "Sd.h"
#include <time.h>
#include <sys/time.h>

void SdLogger::diag_(const char* line)
{
  if (runDiagnostics_)
  {
    File d = SD.open("/SD_DIAG.TXT", FILE_APPEND);
    if (d) { d.println(line); d.close(); }
  }
}

bool SdLogger::begin(uint8_t csPin, uint32_t spiHz)
{
  if (!SD.begin(csPin, SPI, spiHz))
  {
    diag_("BEGIN: SD.begin FAIL");
    ready_ = false;
    return false;
  }

  String fname = makeNextFilename_();
  dataFile_ = SD.open(fname, FILE_WRITE);

  if (!dataFile_)
  {
    diag_("BEGIN: open log FAIL");
    ready_ = false;
    return false;
  }

  dataFile_.println("timestamp,pH1,ORP1,pH2,ORP2,pH3,ORP3,message");
  dataFile_.flush();
  ready_ = true;
  return true;
}

void SdLogger::end()
{
  if (dataFile_)
  {
    dataFile_.flush();
    dataFile_.close();
  }
  ready_ = false;
}

String SdLogger::makeNextFilename_()
{
  char base[16];
  for (int i = 1; i <= 9999; i++)
  {
    snprintf(base, sizeof(base), "LOG%04d.CSV", i);
    String path = String("/") + base;
    if (!SD.exists(path)) return path;
  }
  return String("/LOG9999.CSV");
}

void SdLogger::logData(const ConfigState& config)
{
  if (!ready_ || !dataFile_) return;

  time_t now = time(nullptr);
  struct tm* ti = localtime(&now);
  if (!ti) return;

  dataFile_.printf(
    "%04d-%02d-%02d %02d:%02d:%02d,%.3f,%.0f,%.3f,%.0f,%.3f,%.0f,\n",
    ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday,
    ti->tm_hour, ti->tm_min, ti->tm_sec,
    config.phValues[0], config.orpValues[0],
    config.phValues[1], config.orpValues[1],
    config.phValues[2], config.orpValues[2]
  );

  dataFile_.flush();
}

const char* SdLogger::valveName_(int valveId) const
{
  switch (valveId)
  {
    case 0: return "pH1";
    case 1: return "ORP1";
    case 2: return "pH2";
    case 3: return "ORP2";
    case 4: return "pH3";
    case 5: return "ORP3";
    default: return "UNKNOWN";
  }
}

void SdLogger::setTimeFromBuild()
{
  struct tm tm{};
  if (strptime(__DATE__ " " __TIME__, "%b %d %Y %H:%M:%S", &tm))
  {
    time_t t = mktime(&tm);
    struct timeval now = { .tv_sec = t, .tv_usec = 0 };
    settimeofday(&now, nullptr);
  }
}

void SdLogger::logMessage(const String& message)
{
  if (!ready_ || !dataFile_) return;

  time_t now = time(nullptr);
  struct tm* ti = localtime(&now);
  if (!ti) return;

  dataFile_.printf(
    "%04d-%02d-%02d %02d:%02d:%02d,,,,,,,%s\n",
    ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday,
    ti->tm_hour, ti->tm_min, ti->tm_sec,
    message.c_str()
  );

  dataFile_.flush();
}

void SdLogger::logValveOn(int valveId)
{
  logMessage(String(valveName_(valveId)) + " valve was triggered");
}

void SdLogger::logValveLocked(int valveId)
{
  logMessage(String(valveName_(valveId)) + " valve locked");
}