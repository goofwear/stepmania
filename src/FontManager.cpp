#include "stdafx.h"
/*
-----------------------------------------------------------------------------
 Class: FontManager

 Desc: Interface for loading and releasing fonts.

 Copyright (c) 2001-2003 by the person(s) listed below.  All rights reserved.
	Chris Danford
	Glenn Maynard
-----------------------------------------------------------------------------
*/


#include "FontManager.h"
#include "Font.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "RageException.h"
#include "RageTimer.h"
#include "ThemeManager.h"
#include "GameManager.h"

FontManager::aliasmap FontManager::CharAliases;
map<CString,CString> FontManager::CharAliasRepl;

FontManager*	FONT	= NULL;

FontManager::FontManager()
{
}

FontManager::~FontManager()
{
	for( std::map<CString, Font*>::iterator i = m_mapPathToFont.begin();
		i != m_mapPathToFont.end(); ++i)
	{
		Font* pFont = i->second;
		LOG->Trace( "FONT LEAK: '%s', RefCount = %d.", i->first.GetString(), pFont->m_iRefCount );
		delete pFont;
	}
}

/* A longchar is at least 32 bits.  If c & 0xFF000000, it's a game-custom
 * character; game 0 is 0x01nnnnnn, game 1 is 0x02nnnnnn, and so on. */
longchar FontManager::MakeGameGlyph(longchar c, Game g)
{
	ASSERT(g >= 0 && g <= 0xFF && c >= 0 && c <= 0xFFFFFF);
	return ((g+1) << 24) + c;
}

bool FontManager::ExtractGameGlyph(longchar ch, int &c, Game &g)
{
	if((ch & 0xFF000000) == 0) return false; /* not a game glyph */
	
	g = Game((ch >> 24) - 1);
	c = ch & 0xFFFFFF;

	return true;
}

void FontManager::LoadFontPageSettings(FontPageSettings &cfg, IniFile &ini, const CString &PageName, int NumFrames)
{
	if(NumFrames == 128 || NumFrames == 256)
	{
		/* If it's 128 or 256 frames, default to ASCII or ISO-8859-1,
		 * respectively.  If it's anything else, we don't know what it
		 * is, so don't make any default mappings (the INI needs to do
		 * it itself). */
		/*
		if(NumFrames == 128)
			cfg.MapRange("ASCII", 0, 127, 0);
		else if(NumFrames == 256)
			cfg.MapRange("ISO-8859-1", 0, 255, 0);
		*/
		for( longchar i=0; i<NumFrames; i++ )
			cfg.CharToGlyphNo[i] = i;
	}
	ini.RenameKey("Char Widths", "main");

	ini.GetValueI( PageName, "DrawExtraPixelsLeft", cfg.DrawExtraPixelsLeft );
	ini.GetValueI( PageName, "DrawExtraPixelsRight", cfg.DrawExtraPixelsRight );
	ini.GetValueI( PageName, "AddToAllWidths", cfg.AddToAllWidths );
	ini.GetValueF( PageName, "ScaleAllWidthsBy", cfg.ScaleAllWidthsBy );
	ini.GetValueI( PageName, "LineSpacing", cfg.LineSpacing );

	/* Iterate over all keys. */
	const IniFile::key *k = ini.GetKey(PageName);
	if(k == NULL)
		return;

	for(IniFile::key::const_iterator key = k->begin(); key != k->end(); ++key)
	{
		CString val = key->first;
		CString data = key->second;

		val.MakeUpper();

		/* If val is an integer, it's a width, eg. "10=27". */
		if(IsAnInt(val))
		{
			cfg.GlyphWidths[atoi(val)] = atoi(data);
			continue;
		}

		/* "map XXXX=frame" maps a char to a frame. */
		if(val.substr(0, 4) == "MAP ")
		{
			/* map CODEPOINT=frame. CODEPOINT can be
			 * 1. U+hexval
			 * 2. an alias ("kakumei1")
			 * 3. a game type followed by a game alias, eg "pump menuleft"
			 *
			 * map 1=2 is the same as
			 * range unicode #1-1=2
			 */
			CString codepoint = val.substr(4); /* "XXXX" */
		
			longchar c;
			Game game = GAME_INVALID;

			if(codepoint.find_first_of(' ') != codepoint.npos)
			{
				/* There's a space; the first word should be a game type. Split it. */
				unsigned pos = codepoint.find_first_of(' ');
				CString gamename = codepoint.substr(0, pos);
				codepoint = codepoint.substr(pos+1);

				game = GameManager::StringToGameType(gamename);

				if(game == GAME_INVALID)
					RageException::Throw( "Font definition '%s' uses unknown game type '%s'",
						ini.GetPath().GetString(), gamename.GetString() );
			}

			if(codepoint.substr(0, 2) == "U+" && IsHexVal(codepoint.substr(2)))
				sscanf(codepoint.substr(2).c_str(), "%x", &c);
			else if(CharAliases.find(codepoint) != CharAliases.end())
				c = CharAliases[codepoint];
			else
				RageException::Throw( "Font definition '%s' has an invalid value '%s'.",
					ini.GetPath().GetString(), val.GetString() );

			if(game != GAME_INVALID) c = MakeGameGlyph(c, game);

			int frame = atoi(data);
			if(frame >= NumFrames)
				RageException::Throw( "Font definition '%s' maps to frame %i, but font only has %i frames",
					ini.GetPath().GetString(), frame, NumFrames );

			cfg.CharToGlyphNo[c] = frame;
			continue;
		}

		if(val.substr(0, 6) == "RANGE ")
		{
			/* range RANGE=first_frame
			 *
			 * RANGE is:
			 * CODESET               or
			 * CODESET #start-end
			 * eg
			 * range ISO-8859-1=0   (default for 256-frame fonts)
			 * range ASCII=0        (default for 128-frame fonts)
			 *
			 * (Start and end are in hex.)
			 *
			 * Map two high-bit portions of ISO-8859- to one font:
			 * range ISO-8859-2 #80-FF=0
			 * range ISO-8859-3 #80-FF=128
			 *
			 * Map hiragana to 0-84:
			 * range Unicode #3041-3094=0
			 */
			CString range = val.substr(6);
			unsigned first = 0, last = 0xFFFF; /* all */
			unsigned space = range.find_first_of(' ');
			if(space != val.npos)
			{

			}



			continue;
		}

	}
}

/* "Normal.png", "Normal [foo].png", "Normal 16x16.png", and "Normal [foo] 16x16.png"
 * are all part of the FontName "Normal".  "Normal2 16x16.png" is not.
 *
 * So, FileName must start with FontName, followed by either " [", " DIGIT" or
 * a period.
 *
 * This is to make sure we don't pull in textures from similarly-named fonts.
 */ 
bool FontManager::MatchesFont(CString FontName, CString FileName)
{
	FontName.MakeLower();
	FileName.MakeLower();

	if(FileName.substr(0, FontName.size()) != FontName)
		return false;
	FileName = FileName.substr(FontName.size());

	return regex("^( \\[|\\.| [0-9]+x[0-9]+)", FileName);
}

void FontManager::GetFontPaths(const CString &sFontOrTextureFilePath, 
							   CStringArray &TexturePaths, CString &IniPath)
{
	CString sDrive, sDir, sFName, sExt;
	splitpath( false, sFontOrTextureFilePath, sDrive, sDir, sFName, sExt );

	ASSERT(sExt.CompareNoCase("ini")); /* don't give us an INI! */
	ASSERT(sExt.CompareNoCase("redir")); /* don't give us a redir! */

	if(!sExt.empty())
	{
		TexturePaths.push_back(sFontOrTextureFilePath);
		return;
	}

	/* If we have no extension, we need to search. */
	GetDirListing( sFontOrTextureFilePath + "*.png", TexturePaths, false, true );

	CStringArray IniPaths;
	GetDirListing( sFontOrTextureFilePath + "*.ini", IniPaths, false, true );

	/* Filter out texture files that aren't actually for this font. */
	unsigned i = 0;
	while(i < TexturePaths.size())
	{
		if(!MatchesFont(sFontOrTextureFilePath, TexturePaths[i]))
			TexturePaths.erase(TexturePaths.begin()+i);
		else
			i++;
	}

	for(i = 0; i < IniPaths.size(); ++i)
		if(MatchesFont(sFontOrTextureFilePath, IniPaths[i])) break;
	if(i < IniPaths.size()) IniPath = IniPaths[i];

	ASSERT(!TexturePaths.empty()); /* ThemeManager should have checked this */
}

static CStringArray LoadStack;

/* Load the named font and return it. */
Font *FontManager::LoadFontInt(const CString &sFontOrTextureFilePath, CString sChars)
{
	/* Check for recursion (recursive imports). */
	{
		for(unsigned i = 0; i < LoadStack.size(); ++i)
		{
			if(LoadStack[i] == sFontOrTextureFilePath)
			{
				CString str = join("\n", LoadStack);
				str += "\n" + sFontOrTextureFilePath;
				RageException::Throw("Font import recursion detected\n%s", str.GetString());
			}
		}
		LoadStack.push_back(sFontOrTextureFilePath);
	}
	/* The font is not already loaded.  Figure out what we have. */
//	LOG->Trace( "FontManager::LoadFont(%s).", sFontFilePath.GetString() );

	CStringArray TexturePaths;
	CString IniPath;
	GetFontPaths(sFontOrTextureFilePath, TexturePaths, IniPath);

	Font* pFont = new Font;
	pFont->path = sFontOrTextureFilePath;
	
	bool CapitalsOnly = false;

	IniFile ini;
	if( !IniPath.empty() )
	{
		ini.SetPath( IniPath );
		ini.ReadFile();
		ini.RenameKey("Char Widths", "main");
		ini.GetValueB( "main", "CapitalsOnly", CapitalsOnly );
	}

	{
		/* Check to see if we need to import any other fonts.  Do this
		 * before loading this font, so any characters in this font
		 * override those imported. */
		CString imports;
		ini.GetValue( "main", "import", imports );
		CStringArray ImportList;
		split(imports, ",", ImportList, true);

		/* Load the default font first, unless we're currently loading it. */
		static bool LoadingDefaultFont = false;
		if(!LoadingDefaultFont)
		{
			LoadingDefaultFont = true;
			CString path = THEME->GetPathTo("Fonts", "default font");
			pFont->MergeFont(FONT->LoadFont(path));
			LoadingDefaultFont = false;
		}

		for(unsigned i = 0; i < ImportList.size(); ++i)
		{
			CString path = THEME->GetPathTo("Fonts", ImportList[i]);
			pFont->MergeFont(FONT->LoadFont(path));
		}
	}

	/* Load each font page. */
	int expect = -1;
	for(unsigned i = 0; i < TexturePaths.size(); ++i)
	{
		FontPage *fp = new FontPage;
		int frames;
		{
			int wide, high;
			RageTexture::GetFrameDimensionsFromFileName(TexturePaths[i], &wide, &high );
			frames = wide*high;
		}

		/* Grab the page name, eg "foo" from "Normal [foo].png". */
		CString pagename = GetPageNameFromFileName(TexturePaths[i]);

		FontPageSettings cfg;
		if( !sChars.empty() )
		{
			/* Map characters to frames. */
			for( int i=0; i<sChars.GetLength(); i++ )
			{
				char c = sChars[i];
				cfg.CharToGlyphNo[c] = i;
			}
			expect = sChars.GetLength();
		}

		LoadFontPageSettings(cfg, ini, pagename, frames);
		fp->Load(TexturePaths[i], cfg);
		if( expect != -1 && fp->m_pTexture->GetNumFrames() < expect )
			RageException::Throw( "The font '%s' has %d frames; expected at least %i frames.",
				fp->m_pTexture->GetNumFrames(), expect );

		LOG->Trace("Adding page %s (%s) to %s; %i glyphs",
			TexturePaths[i].GetString(), pagename.GetString(),
			sFontOrTextureFilePath.GetString(), fp->m_iCharToGlyphNo.size());
		pFont->AddPage(fp);
	}

	if(CapitalsOnly)
		pFont->CapsOnly();

	if(pFont->m_iCharToGlyph.empty())
		LOG->Warn("Font %s has no characters", sFontOrTextureFilePath.GetString());

	LoadStack.pop_back();

	return pFont;
}

/* A font set is a set of files, eg:
 *
 * Normal 16x16.png
 * Normal [other] 16x16.png
 * Normal [more] 8x8.png
 * Normal 16x16.ini           (the 16x16 here is optional)
 *
 * Only one texture is required; the INI is optional.  [1] This is
 * designed to be backwards-compatible.
 *
 * sFontOrTextureFilePath can be a partial path, eg.
 * "Themes/default/Fonts/Normal"
 * or a complete path to a texture file (in which case no other
 * files will be searched for).
 *
 * The entire font can be redirected; that's handled in ThemeManager.
 * Individual font files can not be redirected.
 *
 * TODO: 
 * [main]
 * import=FontName,FontName2 (load other fonts)
 *
 * [1] If a file has no INI and sChars is not set, it will receive a default
 * mapping of ASCII or ISO-8859-1 if the font has exactly 128 or 256 frames.
 * However, if it doesn't, we don't know what it is and the font will receive
 * no default mapping.  A font isn't useful with no characters mapped.
 */
Font* FontManager::LoadFont( const CString &sFontOrTextureFilePath, CString sChars )
{
	InitCharAliases();

	// Convert the path to lowercase so that we don't load duplicates.
	// Really, this does not solve the duplicate problem.  We could have to copies
	// of the same bitmap if there are equivalent but different paths
	// (e.g. "graphics\blah.png" and "..\stepmania\graphics\blah.png" ).

	map<CString, Font*>::iterator p = m_mapPathToFont.find(sFontOrTextureFilePath);
	if(p != m_mapPathToFont.end()) {
		Font *pFont=p->second;
//		LOG->Trace( ssprintf("FontManager: The Font '%s' now has %d references.", sFontOrTextureFilePath.GetString(), pFont->m_iRefCount) );
		pFont->m_iRefCount++;
		return pFont;
	}

	Font *f = LoadFontInt(sFontOrTextureFilePath, sChars);
	m_mapPathToFont[sFontOrTextureFilePath] = f;
	return f;
}


void FontManager::UnloadFont( Font *fp )
{
	LOG->Trace( "FontManager::UnloadTexture(%s).", fp->path.GetString() );

	for( std::map<CString, Font*>::iterator i = m_mapPathToFont.begin();
		i != m_mapPathToFont.end(); ++i)
	{
		if(i->second == fp)
		{
			i->second->m_iRefCount--;
	
			if( fp->m_iRefCount == 0 )
			{
				delete i->second;		// free the texture
				m_mapPathToFont.erase( i );	// and remove the key in the map
			}
//	LOG->Trace( "FontManager: '%s' will be deleted.  It has %d references.", sFontFilePath.GetString(), pFont->m_iRefCount );
			return;
		}

	}
	
	RageException::Throw( "Unloaded an unknown font (%p)", fp );
}

CString FontManager::GetPageNameFromFileName(const CString &fn)
{
	unsigned begin = fn.find_first_of('[');
	if(begin == fn.npos) return "main";
	unsigned end = fn.find_first_of(']', begin);
	if(end == fn.npos) return "main";
	begin++; end--;
	if(end == begin) return "main";
	return fn.substr(begin, end-begin+1);
}


void FontManager::InitCharAliases()
{
	if(!CharAliases.empty())
		return;
	/* XXX; these should be in a data file somewhere (theme metrics?) 
		* (The comments here are UTF-8; they won't show up in VC6.) */
	CharAliases["default"]		= Font::DEFAULT_GLYPH; /* ? */
	CharAliases["kakumei1"]		= 0x9769; /* 革 */
	CharAliases["kakumei2"]		= 0x547D; /* 命 */
	CharAliases["matsuri"]		= 0x796D; /* 祭 */
	CharAliases["oni"]			= 0x9B3C; /* 鬼 */
	CharAliases["michi"]		= 0x9053; /* 道 */
	CharAliases["futatsu"]		= 0x5F10; /* 弐 */
	CharAliases["omega"]		= 0x03a9; /* Ω */
	CharAliases["whiteheart"]	= 0x2661; /* ♡ */
	CharAliases["blackstar"]	= 0x2605; /* ★ */

	CharAliases["up"]			= 0x100000;
	CharAliases["down"]			= 0x100001;
	CharAliases["left"]			= 0x100002;
	CharAliases["right"]		= 0x100003;
	CharAliases["menuup"]		= 0x100004;
	CharAliases["menudown"]		= 0x100005;
	CharAliases["menuleft"]		= 0x100006;
	CharAliases["menuright"]	= 0x100007;
	CharAliases["start"]		= 0x100008;

	CharAliases["invalid"]		= INVALID_CHAR; /* 0xFFFFFF */

	for(aliasmap::const_iterator i = CharAliases.begin(); i != CharAliases.end(); ++i)
	{
		CString from = ssprintf("&%s;", i->first.GetString());
		CString to = LcharToUTF8(i->second);
		from.MakeUpper();
		CharAliasRepl[from] = to;
		LOG->Trace("from '%s' to '%s'", from.GetString(), to.GetString());
	}
}

/* Replace all &markers; and &#NNNN;s with UTF-8.  I'm not really
 * sure where to put this; this is used elsewhere, too. */
void FontManager::ReplaceMarkers( CString &sText )
{
	InitCharAliases();
	ReplaceText(sText, FontManager::CharAliasRepl);
	Replace_Unicode_Markers(sText);
}

