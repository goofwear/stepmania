#include "global.h"
/*
-----------------------------------------------------------------------------
 Class: RageTextureManager

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/


//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "RageTextureManager.h"
#include "RageBitmapTexture.h"
#include "arch/MovieTexture/MovieTexture.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "RageException.h"
#include <time.h>

RageTextureManager*		TEXTUREMAN		= NULL;

//-----------------------------------------------------------------------------
// constructor/destructor
//-----------------------------------------------------------------------------
RageTextureManager::RageTextureManager()
{
}

RageTextureManager::~RageTextureManager()
{
	for( std::map<RageTextureID, RageTexture*>::iterator i = m_mapPathToTexture.begin();
		i != m_mapPathToTexture.end(); ++i)
	{
		RageTexture* pTexture = i->second;
		if( pTexture->m_iRefCount )
			LOG->Trace( "TEXTUREMAN LEAK: '%s', RefCount = %d.", i->first.filename.GetString(), pTexture->m_iRefCount );
		SAFE_DELETE( pTexture );
	}
}

void RageTextureManager::Update( float fDeltaTime )
{
	for( std::map<RageTextureID, RageTexture*>::iterator i = m_mapPathToTexture.begin();
		i != m_mapPathToTexture.end(); ++i)
	{
		RageTexture* pTexture = i->second;
		pTexture->Update( fDeltaTime );
	}
}

//-----------------------------------------------------------------------------
// Load/Unload textures from disk
//-----------------------------------------------------------------------------
RageTexture* RageTextureManager::LoadTexture( RageTextureID ID )
{
	/* Don't do this; just get case right to begin with. */
//	ID.filename.MakeLower();

	LOG->Trace( "RageTextureManager::LoadTexture(%s).", ID.filename.GetString() );


	// Convert the path to lowercase so that we don't load duplicates.
	// Really, this does not solve the duplicate problem.  We could have to copies
	// of the same bitmap if there are equivalent but different paths
	// (e.g. "Bitmaps\me.bmp" and "..\Rage PC Edition\Bitmaps\me.bmp" ).

	/* This will return a texture that is equivalent to ID; here, that means it
	 * has the same filename.  (see RageTextureID::operator<).  Once we have that,
	 * we need to search through textures that are equivalent, looking for one
	 * that's equal. */
	std::map<RageTextureID, RageTexture*>::iterator p = m_mapPathToTexture.find(ID);
	while(p != m_mapPathToTexture.end())
	{
		/* Found the texture.  Just increase the refcount and return it. */
		p->second->m_iRefCount++;
		return p->second;
	}

	// the texture is not already loaded.  Load it.

	CString sDir, sFName, sExt;
	splitpath( ID.filename, sDir, sFName, sExt );
	sExt.MakeLower();

	RageTexture* pTexture;
	if( sExt == ".avi" || sExt == ".mpg" || sExt == ".mpeg" )
		pTexture = MakeRageMovieTexture( ID );
	else
		pTexture = new RageBitmapTexture( ID );

	LOG->Trace( "RageTextureManager: Loaded '%s'.", ID.filename.GetString() );

	m_mapPathToTexture[ID] = pTexture;

//	LOG->Trace( "Display: %.2f KB video memory left",	DISPLAY->GetDevice()->GetAvailableTextureMem()/1000000.0f );

	return pTexture;
}

void RageTextureManager::CacheTexture( RageTextureID ID )
{
	RageTexture* pTexture = LoadTexture( ID );
	pTexture->m_bCacheThis |= true;
	UnloadTexture( pTexture );
}

void RageTextureManager::UnloadTexture( RageTexture *t )
{
	t->m_iRefCount--;
	ASSERT( t->m_iRefCount >= 0 );

	/* Always unload movies, so we don't waste time decoding.
	 *
	 * Actually, multiple refs to a movie won't work; they should play independently,
	 * but they'll actually share settings.  Not worth fixing, since we don't currently
	 * using movies for anything except BGAs (though we could).
	 */
	if( t->m_iRefCount==0 )
	{
		bool bDeleteThis = false;
		if( t->IsAMovie() )
			bDeleteThis = true;
		if( !m_bDelayedDelete || !t->m_bCacheThis )
			bDeleteThis = true;
		
		if( bDeleteThis )
			DeleteTexture( t );
	}
}

void RageTextureManager::DeleteTexture( RageTexture *t )
{
	ASSERT( t->m_iRefCount==0 );
	LOG->Trace( "RageTextureManager: deleting '%s'.", t->GetID().filename.GetString() );
	m_mapPathToTexture.erase( t->GetID() );	// remove map entry
	SAFE_DELETE( t );	// free the texture
}

void RageTextureManager::GarbageCollect( GCType type )
{
	// Search for old textures with refcount==0 to unload
	LOG->Trace("Performing texture garbage collection.");

	for( std::map<RageTextureID, RageTexture*>::iterator i = m_mapPathToTexture.begin();
		i != m_mapPathToTexture.end(); )
	{
		std::map<RageTextureID, RageTexture*>::iterator j = i;
		i++;

		CString sPath = j->first.filename;
		RageTexture* t = j->second;

		if( t->m_iRefCount==0 )
		{
			bool bDeleteThis = false;
			if( type==cached_textures && !m_bDelayedDelete )
				bDeleteThis = true;
			if( type==delayed_delete )
				bDeleteThis = true;
				
			if( bDeleteThis )
				DeleteTexture( t );
		}
	}
}

/*

  Redundant.  -Chris

void RageTextureManager::UnloadTexture( RageTextureID ID )
{
	ID.filename.MakeLower();

	if( ID.filename == "" )
	{
		//LOG->Trace( "RageTextureManager::UnloadTexture(): tried to Unload a blank texture." );
		return;
	}
	LOG->Trace( "RageTextureManager::UnloadTexture(%s).", ID.filename.GetString() );

	std::map<RageTextureID, RageTexture*>::iterator p = m_mapPathToTexture.find(ID);
	if(p == m_mapPathToTexture.end())
		RageException::Throw( "Tried to Unload texture '%s' that wasn't loaded.", ID.filename.GetString() );
	
	UnloadTexture(p->second);
	//LOG->Trace( "RageTextureManager: '%s' will not be deleted.  It still has %d references.", sTexturePath.GetString(), pTexture->m_iRefCount );
}
*/


void RageTextureManager::ReloadAll()
{
	for( std::map<RageTextureID, RageTexture*>::iterator i = m_mapPathToTexture.begin();
		i != m_mapPathToTexture.end(); ++i)
	{
		i->second->Reload();
	}
}

/* In some cases, changing the display mode will reset the rendering context,
 * releasing all textures.  We don't want to reload immediately if that happens,
 * since we might be changing texture preferences too, which also may have to reload
 * textures.  Instead, tell all textures that their texture ID is invalid, so it
 * doesn't try to free it later when we really do reload (since that ID might be
 * associated with a different texture).  Ack. */
void RageTextureManager::InvalidateTextures()
{
	std::map<RageTextureID, RageTexture*>::iterator i;
	for( i = m_mapPathToTexture.begin(); i != m_mapPathToTexture.end(); ++i)
	{
		RageTexture* pTexture = i->second;
		pTexture->Invalidate();
	}

	/* We're going to have to reload all loaded textures.  Let's get rid
	 * of all unreferenced textures, so we don't reload a ton of cached
	 * data that we're not necessarily going to use. 
	 *
	 * This must be done *after* we Invalidate above, to let the texture
	 * know its OpenGL texture number is invalid.  If we don't do that,
	 * it'll try to free it. */
	for( i = m_mapPathToTexture.begin();
		i != m_mapPathToTexture.end(); )
	{
		std::map<RageTextureID, RageTexture*>::iterator j = i;
		i++;

		CString sPath = j->first.filename;
		RageTexture* pTexture = j->second;
		if( pTexture->m_iRefCount==0 )
		{
			SAFE_DELETE( pTexture );		// free the texture
			m_mapPathToTexture.erase(j);	// and remove the key in the map
		}
	}
}

bool RageTextureManager::SetPrefs( int iTextureColorDepth, bool bDelayedDelete, int iMaxTextureResolution )
{
	bool need_reload = false;
	if( m_bDelayedDelete != bDelayedDelete ||
		m_iTextureColorDepth != iTextureColorDepth ||
		m_iMaxTextureResolution != iMaxTextureResolution )
		need_reload = true;

	m_bDelayedDelete = bDelayedDelete;
	m_iTextureColorDepth = iTextureColorDepth;
	m_iMaxTextureResolution = iMaxTextureResolution;
	
	ASSERT( m_iTextureColorDepth==16 || m_iTextureColorDepth==32 );
	return need_reload;
}
