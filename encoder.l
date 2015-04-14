%{
#include <stdio.h>
#include <string.h>

typedef void (*encoder_append_t)(const char *str, void *data);
typedef struct _encoder_t encoder_t;

struct _encoder_t {
	encoder_append_t append;
	void *data;
};

extern encoder_t *encoder;

static void
_add_plain(const char *content)
{
	encoder->append(content, encoder->data);
}

static void 
_add_singleton(const char *key)
{
	char buf [64];
	sprintf(buf, "<%s/>", key);
	encoder->append(buf, encoder->data);
}

static void 
_add_markup_begin(const char *key)
{
	char buf [64];
	sprintf(buf, "<%s>", key);
	encoder->append(buf, encoder->data);
}

static void 
_add_markup_end(const char *key)
{
	char buf [64];
	sprintf(buf, "</%s>", key);
	encoder->append(buf, encoder->data);
}

static void 
_add_markup(const char *key, const char *content)
{
	char buf [64];
	sprintf(buf, "<%s>", key);
	encoder->append(buf, encoder->data);
	encoder->append(content, encoder->data);
	sprintf(buf, "</%s>", key);
	encoder->append(buf, encoder->data);
}

enum {
	TK_CALLER = 0x7f,
	TK_AND,
	TK_BREAK,
	TK_DO,
	TK_ELSE,
	TK_ELSEIF,
	TK_END,
	TK_FALSE,
	TK_FOR,
	TK_FUNCTION,
	TK_IF,
	TK_IN,
	TK_LOCAL,
	TK_NIL,
	TK_NOT,
	TK_OR,
	TK_REPEAT,
	TK_RETURN,
	TK_THEN,
	TK_TRUE,
	TK_UNTIL,
	TK_WHILE,
	TK_CONCAT,
	TK_DOTS,
	TK_NUMBER,
	TK_NAME,
	TK_SINGLE_QUOTE_IN,
	TK_SINGLE_QUOTE_OUT,
	TK_SINGLE_QUOTE_ERR,
	TK_DOUBLE_QUOTE_IN,
	TK_DOUBLE_QUOTE_OUT,
	TK_DOUBLE_QUOTE_ERR,
	TK_LONG_STRING_IN,
	TK_LONG_STRING_OUT,
	TK_SHORT_COMMENT_IN,
	TK_SHORT_COMMENT_OUT,
	TK_LONG_COMMENT_IN,
	TK_LONG_COMMENT_OUT,
	TK_WHITESPACE,
	TK_NEWLINE,
	TK_BRACE,
	TK_RAW,
	TK_BADCHAR
};

int firstline=1;

static int
checkKw(char*s)
{
	#define CHECK(S,T) \
		if(!strcmp(#S,s)) \
			return TK_##T;
	
	switch(*s)
	{
		case 'a':
			CHECK(and,AND);
			CHECK(assert,CALLER);
			break;
		case 'b':
			CHECK(break,BREAK);
			CHECK(bit32.band,CALLER);
			CHECK(bit32.rshift,CALLER);
			CHECK(bit32.rrotate,CALLER);
			CHECK(bit32.extract,CALLER);
			CHECK(bit32.bnot,CALLER);
			CHECK(bit32.replace,CALLER);
			CHECK(bit32.lrotate,CALLER);
			CHECK(bit32.arshift,CALLER);
			CHECK(bit32.bxor,CALLER);
			CHECK(bit32.lshift,CALLER);
			CHECK(bit32.bor,CALLER);
			CHECK(bit32.btest,CALLER);
			break;
		case 'c':
			CHECK(coroutine.status,CALLER);
			CHECK(coroutine.yield,CALLER);
			CHECK(coroutine.resume,CALLER);
			CHECK(coroutine.wrap,CALLER);
			CHECK(coroutine.running,CALLER);
			CHECK(coroutine.create,CALLER);
			CHECK(collectgarbage,CALLER);
			break;
		case 'd':
			CHECK(dofile,CALLER);
			CHECK(do,DO);
			//TODO debug
			break;
		case 'e':
			CHECK(elseif,ELSEIF);
			CHECK(else,ELSE);
			CHECK(end,END);
			CHECK(error,CALLER);
			break;
		case 'f':
			CHECK(false,FALSE);
			CHECK(for,FOR);
			CHECK(function,FUNCTION);
			break;
		case 'g':
			CHECK(getmetatable,CALLER);
			break;
		case 'i':
			CHECK(if,IF);
			CHECK(in,IN);
			CHECK(ipairs,CALLER);
			//TODO io
			break;
		case 'l':
			CHECK(local,LOCAL);
			CHECK(loadstring,CALLER);
			CHECK(load,CALLER);
			CHECK(loadfile,CALLER);
			break;
		case 'm':
			CHECK(math.atan,CALLER)
			CHECK(math.tanh,CALLER)
			CHECK(math.frexp,CALLER)
			CHECK(math.atan2,CALLER)
			CHECK(math.abs,CALLER)
			CHECK(math.pow,CALLER)
			CHECK(math.log,CALLER)
			CHECK(math.pi,CALLER)
			CHECK(math.random,CALLER)
			CHECK(math.tan,CALLER)
			CHECK(math.rad,CALLER)
			CHECK(math.sqrt,CALLER)
			CHECK(math.sin,CALLER)
			CHECK(math.randomseed,CALLER)
			CHECK(math.log10,CALLER)
			CHECK(math.min,CALLER)
			CHECK(math.acos,CALLER)
			CHECK(math.exp,CALLER)
			CHECK(math.fmod,CALLER)
			CHECK(math.huge,CALLER)
			CHECK(math.cosh,CALLER)
			CHECK(math.max,CALLER)
			CHECK(math.modf,CALLER)
			CHECK(math.floor,CALLER)
			CHECK(math.ldexp,CALLER)
			CHECK(math.asin,CALLER)
			CHECK(math.ceil,CALLER)
			CHECK(math.sinh,CALLER)
			CHECK(math.deg,CALLER)
			CHECK(math.cos,CALLER)
			CHECK(module,LOCAL)
			break;
		case 'n':
			CHECK(nil,NIL);
			CHECK(not,NOT);
			CHECK(next,CALLER);
			break;
		case 'o':
			CHECK(or,OR);
			//TODO os
			break;
		case 'p':
			CHECK(pairs,CALLER);
			CHECK(print,CALLER);
			CHECK(pcall,CALLER);
			//TODO package
			break;
		case 'r':
			CHECK(repeat,REPEAT);
			CHECK(return,RETURN);
			CHECK(rawget,CALLER);
			CHECK(rawset,CALLER);
			CHECK(rawlen,CALLER);
			CHECK(rawequal,CALLER);
			CHECK(require,CALLER);
			
			CHECK(run,CALLER); // lua.lv2 specific
			break;
		case 's':
			CHECK(string.match,CALLER);
			CHECK(string.reverse,CALLER);
			CHECK(string.upper,CALLER);
			CHECK(string.sub,CALLER);
			CHECK(string.byte,CALLER);
			CHECK(string.char,CALLER);
			CHECK(string.dump,CALLER);
			CHECK(string.gmatch,CALLER);
			CHECK(string.lower,CALLER);
			CHECK(string.len,CALLER);
			CHECK(string.gsub,CALLER);
			CHECK(string.find,CALLER);
			CHECK(string.format,CALLER);
			CHECK(string.rep,CALLER);

			CHECK(select,CALLER);
			CHECK(setmetatable,CALLER);
			break;
		case 't':
			CHECK(then,THEN);
			CHECK(true,TRUE);
			CHECK(type,CALLER);
			CHECK(tonumber,CALLER);
			CHECK(tostring,CALLER);

			CHECK(table.pack,CALLER);
			CHECK(table.remove,CALLER);
			CHECK(table.sort,CALLER);
			CHECK(table.insert,CALLER);
			CHECK(table.concat,CALLER);
			CHECK(table.unpack,CALLER);
			CHECK(table.maxn,CALLER);
			break;
		case 'u':
			CHECK(until,UNTIL);
			CHECK(unpack,CALLER);
			break;
		case 'w':
			CHECK(while,WHILE);
			break;
		case 'x':
			CHECK(xpcall,CALLER);
			break;
	}
	
	return TK_NAME;
	#undef CHECK
}

%}

%option reentrant noyywrap

w              [ \t\v\a]+
o              [ \t\v\a]*
name           [_a-zA-Z][_a-zA-Z0-9\.]*
n              [0-9]+
exp            [Ee][+-]?{n}
number         ({n}|{n}[.]{n}){exp}?
eol            [\n\r]
sep            =*
brace					 [\(\)\[\]\{\}]

%x XLONGSTRING
%x XSHORTCOMMENT
%x XLONGCOMMENT
%x XSTRINGQ
%x XSTRINGA

%%

^#!.*          {if(firstline) firstline=0;}
{name}         return checkKw(yytext);
{number}       return TK_NUMBER;
"--[["  			 BEGIN(XLONGCOMMENT); return TK_LONG_COMMENT_IN;
"[["    			 BEGIN(XLONGSTRING); return TK_LONG_STRING_IN;
"--"           BEGIN( XSHORTCOMMENT); return TK_SHORT_COMMENT_IN;
{w}            return TK_WHITESPACE;
\"             BEGIN(XSTRINGQ); return TK_DOUBLE_QUOTE_IN;
'              BEGIN(XSTRINGA); return TK_SINGLE_QUOTE_IN;
"..."          return TK_DOTS;
".."           return TK_CONCAT;
{brace}				 return TK_BRACE;
{eol}          return TK_NEWLINE;
.              return yytext[0];

<XSTRINGQ>
{
   \\\"	       return TK_RAW;
   \"          BEGIN(0); return TK_DOUBLE_QUOTE_OUT;
   {eol}	     BEGIN(0); return TK_DOUBLE_QUOTE_ERR;
   .           return TK_RAW;
}

<XSTRINGA>
{
   \\'	       return TK_RAW;
   '           BEGIN(0); return TK_SINGLE_QUOTE_OUT;
   {eol}	     BEGIN(0); return TK_SINGLE_QUOTE_ERR;
   .           return TK_RAW;
}

<XSHORTCOMMENT>
{
   {eol}          BEGIN(0); return TK_SHORT_COMMENT_OUT;
   .              return TK_RAW;
}

<XLONGCOMMENT>
{
   "--]]"  				BEGIN(0); return TK_LONG_COMMENT_OUT;
   {eol}          return TK_NEWLINE;
   .              return TK_RAW;
}

<XLONGSTRING>
{
   "]]"				    BEGIN(0); return TK_LONG_STRING_OUT;
   {eol}          return TK_NEWLINE;
   .              return TK_RAW;
}

%%

void
lua_to_markup(const char *utf8)
{
	yyscan_t scanner;
	YY_BUFFER_STATE buf;
	enclex_init(&scanner);
	buf = enc_scan_string(utf8, scanner);

	for(int tok=enclex(scanner); tok; tok=enclex(scanner))
	{
		const char *txt = encget_text(scanner);
		switch(tok)
		{
			case TK_CALLER:
				_add_markup("function", txt);
				break;

			case TK_LOCAL:
				_add_markup("type", txt);
				break;

			case TK_FUNCTION:
			case TK_AND:
			case TK_BREAK:
			case TK_DO:
			case TK_ELSE:
			case TK_ELSEIF:
			case TK_END:
			case TK_FOR:
			case TK_IF:
			case TK_IN:
			case TK_NOT:
			case TK_OR:
			case TK_REPEAT:
			case TK_RETURN:
			case TK_THEN:
			case TK_UNTIL:
			case TK_WHILE:
			case TK_CONCAT:
				_add_markup("keyword", txt);
				break;

			case TK_NUMBER:
			case TK_NIL:
			case TK_TRUE:
			case TK_FALSE:
			case TK_DOTS:
				_add_markup("number", txt);
				break;

			case TK_SINGLE_QUOTE_IN:
				_add_markup_begin("string");
				_add_plain("'");
				break;
			case TK_SINGLE_QUOTE_OUT:
				_add_plain("'");
				_add_markup_end("string");
				break;
			case TK_SINGLE_QUOTE_ERR:
				_add_markup_end("string");
				_add_singleton("br");
				break;
			
			case TK_DOUBLE_QUOTE_IN:
				_add_markup_begin("string");
				_add_plain("\"");
				break;
			case TK_DOUBLE_QUOTE_OUT:
				_add_plain("\"");
				_add_markup_end("string");
				break;
			case TK_DOUBLE_QUOTE_ERR:
				_add_markup_end("string");
				_add_singleton("br");
				break;

			case TK_LONG_STRING_IN:
				_add_markup_begin("string");
				_add_plain("[[");
				break;
			case TK_LONG_STRING_OUT:
				_add_plain("]]");
				_add_markup_end("string");
				break;

			case TK_SHORT_COMMENT_IN:
				_add_markup_begin("brace");
				_add_plain("--");
				break;
			case TK_SHORT_COMMENT_OUT:
				_add_markup_end("brace");
				_add_singleton("br");
				break;
			
			case TK_LONG_COMMENT_IN:
				_add_markup_begin("brace");
				_add_plain("--[[");
				break;
			case TK_LONG_COMMENT_OUT:
				_add_plain("--]]");
				_add_markup_end("brace");
				break;

			case TK_NEWLINE:
				_add_singleton("br");
				break;
			
			case TK_BRACE:
				_add_markup("comment", txt);
				break;

			case TK_BADCHAR:
				break;

			case TK_NAME:
			case TK_WHITESPACE:
			case TK_RAW:
			default:
				_add_plain(txt);
				break;
		}
	}

	enc_delete_buffer(buf, scanner);
	enclex_destroy(scanner);
}
