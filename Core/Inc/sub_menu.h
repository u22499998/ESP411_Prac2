/*
 * sub_menu.h
 *
 *  Created on: 31 Mar 2026
 *      Author: edward
 */

#ifndef INC_SUB_MENU_H_
#define INC_SUB_MENU_H_

#include "ui_types.h"

void SubMenu_Draw(const char* title);
DisplayState_t SubMenu_HandleTouch(uint16_t x, uint16_t y, DisplayState_t currentState);


#endif /* INC_SUB_MENU_H_ */
