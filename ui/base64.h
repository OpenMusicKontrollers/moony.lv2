/*
 * Copyright (c) 2015 Hanspeter Portner (dev@open-music-kontrollers.ch)
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the Artistic License 2.0 as published by
 * The Perl Foundation.
 *
 * This source is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * Artistic License 2.0 for more details.
 *
 * You should have received a copy of the Artistic License 2.0
 * along the source as a COPYING file. If not, obtain it from
 * http://www.perlfoundation.org/artistic_license_2_0.
 */

#ifndef _BASE64_H
#define _BASE64_H

#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>

static const char b64_map [] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static inline void
encode_chunk(char out [4], const uint8_t in [3], size_t n_in)
{
	out[0] = b64_map[in[0] >> 2];
	out[1] = b64_map[((in[0] & 0x03) << 4) | ((in[1] & 0xF0) >> 4)];
	out[2] = (n_in > 1)
		? (b64_map[((in[1] & 0x0F) << 2) | ((in[2] & 0xC0) >> 6)])
		: '=';
	out[3] = (n_in > 2)
		? b64_map[in[2] & 0x3F]
		: '=';
}

static inline char *
base64_encode(const uint8_t *buf, size_t size)
{
	const size_t len = ( (size + 2) / 3) * 4;
	char *str = calloc(1, len + 3); // zero-terminated

	if(str)
	{
		for(size_t i = 0, j = 0 ; i < size; i += 3, j += 4)
		{
			uint8_t in [4] = { 0, 0, 0, 0 };
			const size_t n_in = 3 < size - i
				? 3
				: size - i;
			memcpy(in, buf + i, n_in);

			encode_chunk(str + j, in, n_in);
		}
	}

	return str;
}

static const char b64_unmap [] =
	"$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$m$$$ncdefghijkl$$$$$$"
	"$/0123456789:;<=>?@ABCDEFGH$$$$$$IJKLMNOPQRSTUVWXYZ[\\]^_`ab$$$$"
	"$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$"
	"$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$";

static inline uint8_t
unmap(const char in)
{
	return b64_unmap[in] - 47;
}

static inline size_t
_decode_chunk(const char in [4], uint8_t out [3])
{
	out[0] = ((unmap(in[0]) << 2))        | unmap(in[1]) >> 4;
	out[1] = ((unmap(in[1]) << 4) & 0xF0) | unmap(in[2]) >> 2;
	out[2] = ((unmap(in[2]) << 6) & 0xC0) | unmap(in[3]);
	return 1 + (in[2] != '=') + ((in[2] != '=') && (in[3] != '='));
}

static inline bool
_is_base64(const char c)
{
	return isalpha(c) || isdigit(c) || c == '+' || c == '/' || c == '=';
}

static inline uint8_t *
base64_decode(const char *str, size_t len, size_t *size)
{
	*size = 0;
	uint8_t *buf = malloc( (len * 3) / 4 + 2);

	if(buf)
	{
		for(size_t i = 0, j = 0; i < len; j += 3)
		{
			char in [4] = { '=', '=', '=', '=' };
			size_t n_in = 0;

			for( ; i < len && n_in < 4; ++n_in)
			{
				for( ; i < len && !_is_base64(str[i]); ++i)
				{
					// skip junk
				}
				in[n_in] = str[i++];
			}

			if(n_in > 1)
			{
				*size += _decode_chunk(in, buf + j);
			}
		}
	}

	return buf;
}

#endif
