-- Copyright 2006-2016 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Lua LPeg l.
-- Original written by Peter Odding, 2007/04/04.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local T = {
	WHITESPACE	= 0xdddddd,
	KEYWORD			= 0x00698f,
	FUNCTION		= 0x00aeef,
	CONSTANT		= 0xff6600,
	LIBRARY			= 0x8dff0a,
	IDENTIFIER	= 0xdddddd,
	STRING			= 0x58c554,
	COMMENT			= 0x555555,
	NUMBER			= 0xfbfb00,
	LABEL				= 0xfdc251,
	OPERATOR		= 0xcc0000,
	BRACE				= 0xffffff
}

local M = {_NAME = 'moony'}

-- Whitespace.
local ws = token(T.WHITESPACE, l.space^1)

local longstring = lpeg.Cmt('[' * lpeg.C(P('=')^0) * '[', function(input, index, eq)
	local _, e = input:find(']'..eq..']', index, true)
	return (e or #input) + 1
end)

-- Comments.
local line_comment = '--' * l.nonnewline ^0
local block_comment = '--' * longstring
local comment = token(T.COMMENT, block_comment + line_comment)

-- Strings.
local sq_str = l.delimited_range("'", true, false, true)
local dq_str = l.delimited_range('"', true, false, true)
local string = token(T.STRING, sq_str + dq_str + longstring)

-- Numbers.
local lua_integer = P('-')^-1 * (l.hex_num + l.dec_num)
local number = token(T.NUMBER, l.float + lua_integer)

-- Keywords.
local keyword = token(T.KEYWORD, word_match{
	'while',
	'do',
	'end',
	'repeat',
	'until',
	'if',
	'then',
	'elseif',
	'else',
	'for',
	'goto',
	'break',
	'return',
	'local',
	'function',
	'in'
})

-- Functions.
local func = token(T.FUNCTION, word_match{
	-- Lua basic
	'collectgarbage',
	'error',
	'getmetatable',
	'ipairs',
	'load',
	'next',
	'pairs',
	'pcall',
	'print',
	'rawequal',
	'rawget',
	'rawlen',
	'rawset',
	'select',
	'setmetatable',
	'tonumber',
	'tostring',
	'type',
	'xpcall',
	'assert',
	-- Lua metamethods
	'__add',
	'__sub',
	'__mul',
	'__div',
	'__mod',
	'__pow',
	'__unm',
	'__idiv',
	'__band',
	'__bor',
	'__bxor',
	'__bnot',
	'__shl',
	'__shr',
	'__concat',
	'__len',
	'__eq',
	'__lt',
	'__le',
	'__index',
	'__newindex',
	'__call',
	'__gc',
	'__mode',
	'__name',
	'__tostring',
	'__metatable',
	'__pairs',
	-- moony basic
	'run',
	'once',
	'save',
	'restore',
	'stash',
	'apply',
	'midi2cps',
	'cps2midi',
	'MIDIResponder',
	'OSCResponder',
	'TimeResponder',
	'StateResponder',
	'Map',
	'Unmap',
	'VoiceMap',
	'Stash',
	'Note',
	'HashMap',
	'Parameter',
	'Options'
})

local function lib_func(name, keys)
	local post = nil
	for i, v in ipairs(keys) do
		local pat = P(v)
		post = post and (post + pat) or pat
	end
	return token(T.LIBRARY, P(name)) * (token(T.OPERATOR, S('.')) * token(T.FUNCTION, post))^-1
end

local function lib_const(name, keys)
	local post = nil
	for i, v in ipairs(keys) do
		local pat = P(v)
		post = post and (post + pat) or pat
	end
	return token(T.LIBRARY, P(name)) * (token(T.OPERATOR, S('.')) * token(T.CONSTANT, post))^-1
end

local lpeg_lib_func = lib_func('lpeg', {
	'R',
	'Cf',
	'Cg',
	'Carg',
	'Cb',
	'P',
	'Cc',
	'match',
	'Cp',
	'Ct',
	'Cs',
	'pcode',
	'B',
	'version',
	'ptree',
	'type',
	'setmaxstack',
	'Cm',
	'V',
	'locale',
	'S',
	'C'
})
local lpeg_lib = lpeg_lib_func

local string_lib_func = lib_func('string', {
	'lower',
	'format',
	'packsize',
	'upper',
	'byte',
	'gsub',
	'find',
	'sub',
	'dump',
	'unpack',
	'char',
	'len',
	'gmatch',
	'pack',
	'match',
	'rep',
	'reverse'
})
local string_lib = string_lib_func

local coroutine_lib_func = lib_func('coroutine', {
	'yield',
	'isyieldable',
	'running',
	'wrap',
	'status',
	'create',
	'resume'
})
local coroutine_lib = coroutine_lib_func

local utf8_lib_func = lib_func('utf8', {
	'len',
	'codes',
	'codepoint',
	'char',
	'offset'
})
local utf8_lib_const = lib_const('utf8', {
	'charpattern'
})
local utf8_lib = utf8_lib_func + utf8_lib_const

local table_lib_func = lib_func('table', {
	'pack',
	'sort',
	'unpack',
	'concat',
	'insert',
	'remove',
	'move'
})
local table_lib = table_lib_func

local math_lib_func = lib_func('math', {
	'exp',
	'type',
	'randomseed',
	'deg',
	'atan',
	'cos',
	'floor',
	'rad',
	'sin',
	'asin',
	'fmod',
	'ult',
	'log',
	'tan',
	'min',
	'sqrt',
	'random',
	'tointeger',
	'max',
	'acos',
	'abs',
	'modf',
	'ceil'
})
local math_lib_const = lib_const('math', {
	'mininteger',
	'pi',
	'maxinteger',
	'huge'
})
local math_lib = math_lib_func + math_lib_const

local debug_lib_func = lib_func('debug', {
	'traceback',
	'setupvalue',
	'setmetatable',
	'getinfo',
	'getupvalue',
	'setlocal',
	'getuservalue',
	'sethook',
	'getmetatable',
	'setuservalue',
	'gethook',
	'upvalueid',
	'upvaluejoin',
	'getregistry',
	'debug',
	'getlocal'
})
local debug_lib = debug_lib_func

local base64_lib_func = lib_func('base64', {
	'encode',
	'decode'
})
local base64_lib_const = lib_const('base64', {
	'version'
})
local base64_lib = base64_lib_func + base64_lib_const

local ascii85_lib_func = lib_func('ascii85', {
	'encode',
	'decode'
})
local ascii85_lib_const = lib_const('ascii85', {
	'version'
})
local ascii85_lib = ascii85_lib_func + ascii85_lib_const

local aes128_lib_func = lib_func('aes128', {
	'encode',
	'decode'
})
local aes128_lib = aes128_lib_func

local atom_lib = lib_const('Atom', {
	'Bool',
	'Chunk',
	'Double',
	'Float',
	'Int',
	'Long',
	'Literal',
	'Object',
	'Path',
	'Property',
	'Sequence',
	'String',
	'Tuple',
	'URI',
	'URID',
	'Vector',
	'frameTime',
	'beatTime'
})

local midi_lib = lib_const('MIDI', {
	'MidiEvent',
	'NoteOff',
	'NoteOn',
	'NotePressure',
	'Controller',
	'ProgramChange',
	'ChannelPressure',
	'Bender',
	'SystemExclusive',
	'QuarterFrame',
	'SongPosition',
	'SongSelect',
	'TuneRequest',
	'Clock',
	'Start',
	'Continue',
	'Stop',
	'ActiveSense',
	'Reset',
	'EndOfExclusive',
	'BankSelection_MSB',
	'Modulation_MSB',
	'Breath_MSB',
	'Foot_MSB',
	'PortamentoTime_MSB',
	'DataEntry_MSB',
	'MainVolume_MSB',
	'Balance_MSB',
	'Panpot_MSB',
	'Expression_MSB',
	'Effect1_MSB',
	'Effect2_MSB',
	'GeneralPurpose1_MSB',
	'GeneralPurpose2_MSB',
	'GeneralPurpose3_MSB',
	'GeneralPurpose4_MSB',
	'BankSelection_LSB',
	'Modulation_LSB',
	'Breath_LSB',
	'Foot_LSB',
	'PortamentoTime_LSB',
	'DataEntry_LSB',
	'MainVolume_LSB',
	'Balance_LSB',
	'Panpot_LSB',
	'Expression_LSB',
	'Effect1_LSB',
	'Effect2_LSB',
	'GeneralPurpose1_LSB',
	'GeneralPurpose2_LSB',
	'GeneralPurpose3_LSB',
	'GeneralPurpose4_LSB',
	'SustainPedal',
	'Portamento',
	'Sostenuto',
	'SoftPedal',
	'LegatoFootSwitch',
	'Hold2',
	'SoundVariation',
	'ReleaseTime',
	'Timbre',
	'AttackTime',
	'Brightness',
	'SC1',
	'SC2',
	'SC3',
	'SC4',
	'SC5',
	'SC6',
	'SC7',
	'SC8',
	'SC9',
	'SC10',
	'GeneralPurpose5',
	'GeneralPurpose6',
	'GeneralPurpose7',
	'GeneralPurpose8',
	'PortamentoControl',
	'ReverbDepth',
	'TremoloDepth',
	'ChorusDepth',
	'DetuneDepth',
	'PhaserDepth',
	'E1',
	'E2',
	'E3',
	'E4',
	'E5',
	'DataIncrement',
	'DataDecrement',
	'NRPN_LSB',
	'NRPN_MSB',
	'RPN_LSB',
	'RPN_MSB',
	'AllSoundsOff',
	'ResetControllers',
	'LocalControlSwitch',
	'AllNotesOff',
	'OmniOff',
	'OmniOn',
	'Mono1',
	'Mono2'
})

local time_lib = lib_const('Time', {
	'Position',
	'barBeat',
	'bar',
	'beat',
	'beatUnit',
	'beatsPerBar',
	'beatsPerMinute',
	'frame',
	'framesPerSecond',
	'speed'
})

local osc_lib = lib_const('OSC', {
	'Event',
	'Packet',
	'Bundle',
	'bundleTimetag',
	'bundleItems',
	'Message',
	'messagePath',
	'messageArguments',
	'Timetag',
	'timetagIntegral',
	'timetagFraction',
	'Nil',
	'Impulse',
	'Char',
	'RGBA'
})

local core_lib = lib_const('LV2', {
	'minimum',
	'maximum',
	'scalePoint'
})

local bufsz_lib = lib_const('Buf_Size', {
	'minBlockLength',
	'maxBlockLength',
	'sequenceSize'
})

local patch_lib = lib_const('Patch', {
	'Ack',
	'Delete',
	'Copy',
	'Error',
	'Get',
	'Message',
	'Move',
	'Patch',
	'Post',
	'Put',
	'Request',
	'Response',
	'Set',
	'add',
	'body',
	'destination',
	'property',
	'readable',
	'remove',
	'request',
	'subject',
	'sequenceNumber',
	'value',
	'wildcard',
	'writable'
})

local rdfs_lib = lib_const('RDFS', {
	'label',
	'range',
	'comment'
})

local rdf_lib = lib_const('RDF', {
	'value',
	'type'
})

local units_lib = lib_const('Units', {
	'Conversion',
	'Unit',
	'bar',
	'beat',
	'bpm',
	'cent',
	'cm',
	'coef',
	'conversion',
	'db',
	'degree',
	'frame',
	'hz',
	'inch',
	'khz',
	'km',
	'm',
	'mhz',
	'midiNote',
	'midiController',
	'mile',
	'min',
	'mm',
	'ms',
	'name',
	'oct',
	'pc',
	'prefixConversion',
	'render',
	's',
	'semitone12TET',
	'symbol',
	'unit'
})

local ui_lib = lib_const('Ui', {
	'updateRate'
})

local canvas_lib = lib_const('Canvas', {
	'graph',
	'body',
	'BeginPath',
	'ClosePath',
	'Arc',
	'CurveTo',
	'LineTo',
	'MoveTo',
	'Rectangle',
	'Style',
	'LineWidth',
	'LineDash',
	'LineCap',
	'LineJoin',
	'MiterLimit',
	'Stroke',
	'Fill',
	'Clip',
	'Save',
	'Restore',
	'Translate',
	'Scale',
	'Rotate',
	'Reset',
	'FontSize',
	'FillText',
	'lineCapButt',
	'lineCapRound',
	'lineCapSquare',
	'lineJoinMiter',
	'lineJoinRound',
	'lineJoinBevel',
	'mouseButtonLeft',
	'mouseButtonMiddle',
	'mouseButtonRight',
	'mouseWheelX',
	'mouseWheelY',
	'mousePositionX',
	'mousePositionY',
	'mouseFocus'
})

local moony_lib = lib_const('Moony', {
	'color',
	'syntax'
})

local lua_lib = lib_const('Lua', {
	'lang'
})

local param_lib = lib_const('Param', {
	'sampleRate'
})

local api_lib = atom_lib + midi_lib + time_lib + osc_lib + core_lib + bufsz_lib
	+ patch_lib + rdfs_lib + rdf_lib + units_lib + ui_lib + canvas_lib + moony_lib
	+ lua_lib + param_lib

-- Field functions
local field_func = token(T.OPERATOR, S('.:')) * token(T.FUNCTION, word_match{
	-- moony primitive
	'frameTime',
	'beatTime',
	'time',
	'int',
	'long',
	'float',
	'double',
	'bool',
	'urid',
	'string',
	'literal',
	'uri',
	'path',
	'chunk',
	'midi',
	'atom',
	'raw',
	'typed',
	-- moony OSC
	'bundle',
	'message',
	'impulse',
	'char',
	'rgba',
	'timetag',
	-- moony container
	'tuple',
	'object',
	'key',
	'property',
	'vector',
	'sequence',
	-- moony patch
	'get',
	'set',
	'put',
	'patch',
	'remove',
	'add',
	'ack',
	'error',
	-- moony forge pop
	'pop',
	'autopop',
	-- moony canvas
	'graph',
	'beginPath',
	'closePath',
	'arc',
	'curveTo',
	'lineTo',
	'moveTo',
	'rectangle',
	'style',
	'lineWidth',
	'lineDash',
	'lineCap',
	'lineJoin',
	'miterLimig',
	'stroke',
	'fill',
	'clip',
	'save',
	'restore',
	'translate',
	'scale',
	'rotate',
	'reset',
	'fontSize',
	'fillText',
	-- moony container
	'foreach',
	'unpack',
	'clone',
	-- moony responder
	'stash',
	'apply',
	'register',
	-- moony stash
	'write',
	'read'
})

-- Constants.
local constant = token(T.CONSTANT, word_match{
  'true',
	'false',
	'nil',
	'_G',
	'_VERSION'
})

-- Field constants.
local field_constant = token(T.OPERATOR, P('.')) * token(T.CONSTANT, word_match{
	-- moony primitive
	'type',
	'body',
	'raw',
	-- moony sequence
	'unit',
	'pad',
	-- moony object
	'id',
	'otype',
	-- moony vector
	'child_type',
	'child_size',
	-- moony literal
	'datatype',
	'lang'
})

-- Identifiers.
local identifier = token(T.IDENTIFIER, l.word)

-- Labels.
local label = token(T.LABEL, '::' * l.word * '::')
local self = token(T.LABEL, P('self'))

-- Operators.
local binops = token(T.OPERATOR, word_match{'and', 'or', 'not'})
local operator = token(T.OPERATOR, S('+-*/%^#=<>&|~;:,.'))
local braces = token(T.BRACE, S('{}[]()'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'constant', constant + field_constant},
  {'function', func + field_func},
  {'binops', binops},
  {'library', lpeg_lib + string_lib + coroutine_lib + utf8_lib + table_lib + math_lib + debug_lib + base64_lib + ascii85_lib + aes128_lib + api_lib},
  {'label', label + self},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'operator', operator + braces}
}

return M
