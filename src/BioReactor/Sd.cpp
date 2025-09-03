#include "Sd.h"
#include <time.h>
#include <sys/time.h>

void SdLogger::diag_(const char* line) 
{
  if(runDiagnostics)
  {
    File d = SD.open("/SD_DIAG.TXT", FILE_APPEND);
    if (d) { d.println(line); d.close(); }
  }
}

bool SdLogger::begin(uint8_t csPin, uint32_t spiHz) // dont want unit vals (yet) and dont was spiHz
{
  if (!SD.begin(csPin, SPI, spiHz)) 
  {
    diag_("BEGIN: SD.begin FAIL");
    ready_ = false;
    return false;
  }
  diag_("BEGIN: SD.begin OK");

  String fname = makeNextFilename_();            
  dataFile_ = SD.open(fname, FILE_WRITE);  

  if (!dataFile_) 
  {
    diag_("BEGIN: open log FAIL");
    ready_ = false;
    return false;
  }
  diag_(("BEGIN: opened " + fname).c_str());

  dataFile_.println("timestamp,pH,ORP");
  dataFile_.flush();
  diag_("BEGIN: wrote header");
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
  diag_("END: closed");
}

String SdLogger::makeNextFilename_() 
{
  // Always build absolute paths with a leading '/'
  char base[16];
  for (int i = 1; i <= 9999; i++) 
  {
    snprintf(base, sizeof(base), "LOG%04d.CSV", i); 
    String path = String("/") + base;               
    if (!SD.exists(path)) return path;
  }
  return String("/LOG9999.CSV");
}

void SdLogger::setTimeFromBuild() 
{
  struct tm tm{};
  if (strptime(__DATE__ " " __TIME__, "%b %d %Y %H:%M:%S", &tm)) 
  {
    time_t t = mktime(&tm);
    struct timeval now = { .tv_sec = t, .tv_usec = 0 };
    settimeofday(&now, nullptr);
    diag_("TIME: set from build");
  } else 
  {
    diag_("TIME: parse FAIL");
  }
}

void SdLogger::log(struct ConfigState& config, const String& message) 
{
  if (!ready_ || !dataFile_) 
  {
    diag_("LOG: not ready");
    return;
  }

  time_t now = time(nullptr);
  struct tm* ti = localtime(&now);
  if (!ti) 
  {
    diag_("LOG: localtime FAIL");
    return;
  }

  if (message.length() == 0) 
  {
    dataFile_.printf("%04d-%02d-%02d %02d:%02d:%02d,%.3f,%.0f\n",
      ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday,
      ti->tm_hour, ti->tm_min, ti->tm_sec,
      config.phValue, config.orpValue);
    dataFile_.flush();
    diag_("LOG: data row");
  } else 
  {
    dataFile_.printf("%04d-%02d-%02d %02d:%02d:%02d,%s\n",
      ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday,
      ti->tm_hour, ti->tm_min, ti->tm_sec,
      message.c_str());
    dataFile_.flush();
    diag_("LOG: message row");
  }
}
