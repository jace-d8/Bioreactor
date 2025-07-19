//
// Created by Jace Dunn on 7/14/25.
//

#ifndef EZO_H
#define EZO_H

#include <Ezo_i2c.h>

class EzoBoard{
private:
  Ezo_board ezo; 
  int address;
  String probe_type; 

public: 
  EzoBoard(int a, String s) : ezo(a, s.c_str()), address(a), probe_type(s) {}

  float read()
  {
    ezo.send_cmd("R");         // Send read command
    delay(900);                      // Wait for the sensor to respond
    ezo.receive_cmd(response, sizeoof(response));  // Read response into buffer
    return atof(response);
  }

  void sendCmd(String cmd)
  {
    ezo.send_cmd(cmd.c_str());
  }
};
