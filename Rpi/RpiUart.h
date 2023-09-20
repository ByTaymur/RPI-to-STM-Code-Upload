/*
***************************************************************************
*
* Author: Teunis van Beelen
*
* Copyright (C) 2005 - 2021 Teunis van Beelen
*
* Email: teuniz@protonmail.com
*
***************************************************************************
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
***************************************************************************
*/

/* For more info and how to use this library, visit: http://www.teuniz.net/RS-232/ */


#ifndef RpiUart_INCLUDED
#define RpiUart_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>



#if defined(__linux__) || defined(__FreeBSD__)

#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <sys/file.h>
#include <errno.h>

#else

#include <windows.h>

#endif

int RpiUart_OpenComport(int, int, const char *, int);
int RpiUart_PollComport(int, unsigned char *, int);
int RpiUart_SendByte(int, unsigned char);
int RpiUart_SendBuf(int, char *, int);
void RpiUart_CloseComport(int);
void RpiUart_cputs(int, const char *);
int RpiUart_IsDCDEnabled(int);
int RpiUart_IsRINGEnabled(int);
int RpiUart_IsCTSEnabled(int);
int RpiUart_IsDSREnabled(int);
void RpiUart_enableDTR(int);
void RpiUart_disableDTR(int);
void RpiUart_enableRTS(int);
void RpiUart_disableRTS(int);
void RpiUart_flushRX(int);
void RpiUart_flushTX(int);
void RpiUart_flushRXTX(int);
int RpiUart_GetPortnr(const char *);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif


