#include "Sd.h"

File dataFile;

void setTimeFromBuild()
{
  struct tm tm; // std C++ time struct
  if (strptime(__DATE__ " " __TIME__, "%b %d %Y %H:%M:%S", &tm)) // Taking compile time and parsing it for tm 
   {
    time_t t = mktime(&tm); // unix timestamp 
    struct timeval now = { .tv_sec = t }; // std C++ time struct, seconds since 1970, so esp32 can count further
    settimeofday(&now, nullptr); // setting esp32 clock to laptop time

    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t)); // takes seconds and converts to human readable time
    lcd.print(buf);
  } else 
  {
    lcd.print("Failed to parse build time");
  }
}

void logToSD(String message)
{
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now); // reads our updated esp32 time

  // if (!dataFile) 
  // {
  //   lcd.print("File failed\n");
  //   return;
  // }
  // if (message.length() == 0) 
  // {
  //   dataFile.printf("%04d-%02d-%02d %02d:%02d:%02d,%.3f,%d\n", // logging time from esp32
  //                   timeinfo->tm_year + 1900,
  //                   timeinfo->tm_mon + 1,
  //                   timeinfo->tm_mday,
  //                   timeinfo->tm_hour,
  //                   timeinfo->tm_min,
  //                   timeinfo->tm_sec,
  //                   ph_val,
  //                   orp_val);
  //   dataFile.flush();
  // }
  // else
  // {
  //   dataFile.printf("%04d-%02d-%02d %02d:%02d:%02d,%s\n",
  //                   timeinfo->tm_year + 1900,
  //                   timeinfo->tm_mon + 1,
  //                   timeinfo->tm_mday,
  //                   timeinfo->tm_hour,
  //                   timeinfo->tm_min,
  //                   timeinfo->tm_sec,
  //                   message.c_str());   
  }         
}