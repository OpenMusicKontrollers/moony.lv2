<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	<xsl:output method="text" omit-xml-declaration="yes"/>
	<xsl:template match="/">
local function _test(produce, consume, check)
	local _from = Stash()
	local _to = Stash()

	if produce.seq then
		local _seq = _from:sequence()
		produce.seq(_seq)
		_seq:pop()
	elseif produce.atom then
		produce.atom(_from)
	end

	_from:read()

	if consume.seq then
		local _seq = _to:sequence()
		consume.seq(_from, _seq)
		_seq:pop()
	elseif consume.atom then
		consume.atom(_from, _to)
	end

	_to:read()

	if check then
		check(_to)
	end

	local _state = Stash()

	if stash then
		stash(_state)
	end

	_state:read()

	if apply then
		apply(_state)
	end

	_state:write()

	if save then
		save(_state)
	end

	_state:read()

	if restore then
		restore(_state)
	end

	if once then
		--FIXME
	end

	if run then
		--FIXME
	end
end

	<xsl:for-each select="html/body/div/div/div/pre/code">
do
	run = nil
	once = nil
	stash = nil
	apply = nil
	save = nil
	restore = nil

	print('[test] <xsl:value-of select="current()/@id"/>')
	<xsl:value-of select="current()"/>

	local _produce = {
		seq = produce_seq,
		atom = produce_atom
	}
	local _consume = {
		seq = consume_seq,
		atom = consume_atom
	}
	_test(_produce, _consume, check)
end
</xsl:for-each> 
	</xsl:template>
</xsl:stylesheet>
