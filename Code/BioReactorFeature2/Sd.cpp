#include "Sd.h"
#include "Lcd.h"
#include "Config.h"

File dataFile;

void setTimeFromBuild()
{
  struct tm tm;
  if (strptime(__DATE__ " " __TIME__, "%b %d %Y %H:%M:%S", &tm))
  {
    time_t t = mktime(&tm);
    struct timeval now = { .tv_sec = t };
    settimeofday(&now, nullptr);

    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
    lcd.print(buf);
  }
  else
  {
    lcd.print("Failed to parse build time");
  }
}

void logToSD(String message)
{
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);

  if (!dataFile)
  {
    lcd.print("File failed\n");
    return;
  }

  if (message.length() == 0)
  {
    dataFile.printf("%04d-%02d-%02d %02d:%02d:%02d,%.3f,%d\n",
                    timeinfo->tm_year + 1900,
                    timeinfo->tm_mon + 1,
                    timeinfo->tm_mday,
                    timeinfo->tm_hour,
                    timeinfo->tm_min,
                    timeinfo->tm_sec,
                    phValue,
                    orpValue);
    dataFile.flush();
  }
  else
  {
    dataFile.printf("%04d-%02d-%02d %02d:%02d:%02d,%s\n",
                    timeinfo->tm_year + 1900,
                    timeinfo->tm_mon + 1,
                    timeinfo->tm_mday,
                    timeinfo->tm_hour,
                    timeinfo->tm_min,
                    timeinfo->tm_sec,
                    message.c_str());
  }
}
