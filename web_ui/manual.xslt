<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	<xsl:output method="text" omit-xml-declaration="yes"/>
	<xsl:template match="/">
local function _test()
	local _io = Stash()

	if stash then
		stash(_io)
	end

	_io:read()

	if apply then
		apply(_io)
	end

	_io:write()

	if save then
		save(_io)
	end

	_io:read()

	if restore then
		restore(_io)
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

	_test()
end
</xsl:for-each> 
	</xsl:template>
</xsl:stylesheet>
