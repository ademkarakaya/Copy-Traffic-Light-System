/*
 * I2C LCD test program using BBB I2C1
 *
 * LCD data:  MIDAS  MCCOG22005A6W-FPTLWI  Alphanumeric LCD, 20 x 2, Black on White, 3V to 5V, I2C, English, Japanese, Transflective
 * LCD data sheet, see:
 *                      http://au.element14.com/midas/mccog22005a6w-fptlwi/lcd-alpha-num-20-x-2-white/dp/2425758?ost=2425758&selectedCategoryId=&categoryNameResp=All&searchView=table&iscrfnonsku=false
 *
 *  BBB P9 connections:
 *    - P9_Pin17 - SCL - I2C1	 GPIO3_2
 *    - P9_Pin18 - SDA - I2C1    GPIO3_1
 *
 *  LCD connections:
 *    - pin 1 – VOUT to the 5V     (external supply should be used)
 *    - pin 4 – VDD  to the 5V
 *    - pin 8 – RST  to the 5V
 *
 *    - pin 2 - Not connected  (If 3.3V is used then add two caps)
 *    - pin 3 - Not connected  (If 3.3V is used then add two caps)
 *
 *    - pin 5 - VSS  to Ground
 *    - pin 6 – SDA  to the I2C SDA Pin  pulled high using a 4.7kohm resitor connected to 5V
 *    - pin 7 - SCL  to the I2C SCL Pin  pulled high using a 4.7kohm resitor connected to 5V
 *
 * Author:  Samuel Ippolito
 * Date:	10/04/2017
 */

#pragma once

#include "stdint.h"
#include <devctl.h>
#include <errno.h>
#include <fcntl.h>
#include <hw/i2c.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/netmgr.h>
#include <sys/neutrino.h>
#include <unistd.h>

#include "IntersectionController.h"
#include <traffic_types.h>

#define DATA_SEND 0x40 // sets the Rs value high
#define Co_Ctrl 0x00   // mode to tell LCD we are sending a single command
#ifndef LCD_COMM_H_
#define LCD_COMM_H_

#endif /* LCD_COMM_H_ */

void *LCD_Control(void *data);

int I2cWrite_(int *fd, uint8_t Address, uint8_t mode, uint8_t *pBuffer, uint32_t NbData);
void SetCursor(int *fd, uint8_t LCDi2cAdd, uint8_t row, uint8_t column);
void Initialise_LCD(int *fd, _Uint32t LCDi2cAdd);

int writeLCD(int row, int col, char write[], int *file);
int openLCD();
