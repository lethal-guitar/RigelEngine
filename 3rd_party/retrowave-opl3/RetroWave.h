/*
    This file is part of RetroWave.

    Copyright (C) 2021 ReimuNotMoe <reimu@sudomaker.com>
    Copyright (C) 2021 Yukino Song <yukino@sudomaker.com>

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	RetroWave_Board_Unknown = 0,
	RetroWave_Board_OPL3 = 0x21 << 1,
	RetroWave_Board_MiniBlaster = 0x20 << 1,
	RetroWave_Board_MasterGear = 0x24 << 1
} RetroWaveBoardType;

typedef struct {
	void *user_data;
	void (*callback_io)(void *, uint32_t, const void *, void *, uint32_t);
	uint8_t *cmd_buffer;
	uint32_t cmd_buffer_used, cmd_buffer_size;
	uint32_t transfer_speed_hint;
} RetroWaveContext;

extern void retrowave_init(RetroWaveContext *ctx);
extern void retrowave_deinit(RetroWaveContext *ctx);

extern void retrowave_io_init(RetroWaveContext *ctx);

extern void retrowave_cmd_buffer_init(RetroWaveContext *ctx, RetroWaveBoardType board_type, uint8_t first_reg);

extern void retrowave_flush(RetroWaveContext *ctx);

extern uint8_t retrowave_invert_byte(uint8_t val);

#ifdef __cplusplus
};
#endif
