<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	<xsl:output method="text" omit-xml-declaration="yes"/>
	<xsl:template match="/">
local _snippets = {
	<xsl:for-each select="html/body/div/div/div/pre/code">
	{
		'<xsl:value-of select="current()/@id"/>',
		[[
			<xsl:value-of select="current()"/>
		]]
	},
	</xsl:for-each> 
}

for _, _v in ipairs(_snippets) do
	run = nil
	once = nil
	stash = nil
	apply = nil
	save = nil
	restore = nil

	local _f = load(_v[2])
	assert(_f)

	print('[test] ' .. _v[1])

	_f()

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
	</xsl:template>
</xsl:stylesheet>
