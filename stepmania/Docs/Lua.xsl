<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet version="1.0"
 xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
 xmlns:sm="http://www.stepmania.com">

<xsl:template match="/">
	<html>
		<head>
			<title>StepMania LUA Information</title>
			<style type="text/css">
				th {
					background-color: #FAEBD7;
					border: 1px solid
				}
				table {
					border: 1px solid
				}
				td {
					border: 1px solid
				}
				hr {
					width: 90%
				}
				.trigger {
					cursor: pointer
				}
				.footer {
					text-align: center
				}
			</style>
			<script type="text/javascript">
				function Toggle( id )
				{
					var imgid = 'img_' + id;
					var listid = 'list_' + id;
					var img = document.getElementById( imgid );
					var list = document.getElementById( listid );
					
					if( img.getAttribute('src') == 'closed.gif' )
					{
						img.setAttribute( 'src', 'open.gif' );
						list.style.display = 'block';
					}
					else
					{
						img.setAttribute( 'src', 'closed.gif' );
						list.style.display = 'none';
					}
				}
			</script>
		</head>
		<body>
			<xsl:apply-templates />
		</body>
	</html>
</xsl:template>


<xsl:template match="sm:Lua">
	<div>
		<h2>StepMania LUA Information</h2>
		<table>
			<tr>
				<th><a href="#Singletons">Singletons</a></th>
				<th><a href="#Classes">Classes</a></th>
				<th><a href="#GlobalFunctions">Global Functions</a></th>
				<th><a href="#Enums">Enums</a></th>
				<th><a href="#Constants">Constants</a></th>
			</tr>
		</table>
	</div>
	<xsl:apply-templates select="sm:Singletons" />
	<xsl:apply-templates select="sm:Classes" />
	<xsl:apply-templates select="sm:GlobalFunctions" />
	<xsl:apply-templates select="sm:Enums" />
	<xsl:apply-templates select="sm:Constants" />
	<hr />
	<p class="footer">
		Generated for <xsl:value-of select="sm:Version" /> on
		<xsl:value-of select="sm:Date" />.
	</p>
</xsl:template>


<xsl:template match="sm:Singletons">
	<div>
		<a name="Singletons" />
		<h3>Singletons</h3>
		<ul>
			<xsl:for-each select="sm:Singleton">
				<xsl:sort select="@name" />
				<li>
					<a href="#{@class}">
						<xsl:value-of select="@name" />
					</a>
				</li>
			</xsl:for-each>
		</ul>
	</div>
</xsl:template>


<xsl:template match="sm:Classes">
	<div>
		<a name="Classes" />
		<h3>Classes</h3>
		<xsl:apply-templates select="sm:Class">
			<xsl:sort select="@name" />
		</xsl:apply-templates>
	</div>
</xsl:template>


<xsl:template match="sm:Class">
	<div>
		<a name="{@name}" class="trigger" onClick="Toggle('{@name}')">
			<img src="closed.gif" id="img_{@name}" />
			Class <xsl:value-of select="@name" />
		</a>
		<xsl:if test="@base != ''">
			<xsl:text> : </xsl:text>
			<a href="#{@base}">
				<xsl:value-of select="@base" />
			</a>
		</xsl:if>
		<ul style="display: none" id="list_{@name}">
			<xsl:apply-templates select="sm:Function">
				<xsl:sort select="@name" />
			</xsl:apply-templates>
		</ul>
	</div>
</xsl:template>


<xsl:template match="sm:GlobalFunctions">
	<div>
		<a name="GlobalFunctions" />
		<h3>Global Functions</h3>
		<ul>
			<xsl:apply-templates select="sm:Function">
				<xsl:sort select="@name" />
			</xsl:apply-templates>
		</ul>
	</div>
</xsl:template>

<xsl:template match="sm:Function">
	<li><xsl:value-of select="@name" /></li>
</xsl:template>

<xsl:template match="sm:Enums">
	<div>
		<a name="Enums" />
		<h3>Enums</h3>
		<xsl:apply-templates select="sm:Enum">
			<xsl:sort select="@name" />
		</xsl:apply-templates>
	</div>
</xsl:template>


<xsl:template match="sm:Enum">
	<div>
		<a class="trigger" onClick="Toggle('{@name}')">
		<img src="closed.gif" id="img_{@name}" />
		Enum <xsl:value-of select="@name" /></a>
		<table style="display: none" id="list_{@name}">
			<tr>
				<th>Enum</th>
				<th>Value</th>
			</tr>
			<xsl:for-each select="sm:EnumValue">
				<xsl:sort data-type="number" select="@value" />
				<tr>
					<td><xsl:value-of select="@name" /></td>
					<td><xsl:value-of select="@value" /></td>
				</tr>
			</xsl:for-each>
		</table>
		<br />
	</div>
</xsl:template>


<xsl:template match="sm:Constants">
	<div>
		<a name="Constants" />
		<h3>Constants</h3>
		<table>
			<tr>
				<th>Constant</th>
				<th>Value</th>
			</tr>
			<xsl:for-each select="sm:Constant">
				<xsl:sort select="@name" />
				<tr>
					<td><xsl:value-of select="@name" /></td>
					<td><xsl:value-of select="@value" /></td>
				</tr>
			</xsl:for-each>
		</table>
		<br />
	</div>
</xsl:template>
</xsl:stylesheet>
