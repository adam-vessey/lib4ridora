<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:mods="http://www.loc.gov/mods/v3">

  <xsl:template match="mods:mods">
    <mods:mods>
      <xsl:copy-of select="@*"/>
      <xsl:apply-templates select="mods:titleInfo | mods:name | mods:originInfo | mods:relatedItem | mods:genre | mods:identifier"/>
    </mods:mods>
  </xsl:template>

  <xsl:template match="mods:titleInfo">
    <xsl:call-template name="copy">
      <xsl:with-param name="select" select="mods:title"/>
    </xsl:call-template>
  </xsl:template>
  <xsl:template match="mods:mods/mods:name">
    <xsl:call-template name="copy">
      <xsl:with-param name="select" select="mods:namePart"/>
    </xsl:call-template>
  </xsl:template>
  <xsl:template match="mods:mods/mods:relatedItem[@type='host']">
    <xsl:call-template name="copy">
      <xsl:with-param name="select" select="mods:titleInfo"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template name="copy">
    <xsl:param name="select" select="text() | *"/>

    <xsl:copy>
      <xsl:copy-of select="@*"/>
      <xsl:apply-templates select="$select" mode="copied"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="text()"/>
  <xsl:template match="mods:genre | mods:identifier[@type='doi'] | mods:title | mods:namePart" mode="copied">
    <xsl:call-template name="copy"/>
  </xsl:template>
</xsl:stylesheet>
