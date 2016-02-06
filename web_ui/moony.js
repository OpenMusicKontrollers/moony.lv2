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

function keepalive() {
	$.ajax({
		type: 'POST',
		url: '/keepalive',
		data: {},
		dataType: 'json',
		timeout: 60000, // 1min
		success: function(data) {
			if(data.code) {
				editor.setValue(data.code, 1);
			} else if(data.error) {
				$('#errmsg').html(data.error).fadeIn(300);
			} else if(data.trace) {
				var tracemsg = $('#tracemsg');
				tracemsg.append(data.trace + '<br />').scrollTop(tracemsg.prop('scrollHeight'));
			}
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

function get_code() {
	$.ajax({
		type: 'POST',
		url: '/code/get',
		data: {},
		dataType: 'json',
		success: function(data) {
			//alert(data);
		},
		error: function(request, status, err) {
			if(status == 'timeout') {
				alert('timeout');
			}
		}
	});
}

function set_code(code) {
	$.ajax({
		type: 'POST',
		url: '/code/set',
		data: JSON.stringify({code:code}),
		dataType: 'json',
		success: function(data) {
			//alert(data);
		},
		error: function(request, status, err) {
			if(status == 'timeout') {
				alert('timeout');
			}
		}
	});
}

function set_selection(selection) {
	$.ajax({
		type: 'POST',
		url: '/selection/set',
		data: JSON.stringify({selection:selection}),
		dataType: 'json',
		success: function(data) {
			//alert(data);
		},
		error: function(request, status, err) {
			if(status == 'timeout') {
				alert('timeout');
			}
		}
	});
}

function compile(editor) {
	var sel_range = editor.getSelectionRange();
	if(sel_range.isEmpty()) {
		set_code(editor.getValue());
	} else {
		var start_line = sel_range.start.row;
		var end_line = sel_range.end.row;
		var selection = '';
		for(var i=0; i<start_line; i++)
			selection = selection + '\n';
		selection = selection + editor.getSession().getTextRange(sel_range);
		set_selection(selection);
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
		set_selection(selection);
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
		tabSize: 2
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

	keepalive();
	get_code();
});
