/*
 * output.h
 *
 *  Created on: 10Oct.,2019
 *      Author: Adem Ahmet Karakaya
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

#include "RailController.h"
#include <traffic_types.h>

#define DATA_SEND 0x40 // sets the Rs value high
#define Co_Ctrl 0x00   // mode to tell LCD we are sending a single command
#ifndef LCD_COMM_H_
#define LCD_COMM_H_

#endif /* LCD_COMM_H_ */

int I2cWrite_(int *fd, uint8_t Address, uint8_t mode, uint8_t *pBuffer, uint32_t NbData);
void SetCursor(int *fd, uint8_t LCDi2cAdd, uint8_t row, uint8_t column);
void Initialise_LCD(int *fd, _Uint32t LCDi2cAdd);
void *LCD_Control(void *data);

int writeLCD(int row, int col, char write[], int *file);
int openLCD();
