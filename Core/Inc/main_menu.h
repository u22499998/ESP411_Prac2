/*
 * main_menu.h
 *
 *  Created on: 31 Mar 2026
 *      Author: edward
 */

#ifndef INC_MAIN_MENU_H_
#define INC_MAIN_MENU_H_

#include "ui_types.h"

//Constructor
void MainMenu_Draw(void);

// Touch handler: Pass the raw X/Y, returns the new state
DisplayState_t MainMenu_HandleTouch(uint16_t x, uint16_t y, DisplayState_t currentState);


#endif /* INC_MAIN_MENU_H_ */
