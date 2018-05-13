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

local function ord(o)
	table.sort(o, function(a, b)
		return #a > #b
	end)
	return o
end

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
local keyword = token(T.KEYWORD, word_match(ord{
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
}))

-- Functions.
local func = token(T.FUNCTION, word_match(ord{
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
	'Blank',
	'Stash',
	'Mapper',
	'Parameter',
}))

-- Functions.
local tabs = token(T.LIBRARY, word_match(ord{
	'Options',
	'Note',
	'Map',
	'Unmap'
}))

local function lib_func(name, keys)
	local post = nil
	for i, v in ipairs(ord(keys)) do
		local pat = P(v)
		post = post and (post + pat) or pat
	end
	return token(T.LIBRARY, P(name)) * (token(T.OPERATOR, S('.')) * token(T.FUNCTION, post))^-1
end

local function lib_const(name, keys)
	local post = nil
	for i, v in ipairs(ord(keys)) do
		local pat = P(v)
		post = post and (post + pat) or pat
	end
	return token(T.LIBRARY, P(name)) * (token(T.OPERATOR, S('.')) * token(T.CONSTANT, post))^-1
end

local function lib_mixed(name, func, const)
	local post_func = nil
	for i, v in ipairs(ord(func)) do
		local pat = P(v)
		post_func = post_func and (post_func + pat) or pat
	end

	local post_const = nil
	for i, v in ipairs(ord(const)) do
		local pat = P(v)
		post_const = post_const and (post_const + pat) or pat
	end

	return token(T.LIBRARY, P(name)) * (token(T.OPERATOR, S('.')) * (token(T.FUNCTION, post_func) + token(T.CONSTANT, post_const)))^-1
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

local utf8_lib = lib_mixed('utf8', {
	'len',
	'codes',
	'codepoint',
	'char',
	'offset'
}, {
	'charpattern'
})

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

local math_lib = lib_mixed('math', {
	'exp',
	'type',
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
	'tointeger',
	'max',
	'acos',
	'abs',
	'modf',
	'ceil'
}, {
	'mininteger',
	'pi',
	'maxinteger',
	'huge'
})

local mathx_lib = lib_mixed('mathx', {
	'fabs',
	'acos',
	'acosh',
	'asin',
	'asinh',
	'atan',
	'atan2',
	'atanh',
	'cbrt',
	'ceil',
	'copysign',
	'cos',
	'cosh',
	'deg',
	'erf',
	'erfc',
	'exp',
	'exp2',
	'expm1',
	'fdim',
	'floor',
	'fma',
	'fmax',
	'fmin',
	'fmod',
	'frexp',
	'gamma',
	'hypot',
	'isfinite',
	'isinf',
	'isnan',
	'isnormal',
	'ldexp',
	'lgamma',
	'log',
	'log10',
	'log1p',
	'log2',
	'logb',
	'modf',
	'nearbyint',
	'nextafter',
	'pow',
	'rad',
	'remainder',
	'round',
	'scalbn',
	'sin',
	'sinh',
	'sqrt',
	'tan',
	'tanh',
	'trunc'
}, {
	'version',
	'inf',
	'nan',
	'pi'
})

local complex_lib = lib_mixed('complex', {
	'abs',
	'acos',
	'acosh',
	'arg',
	'asin',
	'asinh',
	'atan',
	'atanh',
	'conj',
	'cos',
	'cosh',
	'exp',
	'imag',
	'log',
	'new',
	'pow',
	'proj',
	'real',
	'sin',
	'sinh',
	'sqrt',
	'tan',
	'tanh',
	'tostring'
}, {
	'version',
	'I'
})

local random_lib = lib_mixed('random', {
	'clone',
	'new',
	'seed',
	'value'
}, {
	'version'
})

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

local base64_lib = lib_mixed('base64', {
	'encode',
	'decode'
}, {
	'version'
})

local ascii85_lib = lib_mixed('ascii85', {
	'encode',
	'decode'
}, {
	'version'
})

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
	'beatTime',
	'childType'
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
	'Insert',
	'Patch',
	'Post',
	'Put',
	'Request',
	'Response',
	'Set',
	'add',
	'accept',
	'body',
	'context',
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
	'aspectRatio',
	'body',
	'BeginPath',
	'ClosePath',
	'Arc',
	'CurveTo',
	'LineTo',
	'MoveTo',
	'Rectangle',
	'PolyLine',
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
local field_func = token(T.OPERATOR, S('.:')) * token(T.FUNCTION, word_match(ord{
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
	'copy',
	'move',
	'insert',
	'delete',
	-- moony forge pop
	'pop',
	'autopop',
	-- moony canvas
	'beginPath',
	'closePath',
	'arc',
	'curveTo',
	'lineTo',
	'moveTo',
	'rectangle',
	'polyLine',
	'style',
	'lineWidth',
	'lineDash',
	'lineCap',
	'lineJoin',
	'miterLimit',
	'stroke',
	'fill',
	'clip',
	'save',
	'restore',
	'translate',
	'scale',
	'rotate',
	'transform',
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
}))

-- Constants.
local constant = token(T.CONSTANT, word_match(ord{
  'true',
	'false',
	'nil',
	'_G',
	'_VERSION'
}))

-- Field constants.
local field_constant = token(T.OPERATOR, P('.')) * token(T.CONSTANT, word_match(ord{
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
	'childType',
	'childSize',
	-- moony literal
	'datatype',
	'lang'
}))

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
  {'library', tabs + lpeg_lib + string_lib + coroutine_lib + utf8_lib + table_lib
		+ mathx_lib + math_lib + complex_lib + random_lib + debug_lib
		+ base64_lib + ascii85_lib + aes128_lib + api_lib},
  {'label', label + self},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'operator', operator + braces}
}

return M
