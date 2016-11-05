define("ace/mode/lua_highlight_rules",["require","exports","module","ace/lib/oop","ace/mode/text_highlight_rules"], function(require, exports, module) {
"use strict";

var oop = require("../lib/oop");
var TextHighlightRules = require("./text_highlight_rules").TextHighlightRules;

var LuaHighlightRules = function() {

    var keywords = (
				"while|do|end|repeat|until|if|then|elseif|else|for|goto|break|return|"+
				"local|function|in");

		var booleanOps = ("and|or|not");

    var builtinConstants = ("true|false|nil|_G|_VERSION");

    var functions = (
				// Lua basic
				"collectgarbage|error|getmetatable|ipairs|load|next|pairs|pcall|print|"+
				"rawequal|rawget|rawlen|rawset|select|setmetatable|tonumber|tostring|type|xpcall|assert|"+
				// Lua metamethods
				"__add|__sub|__mul|__div|__mod|__pow|__unm|__idiv|"+
				"__band|__bor|__bxor|__bnot|__shl|__shr|"+
				"__concat|__len|__eq|__lt|__le|__index|__newindex|__call|"+
				"__gc|__mode|__name|__tostring|__metatable|__pairs|"+
				// moony basic
				"run|once|save|restore|stash|apply|register|midi2cps|cps2midi|encrypt|decrypt|draw");

		var field_functions = (
				// Lua coroutine
				"create|isyieldable|resume|running|status|wrap|yield|"+
				// Lua string
				"byte|char|dump|find|format|gmatch|gsub|len|lower|match|pack|packsize|rep|reverse|sub|unpack|upper|"+
				// Lua UTF-8
				"char|charpattern|codes|codepoint|len|offset|"+
				// Lua table
				"concat|insert|move|pack|remove|sort|unpack|"+
				// Lua math
				"abs|acos|asin|atan|ceil|cos|deg|exp|floor|fmod|log|max|min|modf|"+
				"rad|random|randomseed|sin|sqrt|tan|tointeger|type|ult|"+
				// Lua debug
				"debug|getuservalue|gethook|getinfo|getlocal|getregistry|getmetatable|"+
				"getupvalue|upvaluejoin|upvalueid|setuservalue|sethook|setlocal|"+
				"setmetatable|setupvalue|traceback|"+
				// moony forge
				"frame_time|beat_time|time|atom|int|long|float|double|bool|urid|string|literal|uri|path|"+
				"chunk|midi|raw|bundle|message|tuple|tuplePack|object|objectPack|key|property|vector|sequence|sequencePack|"+
				"typed|get|set|put|patch|remove|add|ack|error|pop|"+
				"beginPath|closePath|arc|curveTo|lineTo|moveTo|rectangle|style|lineWidth|lineDash|lineCap|lineJoin|miterLimig|stroke|fill|clip|save|restore|translate|scale|rotate|reset|fontSize|fillText|"+
				// moony common
				"foreach|unpack|clone|stash|apply|register|"+
				// moony stash
				"write|read");

		var field_constants = (
				// Lua math
				"huge|maxinteger|mininteger|pi|"+
				// moony Atom
				"Bool|Chunk|Double|Float|Int|Long|Literal|Object|Path|Property|Sequence|String|Tuple|URI|URID|Vector|frameTime|beatTime|"+
				// moony MIDI
				"MidiEvent|"+
				"NoteOff|NoteOn|NotePressure|Controller|ProgramChange|ChannelPressure|Bender|SystemExclusive|QuarterFrame|SongPosition|SongSelect|TuneRequest|Clock|Start|Continue|Stop|ActiveSense|Reset|EndOfExclusive|"+
				"BankSelection_MSB|Modulation_MSB|Breath_MSB|Foot_MSB|PortamentoTime_MSB|DataEntry_MSB|MainVolume_MSB|Balance_MSB|Panpot_MSB|Expression_MSB|Effect1_MSB|Effect2_MSB|GeneralPurpose1_MSB|GeneralPurpose2_MSB|GeneralPurpose3_MSB|GeneralPurpose4_MSB|BankSelection_LSB|Modulation_LSB|Breath_LSB|Foot_LSB|PortamentoTime_LSB|DataEntry_LSB|MainVolume_LSB|Balance_LSB|Panpot_LSB|Expression_LSB|Effect1_LSB|Effect2_LSB|GeneralPurpose1_LSB|GeneralPurpose2_LSB|GeneralPurpose3_LSB|GeneralPurpose4_LSB|SustainPedal|Portamento|Sostenuto|SoftPedal|LegatoFootSwitch|Hold2|SoundVariation|ReleaseTime|Timbre|AttackTime|Brightness|SC1|SC2|SC3|SC4|SC5|SC6|SC7|SC8|SC9|SC10|GeneralPurpose5|GeneralPurpose6|GeneralPurpose7|GeneralPurpose8|PortamentoControl|ReverbDepth|TremoloDepth|ChorusDepth|DetuneDepth|PhaserDepth|E1|E2|E3|E4|E5|DataIncrement|DataDecrement|NRPN_LSB|NRPN_MSB|RPN_LSB|RPN_MSB|AllSoundsOff|ResetControllers|LocalControlSwitch|AllNotesOff|OmniOff|OmniOn|Mono1|Mono2|"+
				// moony Time
				"Position|barBeat|bar|beat|beatUnit|beatsPerBar|beatsPerMinute|frame|framesPerSecond|speed|"+
				// moony OSC
				"Event|Packet|Bundle|bundleTimetag|bundleItems|Message|messagePath|messageArguments|Timetag|timetagIntegral|timetagFraction|Nil|Impulse|Char|RGBA|"+
				// moony Core
				"sampleRate|minimum|maximum|scalePoint|"+
				// moony Buf_Size
				"minBlockLength|maxBlockLength|sequenceSize|"+
				// moony Patch
				"Ack|Delete|Copy|Error|Get|Message|Move|Patch|Post|Put|Request|Response|Set|"+
				"add|body|destination|property|readable|remove|request|subject|sequenceNumber|value|wildcard|writable|"+
				// moony RDF
				"value|"+
				// moony RDFS
				"label|range|comment|"+
				// moony Units
				"Conversion|Unit|bar|beat|bpm|cent|cm|coef|conversion|db|degree|frame|hz|inch|khz|km|m|mhz|midiNote|mile|min|mm|ms|name|oct|pc|prefixConversion|render|s|semitone12TET|symbol|unit|"+
				// moony Canvas
				"graph|body|BeginPath|ClosePath|Arc|CurveTo|LineTo|MoveTo|Rectangle|Style|LineWidth|LineDash|LineCap|LineJoin|MiterLimit|Stroke|Fill|Clip|Save|Restore|Translate|Scale|Rotate|Reset|FontSize|FillText|"+

				// moony common
				"type|body|"+
				// moony sequence
				"unit|pad|"+
				// moony object
				"id|otype|"+
				// moony vector
				"child_type|child_size|"+
				// moony literal
				"datatype|lang");

    var libraries = (
				// Lua
				"coroutine|string|utf8|table|math|debug|"+
				// moony
				"Atom|MIDI|Time|OSC|Core|Buf_Size|Patch|RDF|RDFS|Units|Options|Canvas|"+
				"MIDIResponder|OSCResponder|TimeResponder|StateResponder|Map|Unmap|VoiceMap|Stash|Note|HashMap");

    var keywordMapper = this.createKeywordMapper({
        "keyword": keywords,
				"keyword.operator" : booleanOps,
        "support.function": functions,
        "constant.library": libraries,
        "constant.language": builtinConstants,
        "variable.language": "self"
    }, "identifier");

		var fieldMapper = this.createKeywordMapper({
			"constant.language" : field_constants,
			"support.function" : field_functions 
		}, "identifier");

    var decimalInteger = "(?:(?:[1-9]\\d*)|(?:0))";
    var hexInteger = "(?:0[xX][\\dA-Fa-f]+)";
    var integer = "(?:" + decimalInteger + "|" + hexInteger + ")";

    var fraction = "(?:\\.\\d+)";
    var intPart = "(?:\\d+)";
    var pointFloat = "(?:(?:" + intPart + "?" + fraction + ")|(?:" + intPart + "\\.))";
    var floatNumber = "(?:" + pointFloat + ")";

    this.$rules = {
        "start" : [{
            stateName: "bracketedComment",
            onMatch : function(value, currentState, stack){
                stack.unshift(this.next, value.length - 2, currentState);
                return "comment";
            },
            regex : /\-\-\[=*\[/,
            next  : [
                {
                    onMatch : function(value, currentState, stack) {
                        if (value.length == stack[1]) {
                            stack.shift();
                            stack.shift();
                            this.next = stack.shift();
                        } else {
                            this.next = "";
                        }
                        return "comment";
                    },
                    regex : /\]=*\]/,
                    next  : "start"
                }, {
                    token : "comment.fixme",
										regex : /(FIXME|TODO)/
                }, {
                    defaultToken : "comment"
                }
            ]
        },
        {
            token : "comment",
            //regex : "\\-\\-.*$"
            regex : /\-\-/,
						next : [{	
								next : "start",
								regex : /$/
							}, {
								token : "comment.fixme",
								regex : /(FIXME|TODO)/
							}, {
								defaultToken : "comment"
							}]
        },
        {
            stateName: "bracketedString",
            onMatch : function(value, currentState, stack){
                stack.unshift(this.next, value.length, currentState);
                return "string";
            },
            regex : /\[=*\[/,
            next  : [
                {
                    onMatch : function(value, currentState, stack) {
                        if (value.length == stack[1]) {
                            stack.shift();
                            stack.shift();
                            this.next = stack.shift();
                        } else {
                            this.next = "";
                        }
                        return "string";
                    },
                    
                    regex : /\]=*\]/,
                    next  : "start"
                }, {
                    defaultToken : "string"
                }
            ]
        },
        {
            token : "string",           // " string
            regex : '"(?:[^\\\\]|\\\\.)*?"'
        }, {
            token : "string",           // ' string
            regex : "'(?:[^\\\\]|\\\\.)*?'"
        }, {
            token : "constant.numeric", // float
            regex : floatNumber
        }, {
            token : "constant.numeric", // integer
            regex : integer + "\\b"
        }, {
						token: "keyword.operator",
						regex : "[\\:\\.]",
						next: [{
							token : "keyword.operator",
							regex : "[\\:\\.]+",
							next: "start"
						}, {
							//token: "string",
							token : fieldMapper,
							regex : "[a-zA-Z_$][a-zA-Z0-9_$]*\\b",
							next: "start"
						}]
        }, {
            token : keywordMapper,
            regex : "[a-zA-Z_$][a-zA-Z0-9_$]*\\b",
        }, {
            token : "keyword.operator",
            regex : "\\+|\\-|\\*|\\/|\\/|%|\\^|"+
							"\\&|\\||~|"+
							"=|<|>|"+
							"#"
        }, {
            token : "paren.lparen",
            regex : "[\\[\\(\\{]"
        }, {
            token : "paren.rparen",
            regex : "[\\]\\)\\}]"
        }, {
            token : "text",
            regex : "\\s+|\\w+"
        }]
    };
    
    this.normalizeRules();
}

oop.inherits(LuaHighlightRules, TextHighlightRules);

exports.LuaHighlightRules = LuaHighlightRules;
});

define("ace/mode/folding/lua",["require","exports","module","ace/lib/oop","ace/mode/folding/fold_mode","ace/range","ace/token_iterator"], function(require, exports, module) {
"use strict";

var oop = require("../../lib/oop");
var BaseFoldMode = require("./fold_mode").FoldMode;
var Range = require("../../range").Range;
var TokenIterator = require("../../token_iterator").TokenIterator;


var FoldMode = exports.FoldMode = function() {};

oop.inherits(FoldMode, BaseFoldMode);

(function() {

    this.foldingStartMarker = /\b(function|then|do|repeat)\b|{\s*$|(\[=*\[)/;
    this.foldingStopMarker = /\bend\b|^\s*}|\]=*\]/;

    this.getFoldWidget = function(session, foldStyle, row) {
        var line = session.getLine(row);
        var isStart = this.foldingStartMarker.test(line);
        var isEnd = this.foldingStopMarker.test(line);

        if (isStart && !isEnd) {
            var match = line.match(this.foldingStartMarker);
            if (match[1] == "then" && /\belseif\b/.test(line))
                return;
            if (match[1]) {
                if (session.getTokenAt(row, match.index + 1).type === "keyword")
                    return "start";
            } else if (match[2]) {
                var type = session.bgTokenizer.getState(row) || "";
                if (type[0] == "bracketedComment" || type[0] == "bracketedString")
                    return "start";
            } else {
                return "start";
            }
        }
        if (foldStyle != "markbeginend" || !isEnd || isStart && isEnd)
            return "";

        var match = line.match(this.foldingStopMarker);
        if (match[0] === "end") {
            if (session.getTokenAt(row, match.index + 1).type === "keyword")
                return "end";
        } else if (match[0][0] === "]") {
            var type = session.bgTokenizer.getState(row - 1) || "";
            if (type[0] == "bracketedComment" || type[0] == "bracketedString")
                return "end";
        } else
            return "end";
    };

    this.getFoldWidgetRange = function(session, foldStyle, row) {
        var line = session.doc.getLine(row);
        var match = this.foldingStartMarker.exec(line);
        if (match) {
            if (match[1])
                return this.luaBlock(session, row, match.index + 1);

            if (match[2])
                return session.getCommentFoldRange(row, match.index + 1);

            return this.openingBracketBlock(session, "{", row, match.index);
        }

        var match = this.foldingStopMarker.exec(line);
        if (match) {
            if (match[0] === "end") {
                if (session.getTokenAt(row, match.index + 1).type === "keyword")
                    return this.luaBlock(session, row, match.index + 1);
            }

            if (match[0][0] === "]")
                return session.getCommentFoldRange(row, match.index + 1);

            return this.closingBracketBlock(session, "}", row, match.index + match[0].length);
        }
    };

    this.luaBlock = function(session, row, column) {
        var stream = new TokenIterator(session, row, column);
        var indentKeywords = {
            "function": 1,
            "do": 1,
            "then": 1,
            "elseif": -1,
            "end": -1,
            "repeat": 1,
            "until": -1
        };

        var token = stream.getCurrentToken();
        if (!token || token.type != "keyword")
            return;

        var val = token.value;
        var stack = [val];
        var dir = indentKeywords[val];

        if (!dir)
            return;

        var startColumn = dir === -1 ? stream.getCurrentTokenColumn() : session.getLine(row).length;
        var startRow = row;

        stream.step = dir === -1 ? stream.stepBackward : stream.stepForward;
        while(token = stream.step()) {
            if (token.type !== "keyword")
                continue;
            var level = dir * indentKeywords[token.value];

            if (level > 0) {
                stack.unshift(token.value);
            } else if (level <= 0) {
                stack.shift();
                if (!stack.length && token.value != "elseif")
                    break;
                if (level === 0)
                    stack.unshift(token.value);
            }
        }

        var row = stream.getCurrentTokenRow();
        if (dir === -1)
            return new Range(row, session.getLine(row).length, startRow, startColumn);
        else
            return new Range(startRow, startColumn, row, stream.getCurrentTokenColumn());
    };

}).call(FoldMode.prototype);

});

define("ace/mode/lua",["require","exports","module","ace/lib/oop","ace/mode/text","ace/mode/lua_highlight_rules","ace/mode/folding/lua","ace/range","ace/worker/worker_client"], function(require, exports, module) {
"use strict";

var oop = require("../lib/oop");
var TextMode = require("./text").Mode;
var LuaHighlightRules = require("./lua_highlight_rules").LuaHighlightRules;
var LuaFoldMode = require("./folding/lua").FoldMode;
var Range = require("../range").Range;
var WorkerClient = require("../worker/worker_client").WorkerClient;

var Mode = function() {
    this.HighlightRules = LuaHighlightRules;
    
    this.foldingRules = new LuaFoldMode();
};
oop.inherits(Mode, TextMode);

(function() {
   
    this.lineCommentStart = "--";
    this.blockComment = {start: "--[", end: "]--"};
    
    var indentKeywords = {
        "function": 1,
        "then": 1,
        "do": 1,
        "else": 1,
        "elseif": 1,
        "repeat": 1,
        "end": -1,
        "until": -1
    };
    var outdentKeywords = [
        "else",
        "elseif",
        "end",
        "until"
    ];

    function getNetIndentLevel(tokens) {
        var level = 0;
        for (var i = 0; i < tokens.length; i++) {
            var token = tokens[i];
            if (token.type == "keyword") {
                if (token.value in indentKeywords) {
                    level += indentKeywords[token.value];
                }
            } else if (token.type == "paren.lparen") {
                level += token.value.length;
            } else if (token.type == "paren.rparen") {
                level -= token.value.length;
            }
        }
        if (level < 0) {
            return -1;
        } else if (level > 0) {
            return 1;
        } else {
            return 0;
        }
    }

    this.getNextLineIndent = function(state, line, tab) {
        var indent = this.$getIndent(line);
        var level = 0;

        var tokenizedLine = this.getTokenizer().getLineTokens(line, state);
        var tokens = tokenizedLine.tokens;

        if (state == "start") {
            level = getNetIndentLevel(tokens);
        }
        if (level > 0) {
            return indent + tab;
        } else if (level < 0 && indent.substr(indent.length - tab.length) == tab) {
            if (!this.checkOutdent(state, line, "\n")) {
                return indent.substr(0, indent.length - tab.length);
            }
        }
        return indent;
    };

    this.checkOutdent = function(state, line, input) {
        if (input != "\n" && input != "\r" && input != "\r\n")
            return false;

        if (line.match(/^\s*[\)\}\]]$/))
            return true;

        var tokens = this.getTokenizer().getLineTokens(line.trim(), state).tokens;

        if (!tokens || !tokens.length)
            return false;

        return (tokens[0].type == "keyword" && outdentKeywords.indexOf(tokens[0].value) != -1);
    };

    this.autoOutdent = function(state, session, row) {
        var prevLine = session.getLine(row - 1);
        var prevIndent = this.$getIndent(prevLine).length;
        var prevTokens = this.getTokenizer().getLineTokens(prevLine, "start").tokens;
        var tabLength = session.getTabString().length;
        var expectedIndent = prevIndent + tabLength * getNetIndentLevel(prevTokens);
        var curIndent = this.$getIndent(session.getLine(row)).length;
        if (curIndent < expectedIndent) {
            return;
        }
        session.outdentRows(new Range(row, 0, row + 2, 0));
    };

    this.createWorker = function(session) {
        var worker = new WorkerClient(["ace"], "ace/mode/lua_worker", "Worker");
        worker.attachToDocument(session.getDocument());
        
        worker.on("annotate", function(e) {
            session.setAnnotations(e.data);
        });
        
        worker.on("terminate", function() {
            session.clearAnnotations();
        });
        
        return worker;
    };

    this.$id = "ace/mode/lua";
}).call(Mode.prototype);

exports.Mode = Mode;
});
