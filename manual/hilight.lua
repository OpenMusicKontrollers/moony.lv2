package.path = package.path .. ';../plugin/?.lua'

local lexer = require('lexer')
local moony = lexer.load('moony')

local f = io.input('../VERSION')
if f then
	version = f:read('*all')
	f:close()
end

local function parse(pin)
	if not pin then return end

	local fin = io.input(pin)
	if not fin then return end

	local txt = fin:read('*all')
	fin:close()

	txt = string.gsub(txt, '<pre><code%s*data%-ref%s*=%s*".-"%s*>(.-)</code></pre>', function(o)
		o = string.gsub(o, '&amp;', '&')
		o = string.gsub(o, '&lt;', '<')
		o = string.gsub(o, '&gt;', '>')

		local tokens = lexer.lex(moony, o)
		local ret = ''
		local from = 1
		local n = #tokens

		for i = 1, n, 2 do
			local token = tokens[i]
			local to = tokens[i+1]

			local sub = string.sub(o, from, to - 1)
			sub = string.gsub(sub, '&', '&amp;')
			sub = string.gsub(sub, '<', '&lt;')
			sub = string.gsub(sub, '>', '&gt;')
			sub = string.gsub(sub, '\t', '  ')

			if token then
				ret = ret .. string.format('<span style="color:#%06x;">%s</span>', token, sub)
			else
				ret = ret .. sub
			end

			from = to
		end

		return '<pre><code>' .. ret .. '</code></pre>'
	end)
	
	txt = string.gsub(txt, '@MOONY_VERSION@', version)
	io.stdout:write(txt)
end

parse('../manual/manual.html.in')
