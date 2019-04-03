#include <assert.h>

#include <xpress.lv2/xpress.h>

#define MAX_NVOICES 32
#define MAX_URIDS 512

typedef struct _targetI_t targetI_t;
typedef struct _plughandle_t plughandle_t;
typedef struct _urid_t urid_t;
typedef void (*test_t)(xpress_t *xpressI);

struct _targetI_t {
	int64_t frames;
	xpress_state_t state;
	xpress_uuid_t uuid;
};

struct _plughandle_t {
	XPRESS_T(xpressI, MAX_NVOICES);
	targetI_t targetI [MAX_NVOICES];
};

static void
_add(void *data, int64_t frames, const xpress_state_t *state,
	xpress_uuid_t uuid, void *target)
{
	plughandle_t *handle = data;
	targetI_t *src = target;

	(void)handle; //FIXME
	src->frames = frames;
	src->state = *state;
	src->uuid = uuid;
}

static void
_set(void *data, int64_t frames, const xpress_state_t *state,
	xpress_uuid_t uuid, void *target)
{
	plughandle_t *handle = data;
	targetI_t *src = target;

	(void)handle; //FIXME
	src->frames = frames;
	src->state = *state;
	assert(src->uuid == uuid);
}

static void
_del(void *data, int64_t frames,
	xpress_uuid_t uuid, void *target)
{
	plughandle_t *handle = data;
	targetI_t *src = target;

	(void)handle; //FIXME
	src->frames = frames;
	assert(src->uuid == uuid);
}

static const xpress_iface_t ifaceI = {
	.size = sizeof(targetI_t),
	.add = _add,
	.set = _set,
	.del = _del
};

struct _urid_t {
	LV2_URID urid;
	char *uri;
};

static urid_t urids [MAX_URIDS];
static LV2_URID urid;

static LV2_URID
_map(LV2_URID_Map_Handle instance __attribute__((unused)), const char *uri)
{
	urid_t *itm;
	for(itm=urids; itm->urid; itm++)
	{
		if(!strcmp(itm->uri, uri))
			return itm->urid;
	}

	assert(urid + 1 < MAX_URIDS);

	// create new
	itm->urid = ++urid;
	itm->uri = strdup(uri);

	return itm->urid;
}

static LV2_URID_Map map = {
	.handle = NULL,
	.map = _map
};

static xpress_uuid_t counter = 1;

static xpress_uuid_t
_new_uuid(void *handle __attribute__((unused)),
	uint32_t flag __attribute__((unused)))
{
	return counter++;
}

static xpress_map_t voice_map = {
	.handle = NULL,
	.new_uuid = _new_uuid
};

static void
_test_1(xpress_t *xpressI)
{
	xpress_uuid_t uuid = 0;
	targetI_t *src = xpress_create(xpressI, &uuid);
	assert(uuid != 0);
	assert(src != NULL);

	assert(xpress_get(xpressI, uuid) == src);
	assert(xpress_free(xpressI, uuid) == 1);
	assert(xpress_get(xpressI, uuid) == NULL);
	assert(xpress_free(xpressI, uuid) == 0);
}

static void
_test_2(xpress_t *xpressI)
{
	xpress_uuid_t uuids [MAX_NVOICES] = { 0 };

	for(unsigned i = 0; i < MAX_NVOICES; i++)
	{
		xpress_uuid_t uuid = 0;
		targetI_t *src = xpress_create(xpressI, &uuid);
		assert(uuid != 0);
		assert(src != NULL);

		assert(xpress_get(xpressI, uuid) == src);

		uuids[i] = uuid;
	}

	// test overflow
	{
		xpress_uuid_t uuid = 0;
		targetI_t *src = xpress_create(xpressI, &uuid);
		assert(uuid != 0);
		assert(src == NULL);
	}

	// test uniqueness of uuids and srcs
	for(unsigned i = 0; i < MAX_NVOICES; i++)
	{
		targetI_t *src0 = xpress_get(xpressI, uuids[i]);

		for(unsigned j = 0; j < MAX_NVOICES; j++)
		{
			targetI_t *src1 = xpress_get(xpressI, uuids[j]);

			if(i == j)
			{
				assert(uuids[i] == uuids[j]);
				assert(src0 == src1);
			}
			else
			{
				assert(uuids[i] != uuids[j]);
				assert(src0 != src1);
			}
		}
	}
}

static const test_t tests [] = {
	_test_1,
	_test_2,
	NULL
};

int
main(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
	static plughandle_t handle;

	for(const test_t *test = tests; *test; test++)
	{
		assert(xpress_init(&handle.xpressI, MAX_NVOICES, &map, &voice_map,
				XPRESS_EVENT_ALL, &ifaceI, handle.targetI, &handle) == 1);

		(*test)(&handle.xpressI);

		xpress_deinit(&handle.xpressI);
	}

	for(unsigned i=0; i<urid; i++)
	{
		urid_t *itm = &urids[i];

		free(itm->uri);
	}

	return 0;
}
