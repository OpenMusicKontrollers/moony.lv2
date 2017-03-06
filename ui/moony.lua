-- Copyright 2006-2016 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Lua LPeg l.
-- Original written by Peter Odding, 2007/04/04.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'moony'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

local longstring = lpeg.Cmt('[' * lpeg.C(P('=')^0) * '[',
                            function(input, index, eq)
                              local _, e = input:find(']'..eq..']', index, true)
                              return (e or #input) + 1
                            end)

-- Comments.
local line_comment = '--' * l.nonnewline^0
local block_comment = '--' * longstring
local comment = token(l.COMMENT, block_comment + line_comment)

-- Strings.
local sq_str = l.delimited_range("'", true, false, true)
local dq_str = l.delimited_range('"', true, false, true)
local string = token(l.STRING, sq_str + dq_str) +
               token('longstring', longstring)

-- Numbers.
local lua_integer = P('-')^-1 * (l.hex_num + l.dec_num)
local number = token(l.NUMBER, l.float + lua_integer)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
	'while', 'do', 'end', 'repeat', 'until', 'if', 'then', 'elseif', 'else', 'for', 'goto', 'break', 'return',
	'local', 'function', 'in'
})

-- Functions.
local func = token(l.FUNCTION, word_match{
	-- Lua basic
	'collectgarbage', 'error', 'getmetatable', 'ipairs', 'load', 'next', 'pairs', 'pcall', 'print', 'rawequal', 'rawget', 'rawlen', 'rawset', 'select', 'setmetatable', 'tonumber', 'tostring', 'type', 'xpcall', 'assert',
	-- Lua metamethods
	'__add', '__sub', '__mul', '__div', '__mod', '__pow', '__unm', '__idiv', '__band', '__bor', '__bxor', '__bnot', '__shl', '__shr', '__concat', '__len', '__eq', '__lt', '__le', '__index', '__newindex', '__call', '__gc', '__mode', '__name', '__tostring', '__metatable', '__pairs',
	-- moony basic
	'run', 'once', 'save', 'restore', 'stash', 'apply', 'register', 'midi2cps', 'cps2midi',
	'MIDIResponder', 'OSCResponder', 'TimeResponder', 'StateResponder', 'Map', 'Unmap', 'VoiceMap', 'Stash', 'Note', 'HashMap', 'Parameter',
})

-- Field functions
local field_func = token(l.OPERATOR, S('.:')) * token(l.FUNCTION, word_match{
	-- Lua coroutine
	'create', 'isyieldable', 'resume', 'running', 'status', 'wrap', 'yield',
	-- Lua string
	'byte', 'char', 'dump', 'find', 'format', 'gmatch', 'gsub', 'len', 'lower', 'match', 'pack', 'packsize', 'rep', 'reverse', 'sub', 'unpack', 'upper',
	-- Lua UTF-8
	'char', 'charpattern', 'codes', 'codepoint', 'len', 'offset',
	-- Lua table
	'concat', 'insert', 'move', 'pack', 'remove', 'sort', 'unpack',
	-- Lua math
	'abs', 'acos', 'asin', 'atan', 'ceil', 'cos', 'deg', 'exp', 'floor', 'fmod', 'log', 'max', 'min', 'modf',
	'rad', 'random', 'randomseed', 'sin', 'sqrt', 'tan', 'tointeger', 'type', 'ult',
	-- Lua debug
	'debug', 'getuservalue', 'gethook', 'getinfo', 'getlocal', 'getregistry', 'getmetatable',
	'getupvalue', 'upvaluejoin', 'upvalueid', 'setuservalue', 'sethook', 'setlocal',
	'setmetatable', 'setupvalue', 'traceback',
	-- Lua lpeg
	'setmaxstack', 'Ct', 'Cp', 'pcode', 'S', 'Cc', 'Cb', 'version', 'match', 'locale', 'Cf', 'Cg', 'B', 'C', 'V', 'Cs', 'R', 'P', 'Carg', 'ptree', 'type', 'Cmt',
	-- Lua base64/ascii85/aes128
	'encode', 'decode',
	-- moony forge
	'frameTime', 'beatTime', 'time', 'atom', 'int', 'long', 'float', 'double', 'bool', 'urid', 'string', 'literal', 'uri', 'path',
	'chunk', 'midi', 'raw', 'bundle', 'message', 'impulse', 'char', 'rgba', 'timetag', 'tuple', 'tuplePack',
	'object', 'objectPack', 'key', 'property', 'vector', 'sequence', 'sequencePack',
	'typed', 'get', 'set', 'put', 'patch', 'remove', 'add', 'ack', 'error', 'pop', 'autopop',
	'graph', 'beginPath', 'closePath', 'arc', 'curveTo', 'lineTo', 'moveTo', 'rectangle', 'style', 'lineWidth', 'lineDash', 'lineCap', 'lineJoin', 'miterLimig', 'stroke', 'fill', 'clip', 'save', 'restore', 'translate', 'scale', 'rotate', 'reset', 'fontSize', 'fillText',
	-- moony common
	'foreach', 'unpack', 'clone', 'stash', 'apply', 'register',
	-- moony stash
	'write', 'read'
})

-- Constants.
local constant = token(l.CONSTANT, word_match{
  'true', 'false', 'nil', '_G', '_VERSION'
})

-- Field constants.
local field_constant = token(l.OPERATOR, P('.')) * token(l.CONSTANT, word_match{
	-- Lua math
	'huge', 'maxinteger', 'mininteger', 'pi',
	-- moony Atom
	'Bool', 'Chunk', 'Double', 'Float', 'Int', 'Long', 'Literal', 'Object', 'Path', 'Property', 'Sequence', 'String', 'Tuple', 'URI', 'URID', 'Vector', 'frameTime', 'beatTime',
	-- Lua base64/ascii85
	'version',
	-- moony MIDI
	'MidiEvent',
	'NoteOff', 'NoteOn', 'NotePressure', 'Controller', 'ProgramChange', 'ChannelPressure', 'Bender', 'SystemExclusive', 'QuarterFrame', 'SongPosition', 'SongSelect', 'TuneRequest', 'Clock', 'Start', 'Continue', 'Stop', 'ActiveSense', 'Reset', 'EndOfExclusive',
	'BankSelection_MSB', 'Modulation_MSB', 'Breath_MSB', 'Foot_MSB', 'PortamentoTime_MSB', 'DataEntry_MSB', 'MainVolume_MSB', 'Balance_MSB', 'Panpot_MSB', 'Expression_MSB', 'Effect1_MSB', 'Effect2_MSB', 'GeneralPurpose1_MSB', 'GeneralPurpose2_MSB', 'GeneralPurpose3_MSB', 'GeneralPurpose4_MSB', 'BankSelection_LSB', 'Modulation_LSB', 'Breath_LSB', 'Foot_LSB', 'PortamentoTime_LSB', 'DataEntry_LSB', 'MainVolume_LSB', 'Balance_LSB', 'Panpot_LSB', 'Expression_LSB', 'Effect1_LSB', 'Effect2_LSB', 'GeneralPurpose1_LSB', 'GeneralPurpose2_LSB', 'GeneralPurpose3_LSB', 'GeneralPurpose4_LSB', 'SustainPedal', 'Portamento', 'Sostenuto', 'SoftPedal', 'LegatoFootSwitch', 'Hold2', 'SoundVariation', 'ReleaseTime', 'Timbre', 'AttackTime', 'Brightness', 'SC1', 'SC2', 'SC3', 'SC4', 'SC5', 'SC6', 'SC7', 'SC8', 'SC9', 'SC10', 'GeneralPurpose5', 'GeneralPurpose6', 'GeneralPurpose7', 'GeneralPurpose8', 'PortamentoControl', 'ReverbDepth', 'TremoloDepth', 'ChorusDepth', 'DetuneDepth', 'PhaserDepth', 'E1', 'E2', 'E3', 'E4', 'E5', 'DataIncrement', 'DataDecrement', 'NRPN_LSB', 'NRPN_MSB', 'RPN_LSB', 'RPN_MSB', 'AllSoundsOff', 'ResetControllers', 'LocalControlSwitch', 'AllNotesOff', 'OmniOff', 'OmniOn', 'Mono1', 'Mono2',
	-- moony Time
	'Position', 'barBeat', 'bar', 'beat', 'beatUnit', 'beatsPerBar', 'beatsPerMinute', 'frame', 'framesPerSecond', 'speed',
	-- moony OSC
	'Event', 'Packet', 'Bundle', 'bundleTimetag', 'bundleItems', 'Message', 'messagePath', 'messageArguments', 'Timetag', 'timetagIntegral', 'timetagFraction', 'Nil', 'Impulse', 'Char', 'RGBA',
	-- moony Core
	'sampleRate', 'minimum', 'maximum', 'scalePoint',
	-- moony Buf_Size
	'minBlockLength', 'maxBlockLength', 'sequenceSize',
	-- moony Patch
	'Ack', 'Delete', 'Copy', 'Error', 'Get', 'Message', 'Move', 'Patch', 'Post', 'Put', 'Request', 'Response', 'Set',
	'add', 'body', 'destination', 'property', 'readable', 'remove', 'request', 'subject', 'sequenceNumber', 'value', 'wildcard', 'writable',
	-- moony RDF
	'value',
	-- moony RDFS
	'label', 'range', 'comment',
	-- moony Units
	'Conversion', 'Unit', 'bar', 'beat', 'bpm', 'cent', 'cm', 'coef', 'conversion', 'db', 'degree', 'frame', 'hz', 'inch', 'khz', 'km', 'm', 'mhz', 'midiNote', 'midiController', 'mile', 'min', 'mm', 'ms', 'name', 'oct', 'pc', 'prefixConversion', 'render', 's', 'semitone12TET', 'symbol', 'unit',
	-- moony Canvas
	'graph', 'body', 'BeginPath', 'ClosePath', 'Arc', 'CurveTo', 'LineTo', 'MoveTo', 'Rectangle', 'Style', 'LineWidth', 'LineDash', 'LineCap', 'LineJoin',
	'MiterLimit', 'Stroke', 'Fill', 'Clip', 'Save', 'Restore', 'Translate', 'Scale', 'Rotate', 'Reset', 'FontSize', 'FillText',
	'lineCapButt', 'lineCapRound', 'lineCapSquare', 'lineJoinMiter', 'lineJoinRound', 'lineJoinBevel',
	'mouseButtonLeft', 'mouseButtonMiddle', 'mouseButtonRight', 'mouseWheelX', 'mouseWheelY', 'mousePositionX', 'mousePositionY', 'mouseFocus',

	-- moony common
	'type', 'body', 'raw',
	-- moony sequence
	'unit', 'pad',
	-- moony object
	'id', 'otype',
	-- moony vector
	'child_type', 'child_size',
	-- moony literal
	'datatype', 'lang'
})

-- Libraries.
local library = token('library', word_match{
	-- Lua
	'coroutine', 'string', 'utf8', 'table', 'math', 'debug', 'lpeg', 'base64', 'ascii85', 'aes128',
	-- Moony
	'Atom', 'MIDI', 'Time', 'OSC', 'Core', 'Buf_Size', 'Patch', 'RDF', 'RDFS', 'Units', 'Options', 'Canvas'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Labels.
local label = token(l.LABEL, '::' * l.word * '::')
local self = token(l.LABEL, P('self'))

-- Operators.
local binops = token('binop', word_match{'and', 'or', 'not'})
local operator = token(l.OPERATOR, S('+-*/%^#=<>&|~;:,.'))
local braces = token('brace', S('{}[]()'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'constant', constant + field_constant},
  {'function', func + field_func},
  {'binops', binops},
  {'library', library},
  {'label', label + self},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'operator', operator + braces}
}

M._tokenstyles = {
  longstring = l.STYLE_STRING,
  library = l.STYLE_TYPE,
}

local function fold_longcomment(text, pos, line, s, match)
  if match == '[' then
    if line:find('^%[=*%[', s) then return 1 end
  elseif match == ']' then
    if line:find('^%]=*%]', s) then return -1 end
  end
  return 0
end

M._foldsymbols = {
  _patterns = {'%l+', '[%({%)}]', '[%[%]]', '%-%-'},
  [l.KEYWORD] = {
    ['if'] = 1, ['do'] = 1, ['function'] = 1, ['end'] = -1, ['repeat'] = 1,
    ['until'] = -1
  },
  [l.COMMENT] = {
    ['['] = fold_longcomment, [']'] = fold_longcomment,
    ['--'] = l.fold_line_comments('--')
  },
  longstring = {['['] = 1, [']'] = -1},
  [l.OPERATOR] = {['('] = 1, ['{'] = 1, [')'] = -1, ['}'] = -1}
}

return M
