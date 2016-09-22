/*
 * Copyright (c) 2015-2016 Hanspeter Portner (dev@open-music-kontrollers.ch)
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

#include <sord/sord.h>

#include <lv2/lv2plug.in/ns/ext/state/state.h>
#include <lv2/lv2plug.in/ns/ext/presets/presets.h>

#define LILV_NS_RDF  "http://www.w3.org/1999/02/22-rdf-syntax-ns#"

#define U8(STR) (const uint8_t *)(STR)

int
main(int argc, char **argv)
{
	const char *preset_ttl = argv[1]; // e.g. 'presets.ttl'
	if(!preset_ttl)
		return -1;

	const uint8_t *state_uri = U8(LV2_STATE__state);
	const uint8_t *type_uri = U8(LILV_NS_RDF"type");
	const uint8_t *pset_uri = U8(LV2_PRESETS__Preset);
	const uint8_t *code_uri = U8("http://open-music-kontrollers.ch/lv2/moony#code");

	const uint8_t *input = serd_uri_to_path((const uint8_t *)preset_ttl);
	SerdNode base = serd_node_new_file_uri(input, NULL, NULL, false);

	int ret = -1;

	FILE *out_fd = argc > 2
		? fopen(argv[2], "wb")
		: stdout;
	if(!out_fd)
		return -1;

	fprintf(out_fd,
		"local function _test()\n"
		"	local _state = Stash()\n"
		"	if stash then\n"
		"		stash(_state)\n"
		"	end\n"
		"	_state:read()\n"
		"	if apply then\n"
		"		apply(_state)\n"
		"	end\n"
		"	_state:write()\n"
		"	if save then\n"
		"		save(_state)\n"
		"	end\n"
		"	_state:read()\n"
		"	if restore then\n"
		"		restore(_state)\n"
		"	end\n"
		"	local n = 128\n"
		"	local _seq1 = Stash()\n"
		"	local _seq2 = Stash()\n"
		"	local _seq3 = Stash()\n"
		"	local _seq4 = Stash()\n"
		"	local _forge1 = Stash()\n"
		"	local _forge2 = Stash()\n"
		"	local _forge3 = Stash()\n"
		"	local _forge4 = Stash()\n"
		"	_seq1:sequence():pop()\n"
		"	_seq1:read()\n"
		"	_seq2:sequence():pop()\n"
		"	_seq2:read()\n"
		"	_seq3:sequence():pop()\n"
		"	_seq3:read()\n"
		"	_seq4:sequence():pop()\n"
		"	_seq4:read()\n"
		"	if once then\n"
		"		once(n, _seq1, _forge1, _seq2, _forge2, _seq3, _forge3, _seq4, _forge4)\n"
		"	end\n"
		"	if run then\n"
		"		run(n, _seq1, _forge1, _seq2, _forge2, _seq3, _forge3, _seq4, _forge4)\n"
		"	end\n"
		"end\n"
		"\n");

	FILE *in_fd = fopen(preset_ttl, "rb");
	if(in_fd)
	{
		SordWorld *world = sord_world_new();
		if(world)
		{
			SordModel *sord = sord_new(world, SORD_POS|SORD_SPO, false);
			if(sord)
			{
				SerdEnv *env = serd_env_new(&base);
				if(env)
				{
					SerdReader *reader = sord_new_reader(sord, env, SERD_TURTLE, NULL);
					if(reader)
					{
						serd_reader_read_file_handle(reader, in_fd, U8(preset_ttl));

						SordNode *P0 = sord_new_uri(world, U8(type_uri));
						if(P0)
						{
							SordNode *O = sord_new_uri(world, U8(pset_uri));
							if(O)
							{
								SordNode *P1 = sord_new_uri(world, state_uri);
								if(P1)
								{
									SordNode *P2 = sord_new_uri(world, code_uri);
									if(P2)
									{
										SordIter *presets = sord_search(sord, NULL, P0, O, NULL);
										if(presets)
										{
											int count = 0;
											int parsed = 0;

											for( ; !sord_iter_end(presets); sord_iter_next(presets))
											{
												count += 1;
												const SordNode *S1 = sord_iter_get_node(presets, SORD_SUBJECT);

												fprintf(out_fd,
														"do\n"
														"print('[test] %s')\n"
														"stash = nil\n"
														"apply = nil\n"
														"save = nil\n"
														"restore = nil\n"
														"once = nil\n"
														"run = nil\n\n", sord_node_get_string(S1));

												SordNode *preset = sord_get(sord, S1, P1, NULL, NULL);
												if(preset)
												{
													SordNode *code = sord_get(sord, preset, P2, NULL, NULL);
													if(code)
													{
														const char *str = (const char *)sord_node_get_string(code);
														if(str)
														{
															fprintf(out_fd, "%s\n", str);
															parsed += 1;
														}
														sord_node_free(world, code);
													}
													sord_node_free(world, preset);
												}
												fprintf(out_fd,
													"_test()\n"
													"collectgarbage()\n"
													"end\n\n");
											}
											sord_iter_free(presets);

											if(count == parsed) // have we parsed all presets?
												ret = 0;
										}
										sord_node_free(world, P2);
									}
									sord_node_free(world, P1);
								}
								sord_node_free(world, O);
							}
							sord_node_free(world, P0);
						}
						serd_reader_free(reader);
					}
					serd_env_free(env);
				}
				sord_free(sord);
			}
			sord_world_free(world);
		}
		fclose(in_fd);
	}
	serd_node_free(&base);

	if(argc > 2)
		fclose(out_fd);

	return ret;
}
