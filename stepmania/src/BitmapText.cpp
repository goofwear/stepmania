#include "stdafx.h"
/*
-----------------------------------------------------------------------------
 File: BitmapText

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "BitmapText.h"
#include "IniFile.h"
#include "FontManager.h"
#include "RageLog.h"
#include "RageException.h"
#include "RageTimer.h"
#include "RageDisplay.h"
#include "PrefsManager.h"
#include "ThemeManager.h"
#include "GameConstantsAndTypes.h"
#include "Font.h"

char *g_utf8_find_next_char (const char *p,
                       const char *end)
{
  if (*p) {
      if (end)
        for (++p; p < end && (*p & 0xc0) == 0x80; ++p)
          ;
      else
        for (++p; (*p & 0xc0) == 0x80; ++p)
          ;
  }
  return (p == end) ? NULL : (char *)p;
}

#define UTF8_GET                             \

wchar_t
g_utf8_get_char (const char *p)
{
  int i, mask = 0, len;
  wchar_t result;
  unsigned char c = (unsigned char) *p;

  if (c < 128) {
      len = 1;
      mask = 0x7f;
  } else if ((c & 0xe0) == 0xc0) {
      len = 2;
      mask = 0x1f;
  } else if ((c & 0xf0) == 0xe0) {
      len = 3;
      mask = 0x0f;
  } else if ((c & 0xf8) == 0xf0) {
      len = 4;
      mask = 0x07;
  } else if ((c & 0xfc) == 0xf8) {
      len = 5;
      mask = 0x03;
  } else if ((c & 0xfe) == 0xfc) {
      len = 6;
      mask = 0x01;
  } else
    return (wchar_t)-1;

  result = wchar_t(p[0] & mask);
  for (i = 1; i < len; ++i) {
      if ((p[i] & 0xc0) != 0x80)
          return (wchar_t) -1;

	  result <<= 6;
      result |= p[i] & 0x3f;
  }

  return result;
}

wstring CStringToWstring(const CString &str)
{
	const char *ptr = str.c_str(), *end = str.c_str()+str.size();

	wstring ret;

	while(ptr)
	{
		wchar_t c = g_utf8_get_char (ptr);
		if(c != wchar_t(-1))
			ret.push_back(c);

		ptr = g_utf8_find_next_char (ptr, end);

	}

	return ret;
}







#define RAINBOW_COLOR(n)	THEME->GetMetricC("BitmapText",ssprintf("RainbowColor%i", n+1))

const int NUM_RAINBOW_COLORS = 7;
RageColor RAINBOW_COLORS[NUM_RAINBOW_COLORS];


BitmapText::BitmapText()
{
	// Loading these theme metrics is slow, so only do it ever 20th time.
	static int iReloadCounter = 0;
	if( iReloadCounter%20==0 )
	{
		for(int i = 0; i < NUM_RAINBOW_COLORS; ++i)
			RAINBOW_COLORS[i] = RAINBOW_COLOR(i);
	}
	iReloadCounter++;

	m_HorizAlign = align_center;
	m_VertAlign = align_middle;

	m_pFont = NULL;

	m_iWidestLineWidth = 0;
	
	m_bShadow = true;

	m_bRainbow = false;
}

BitmapText::~BitmapText()
{
	if( m_pFont )
		FONT->UnloadFont( m_pFont );
}

bool BitmapText::LoadFromFont( CString sFontFilePath )
{
	LOG->Trace( "BitmapText::LoadFromFontName(%s)", sFontFilePath.GetString() );

	if( m_pFont ) {
		FONT->UnloadFont( m_pFont );
		m_pFont = NULL;
	}

	// load font
	m_pFont = FONT->LoadFont( sFontFilePath );

	BuildChars();

	return true;
}


bool BitmapText::LoadFromTextureAndChars( CString sTexturePath, CString sChars )
{
	LOG->Trace( "BitmapText::LoadFromTextureAndChars(%s)", sTexturePath.GetString() );

	if( m_pFont ) {
		FONT->UnloadFont( m_pFont );
		m_pFont = NULL;
	}

	// load font
	m_pFont = FONT->LoadFont( sTexturePath, sChars );

	BuildChars();

	return true;
}

void BitmapText::BuildChars()
{
	/* If we don't have a font yet, we'll do this when it loads. */
	if(m_pFont == NULL)
		return;

	verts.clear();
	tex.clear();
	
	int TotalHeight = 0;
	unsigned i;
	for(i = 0; i < m_szTextLines.size(); ++i)
		TotalHeight += m_iLineHeights[i];

	int iY;	//	 the top position of the first row of characters
	switch( m_VertAlign )
	{
	case align_top:		iY = 0;					break;
	case align_middle:	iY = -TotalHeight/2;	break;
	case align_bottom:	iY = -TotalHeight;		break;
	default:			ASSERT( false );		return;
	}

	for( i=0; i<m_szTextLines.size(); i++ )		// foreach line
	{
		const wstring &szLine = m_szTextLines[i];
		const int iLineWidth = m_iLineWidths[i];
		
		int iX;
		switch( m_HorizAlign )
		{
		case align_left:	iX = 0;				break;
		case align_center:	iX = -iLineWidth/2;	break;
		case align_right:	iX = -iLineWidth;	break;
		default:			ASSERT( false );	return;
		}

		for( unsigned j=0; j<szLine.size(); j++ )	// for each character in the line
		{
			RageVertex v[4];
			const glyph &g = m_pFont->GetGlyph(szLine[j]);

			/* set vertex positions */
			v[0].p = RageVector3( iX+g.hshift,			iY+g.vshift,		  0 );	// top left
			v[1].p = RageVector3( iX+g.hshift,			iY+g.vshift+g.height, 0 );	// bottom left
			v[2].p = RageVector3( iX+g.hshift+g.width,	iY+g.vshift+g.height, 0 );	// bottom right
			v[3].p = RageVector3( iX+g.hshift+g.width,	iY+g.vshift,		  0 );	// top right

			/* Advance the cursor. */
			iX += g.hadvance;

			/* set texture coordinates */
			v[0].t = RageVector2( g.rect.left,	g.rect.top );
			v[1].t = RageVector2( g.rect.left,	g.rect.bottom );
			v[2].t = RageVector2( g.rect.right,	g.rect.bottom );
			v[3].t = RageVector2( g.rect.right,	g.rect.top );

			verts.insert(verts.end(), &v[0], &v[4]);
			tex.push_back(m_pFont->GetGlyphTexture(szLine[j]));
		}

		iY += m_iLineHeights[i];
	}
}

void BitmapText::DrawChars()
{
	for(unsigned start = 0; start < tex.size(); )
	{
		unsigned end = start;
		while(end < tex.size() && tex[end] == tex[start])
			end++;
		DISPLAY->SetTexture( tex[start] );
		DISPLAY->DrawQuads( &verts[start*4], (end-start)*4 );
		
		start = end;
	}
}


/* sText is UTF-8. */
void BitmapText::SetText( CString sText )
{
	ASSERT( m_pFont );
	if(m_szText == sText)
		return;

	m_szText = sText;

	/* Break the string into lines. */
	m_szTextLines.clear();
	m_iLineWidths.clear();
	m_iLineHeights.clear();

	split(CStringToWstring(sText), L"\n", m_szTextLines, false);
	
	/* calculate line lengths and widths */
	m_iWidestLineWidth = 0;

	for( unsigned l=0; l<m_szTextLines.size(); l++ )	// for each line
	{
		m_iLineWidths.push_back(m_pFont->GetLineWidthInSourcePixels( m_szTextLines[l] ));
		m_iLineHeights.push_back(m_pFont->GetLineHeightInSourcePixels( m_szTextLines[l] ));
		m_iWidestLineWidth = max(m_iWidestLineWidth, m_iLineWidths.back());
	}

	BuildChars();
}

void BitmapText::CropToWidth( int iMaxWidthInSourcePixels )
{
	iMaxWidthInSourcePixels = max( 0, iMaxWidthInSourcePixels );

	for( unsigned l=0; l<m_szTextLines.size(); l++ )	// for each line
	{
		while( m_iLineWidths[l] > iMaxWidthInSourcePixels )
		{
			m_szTextLines[l].erase(m_szTextLines[l].end()-1, m_szTextLines[l].end());
			m_iLineWidths[l] = m_pFont->GetLineWidthInSourcePixels( m_szTextLines[l] );
		}
	}
}

// draw text at x, y using colorTop blended down to colorBottom, with size multiplied by scale
void BitmapText::DrawPrimitives()
{
	if( m_szTextLines.empty() )
		return;

	DISPLAY->SetTextureModeModulate();
	if( m_bBlendAdd )
		DISPLAY->SetBlendModeAdd();
	else
		DISPLAY->SetBlendModeNormal();

	/* Draw if we're not fully transparent or the zbuffer is enabled (which ignores
	 * alpha). */
	if( m_temp.diffuse[0].a != 0 || DISPLAY->ZBufferEnabled())
	{
		//////////////////////
		// render the shadow
		//////////////////////
		if( m_bShadow )
		{
			DISPLAY->PushMatrix();
			DISPLAY->TranslateLocal( m_fShadowLength, m_fShadowLength, 0 );	// shift by 5 units

			RageColor dim(0,0,0,0.5f*m_temp.diffuse[0].a);	// semi-transparent black

			for( unsigned i=0; i<verts.size(); i++ )
				verts[i].c = dim;
			DrawChars();

			DISPLAY->PopMatrix();
		}

		//////////////////////
		// render the diffuse pass
		//////////////////////
		if( m_bRainbow )
		{
			int color_index = int(RageTimer::GetTimeSinceStart() / 0.200) % NUM_RAINBOW_COLORS;
			for( unsigned i=0; i<verts.size(); i+=4 )
			{
				const RageColor color = RAINBOW_COLORS[color_index];
				for( unsigned j=i; j<i+4; j++ )
					verts[j].c = color;

				color_index = (color_index+1)%NUM_RAINBOW_COLORS;
			}
		}
		else
		{
			for( unsigned i=0; i<verts.size(); i+=4 )
			{
				verts[i+0].c = m_temp.diffuse[0];	// top left
				verts[i+1].c = m_temp.diffuse[2];	// bottom left
				verts[i+2].c = m_temp.diffuse[3];	// bottom right
				verts[i+3].c = m_temp.diffuse[1];	// top right
			}
		}

		DrawChars();
	}

	/* render the glow pass */
	if( m_temp.glow.a != 0 )
	{
		DISPLAY->SetTextureModeGlow();

		for( unsigned i=0; i<verts.size(); i++ )
			verts[i].c = m_temp.glow;
		DrawChars();
	}
}

/* Rebuild when these change. */
void BitmapText::SetHorizAlign( HorizAlign ha )
{
	if(ha == m_HorizAlign) return;
	Actor::SetHorizAlign(ha);
	BuildChars();
}

void BitmapText::SetVertAlign( VertAlign va )
{
	if(va == m_VertAlign) return;
	Actor::SetVertAlign(va);
	BuildChars();
}
