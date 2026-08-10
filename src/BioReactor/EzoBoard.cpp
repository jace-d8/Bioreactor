#include "EzoBoard.h"
#include <stdlib.h>
#include <string.h>

EzoBoard::EzoBoard(int address, String s)
  : ezo_(address, s.c_str()) {}

void EzoBoard::resetReadState()
{
  state_ = ProbeState::Reading;
  readTimer_.reset();
}

float EzoBoard::read()
{
  switch (state_)
  {
    case ProbeState::Reading:
      ezo_.send_cmd("R");
      readTimer_.reset();
      state_ = ProbeState::Waiting;
      return lastValue_;

    case ProbeState::Waiting:
      if (readTimer_.isReady())
        state_ = ProbeState::Receiving;
      return lastValue_;

    case ProbeState::Receiving:
    {
      memset(response_, 0, sizeof(response_));

      // Fix 1: correct enum type is Ezo_board::errors, not Ezo_board::read_state.
      // Fix 2: renamed from state_ to ezoStatus to avoid shadowing the member variable,
      //         which caused the erroneous assignment on the line below the if-block.
      Ezo_board::errors ezoStatus = ezo_.receive_cmd(response_, sizeof(response_));

      if (ezoStatus == Ezo_board::SUCCESS)
      {
        char* end;
        float val = strtof(response_, &end);

        if (end != response_)
        {
          lastValue_ = val;
          valid_ = true;
        }
        else
        {
          valid_ = false;
        }
      }
      // NOT_READY / FAIL: leave lastValue_ and valid_ unchanged;
      // the probe re-enters Reading on the next call and sends a fresh "R".

      state_ = ProbeState::Reading;  // assigns to the member, no longer shadowed
      return lastValue_;
    }
  }  // Fix 3: closes switch — was missing, causing read() to fall into exportCalibration
}    // closes read()

bool EzoBoard::exportCalibration(char* out, size_t outLen)
{
  if (!out || outLen < 2) return false;

  out[0] = '\0';
  size_t used = 0;

  ezo_.send_cmd("Export,?");
  delay(300UL);
  char info[32];
  memset(info, 0, sizeof(info));
  ezo_.receive_cmd(info, sizeof(info));

  int numStrings = 24;  // safe fallback if the query fails
  if (strncmp(info, "?EXPORT,", 8) == 0)
  {
    int n = atoi(info + 8);
    if (n > 0 && n <= 24) numStrings = n;
  }

  // Each Export command takes ~300 ms per Atlas Scientific docs.
  // 600 ms gives a 2x safety margin without inflating the total time.
  const unsigned long exportDelayMs = 600UL;
  int validChunks = 0;

  // Allow up to numStrings * 2 iterations to tolerate occasional garbled
  // responses without endlessly looping if DONE is never cleanly received.
  // Fix 4: removed the duplicate old function body (lines 142-182 in the
  //         uploaded file) that was left as orphan code outside any function,
  //         causing all the "not declared in this scope" errors.
  for (int i = 0; i < numStrings * 2; ++i)
  {
    ezo_.send_cmd("Export");
    delay(exportDelayMs);

    char buf[32];
    memset(buf, 0, sizeof(buf));
    ezo_.receive_cmd(buf, sizeof(buf));

    if (buf[0] == '\0' || strcmp(buf, "*ER") == 0)
      continue;

    // Check for DONE *before* sanitizing — "DONE" contains non-hex chars
    // but must be recognised as the end-of-export marker.
    if (strstr(buf, "DONE"))
      break;

    if (strcmp(buf, "*OK") == 0)
      continue;

    // Truncate buf at the first byte that is not a valid hex digit.
    // If the I2C null-terminator was bit-flipped, the open bus fills buf
    // with 0xFF bytes; isxdigit('\xFF') is false (with unsigned cast), so
    // those bytes are stripped here before strlen is called.
    for (int k = 0; k < (int)sizeof(buf); ++k)
    {
      if (buf[k] == '\0') break;
      if (!isxdigit((unsigned char)buf[k]))
      {
        buf[k] = '\0';
        break;
      }
    }

    if (buf[0] == '\0') continue;  // nothing left after sanitization

    const size_t chunkLen = strlen(buf);  // safe: only hex chars remain
    if (used > 0)
    {
      if (used + 1 >= outLen) return true;
      out[used++] = '|';
    }
    if (used + chunkLen >= outLen) return used > 0;

    memcpy(out + used, buf, chunkLen);
    used += chunkLen;
    out[used] = '\0';

    // Exit as soon as the expected number of valid chunks is collected,
    // even if DONE was garbled and never matched.
    if (++validChunks == numStrings)
      break;
  }

  resetReadState();
  return used > 0;
}

bool EzoBoard::importCalibration(const char* payload)
{
  if (!payload || payload[0] == '\0')
    return false;

  char copy[CalibrationStorage::EXPORT_PAYLOAD_MAX];
  strncpy(copy, payload, sizeof(copy) - 1);
  copy[sizeof(copy) - 1] = '\0';

  char* savePtr = nullptr;
  char* part = strtok_r(copy, "|", &savePtr);
  while (part)
  {
    char cmd[48];
    snprintf(cmd, sizeof(cmd), "Import,%s", part);
    ezo_.send_cmd(cmd);
    delay(TimingIntervals::EZO_EXPORT_IMPORT_DELAY);

    char buf[32];
    memset(buf, 0, sizeof(buf));
    ezo_.receive_cmd(buf, sizeof(buf));
    if (strstr(buf, "*ER")) { resetReadState(); return false; }

    part = strtok_r(nullptr, "|", &savePtr);
  }

  resetReadState();
  return true;
}

void EzoBoard::setTemperature(float tempC)
{
  char cmd[16];
  snprintf(cmd, sizeof(cmd), "T,%.1f", tempC);
  ezo_.send_cmd(cmd);
}

void EzoBoard::sendCmd(const char* cmd)
{
  ezo_.send_cmd(cmd);
}
