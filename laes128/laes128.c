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

#include <stdbool.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <laes128.h>

#include <lauxlib.h>

#define ECB 0
#define CBC 1
#include <aes.h>

static int
_hex2data(uint8_t *key, const char *pass)
{
	const char *pos = pass;
	char *endptr;

	for(unsigned count = 0; count < 16; count++, pos += 2)
	{
		char buf [5] = {'0', 'x', pos[0], pos[1], '\0'};

		key[count] = strtol(buf, &endptr, 16);

		if(endptr[0] != '\0') //non-hexadecimal character encountered
			return -1;
	}

	return 0;
}

static bool
_parse_key(lua_State *L, int idx, uint8_t key [16])
{
	size_t key_len;
	const char *pass = luaL_checklstring(L, idx, &key_len);

	switch(key_len)
	{
		case 16: // raw key
			memcpy(key, pass, 16);
			break;
		case 32: // hex-encoded key
			if(_hex2data(key, pass))
				luaL_error(L, "invalid  hexadecimal string");
			break;
		default: // invalid key
			return false;
	}

	return true; // success
}

static const uint8_t iv [] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};

static int
_laes128_encode(lua_State *L)
{
	size_t input_len;
	const uint8_t *input = (const uint8_t *)luaL_checklstring(L, 1, &input_len);

	uint8_t key [16];
	if(!_parse_key(L, 2, key))
		luaL_error(L, "invalid key length");

	const size_t output_len = ((input_len + 15) & (~15)); // round to next 16 byte boundary

	luaL_Buffer input_buf;
	luaL_Buffer output_buf;

	// create a 16-byte padded copy of input
	uint8_t *input_copy = (uint8_t *)luaL_buffinitsize(L, &input_buf, output_len);
	memset(input_copy + output_len - 16, 0x0, 16); // pad last 16 bytes
	memcpy(input_copy, input, input_len);
	luaL_pushresultsize(&input_buf, output_len);

	uint8_t *output = (uint8_t *)luaL_buffinitsize(L, &output_buf, output_len);

	aes_t aes;
	memset(&aes, 0x0, sizeof(aes_t));
	AES128_CBC_encrypt_buffer(&aes, output, input_copy, output_len, key, iv);

	luaL_pushresultsize(&output_buf, output_len);
	lua_pushinteger(L, input_len);

	return 2;
}

static int
_laes128_decode(lua_State *L)
{
	size_t input_len;
	const uint8_t *input = (const uint8_t *)luaL_checklstring(L, 1, &input_len);

	uint8_t key [16];
	if(!_parse_key(L, 2, key))
		luaL_error(L, "invalid key length");

	const size_t encoded_len = luaL_optinteger(L, 3, input_len);

	const size_t output_len = ((input_len + 15) & (~15)); // round to next 16 byte boundary

	luaL_Buffer input_buf;
	luaL_Buffer output_buf;

	// create a 16-byte padded copy of input
	uint8_t *input_copy = (uint8_t *)luaL_buffinitsize(L, &input_buf, output_len);
	memset(input_copy + output_len - 16, 0x0, 16); // pad last 16 bytes
	memcpy(input_copy, input, input_len);
	luaL_pushresultsize(&input_buf, output_len);

	uint8_t *output = (uint8_t *)luaL_buffinitsize(L, &output_buf, output_len);

	aes_t aes;
	memset(&aes, 0x0, sizeof(aes_t));
	AES128_CBC_decrypt_buffer(&aes, output, input_copy, output_len, key, iv);

	// crop output to encoded_len
	luaL_pushresultsize(&output_buf, encoded_len);

	return 1;
}

static const luaL_Reg laes128 [] = {
	{"encode", _laes128_encode},
	{"decode", _laes128_decode},

	{NULL, NULL}
};

LUA_API int
luaopen_aes128(lua_State *L)
{
	luaL_newlib(L, laes128);

	return 1;
}
