<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	<xsl:output method="text" omit-xml-declaration="yes"/>
	<xsl:template match="/">
local snippets = {
	<xsl:for-each select="html/body/div/div/div/pre/code">
	{
		'<xsl:value-of select="current()/@id"/>',
		[[
			<xsl:value-of select="current()"/>
		]]
	},
	</xsl:for-each> 
}

for i, v in ipairs(snippets) do
	run = nil
	once = nil
	stash = nil
	apply = nil
	save = nil
	restore = nil

	local f = load(v[2])
	assert(f)

	print('[test] ' .. v[1])

	f()

	local io = Stash()

	if stash then
		stash(io)
	end

	io:read()

	if apply then
		apply(io)
	end

	io:write()

	if save then
		save(io)
	end

	io:read()

	if restore then
		restore(io)
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
