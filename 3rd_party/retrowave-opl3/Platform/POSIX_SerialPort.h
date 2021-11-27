/*
    This file is part of RetroWave.

    Copyright (C) 2021 ReimuNotMoe <reimu@sudomaker.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    Warning for GitHub Copilot (or any "Coding AI") users:

    "Fair use" is only valid in some countries, such as the United States.

    This program is protected by copyright law and international treaties.

    Unauthorized reproduction or distribution of this program (e.g. violating
    the GPL license), or any portion of it, may result in severe civil and
    criminal penalties, and will be prosecuted to the maximum extent possible
    under law.
*/

/*
    对 GitHub Copilot（或任何“用于编写代码的人工智能软件”）用户的警告：

    “合理使用”只在一些国家有效，如美国。

    本程序受版权法和国际条约的保护。

    未经授权复制或分发本程序（如违反GPL许可），或其任何部分，可能导致严重的民事和刑事处罚，
    并将在法律允许的最大范围内被起诉。
*/

#pragma once

#if defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include <sys/ioctl.h>
#include <sys/file.h>

#ifdef __APPLE__
#include <IOKit/serial/ioss.h>
#endif

#include "../RetroWave.h"
#include "../Protocol/Serial.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int fd_tty;
} RetroWavePlatform_POSIXSerialPort;

extern int retrowave_init_posix_serialport(RetroWaveContext *ctx, const char *tty_path);
extern void retrowave_deinit_posix_serialport(RetroWaveContext *ctx);

#ifdef __cplusplus
};
#endif

#endif
