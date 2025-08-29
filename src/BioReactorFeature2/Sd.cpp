#include "Sd.h"
#include <time.h>
#include <sys/time.h>

SdLogger::SdLogger(Lcd& lcdRef, int csPin) : lcd(lcdRef), cs_(csPin) 
{
  if (SD.begin(cs_))
  { 
    beginSession(); 
  }
}

SdLogger::~SdLogger()
{
  if (dataFile_) dataFile_.close();
}

String SdLogger::makeNextFilename_()
{
  char name[16];
  for (int i = 1; i <= 9999; i++) 
  {
    snprintf(name, sizeof(name), "LOG%04d.CSV", i);
    if (!SD.exists(name)) return String(name);
  }
  return "LOG9999.CSV";
}

void SdLogger::beginSession()
{
  if (dataFile_) dataFile_.close();
  String fname = makeNextFilename_();
  dataFile_ = SD.open(fname, FILE_WRITE);

  if (dataFile_) 
  {
    dataFile_.println("timestamp,pH,ORP"); // header row
    dataFile_.flush();                     // keep header
  }
}

void SdLogger::setTimeFromBuild()
{
  struct tm tm;
  if (strptime(__DATE__ " " __TIME__, "%b %d %Y %H:%M:%S", &tm)) {
    time_t t = mktime(&tm);
    struct timeval now = { .tv_sec = t };
    settimeofday(&now, nullptr);
  }
}

void SdLogger::log(ConfigState& config, const String& message)
{
  if (!dataFile_) return;

  time_t now = time(nullptr);
  struct tm* ti = localtime(&now);

  if (message.length() == 0) 
  {
    dataFile_.printf("%04d-%02d-%02d %02d:%02d:%02d,%.3f,%d\n",
                     ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday,
                     ti->tm_hour, ti->tm_min, ti->tm_sec,
                     config.phValue, config.orpValue);
    dataFile_.flush();
  } else 
  {
    dataFile_.printf("%04d-%02d-%02d %02d:%02d:%02d,%s\n",
                     ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday,
                     ti->tm_hour, ti->tm_min, ti->tm_sec,
                     message.c_str());
    dataFile_.flush();  
  }
}
