#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

#include <osc.lv2/osc.h>
#include <osc.lv2/reader.h>
#include <osc.lv2/writer.h>
#include <osc.lv2/forge.h>
#include <osc.lv2/stream.h>

#define BUF_SIZE 8192
#define MAX_URIDS 512

typedef void (*test_t)(LV2_OSC_Writer *writer);
typedef struct _urid_t urid_t;
typedef struct _handle_t handle_t;

struct _urid_t {
	LV2_URID urid;
	char *uri;
};

struct _handle_t {
	urid_t urids [MAX_URIDS];
	LV2_URID urid;
};

static handle_t __handle;
static uint8_t buf0 [BUF_SIZE];
static uint8_t buf1 [BUF_SIZE];
static uint8_t buf2 [BUF_SIZE];
static const LV2_Atom_Object *obj2= (const LV2_Atom_Object *)buf2;

const uint8_t raw_0 [] = {
	'/', 0x0, 0x0, 0x0,
	',', 0x0, 0x0, 0x0
};

const uint8_t raw_1 [] = {
	'/', 'p', 'i', 'n',
	'g', 0x0, 0x0, 0x0,
	',', 'i', 'f', 's',
	0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0xc,
	0x40, 0x59, 0x99, 0x9a,
	'w', 'o', 'r', 'l',
	'd', 0x0, 0x0, 0x0
};

const uint8_t raw_2 [] = {
	'/', 'p', 'i', 'n',
	'g', 0x0, 0x0, 0x0,
	',', 'h', 'd', 'S',
	0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0xc,
	0x40, 0x0b, 0x33, 0x33,
	0x33, 0x33, 0x33, 0x33,
	'h', 't', 't', 'p',
	':', '/', '/', 'e',
	'x', 'a', 'm', 'p',
	'l', 'e', '.', 'c',
	'o', 'm',  0x0, 0x0
};

const uint8_t raw_3 [] = {
	'/', 'p', 'i', 'n',
	'g', 0x0, 0x0, 0x0,
	',', 'T', 'F', 'N',
	'I', 0x0, 0x0, 0x0
};

const uint8_t raw_4 [] = {
	'/', 'm', 'i', 'd',
	'i', 0x0, 0x0, 0x0,
	',', 'm', 0x0, 0x0,
	0x0, 0x90, 24, 0x7f
};

const uint8_t raw_5 [] = {
	'/', 'b', 'l', 'o',
	'b', 0x0, 0x0, 0x0,
	',', 'b', 0x0, 0x0,
	0x0, 0x0, 0x0, 0x6,
	0x1, 0x2, 0x3, 0x4,
	0x5, 0x6, 0x0, 0x0
};

const uint8_t raw_6 [] = {
	'#', 'b', 'u', 'n',
	'd', 'l', 'e', 0x0,
	0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x1,

	0x0, 0x0, 0x0, 0x8,
		'/', 0x0, 0x0, 0x0,
		',', 0x0, 0x0, 0x0
};

const uint8_t raw_7 [] = {
	'#', 'b', 'u', 'n',
	'd', 'l', 'e', 0x0,
	0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x1,

	0x0, 0x0, 0x0, 0x1c,
		'#', 'b', 'u', 'n',
		'd', 'l', 'e', 0x0,
		0x0, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x1,

		0x0, 0x0, 0x0, 0x8,
			'/', 0x0, 0x0, 0x0,
			',', 0x0, 0x0, 0x0,

	0x0, 0x0, 0x0, 0x8,
		'/', 0x0, 0x0, 0x0,
		',', 0x0, 0x0, 0x0
};

const uint8_t raw_8 [] = {
	'/', 'p', 'i', 'n',
	'g', 0x0, 0x0, 0x0,
	',', 't', 'c', 'r',
	0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x1,
	0x0, 0x0, 0x0, 'o',
	0x1, 0x2, 0x3, 0x4
};

static LV2_URID
_map(LV2_URID_Map_Handle instance, const char *uri)
{
	handle_t *handle = instance;

	urid_t *itm;
	for(itm=handle->urids; itm->urid; itm++)
	{
		if(!strcmp(itm->uri, uri))
			return itm->urid;
	}

	assert(handle->urid + 1 < MAX_URIDS);

	// create new
	itm->urid = ++handle->urid;
	itm->uri = strdup(uri);

	return itm->urid;
}

static const char *
_unmap(LV2_URID_Unmap_Handle instance, LV2_URID urid)
{
	handle_t *handle = instance;

	urid_t *itm;
	for(itm=handle->urids; itm->urid; itm++)
	{
		if(itm->urid == urid)
			return itm->uri;
	}

	// not found
	return NULL;
}

static LV2_URID_Map map = {
	.handle = &__handle,
	.map = _map
};

static LV2_URID_Unmap unmap = {
	.handle = &__handle,
	.unmap = _unmap
};

//#define DUMP
#if defined(DUMP)
static void
_dump(const uint8_t *src, const uint8_t *dst, size_t size)
{
	for(size_t i = 0; i < size; i++)
		printf("%zu %02x %02x\n", i, src[i], dst[i]);
	printf("\n");
}
#endif

static void
_clone(LV2_OSC_Reader *reader, LV2_OSC_Writer *writer, size_t size)
{
	if(lv2_osc_reader_is_bundle(reader))
	{
		LV2_OSC_Item *itm = OSC_READER_BUNDLE_BEGIN(reader, size);
		assert(itm);

		LV2_OSC_Writer_Frame frame_bndl;
		assert(lv2_osc_writer_push_bundle(writer, &frame_bndl, itm->timetag));

		OSC_READER_BUNDLE_ITERATE(reader, itm)
		{
			LV2_OSC_Reader reader2;
			lv2_osc_reader_initialize(&reader2, itm->body, itm->size);

			LV2_OSC_Writer_Frame frame_itm;
			assert(lv2_osc_writer_push_item(writer, &frame_itm));
			_clone(&reader2, writer, itm->size);
			assert(lv2_osc_writer_pop_item(writer, &frame_itm));
		}

		assert(lv2_osc_writer_pop_bundle(writer, &frame_bndl));
	}
	else if(lv2_osc_reader_is_message(reader))
	{
		LV2_OSC_Arg *arg = OSC_READER_MESSAGE_BEGIN(reader, size);
		assert(arg);

		assert(lv2_osc_writer_add_path(writer, arg->path));
		assert(lv2_osc_writer_add_format(writer, arg->type));

		OSC_READER_MESSAGE_ITERATE(reader, arg)
		{
			switch((LV2_OSC_Type)*arg->type)
			{
				case LV2_OSC_INT32:
					assert(lv2_osc_writer_add_int32(writer, arg->i));
					break;
				case LV2_OSC_FLOAT:
					assert(lv2_osc_writer_add_float(writer, arg->f));
					break;
				case LV2_OSC_STRING:
					assert(lv2_osc_writer_add_string(writer, arg->s));
					break;
				case LV2_OSC_BLOB:
					assert(lv2_osc_writer_add_blob(writer, arg->size, arg->b));
					break;

				case LV2_OSC_INT64:
					assert(lv2_osc_writer_add_int64(writer, arg->h));
					break;
				case LV2_OSC_DOUBLE:
					assert(lv2_osc_writer_add_double(writer, arg->d));
					break;
				case LV2_OSC_TIMETAG:
					assert(lv2_osc_writer_add_timetag(writer, arg->t));
					break;

				case LV2_OSC_TRUE:
				case LV2_OSC_FALSE:
				case LV2_OSC_NIL:
				case LV2_OSC_IMPULSE:
					break;

				case LV2_OSC_MIDI:
					assert(lv2_osc_writer_add_midi(writer, arg->size, arg->m));
					break;
				case LV2_OSC_SYMBOL:
					assert(lv2_osc_writer_add_symbol(writer, arg->S));
					break;
				case LV2_OSC_CHAR:
					assert(lv2_osc_writer_add_char(writer, arg->c));
					break;
				case LV2_OSC_RGBA:
					assert(lv2_osc_writer_add_rgba(writer, arg->R, arg->G, arg->B, arg->A));
					break;
			}
		}
	}
}

static void
_test_a(LV2_OSC_Writer *writer, const uint8_t *raw, size_t size)
{
	LV2_OSC_URID osc_urid;
	lv2_osc_urid_init(&osc_urid, &map);

	// check writer against raw bytes
	size_t len;
	assert(lv2_osc_writer_finalize(writer, &len) == buf0);
	assert(len == size);
#if defined(DUMP)
	if(memcmp(raw, buf0, size) != 0)
		_dump(raw, buf0, size);
#endif
	assert(memcmp(raw, buf0, size) == 0);

	// check reader & writer
	LV2_OSC_Reader reader;
	lv2_osc_reader_initialize(&reader, buf0, size);
	lv2_osc_writer_initialize(writer, buf1, BUF_SIZE);
	_clone(&reader, writer, size);

	// check cloned against raw bytes
	assert(lv2_osc_writer_finalize(writer, &len) == buf1);
	assert(len == size);
#if defined(DUMP)
	if(memcmp(raw, buf1, size) != 0)
		_dump(raw, buf1, size);
#endif
	assert(memcmp(raw, buf1, size) == 0);

	// check forge 
	LV2_Atom_Forge forge;
	lv2_atom_forge_init(&forge, &map);
	lv2_atom_forge_set_buffer(&forge, buf2, BUF_SIZE);
	assert(lv2_osc_forge_packet(&forge, &osc_urid, &map, buf0, size));

	// check deforge 
	lv2_osc_writer_initialize(writer, buf1, BUF_SIZE);
	assert(lv2_osc_writer_packet(writer, &osc_urid, &unmap, obj2->atom.size, &obj2->body));

	// check deforged against raw bytes
	assert(lv2_osc_writer_finalize(writer, &len) == buf1);
	assert(len == size);
#if defined(DUMP)
	if(memcmp(raw, buf1, size) != 0)
		_dump(raw, buf1, size);
#endif
	assert(memcmp(raw, buf1, size) == 0);
}

static void
test_0_a(LV2_OSC_Writer *writer)
{
	assert(lv2_osc_writer_message_vararg(writer, "/", ""));
	_test_a(writer, raw_0, sizeof(raw_0));
}

static void
test_1_a(LV2_OSC_Writer *writer)
{
	assert(lv2_osc_writer_message_vararg(writer, "/ping", "ifs",
		12, 3.4f, "world"));
	_test_a(writer, raw_1, sizeof(raw_1));
}

static void
test_2_a(LV2_OSC_Writer *writer)
{
	assert(lv2_osc_writer_message_vararg(writer, "/ping", "hdS",
		12, 3.4, "http://example.com"));
	_test_a(writer, raw_2, sizeof(raw_2));
}

static void
test_3_a(LV2_OSC_Writer *writer)
{
	assert(lv2_osc_writer_message_vararg(writer, "/ping", "TFNI"));
	_test_a(writer, raw_3, sizeof(raw_3));
}

static void
test_4_a(LV2_OSC_Writer *writer)
{
	const uint8_t m [] = {0x00, 0x90, 24, 0x7f};
	const int32_t len = sizeof(m);
	assert(lv2_osc_writer_message_vararg(writer, "/midi", "m", len, m));
	_test_a(writer, raw_4, sizeof(raw_4));
}

static void
test_5_a(LV2_OSC_Writer *writer)
{
	const uint8_t b [] = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6};
	const int32_t len = sizeof(b);
	assert(lv2_osc_writer_message_vararg(writer, "/blob", "b", len, b));
	_test_a(writer, raw_5, sizeof(raw_5));
}

static void
test_6_a(LV2_OSC_Writer *writer)
{
	LV2_OSC_Writer_Frame frame_bndl, frame_itm;

	assert(lv2_osc_writer_push_bundle(writer, &frame_bndl, LV2_OSC_IMMEDIATE));
	{
		assert(lv2_osc_writer_push_item(writer, &frame_itm));
		{
			assert(lv2_osc_writer_message_vararg(writer, "/", ""));
		}
		assert(lv2_osc_writer_pop_item(writer, &frame_itm));
	}
	assert(lv2_osc_writer_pop_bundle(writer, &frame_bndl));

	_test_a(writer, raw_6, sizeof(raw_6));
}

static void
test_7_a(LV2_OSC_Writer *writer)
{
	LV2_OSC_Writer_Frame frame_bndl[2], frame_itm[2];

	assert(lv2_osc_writer_push_bundle(writer, &frame_bndl[0], LV2_OSC_IMMEDIATE));
	{
		assert(lv2_osc_writer_push_item(writer, &frame_itm[0]));
		{
			assert(lv2_osc_writer_push_bundle(writer, &frame_bndl[1], LV2_OSC_IMMEDIATE));
			{
				assert(lv2_osc_writer_push_item(writer, &frame_itm[1]));
				{
					assert(lv2_osc_writer_message_vararg(writer, "/", ""));
				}
				assert(lv2_osc_writer_pop_item(writer, &frame_itm[1]));
			}
			assert(lv2_osc_writer_pop_bundle(writer, &frame_bndl[1]));
		}
		assert(lv2_osc_writer_pop_item(writer, &frame_itm[0]));

		assert(lv2_osc_writer_push_item(writer, &frame_itm[0]));
		{
			assert(lv2_osc_writer_message_vararg(writer, "/", ""));
		}
		assert(lv2_osc_writer_pop_item(writer, &frame_itm[0]));
	}
	assert(lv2_osc_writer_pop_bundle(writer, &frame_bndl[0]));

	_test_a(writer, raw_7, sizeof(raw_7));
}

static void
test_8_a(LV2_OSC_Writer *writer)
{
	assert(lv2_osc_writer_message_vararg(writer, "/ping", "tcr",
		1ULL,
		'o',
		0x1, 0x2, 0x3, 0x4));
	_test_a(writer, raw_8, sizeof(raw_8));
}

static test_t tests [] = {
	test_0_a,
	test_1_a,
	test_2_a,
	test_3_a,
	test_4_a,
	test_5_a,
	test_6_a,
	test_7_a,
	test_8_a,

	NULL
}
;
static int
_run_tests()
{
	LV2_OSC_Writer writer;

	for(test_t *test=tests; *test; test++)
	{
		test_t cb = *test;

		memset(buf0, 0x0, BUF_SIZE);
		memset(buf1, 0x0, BUF_SIZE);

		lv2_osc_writer_initialize(&writer, buf0, BUF_SIZE);

		cb(&writer);
	}

	assert(unmap.unmap(unmap.handle, 0)== NULL);

	return 0;
}

typedef struct _item_t item_t;
typedef struct _stash_t stash_t;

struct _item_t {
	size_t size;
	uint8_t buf [];
};

struct _stash_t {
	size_t size;
	item_t **items;
	item_t *rsvd;
};

static uint8_t *
_stash_write_req(stash_t *stash, size_t minimum, size_t *maximum)
{
	if(!stash->rsvd || (stash->rsvd->size < minimum))
	{
		const size_t sz = sizeof(item_t) + minimum;
		stash->rsvd = realloc(stash->rsvd, sz);
		assert(stash->rsvd);
		stash->rsvd->size = minimum;
	}

	if(maximum)
	{
		*maximum = stash->rsvd->size;
	}

	return stash->rsvd->buf;
}

static void
_stash_write_adv(stash_t *stash, size_t written)
{
	assert(stash->rsvd);
	assert(stash->rsvd->size >= written);
	stash->rsvd->size = written;
	stash->size += 1;
	stash->items = realloc(stash->items, sizeof(item_t *) * stash->size);
	stash->items[stash->size - 1] = stash->rsvd;
	stash->rsvd = NULL;
}

static const uint8_t *
_stash_read_req(stash_t *stash, size_t *size)
{
	if(stash->size == 0)
	{
		if(size)
		{
			*size = 0;
		}

		return NULL;
	}

	item_t *item = stash->items[0];

	if(size)
	{
		*size = item->size;
	}

	return item->buf;
}

static void
_stash_read_adv(stash_t *stash)
{
	assert(stash->size);

	free(stash->items[0]);
	stash->size -= 1;

	for(unsigned i = 0; i < stash->size; i++)
	{
		stash->items[i] = stash->items[i+1];
	}

	stash->items = realloc(stash->items, sizeof(item_t *) * stash->size);
}

static void *
_write_req(void *data, size_t minimum, size_t *maximum)
{
	stash_t *stash = data;

	return _stash_write_req(&stash[0], minimum, maximum);
}

static void
_write_adv(void *data, size_t written)
{
	stash_t *stash = data;

	_stash_write_adv(&stash[0], written);
}

static const void *
_read_req(void *data, size_t *toread)
{
	stash_t *stash = data;

	return _stash_read_req(&stash[1], toread);
}

static void
_read_adv(void *data)
{
	stash_t *stash = data;

	_stash_read_adv(&stash[1]);
}

static const LV2_OSC_Driver driv = {
	.write_req = _write_req,
	.write_adv = _write_adv,
	.read_req = _read_req,
	.read_adv = _read_adv
};

#define COUNT 1024

static void *
_thread_1(void *data)
{
	const char *uri = data;

	LV2_OSC_Stream stream;
	stash_t stash [2];
	uint8_t check [COUNT];

	memset(&stream, 0x0, sizeof(stream));
	memset(stash, 0x0, sizeof(stash));
	memset(check, 0x0, sizeof(check));

	assert(lv2_osc_stream_init(&stream, uri, &driv, stash) == 0);

	unsigned count = 0;
	while(true)
	{
		const LV2_OSC_Enum ev = lv2_osc_stream_run(&stream);

		if(ev & LV2_OSC_RECV)
		{
			const uint8_t *buf_rx;
			size_t reat;

			while( (buf_rx = _stash_read_req(&stash[0], &reat)) )
			{
				LV2_OSC_Reader reader;

				lv2_osc_reader_initialize(&reader, buf_rx, reat);
				assert(lv2_osc_reader_is_message(&reader));

				OSC_READER_MESSAGE_FOREACH(&reader, arg, reat)
				{
					assert(strcmp(arg->path, "/trip") == 0);
					assert(*arg->type == 'i');
					assert(arg->size == sizeof(int32_t));
					assert(check[arg->i] == 0);
					check[arg->i] = 1;
				}

				count++;

				while(true)
				{
					// send back
					uint8_t *buf_tx;
					if( (buf_tx = _stash_write_req(&stash[1], reat, NULL)) )
					{
						memcpy(buf_tx, buf_rx, reat);

						_stash_write_adv(&stash[1], reat);
						break;
					}
				}

				_stash_read_adv(&stash[0]);
			}
		}

		if(count >= COUNT)
		{
			break;
		}
	}

	LV2_OSC_Enum ev;
	do
	{
		ev = lv2_osc_stream_run(&stream);
	} while( (ev & LV2_OSC_SEND) || (stream.fd > 0) );

	sleep(1);

	assert(lv2_osc_stream_deinit(&stream) == 0);

	if(stash[0].rsvd)
	{
		free(stash[0].rsvd);
		stash[0].rsvd = NULL;
	}

	assert(stash[1].rsvd == 0);

	return NULL;
}

static void *
_thread_2(void *data)
{
	const char *uri = data;

	LV2_OSC_Stream stream;
	stash_t stash [2];
	uint8_t check [COUNT];

	memset(&stream, 0x0, sizeof(stream));
	memset(stash, 0x0, sizeof(stash));
	memset(check, 0x0, sizeof(check));

	assert(lv2_osc_stream_init(&stream, uri, &driv, stash) == 0);

	unsigned count = 0;
	for(int32_t i = 0; i < COUNT; i++)
	{
		LV2_OSC_Writer writer;

		while(true)
		{
			uint8_t *buf_tx;
			size_t max;
			if( (buf_tx = _stash_write_req(&stash[1], 1024, &max)) )
			{
				size_t writ;
				lv2_osc_writer_initialize(&writer, buf_tx, max);
				assert(lv2_osc_writer_message_vararg(&writer, "/trip", "i", i));
				assert(lv2_osc_writer_finalize(&writer, &writ) == buf_tx);
				assert(writ == 16);
				assert(check[i] == 0);
				check[i] = 1;

				_stash_write_adv(&stash[1], writ);
				break;
			}
		}

		const LV2_OSC_Enum ev = lv2_osc_stream_run(&stream);

		if(ev & LV2_OSC_RECV)
		{
			const uint8_t *buf_rx;
			size_t reat;

			while( (buf_rx = _stash_read_req(&stash[0], &reat)) )
			{
				LV2_OSC_Reader reader;

				lv2_osc_reader_initialize(&reader, buf_rx, reat);
				assert(lv2_osc_reader_is_message(&reader));

				OSC_READER_MESSAGE_FOREACH(&reader, arg, reat)
				{
					assert(strcmp(arg->path, "/trip") == 0);
					assert(*arg->type == 'i');
					assert(arg->size == sizeof(int32_t));
					assert(check[arg->i] == 1);
					check[arg->i] = 2;
				}

				count++;

				_stash_read_adv(&stash[0]);
			}
		}
	}

	while(count <= (COUNT - 1))
	{
		const LV2_OSC_Enum ev = lv2_osc_stream_run(&stream);

		if(ev & LV2_OSC_RECV)
		{
			const uint8_t *buf_rx;
			size_t reat;

			while( (buf_rx = _stash_read_req(&stash[0], &reat)) )
			{
				LV2_OSC_Reader reader;

				lv2_osc_reader_initialize(&reader, buf_rx, reat);
				assert(lv2_osc_reader_is_message(&reader));

				OSC_READER_MESSAGE_FOREACH(&reader, arg, reat)
				{
					assert(strcmp(arg->path, "/trip") == 0);
					assert(*arg->type == 'i');
					assert(arg->size == sizeof(int32_t));
					assert(check[arg->i] == 1);
					check[arg->i] = 2;
				}

				count++;

				_stash_read_adv(&stash[0]);
			}
		}
	}

	assert(count == COUNT);

	sleep(1);

	assert(lv2_osc_stream_deinit(&stream) == 0);

	if(stash[0].rsvd)
	{
		free(stash[0].rsvd);
		stash[0].rsvd = NULL;
	}

	assert(stash[1].rsvd == NULL);

	return NULL;
}

typedef struct _pair_t pair_t;

struct _pair_t {
	const char *server;
	const char *client;
};

static const pair_t pairs [] = {
	{
		.server = "osc.udp://:2222",
		.client = "osc.udp://localhost:2222"
	},
	{
		.server = "osc.udp://[]:3333",
		.client = "osc.udp://[::1]:3333"
	},

	{
		.server = "osc.udp://:3344",
		.client = "osc.udp://255.255.255.255:3344"
	},

	{
		.server = "osc.tcp://:4444",
		.client = "osc.tcp://localhost:4444"
	},
	{
		.server = "osc.tcp://[]:5555",
		.client = "osc.tcp://[::1]:5555"
	},

	{
		.server = "osc.slip.tcp://:6666",
		.client = "osc.slip.tcp://localhost:6666"
	},
	{
		.server = "osc.slip.tcp://[]:7777",
		.client = "osc.slip.tcp://[::1]:7777"
	},

	{
		.server = "osc.prefix.tcp://:8888",
		.client = "osc.prefix.tcp://localhost:8888"
	},
	{
		.server = "osc.prefix.tcp://[%lo]:9999",
		.client = "osc.prefix.tcp://[::1%lo]:9999"
	},

	{
		.server = NULL,
		.client = NULL
	}
};

int
main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	fprintf(stdout, "running main tests:\n");
	assert(_run_tests() == 0);

	for(const pair_t *pair = pairs; pair->server; pair++)
	{
		pthread_t thread_1;
		pthread_t thread_2;

		const char *uri_1 = pair->server;
		const char *uri_2 = pair->client;

		fprintf(stdout, "running stream test: <%s> <%s>\n", uri_1, uri_2);

		assert(pthread_create(&thread_1, NULL, _thread_1, (void *)uri_1) == 0);
		assert(pthread_create(&thread_2, NULL, _thread_2, (void *)uri_2) == 0);

		assert(pthread_join(thread_1, NULL) == 0);
		assert(pthread_join(thread_2, NULL) == 0);
	}

	for(unsigned i=0; i<__handle.urid; i++)
	{
		urid_t *itm = &__handle.urids[i];

		free(itm->uri);
	}

	return 0;
}
