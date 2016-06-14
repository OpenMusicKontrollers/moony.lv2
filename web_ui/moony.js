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

var editor = null;
var tracecnt = 0;
var socket_di = null;

var LV2_CORE_PREFIX = 'http://lv2plug.in/ns/lv2core#';
var LV2_ATOM_PREFIX = 'http://lv2plug.in/ns/ext/atom#';
var LV2_PATCH_PREFIX= 'http://lv2plug.in/ns/ext/patch#';
var LV2_UI_PREFIX   = 'http://lv2plug.in/ns/extensions/ui#';
var LV2_UNITS_PREFIX= 'http://lv2plug.in/ns/extensions/units#';
var RDF_PREFIX      = 'http://www.w3.org/1999/02/22-rdf-syntax-ns#';
var RDFS_PREFIX     = 'http://www.w3.org/2000/01/rdf-schema#';
var MOONY_PREFIX    = 'http://open-music-kontrollers.ch/lv2/moony#';

var context = {
	'lv2'             : LV2_CORE_PREFIX,
	'atom'            : LV2_ATOM_PREFIX,
	'patch'           : LV2_PATCH_PREFIX,
	'ui'              : LV2_UI_PREFIX,
	'units'           : LV2_UNITS_PREFIX,
	'rdf'             : RDF_PREFIX,
	'rdfs'            : RDFS_PREFIX,
	'moony'           : MOONY_PREFIX
};

var options = {
	'compactArrays'   : true,
	'skipExpansion'   : false,
	'expandContext'   : context,
};

var SUBJECT         = undefined;

var RDF = {
	value             : '@value',
	type              : '@type',
	id                : '@id',
	list              : '@list',
	set               : '@set'
};

var RDFS = {
	label             : 'rdfs:label',
	range             : 'rdfs:range',
	comment           : 'rdfs:comment'
};

var ATOM = {
	Int               : 'atom:Int',
	Long              : 'atom:Long',
	Float             : 'atom:Float',
	Double            : 'atom:Double',
	Bool              : 'atom:Bool',
	URID              : 'atom:URID',
	String            : 'atom:String',
	URI               : 'atom:URI',
	Path              : 'atom:Path',
	Property          : 'atom:Property',
	Literal           : 'atom:Literal',
	Chunk             : 'atom:Chunk',
	Tuple             : 'atom:Tuple',
	Object            : 'atom:Object',
	Sequence          : 'atom:Sequence',
	frameTime         : 'atom:frameTime',
	beatTime          : 'atom:beatTime',
	Event             : 'atom:Event',
	Vector            : 'atom:Vector',
	childType         : 'atom:childType',

	eventTransfer     : 'atom:eventTransfer',
	atomTransfer      : 'atom:atomTransfer'
};

var UI = {
	portNotification  : 'ui:portNotification',
	portEvent         : 'ui:portEvent',
	protocol          : 'ui:protocol',
	floatProtocol     : 'ui:floatProtocol',
	peakProtocol      : 'ui:peakProtocol',
	periodStart       : 'ui:periodStart',
	periodSize        : 'ui:periodSize',
	peak              : 'ui:peak',
	windowTitle       : 'ui:windowTitle'
};

var PATCH = {
	Patch             : 'patch:Patch',
	Get               : 'patch:Get',
	Set               : 'patch:Set',
	subject           : 'patch:subject',
	property          : 'patch:property',
	value             : 'patch:value',
	remove            : 'patch:remove',
	add               : 'patch:add',
	wildcard          : 'patch:wildcard',
	writable          : 'patch:writable',
	readable          : 'patch:readable',
	destination       : 'patch:destination'
};

var LV2 = {
	minimum           : 'lv2:minimum',
	maximum           : 'lv2:maximum',
	scalePoint        : 'lv2:scalePoint',
	symbol            : 'lv2:symbol'
};

var UNITS = {
	unit              : 'units:unit',
	s                 : 'units:s',
	ms                : 'units:ms',
	min               : 'units:min',
	bar               : 'units:bar',
	beat              : 'units:beat',
	frame             : 'units:frame',
	m                 : 'units:m',
	cm                : 'units:cm',
	mm                : 'units:mm',
	km                : 'units:km',
	inch              : 'units:inch',
	mile              : 'units:mile',
	dB                : 'units:db',
	pc                : 'units:pc',
	coef              : 'units:coef',
	hz                : 'units:hz',
	khz               : 'units:khz',
	mhz               : 'units:mhz',
	bpm               : 'units:bpm',
	oct               : 'units:oct',
	cent              : 'units:cent',
	semitone12TET     : 'units:semitone12TET',
	degree            : 'units:degree',
	midiNote          : 'units:midiNote',
	midiController    : 'units:midiController'
};

var MOONY = {
	code              : 'moony:code',
	selection         : 'moony:selection',
	error             : 'moony:error',
	trace             : 'moony:trace',
	ui                : 'moony:ui',
	dsp               : 'moony:dsp',
	destination       : 'moony:destination'
};

var format = {
	[UNITS.s]             : 's',
	[UNITS.ms]            : 'ms',
	[UNITS.min]           : 'min',
	[UNITS.bar]           : 'bars',
	[UNITS.beat]          : 'beats',
	[UNITS.frame]         : 'frames',
	[UNITS.m]             : 'm',
	[UNITS.cm]            : 'cm',
	[UNITS.mm]            : 'mm',
	[UNITS.km]            : 'km',
	[UNITS.inch]          : 'in',
	[UNITS.mile]          : 'mi',
	[UNITS.dB]            : 'dB',
	[UNITS.pc]            : '%',
	[UNITS.coef]          : '',
	[UNITS.hz]            : 'Hz',
	[UNITS.khz]           : 'kHz',
	[UNITS.mhz]           : 'MHz',
	[UNITS.bpm]           : 'BPM',
	[UNITS.oct]           : 'oct',
	[UNITS.cent]          : 'ct',
	[UNITS.semitone12TET] : 'semi',
	[UNITS.degree]        : 'deg',
	[UNITS.midiNote]      : 'note', //TODO function callback
	[UNITS.midiController]: 'controller' //TODO function callback
};

var trim = /[^a-zA-Z0-9_]+/g;

function node_is_a(node, id) {
	var types = node[RDF.type];
	return types && ( (types == id) || (types.indexOf(id) != -1) );
}

function lv2_get(destination, property) {
	var req = {
		[RDF.type]            : [ATOM.Object, UI.portNotification],
		[LV2.symbol] : {
			[RDF.type]          : [ATOM.String],
			[RDF.value]         : 'control'
		},
		[MOONY.destination]   : {[RDF.id] : destination},
		[UI.protocol]         : {[RDF.id] : ATOM.eventTransfer},
		[UI.portEvent] : {
			[RDF.type]          : [ATOM.Object, PATCH.Get],
			[PATCH.property]    : {[RDF.id] : property}
		}
	};
	if(SUBJECT) {
		req[UI.portEvent][PATCH.subject] = {[RDF.id] : SUBJECT}
	}
	lv2_write(req);
}

function lv2_get_all(destination) {
	var req = {
		[RDF.type]            : [ATOM.Object, UI.portNotification],
		[LV2.symbol] : {
			[RDF.type]          : [ATOM.String],
			[RDF.value]         : 'control'
		},
		[MOONY.destination]   : {[RDF.id] : destination},
		[UI.protocol]         : {[RDF.id] : ATOM.eventTransfer},
		[UI.portEvent] : {
			[RDF.type]          : [ATOM.Object, PATCH.Get]
		}
	};
	if(SUBJECT) {
		req[UI.portEvent][PATCH.subject] = {[RDF.id] : SUBJECT}
	}
	lv2_write(req);
}

function lv2_set(destination, property, range, value) {
	var req = {
		[RDF.type]            : [ATOM.Object, UI.portNotification],
		[LV2.symbol] : {
			[RDF.type]          : [ATOM.String],
			[RDF.value]         : 'control'
		},
		[MOONY.destination]   : {[RDF.id] : destination},
		[UI.protocol]         : {[RDF.id] : ATOM.eventTransfer},
		[UI.portEvent] : {
			[RDF.type]          : [ATOM.Object, PATCH.Set],
			[PATCH.property]    : {[RDF.id] : property},
			[PATCH.value] : {
				[RDF.type]        : [range],
				[RDF.value]       : value
			}
		}
	};
	if(SUBJECT) {
		req[UI.portEvent][PATCH.subject] = {[RDF.id] : SUBJECT}
	}
	lv2_write(req);
}

function lv2_control(destination, symbol, value) {
	var req = {
		[RDF.type]            : [ATOM.Object, UI.portNotification],
		[LV2.symbol] : {
			[RDF.type]          : [ATOM.String],
			[RDF.value]         : symbol
		},
		[MOONY.destination]   : {[RDF.id] : destination},
		[UI.protocol]         : {[RDF.id] : UI.floatProtocol},
		[UI.portEvent] : {
			[RDF.type]          : [ATOM.Float],
			[RDF.value]         : value
		}
	};
	lv2_write(req);
}

function float_wheel(e) {
	var item = $(this);
	var delta = e.originalEvent.deltaY;
	var symbol = item.attr('id');

	var min = Number(item.attr('min'));
	var max = Number(item.attr('max'));
	var step = Number(item.attr('step'));
	var newval = Number(item.val());
	if(delta < 0)
		newval = newval - step;
	else if(delta > 0)
		newval = newval + step;
	if( (newval >= min) && (newval <= max) )
	{
		item.val(newval);
		lv2_control(MOONY.dsp, symbol, newval);
	}

	e.preventDefault();
}

function float_change(e) {
	var item = $(this);
	var symbol = item.attr('id');
	var newval = Number(item.val());

	lv2_control(MOONY.dsp, symbol, newval);
}

function lv2_read_float(symbol, obj) {
	var props = $('#properties');
	var control = $('#' + symbol);
	var value = obj[RDF.value];
	if(control.length !== 0) {
		control.val(value);
	} else {
		props.append('<tr class="control" data-id="'+symbol+'"><td class="label">'+symbol+'</td><td><input id="'+symbol+'" min="0" max="1" step="0.001" type="number" /></td></tr>');
		control = $('#' + symbol);
		if(symbol.indexOf('output') !== -1)
			control.prop('disabled', true);
		control.bind('wheel', float_wheel).change(float_change);
		control.val(value);

		sort_properties();
	}
}

function lv2_read_peak(symbol, obj) {
	//TODO
}

function lv2_read_atom(symbol, obj) {
	//TODO
}

function sort_properties() {
	var props = $('#properties');
	var childs = props.children();

	childs.sort(function(a, b) {
		var id_a = a.getAttribute('data-id');
		var id_b = b.getAttribute('data-id');
		var class_a = a.getAttribute('class');
		var class_b = b.getAttribute('class');
		var cmp = class_b.localeCompare(class_a);
		if(cmp)
			return cmp;
		else
			return id_a.localeCompare(id_b);
	});

	childs.detach().appendTo(props);
}

function sort_options(item) {
	var childs = item.children();

	childs.sort(function(a, b) {
		var val_a = Number(a.getAttribute('value'));
		var val_b = Number(b.getAttribute('value'));
		if(val_a < val_b) return -1;
		else if(val_a > val_b) return 1;
		else return 0;
	});

	childs.detach().appendTo(item);
}

function property_wheel(e) {
	var item = $(this);
	var delta = e.originalEvent.deltaY;
	var name = item.attr('name');
	var range = item.attr('data-range');

	if(item.is('input')) {
		if(range == ATOM.Bool) {
			var newval = !item.prop('checked');
			item.prop('checked', newval);
			lv2_set(MOONY.dsp, name, range, newval);
		} else {
			var min = Number(item.attr('min'));
			var max = Number(item.attr('max'));
			var step = Number(item.attr('step'));
			var newval = Number(item.val());
			if(delta < 0)
				newval = newval - step;
			else if(delta > 0)
				newval = newval + step;
			if( (newval >= min) && (newval <= max) )
			{
				item.val(newval);
				lv2_set(MOONY.dsp, name, range, newval);
			}
		}
	} else if(item.is('select')) {
		var selected = item.find(':selected');
		var newval;
		selected.prop('selected', false);
		if(delta < 0)
			selected.prev().prop('selected', true);
		else if(delta > 0)
			selected.next().prop('selected', true);
		newval = Number(item.val());
		lv2_set(MOONY.dsp, name, range, newval);
	}

	e.preventDefault();
}

function property_change(e) {
	var item = $(this);
	var name = item.attr('name');
	var range = item.attr('data-range');
	var newval;
	if(  (range == ATOM.Int)
		|| (range == ATOM.Long)
		|| (range == ATOM.Float)
		|| (range == ATOM.Double) ) {
		newval = Number(item.val());
	} else if(range == ATOM.Bool) {
		newval = item.prop('checked');
	} else {
		newval = item.val();
	}

	lv2_set(MOONY.dsp, name, range, newval);
}

function update_property_step(item) {
	var range = item.attr('data-range');
	var min = item.attr('min');
	var max = item.attr('max');
	if(range && min && max) {
		if( (range == ATOM.Float) || (range == ATOM.Double) )
			item.attr('step', (Number(max) - Number(min)) / 100.0);
	}
}

function lv2_read_event(symbol, obj) {
	if( ( (symbol == 'notify') || (symbol == 'control') )
	  && node_is_a(obj, ATOM.Object) )
	{
		if(node_is_a(obj, PATCH.Patch)) {
			var subject = obj[PATCH.subject];
			var remove = obj[PATCH.remove];
			var add = obj[PATCH.add];

			if(remove && node_is_a(remove, ATOM.Object) && add && node_is_a(add, ATOM.Object)) {
				for(var key in remove) {
					var value = remove[key];

					if( (key == RDF.type) || (key == RDF.id) )
						continue; // skip

					if(key == PATCH.writable) {
						var uri = value[RDF.id];
						//TODO check subject
						if(uri == PATCH.wildcard) {
							$('.writable').remove();
						} else {
							var id = uri.replace(trim, '');
							$('#' + id).remove();
						}
					} else if(key == PATCH.readable) {
						var uri = value[RDF.id];
						//TODO check subject
						if(uri == PATCH.wildcard) {
							$('.readable').remove();
						} else {
							var id = uri.replace(trim, '');
							$('#' + id).remove();
						}
					}
					else {
						//TODO
					}
				}

				console.log(add);
				for(var key in add) {
					var value = add[key];

					if( (key == RDF.type) || (key == RDF.id) )
						continue; // skip

					if(key == PATCH.writable) {
						//TODO check subject
						var uri = value[RDF.id];
						var id = uri.replace(trim, '');
						var props = $('#properties');

						props.append('<tr class="writable" data-id="'+id+'"><td class="label"></td><td><input id="'+id+'" name="'+uri+'" /></td><td class="unit"></td></tr>');
						sort_properties();

						$('#' + id).bind('wheel', property_wheel).change(property_change);
						
						lv2_get(MOONY.dsp, uri);
					} else if(key == PATCH.readable) {
						//TODO check subject
						var uri = value[RDF.id];
						var id = uri.replace(trim, '');
						var props = $('#properties');

						props.append('<tr class="readable" data-id="'+id+'"><td class="label"></td><td><input id="'+id+'" name="'+uri+'" disabled /></td><td class="unit"></td></tr>');
						sort_properties();

						lv2_get(MOONY.dsp, uri);
					} else {
						var id = subject[RDF.id].replace(trim, '');
						var item = $('#' + id);
						if(key == RDFS.label) {
							item.parent().parent().children('.label').html(value[RDF.value]);
						} else if(key == RDFS.comment) {
							item.attr('title', value[RDF.value]).attr('alt', value[RDF.value])
						} else if(key == RDFS.range) {
							var range = value[RDF.id];
							item.attr('data-range', range);
							if(  (range == ATOM.Int)
								|| (range == ATOM.Long) ) {
								item.attr('type', 'number').attr('step', 1);
							} else if( (range == ATOM.Float)
								|| (range == ATOM.Double) ) {
								item.attr('type', 'number').attr('step', 'any');
							} else if(range == ATOM.Bool) {
								item.attr('type', 'checkbox');
							} else if(range == ATOM.Path) {
								item.attr('type', 'file');
							} else if( (range == ATOM.URI)
								|| (range == ATOM.URID) ) {
								item.attr('type', 'url');
							} else if( (range == ATOM.String)
								|| (range == ATOM.Literal)
								|| (range == ATOM.Chunk) ) {
								item.attr('type', 'text');
							}
						} else if(key == LV2.minimum) {
							item.attr('min', value[RDF.value]);
							update_property_step(item);
						} else if(key == LV2.maximum) {
							item.attr('max', value[RDF.value]);
							update_property_step(item);
						} else if(key == LV2.scalePoint) {
							var list = value[RDF.list];
							for(i in list) {
								var itm = list[i];
								var label = itm[RDFS.label];
								var val = itm['rdf:value']; //FIXME
					
								if(item.is('input')) { // change type from input -> select
									var name = item.attr('name');
									var id = item.attr('id');
									var title = item.attr('title');
									var alt = item.attr('alt');
									var range = item.attr('data-range');
									var min = item.attr('min');
									var max = item.attr('max');
									var step = item.attr('step');
									item.replaceWith('<select id="'+id+'" />');
									item = $('#' + id);
									item.attr('name', name).attr('title', title).attr('alt', alt);
									item.attr('min', min).attr('max', max).attr('step', step).attr('data-range', range);
									item.bind('wheel', property_wheel).change(property_change);
								}

								item.append('<option value="'+val[RDF.value]+'">'+label[RDF.value]+'</option>');
								sort_options(item);
								if(Number(item.attr('min')) > val[RDF.value])
									item.attr('min', val[RDF.value]);
								if(Number(item.attr('max')) < val[RDF.value])
									item.attr('max', val[RDF.value]);
							}
						} else if(key == UNITS.unit) {
							item.parent().parent().children('.unit').html(format[value[RDF.id]]);
						}
					}
				}
			}
		}
		else if(node_is_a(obj, PATCH.Set))
		{
			//var subject = obj[PATCH.subject];
			var property = obj[PATCH.property][RDF.id];
			var value = obj[PATCH.value]

			//TODO check for matching subject

			if(property && value) {
				if( (property == UI.windowTitle) && node_is_a(value, ATOM.String) ) {
					document.title = value[RDF.value];
				} else if( (property == PATCH.subject) && value[RDF.id] ) {
					SUBJECT = value[RDF.id];
				} else if( (property == MOONY.code) && node_is_a(value, ATOM.String) ) {
					editor.setValue(value[RDF.value], 1);
					lv2_get_all(MOONY.dsp); // update properties
				} else if( (property == MOONY.error) && node_is_a(value, ATOM.String) ) {
					var errmsg = $('#errmsg');
					errmsg.html(value[RDF.value]).fadeIn(300);
				} else if( (property == MOONY.trace) && node_is_a(value, ATOM.String) ) {
					var tracemsg = $('#tracemsg');
					tracemsg.append(tracecnt + ': ' + value[RDF.value] + '<br />').scrollTop(tracemsg.prop('scrollHeight'));
					tracecnt += 1;
				} else {
					var item = $('#' + property.replace(trim, ''));
					var range = item.attr('data-range');
					if(range == ATOM.Bool) {
						item.prop('checked', value[RDF.value]);
					} else {
						item.val(value[RDF.value]);
					}
				}
			}
		}
	}
}

var lv2_read = {
	[UI.floatProtocol]  : lv2_read_float,
	[UI.peakProtocol]   : lv2_read_peak,
	[ATOM.atomTransfer] : lv2_read_atom,
	[ATOM.eventTransfer] : lv2_read_event
};

function lv2_success(data) {
	jsonld.compact(data, context, options, function(err, compacted, ctx) {
		if(err)
			return;
		
		if(node_is_a(compacted, UI.portNotification)) {
			var symbol = compacted[LV2.symbol][RDF.value];
			var prot = compacted[UI.protocol][RDF.id];
			var value = compacted[UI.portEvent];

			if(symbol && prot)
			{
				var callback = lv2_read[prot];
				if(callback)
					callback(symbol, value);
			}
		}
	})
}

function lv2_write(o) {
	jsonld.expand(o, options, function(err, expanded) {
		if(err)
			return;
		socket_di.send(JSON.stringify(expanded));
	})
}

function compile(editor) {
	var sel_range = editor.getSelectionRange();
	if(sel_range.isEmpty()) {
		var selection = editor.getValue();
		lv2_set(MOONY.dsp, MOONY.code, ATOM.String, selection);
	} else {
		var start_line = sel_range.start.row;
		var end_line = sel_range.end.row;
		var selection = '';
		for(var i=0; i<start_line; i++)
			selection = selection + '\n';
		selection = selection + editor.getSession().getTextRange(sel_range);
		lv2_set(MOONY.dsp, MOONY.selection, ATOM.String, selection);
		editor.clearSelection();
	}
	$('#errmsg').html('<br />').fadeOut(100);
	$('#compile').parent().fadeOut(100).fadeIn(300);
}

function compile_line(editor) {
	var cur_line = editor.getCursorPosition().row;
	if(cur_line) {
		var selection = '';
		for(var i=0; i<cur_line; i++)
			selection = selection + '\n';
		selection = selection + editor.getSession().getLine(cur_line);
		lv2_set(MOONY.dsp, MOONY.selection, ATOM.String, selection);
		editor.clearSelection();
	}
	$('#errmsg').html('<br />').fadeOut(100);
	$("#compile_line").parent().fadeOut(100).fadeIn(300);
}

function clear_log() {
	$('#tracemsg').empty();
	$("#clear").parent().fadeOut(100).fadeIn(300);
	tracecnt = 0;
}

function ws_url() {
	var pcol;
	var u = document.URL;

	pcol = 'ws://';
	if (u.substring(0, 4) == 'http')
		u = u.substr(7);

	u = u.split('/');

	/* + "/xxx" bit is for IE10 workaround */

	return pcol + u[0] + '/xxx';
}

function ws_connect() {
	if(typeof MozWebSocket != 'undefined') {
		socket_di = new MozWebSocket(ws_url(), 'lv2-protocol');
	} else {
		socket_di = new WebSocket(ws_url(), 'lv2-protocol');
	}

	try {
		socket_di.onopen = function() {
			$('#status').html('connected');
			$('.clip').show();

			// query initial props
			lv2_get(MOONY.ui, UI.windowTitle);
			lv2_get(MOONY.ui, PATCH.subject);
			lv2_get(MOONY.dsp, MOONY.code);
			lv2_get(MOONY.dsp, MOONY.error);
			lv2_get_all(MOONY.dsp);
		} 

		socket_di.onmessage =function got_packet(msg) {
			if(msg && msg.data) {
				var data = JSON.parse(msg.data);

				if(data && data.jobs) {
					for(i in data.jobs) {
						lv2_success(data.jobs[i]);
					}
				}
			}
		} 

		socket_di.onclose = function(){
			$('#status').html('disconnected');
			$('.clip').hide();
			clear_log();
		}
	} catch(exception) {
		console.log('webocket exception', exception);
	}
}

$(document).ready(function() {
	var session = null;
	editor = ace.edit("editor");
	session = editor.getSession();

	editor.setTheme("ace/theme/chaos");
	editor.setKeyboardHandler("ace/keyboard/vim");
	session.setUseWorker(false);
	session.setMode("ace/mode/lua");
	$('#vim').prop('checked', true);
	
	editor.commands.addCommand({
		name: 'compile selection/all',
		bindKey: {win: 'Shift-Return',  mac: 'Shift-Return'},
		exec: function(editor) {
			compile(editor);
		},
		readOnly: true // false if this command should not apply in readOnly mode
	});
	editor.commands.addCommand({
		name: 'compile line',
		bindKey: {win: 'Ctrl-Shift-Return',  mac: 'Ctrl-Shift-Return'},
		exec: function(editor) {
			compile_line(editor);
		},
		readOnly: true // false if this command should not apply in readOnly mode
	});
	editor.commands.addCommand({
		name: 'clear log',
		bindKey: {win: 'Ctrl-L',  mac: 'Ctrl-L'},
		exec: function(editor) {
			clear_log();
		},
		readOnly: true // false if this command should not apply in readOnly mode
	});

	editor.$blockScrolling = Infinity
	editor.setOptions({
		minLines: 32,
    maxLines: Infinity,
		showPrintMargin: false,
		fontSize: 14,
		tabSize: 2,
		displayIndentGuides: false
	});

	$('input[name=mode]').click(function(e) {
		editor.setKeyboardHandler($(this).prop('value'));
	});
	$('#compile').click(function(e) {
		compile(editor);
		e.preventDefault();
	});
	$('#compile_line').click(function(e) {
		compile_line(editor);
		e.preventDefault();
	});
	$('#clear').click(function(e) {
		clear_log();
		e.preventDefault();
	});

	ws_connect();
});
