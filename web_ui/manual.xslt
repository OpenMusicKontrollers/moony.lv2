<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	<xsl:output method="text" omit-xml-declaration="yes"/>
	<xsl:template match="/">
local snippets = {
		<xsl:for-each select="html/body/div/pre/code">
[[
			<xsl:value-of select="current()"/>
]],
		</xsl:for-each> 
}

for i, v in ipairs(snippets) do
	local f = load(v)
	if f then
		print('[manual] ' .. i)
		f()
	end
end
	</xsl:template>
</xsl:stylesheet>
