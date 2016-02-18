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

var LV2_ATOM_PREFIX = "http://lv2plug.in/ns/ext/atom#";
var LV2_PATCH_PREFIX= "http://lv2plug.in/ns/ext/patch#";
var LV2_UI_PREFIX   = "http://lv2plug.in/ns/extensions/ui#";
var RDF_PREFIX      = "http://www.w3.org/1999/02/22-rdf-syntax-ns#";
var RDFS_PREFIX     = "http://www.w3.org/2000/01/rdf-schema#";
var MOONY_PREFIX    = "http://open-music-kontrollers.ch/lv2/moony#";

var SUBJECT         = "http://open-music-kontrollers.ch/lv2/moony"

var RDF = {
	value             : RDF_PREFIX + "value",
	type              : RDF_PREFIX + "type"
};

var RDFS = {
	label             : RDFS_PREFIX + "label",
	range             : RDFS_PREFIX + "range",
	comment           : RDFS_PREFIX + "comment"
};

var LV2 = {
	ATOM : {
		Int             : LV2_ATOM_PREFIX + "Int",
		Long            : LV2_ATOM_PREFIX + "Long",
		Float           : LV2_ATOM_PREFIX + "Float",
		Double          : LV2_ATOM_PREFIX + "Double",
		Bool            : LV2_ATOM_PREFIX + "Bool",
		URID            : LV2_ATOM_PREFIX + "URID",
		String          : LV2_ATOM_PREFIX + "String",
		URI             : LV2_ATOM_PREFIX + "URI",
		Path            : LV2_ATOM_PREFIX + "Path",
		Literal         : LV2_ATOM_PREFIX + "Literal",
		Chunk           : LV2_ATOM_PREFIX + "Chunk",
		Tuple           : LV2_ATOM_PREFIX + "Tuple",
		Object          : LV2_ATOM_PREFIX + "Object",
		Sequence        : LV2_ATOM_PREFIX + "Sequence",
		frameTime       : LV2_ATOM_PREFIX + "frameTime",
		beatTime        : LV2_ATOM_PREFIX + "beatTime",
		Event           : LV2_ATOM_PREFIX + "Event",
		Vector          : LV2_ATOM_PREFIX + "Vector",
		childType       : LV2_ATOM_PREFIX + "childType",

		eventTransfer   : LV2_ATOM_PREFIX + "eventTransfer",
		atomTransfer    : LV2_ATOM_PREFIX + "atomTransfer"
	},
	UI : {
		protocol        : LV2_UI_PREFIX + "protocol",
		floatProtocol   : LV2_UI_PREFIX + "floatProtocol",
		peakProtocol    : LV2_UI_PREFIX + "peakProtocol",
		portIndex       : LV2_UI_PREFIX + "portIndex",
		periodStart     : LV2_UI_PREFIX + "periodStart",
		periodSize      : LV2_UI_PREFIX + "periodSize",
		peak            : LV2_UI_PREFIX + "peak",
		windowTitle     : LV2_UI_PREFIX + "windowTitle"
	},
	PATCH : {
		Patch             : LV2_PATCH_PREFIX + "Patch",
		Get               : LV2_PATCH_PREFIX + "Get",
		Set               : LV2_PATCH_PREFIX + "Set",
		subject           : LV2_PATCH_PREFIX + "subject",
		property          : LV2_PATCH_PREFIX + "property",
		value             : LV2_PATCH_PREFIX + "value"
	}
};

var MOONY = {
	code              : MOONY_PREFIX + "code",
	selection         : MOONY_PREFIX + "selection",
	error             : MOONY_PREFIX + "error",
	trace             : MOONY_PREFIX + "trace"
};

function lv2_read_float(idx, value) {
	cosole.log(idx, value);
	//TODO
}

function lv2_read_peak(idx, value) {
	cosole.log(idx, value[LV2.UI.periodStart], value[LV2.UI.periodSize], value[LV2.UI2.peak]);
	//TODO
}

function lv2_read_atom(idx, value) {
	//TODO
}

function lv2_read_event(idx, obj) {
	var type = obj[RDF.type];
	var atom = obj[RDF.value];

	//TODO check idx == 'notify'
	//TODO check obj[RDFS.range] == LV2.ATOM.Object

	if(type == LV2.PATCH.Patch)
	{
		//TODO handle dynamic properties
	}
	else if(type == LV2.PATCH.Set)
	{
		var subject = atom[LV2.PATCH.subject];
		var property = atom[LV2.PATCH.property];
		var value = atom[LV2.PATCH.value];

		//TODO check for matching subject

		if(property && (property[RDFS.range] == LV2.ATOM.URID) && value) {
			if( (property[RDF.value] == LV2.UI.windowTitle) && (value[RDFS.range] == LV2.ATOM.String) ) {
				document.title = (value[RDF.value]);
			} else if( (property[RDF.value] == MOONY.code) && (value[RDFS.range] == LV2.ATOM.String) ) {
				editor.setValue(value[RDF.value], 1);
			} else if( (property[RDF.value] == MOONY.error) && (value[RDFS.range] == LV2.ATOM.String) ) {
				var errmsg = $('#errmsg');
				errmsg.html(value[RDF.value]).fadeIn(300);
			} else if( (property[RDF.value] == MOONY.trace) && (value[RDFS.range] == LV2.ATOM.String) ) {
				var tracemsg = $('#tracemsg');
				tracemsg.append(value[RDF.value] + '<br />').scrollTop(tracemsg.prop('scrollHeight'));
			} else {
				//TODO handle dynamic properties
			}
		}
	}
}

var lv2_read = {
	[LV2.UI.floatProtocol]  : lv2_read_float,
	[LV2.UI.peakProtocol]   : lv2_read_peak,
	[LV2.ATOM.atomTransfer] : lv2_read_atom,
	[LV2.ATOM.eventTransfer] : lv2_read_event
};

function lv2_success(data) {
	var idx = data[LV2.UI.portIndex];
	var prot = data[LV2.UI.protocol];
	var value = data[RDF.value];

	if(idx && prot && value)
	{
		var callback = lv2_read[prot];
		if(callback)
			callback(idx, value);
	}
}

function keepalive() {
	$.ajax({
		type: 'POST',
		url: '/keepalive',
		data: JSON.stringify({}),
		dataType: 'json',
		timeout: 60000, // 1min
		success: function(data) {
			lv2_success(data);

			$('#status').html('connected');
			$('.clip').show();
			keepalive();
		},
		error: function(request, status, err) {
			if(status == 'timeout') {
				//alert('timeout');
				$('#status').html('connected');
				$('.clip').show();
				keepalive();
			} else {
				$('#status').html('disconnected');
				$('.clip').hide();
				$('#tracemsg').empty();
			}
		}
	});
}

function lv2_write(url, o) {
	$.ajax({
		type: 'POST',
		url: url,
		data: JSON.stringify(o),
		dataType: 'json',
		success: function(data) {
			lv2_success(data);
		},
		error: function(request, status, err) {
			if(status == 'timeout') {
				alert('timeout');
			}
		}
	});
}

function lv2_ui(o) {
	lv2_write('/lv2/ui', o);
}

function lv2_dsp(o) {
	lv2_write('/lv2/dsp', o);
}

function lv2_get(func, property) {
	func({
		[LV2.UI.protocol] : LV2.ATOM.eventTransfer,
		[LV2.UI.portIndex] : 'control',
		[RDF.value] : {
			[RDFS.range] : LV2.ATOM.Object,
			[RDF.type] : LV2.PATCH.Get,
			[RDF.value] : {
				[LV2.PATCH.subject] : {
					[RDFS.range] : LV2.ATOM.URID,
					[RDF.value] : SUBJECT
				},
				[LV2.PATCH.property] : {
					[RDFS.range] : LV2.ATOM.URID,
					[RDF.value] : property
				}
			}
		}
	});
}

function lv2_set(func, property, range, value) {
	func({
		[LV2.UI.protocol] : LV2.ATOM.eventTransfer,
		[LV2.UI.portIndex] : 'control',
		[RDF.value] : {
			[RDFS.range] : LV2.ATOM.Object,
			[RDF.type] : LV2.PATCH.Set,
			[RDF.value] : {
				[LV2.PATCH.subject] : {
					[RDFS.range] : LV2.ATOM.URID,
					[RDF.value] : SUBJECT
				},
				[LV2.PATCH.property] : {
					[RDFS.range] : LV2.ATOM.URID,
					[RDF.value] : property
				},
				[LV2.PATCH.value] : {
					[RDFS.range] : range,
					[RDF.value] : value
				}
			}
		}
	});
}

function compile(editor) {
	var sel_range = editor.getSelectionRange();
	if(sel_range.isEmpty()) {
		var selection = editor.getValue();
		lv2_set(lv2_dsp, MOONY.code, LV2.ATOM.String, selection);
	} else {
		var start_line = sel_range.start.row;
		var end_line = sel_range.end.row;
		var selection = '';
		for(var i=0; i<start_line; i++)
			selection = selection + '\n';
		selection = selection + editor.getSession().getTextRange(sel_range);
		lv2_set(lv2_dsp, MOONY.selection, LV2.ATOM.String, selection);
		editor.clearSelection();
	}
	$('#errmsg').html('<br />').fadeOut(100);
	$("#compile").parent().fadeOut(100).fadeIn(300);
}

function compile_line(editor) {
	var cur_line = editor.getCursorPosition().row;
	if(cur_line) {
		var selection = '';
		for(var i=0; i<cur_line; i++)
			selection = selection + '\n';
		selection = selection + editor.getSession().getLine(cur_line);
		lv2_set(lv2_dsp, MOONY.selection, LV2.ATOM.String, selection);
		editor.clearSelection();
	}
	$('#errmsg').html('<br />').fadeOut(100);
	$("#compile_line").parent().fadeOut(100).fadeIn(300);
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
		$('#tracemsg').empty();
		e.preventDefault();
	});

	// start keepalive loop
	keepalive();

	// query initial props
	lv2_get(lv2_ui, LV2.UI.windowTitle);
	lv2_get(lv2_dsp, MOONY.code);
	lv2_get(lv2_dsp, MOONY.error);
});
