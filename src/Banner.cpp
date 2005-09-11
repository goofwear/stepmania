#include "global.h"
#include "Banner.h"
#include "SongManager.h"
#include "RageUtil.h"
#include "song.h"
#include "RageTextureManager.h"
#include "Course.h"
#include "Character.h"
#include "ThemeMetric.h"
#include "CharacterManager.h"
#include "ActorUtil.h"

REGISTER_ACTOR_CLASS( Banner )

ThemeMetric<bool> SCROLL_RANDOM		("Banner","ScrollRandom");
ThemeMetric<bool> SCROLL_ROULETTE		("Banner","ScrollRoulette");

Banner::Banner()
{
	m_bScrolling = false;
	m_fPercentScrolling = 0;
}

/* Ugly: if sIsBanner is false, we're actually loading something other than a banner. */
bool Banner::Load( RageTextureID ID, bool bIsBanner )
{
	if( ID.filename == "" )
	{
		LoadFallback();
		return true;
	}

	if( bIsBanner )
		ID = SongBannerTexture(ID);

	m_fPercentScrolling = 0;
	m_bScrolling = false;

	TEXTUREMAN->DisableOddDimensionWarning();
	TEXTUREMAN->VolatileTexture( ID );
	bool ret = Sprite::Load( ID );
	TEXTUREMAN->EnableOddDimensionWarning();

	return ret;
};

void Banner::Update( float fDeltaTime )
{
	Sprite::Update( fDeltaTime );

	if( m_bScrolling )
	{
		m_fPercentScrolling += fDeltaTime/2;
		m_fPercentScrolling -= (int)m_fPercentScrolling;

		const RectF *pTextureRect = m_pTexture->GetTextureCoordRect(0);
 
		float fTexCoords[8] = 
		{
			0+m_fPercentScrolling, pTextureRect->top,		// top left
			0+m_fPercentScrolling, pTextureRect->bottom,	// bottom left
			1+m_fPercentScrolling, pTextureRect->bottom,	// bottom right
			1+m_fPercentScrolling, pTextureRect->top,		// top right
		};
		Sprite::SetCustomTextureCoords( fTexCoords );
	}
}

void Banner::SetScrolling( bool bScroll, float Percent)
{
	m_bScrolling = bScroll;
	m_fPercentScrolling = Percent;

	/* Set up the texture coord rects for the current state. */
	Update(0);
}

void Banner::LoadFromSong( Song* pSong )		// NULL means no song
{
	if( pSong == NULL )					LoadFallback();
	else if( pSong->HasBanner() )		Load( pSong->GetBannerPath() );
	else								LoadFallback();

	m_bScrolling = false;
}

void Banner::LoadAllMusic()
{
	Load( THEME->GetPathG("Banner","All") );
	m_bScrolling = false;
}

void Banner::LoadMode()
{
	Load( THEME->GetPathG("Banner","Mode") );
	m_bScrolling = false;
}

void Banner::LoadFromSongGroup( CString sSongGroup )
{
	CString sGroupBannerPath = SONGMAN->GetSongGroupBannerPath( sSongGroup );
	if( sGroupBannerPath != "" )	Load( sGroupBannerPath );
	else							LoadFallback();
	m_bScrolling = false;
}

void Banner::LoadFromCourse( Course* pCourse )		// NULL means no course
{
	if( pCourse == NULL )						LoadFallback();
	else if( pCourse->m_sBannerPath != "" )		Load( pCourse->m_sBannerPath );
	else										LoadCourseFallback();

	m_bScrolling = false;
}

void Banner::LoadCardFromCharacter( Character* pCharacter )	
{
	if( pCharacter == NULL )					LoadFallback();
	else if( pCharacter->GetCardPath() != "" )	Load( pCharacter->GetCardPath() );
	else										LoadFallback();

	m_bScrolling = false;
}

void Banner::LoadIconFromCharacter( Character* pCharacter )	
{
	if( pCharacter == NULL )					LoadFallbackCharacterIcon();
	else if( pCharacter->GetIconPath() != "" )	Load( pCharacter->GetIconPath(), false );
	else										LoadFallbackCharacterIcon();

	m_bScrolling = false;
}

void Banner::LoadTABreakFromCharacter( Character* pCharacter )
{
	if( pCharacter == NULL )					Load( THEME->GetPathG("Common","fallback takingabreak") );
	else 
	{
		Load( pCharacter->GetTakingABreakPath() );
		m_bScrolling = false;
	}
}

void Banner::LoadFallback()
{
	Load( THEME->GetPathG("Common","fallback banner") );
}

void Banner::LoadCourseFallback()
{
	Load( THEME->GetPathG("Banner","course fallback") );
}

void Banner::LoadFallbackCharacterIcon()
{
	Character *pCharacter = CHARMAN->GetDefaultCharacter();
	if( pCharacter  &&  !pCharacter->GetIconPath().empty() )
		Load( pCharacter->GetIconPath(), false );
	else
		LoadFallback();
}

void Banner::LoadRoulette()
{
	Load( THEME->GetPathG("Banner","roulette") );
	m_bScrolling = (bool)SCROLL_RANDOM;
}

void Banner::LoadRandom()
{
	Load( THEME->GetPathG("Banner","random") );
	m_bScrolling = (bool)SCROLL_ROULETTE;
}

// lua start
#include "LuaBinding.h"

class LunaBanner: public Luna<Banner>
{
public:
	LunaBanner() { LUA->Register( Register ); }

	static int scaletoclipped( T* p, lua_State *L )			{ p->ScaleToClipped(FArg(1),FArg(2)); return 0; }
	static int ScaleToClipped( T* p, lua_State *L )			{ p->ScaleToClipped(FArg(1),FArg(2)); return 0; }
	static int LoadFromSong( T* p, lua_State *L )
	{ 
		if( lua_isnil(L,1) ) { p->LoadFromSong( NULL ); }
		else { Song *pS = Luna<Song>::check(L,1); p->LoadFromSong( pS ); }
		return 0;
	}
	static int LoadFromCourse( T* p, lua_State *L )
	{ 
		if( lua_isnil(L,1) ) { p->LoadFromCourse( NULL ); }
		else { Course *pC = Luna<Course>::check(L,1); p->LoadFromCourse( pC ); }
		return 0;
	}
	static int LoadIconFromCharacter( T* p, lua_State *L )
	{ 
		if( lua_isnil(L,1) ) { p->LoadIconFromCharacter( NULL ); }
		else { Character *pC = Luna<Character>::check(L,1); p->LoadIconFromCharacter( pC ); }
		return 0;
	}
	static int LoadCardFromCharacter( T* p, lua_State *L )
	{ 
		if( lua_isnil(L,1) ) { p->LoadIconFromCharacter( NULL ); }
		else { Character *pC = Luna<Character>::check(L,1); p->LoadIconFromCharacter( pC ); }
		return 0;
	}

	static void Register(lua_State *L) 
	{
		ADD_METHOD( scaletoclipped );
		ADD_METHOD( ScaleToClipped );
		ADD_METHOD( LoadFromSong );
		ADD_METHOD( LoadFromCourse );
		ADD_METHOD( LoadIconFromCharacter );
		ADD_METHOD( LoadCardFromCharacter );

		Luna<T>::Register( L );
	}
};

LUA_REGISTER_DERIVED_CLASS( Banner, Sprite )
// lua end


/*
 * (c) 2001-2004 Chris Danford
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
