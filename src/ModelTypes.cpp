#include "global.h"
#include "ModelTypes.h"
#include "IniFile.h"
#include "RageUtil.h"
#include "RageTexture.h"
#include "RageTextureManager.h"
#include "RageLog.h"
#include "RageDisplay.h"

AnimatedTexture::AnimatedTexture()
{
	iCurState = 0;
	fSecsIntoFrame = 0;
}

AnimatedTexture::~AnimatedTexture()
{
	Unload();
}

void AnimatedTexture::Load( CString sTexOrIniPath )
{
	ASSERT( vFrames.empty() );	// don't load more than once

	if( GetExtension(sTexOrIniPath).CompareNoCase("ini")==0 )
	{
		IniFile ini;
		if( !ini.ReadFile( sTexOrIniPath ) )
			RageException::Throw( "Error reading %s: %s", sTexOrIniPath.c_str(), ini.GetError().c_str() );

		if( !ini.GetKey("AnimatedTexture") )
			RageException::Throw( "The animated texture file '%s' doesn't contain a section called 'AnimatedTexture'.", sTexOrIniPath.c_str() );
		for( int i=0; i<1000; i++ )
		{
			CString sFileKey = ssprintf( "Frame%04d", i );
			CString sDelayKey = ssprintf( "Delay%04d", i );

			CString sFileName;
			float fDelay = 0;
			if( ini.GetValue( "AnimatedTexture", sFileKey, sFileName ) &&
				ini.GetValue( "AnimatedTexture", sDelayKey, fDelay ) ) 
			{
				RageTextureID ID;
				ID.filename = Dirname(sTexOrIniPath) + sFileName;
				ID.bStretch = true;
				ID.bHotPinkColorKey = true;
				ID.bMipMaps = true;	// use mipmaps in Models
				AnimatedTextureState state = { 
					TEXTUREMAN->LoadTexture( ID ),
					fDelay
				};
				vFrames.push_back( state );
			}
			else
				break;
		}
	}
	else
	{
		RageTextureID ID;
		ID.filename = sTexOrIniPath;
		ID.bHotPinkColorKey = true;
		ID.bStretch = true;
		ID.bMipMaps = true;	// use mipmaps in Models
		AnimatedTextureState state = { 
			TEXTUREMAN->LoadTexture( ID ),
			10
		};
		vFrames.push_back( state );
	}
}


void AnimatedTexture::Update( float fDelta )
{
	if( vFrames.empty() )
		return;
	ASSERT( iCurState < (int)vFrames.size() );
	fSecsIntoFrame += fDelta;
	if( fSecsIntoFrame > vFrames[iCurState].fDelaySecs )
	{
		fSecsIntoFrame -= vFrames[iCurState].fDelaySecs;
		iCurState = (iCurState+1) % vFrames.size();
	}
}

RageTexture* AnimatedTexture::GetCurrentTexture()
{
	if( vFrames.empty() )
		return NULL;
	ASSERT( iCurState < (int)vFrames.size() );
	return vFrames[iCurState].pTexture;
}

void AnimatedTexture::SetState( int iState )
{
	CLAMP( iState, 0, GetNumStates()-1 );
	iCurState = iState;
}

int AnimatedTexture::GetNumStates()
{
	return vFrames.size();
}

void AnimatedTexture::Unload()
{
	for(unsigned i = 0; i < vFrames.size(); ++i)
		TEXTUREMAN->UnloadTexture(vFrames[i].pTexture);
	vFrames.clear();
	iCurState = 0;
	fSecsIntoFrame = 0;
}


msMesh::msMesh()
{
	ZERO( szName );
}

msMesh::~msMesh()
{
}

/*
 * (c) 2003-2004 Chris Danford
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
