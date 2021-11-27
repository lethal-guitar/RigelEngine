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

#include "Serial.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t retrowave_protocol_serial_packed_length(uint32_t len_in) {
	return ceil((double)len_in * 8 / 7) + 2;
}

uint32_t retrowave_protocol_serial_pack(const void *_buf_in, uint32_t len_in, void *_buf_out) {
	uint32_t in_cursor = 0;
	uint32_t out_cursor = 0;

	uint8_t* buf_in = (uint8_t*)_buf_in;
	uint8_t* buf_out = (uint8_t*)_buf_out;

	buf_out[out_cursor] = 0x00;
	out_cursor += 1;

	uint8_t shift_count = 0;

	while(in_cursor < len_in) {
		uint8_t cur_byte_out = buf_in[in_cursor] >> shift_count;
		if (in_cursor > 0) {
			cur_byte_out |= (buf_in[in_cursor - 1] << (8 - shift_count));
		}

		cur_byte_out |= 0x01;
		buf_out[out_cursor] = cur_byte_out;

		shift_count += 1;
		in_cursor += 1;
		out_cursor += 1;
		if (shift_count > 7) {
			shift_count = 0;
			in_cursor -= 1;
		}
	}

	if (shift_count) {
		buf_out[out_cursor] = buf_in[in_cursor - 1] << (8 - shift_count);
		buf_out[out_cursor] |= 0x01;
		out_cursor += 1;
	}

	buf_out[out_cursor] = 0x02;
	out_cursor += 1;

	return out_cursor;
}

#ifdef __cplusplus
};
#endif