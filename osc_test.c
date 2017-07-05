#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <osc.lv2/osc.h>
#include <osc.lv2/reader.h>
#include <osc.lv2/writer.h>
#include <osc.lv2/forge.h>

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

static void
_dump(const uint8_t *src, const uint8_t *dst, size_t size)
{
	for(size_t i = 0; i < size; i++)
		printf("%zu %02x %02x\n", i, src[i], dst[i]);
	printf("\n");
}

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
	assert(memcmp(raw, buf0, size) == 0);

	// check reader & writer
	LV2_OSC_Reader reader;
	lv2_osc_reader_initialize(&reader, buf0, size);
	lv2_osc_writer_initialize(writer, buf1, BUF_SIZE);
	_clone(&reader, writer, size);

	// check cloned against raw bytes
	assert(lv2_osc_writer_finalize(writer, &len) == buf1);
	assert(len == size);
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
	uint8_t m [] = {0x00, 0x90, 24, 0x7f};
	assert(lv2_osc_writer_message_vararg(writer, "/midi", "m", 4, m));
	_test_a(writer, raw_4, sizeof(raw_4));
}

static void
test_5_a(LV2_OSC_Writer *writer)
{
	uint8_t b [] = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6};
	assert(lv2_osc_writer_message_vararg(writer, "/blob", "b", 6, b));
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

static test_t tests [] = {
	test_0_a,
	test_1_a,
	test_2_a,
	test_3_a,
	test_4_a,
	test_5_a,
	test_6_a,
	test_7_a,

	NULL
};

int
main(int argc, char **argv)
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

	return 0;
}
