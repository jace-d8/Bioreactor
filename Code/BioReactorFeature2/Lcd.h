//
// Created by Jace Dunn on 7/28/25.
//

#ifndef LCD_H
#define LCD_H

void toggleMenu(); // the if(!digitalRead(SW_pin)) needs to become external to the function 

void printLcdMenu(int selectedItem);

void updateGlobalBlink();

void printMenuItem(int col, int row, const char* label, int itemIndex, int selectedIndex);