To be populated before "publication" of repo 

Include: names and links of every part used, what it does, who is it for, how to use it, 
dependicies, video of use, etc




## Naming Conventions

| Item            | Style           | Example              |
|-----------------|----------------|----------------------|
| Classes         | PascalCase     | `EzoBoard`    |
| Functions       | camelCase      | `sendCmd()`  |
| Variables       | camelCase      | `orpValue`         |
| Private Members | trailing `_`   | `lastTrigger_`         |
| Constants       | ALL_CAPS       | `MAX_BUFFER_SIZE`    |


Libraries needed for this project:
SD by SparkFun
Adafruit MCP23017 Arduino Library by Adafruit
I2C_LCD by Rob Tillaart
LiquidCrystal I2C by Frank de Barbander
External Library: Ezo_I2c_lib by Atlas-Scientific - this needs to be downloaded from the Atlas-Scientific GitHub Page and installed manually to Arduino IDE

Also, in Arduino IDE, the Pin Numbering needs to be set as "By GRIO number (legacy)" under "Tools".
