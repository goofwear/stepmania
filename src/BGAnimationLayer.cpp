#include "global.h"
#include "BGAnimationLayer.h"
#include "GameState.h"
#include "IniFile.h"
#include "RageMath.h"
#include "RageLog.h"
#include "song.h"
#include "ActorCollision.h"
#include "Sprite.h"
#include "RageDisplay.h"
#include "ActorUtil.h"
#include "arch/ArchHooks/ArchHooks.h"
#include "LuaHelpers.h"


const float PARTICLE_SPEED = 300;

const float SPIRAL_MAX_ZOOM = 2;
const float SPIRAL_MIN_ZOOM = 0.3f;

#define MAX_TILES_WIDE int(SCREEN_WIDTH/32+2)
#define MAX_TILES_HIGH int(SCREEN_HEIGHT/32+2)
#define MAX_SPRITES (MAX_TILES_WIDE*MAX_TILES_HIGH)


BGAnimationLayer::BGAnimationLayer( bool bGeneric )
{
	/*
	 * If Generic is false, this is a layer in a real BGA--one that was loaded
	 * by simply constructing a BGAnimation.  In this mode, loaded images are given
	 * a default position of SCREEN_CENTER_X, SCREEN_CENTER_Y.
	 */
	m_bGeneric = bGeneric;

	Init();
}

BGAnimationLayer::~BGAnimationLayer()
{
	Unload();
}

void BGAnimationLayer::Unload()
{
	ActorFrame::DeleteAllChildren();
}

void BGAnimationLayer::Init()
{
	Unload();

	m_fUpdateRate = 1;
	m_fFOV = -1;	// no change
	m_bLighting = false;

	m_vParticleVelocity.clear();

	m_Type = TYPE_SPRITE;

	m_fTexCoordVelocityX = 0;
	m_fTexCoordVelocityY = 0;
	m_bParticlesBounce = false;
	m_iNumTilesWide = -1;
	m_iNumTilesHigh = -1;
	m_fTilesStartX = 0;
	m_fTilesStartY = 0;
	m_fTilesSpacingX = -1;
	m_fTilesSpacingY = -1;
	m_fTileVelocityX = 0;
	m_fTileVelocityY = 0;
}

void BGAnimationLayer::LoadFromAniLayerFile( const CString& sPath )
{
	/* Generic BGAs are new.  Animation directories with no INI are old and obsolete. 
	 * Don't combine them. */
	ASSERT( !m_bGeneric );
	Init();
	CString lcPath = sPath;
	lcPath.MakeLower();

	if( lcPath.Find("usesongbg") != -1 )
	{
		const Song* pSong = GAMESTATE->m_pCurSong;
		CString sSongBGPath;
		if( pSong && pSong->HasBackground() )
			sSongBGPath = pSong->GetBackgroundPath();
		else
			sSongBGPath = THEME->GetPathToG("Common fallback background");

		Sprite* pSprite = new Sprite;
		pSprite->LoadBG( sSongBGPath );
		pSprite->StretchTo( FullScreenRectF );
		this->AddChild( pSprite );

		return;		// this will ignore other effects in the file name
	}

	const CString EFFECT_STRING[NUM_EFFECTS] = {
		"center",
		"stretchstill",
		"stretchscrollleft",
		"stretchscrollright",
		"stretchscrollup",
		"stretchscrolldown",
		"stretchwater",
		"stretchbubble",
		"stretchtwist",
		"stretchspin",
		"particlesspiralout",
		"particlesspiralin",
		"particlesfloatup",
		"particlesfloatdown",
		"particlesfloatleft",
		"particlesfloatright",
		"particlesbounce",
		"tilestill",
		"tilescrollleft",
		"tilescrollright",
		"tilescrollup",
		"tilescrolldown",
		"tileflipx",
		"tileflipy",
		"tilepulse",
	};

	Effect effect = EFFECT_CENTER;

	for( int i=0; i<NUM_EFFECTS; i++ )
		if( lcPath.Find(EFFECT_STRING[i]) != -1 )
			effect = (Effect)i;

	switch( effect )
	{
	case EFFECT_CENTER:
		{
			m_Type = TYPE_SPRITE;
			Sprite* pSprite = new Sprite;
			this->AddChild( pSprite );
			pSprite->Load( sPath );
			pSprite->SetXY( SCREEN_CENTER_X, SCREEN_CENTER_Y );
		}
		break;
	case EFFECT_STRETCH_STILL:
	case EFFECT_STRETCH_SCROLL_LEFT:
	case EFFECT_STRETCH_SCROLL_RIGHT:
	case EFFECT_STRETCH_SCROLL_UP:
	case EFFECT_STRETCH_SCROLL_DOWN:
	case EFFECT_STRETCH_WATER:
	case EFFECT_STRETCH_BUBBLE:
	case EFFECT_STRETCH_TWIST:
		{
			m_Type = TYPE_SPRITE;
			Sprite* pSprite = new Sprite;
			this->AddChild( pSprite );
			RageTextureID ID(sPath);
			ID.bStretch = true;
			pSprite->LoadBG( ID );
			pSprite->StretchTo( FullScreenRectF );
			pSprite->SetCustomTextureRect( RectF(0,0,1,1) );

			switch( effect )
			{
			case EFFECT_STRETCH_SCROLL_LEFT:	m_fTexCoordVelocityX = +0.5f; m_fTexCoordVelocityY = 0;	break;
			case EFFECT_STRETCH_SCROLL_RIGHT:	m_fTexCoordVelocityX = -0.5f; m_fTexCoordVelocityY = 0;	break;
			case EFFECT_STRETCH_SCROLL_UP:		m_fTexCoordVelocityX = 0; m_fTexCoordVelocityY = +0.5f;	break;
			case EFFECT_STRETCH_SCROLL_DOWN:	m_fTexCoordVelocityX = 0; m_fTexCoordVelocityY = -0.5f;	break;
				break;
			}
		}
		break;
	case EFFECT_STRETCH_SPIN:
		{
			m_Type = TYPE_SPRITE;
			Sprite* pSprite = new Sprite;
			this->AddChild( pSprite );
			pSprite->LoadBG( sPath );
			const RectF StretchedFullScreenRectF(
				FullScreenRectF.left-200,
				FullScreenRectF.top-200,
				FullScreenRectF.right+200,
				FullScreenRectF.bottom+200 );

			pSprite->ScaleToCover( StretchedFullScreenRectF );
			pSprite->SetEffectSpin( RageVector3(0,0,60) );
		}
		break;
	case EFFECT_PARTICLES_SPIRAL_OUT:
	case EFFECT_PARTICLES_SPIRAL_IN:
	case EFFECT_PARTICLES_FLOAT_UP:
	case EFFECT_PARTICLES_FLOAT_DOWN:
	case EFFECT_PARTICLES_FLOAT_LEFT:
	case EFFECT_PARTICLES_FLOAT_RIGHT:
	case EFFECT_PARTICLES_BOUNCE:
		{
			m_Type = TYPE_PARTICLES;
			Sprite s;
			s.Load( sPath );
			int iSpriteArea = int( s.GetUnzoomedWidth()*s.GetUnzoomedHeight() );
			const int iMaxArea = int(SCREEN_WIDTH*SCREEN_HEIGHT);
			int iNumParticles = iMaxArea / iSpriteArea;
			iNumParticles = min( iNumParticles, MAX_SPRITES );

			for( int i=0; i<iNumParticles; i++ )
			{
				Sprite* pSprite = new Sprite;
				this->AddChild( pSprite );
				pSprite->Load( sPath );
				pSprite->SetZoom( 0.7f + 0.6f*i/(float)iNumParticles );
				pSprite->SetX( randomf( GetGuardRailLeft(pSprite), GetGuardRailRight(pSprite) ) );
				pSprite->SetY( randomf( GetGuardRailTop(pSprite), GetGuardRailBottom(pSprite) ) );

				switch( effect )
				{
				case EFFECT_PARTICLES_FLOAT_UP:
				case EFFECT_PARTICLES_SPIRAL_OUT:
					m_vParticleVelocity.push_back( RageVector3( 0, -PARTICLE_SPEED*pSprite->GetZoom(), 0 ) );
					break;
				case EFFECT_PARTICLES_FLOAT_DOWN:
				case EFFECT_PARTICLES_SPIRAL_IN:
					m_vParticleVelocity.push_back( RageVector3( 0, PARTICLE_SPEED*pSprite->GetZoom(), 0 ) );
					break;
				case EFFECT_PARTICLES_FLOAT_LEFT:
					m_vParticleVelocity.push_back( RageVector3( -PARTICLE_SPEED*pSprite->GetZoom(), 0, 0 ) );
					break;
				case EFFECT_PARTICLES_FLOAT_RIGHT:
					m_vParticleVelocity.push_back( RageVector3( +PARTICLE_SPEED*pSprite->GetZoom(), 0, 0 ) );
					break;
				case EFFECT_PARTICLES_BOUNCE:
					m_bParticlesBounce = true;
					pSprite->SetZoom( 1 );
					m_vParticleVelocity.push_back( RageVector3( randomf(), randomf(), 0 ) );
					RageVec3Normalize( &m_vParticleVelocity[i], &m_vParticleVelocity[i] );
					break;
				default:
					ASSERT(0);
				}
			}
		}
		break;
	case EFFECT_TILE_STILL:
	case EFFECT_TILE_SCROLL_LEFT:
	case EFFECT_TILE_SCROLL_RIGHT:
	case EFFECT_TILE_SCROLL_UP:
	case EFFECT_TILE_SCROLL_DOWN:
	case EFFECT_TILE_FLIP_X:
	case EFFECT_TILE_FLIP_Y:
	case EFFECT_TILE_PULSE:
		{
			m_Type = TYPE_TILES;
			RageTextureID ID(sPath);
			ID.bStretch = true;
			Sprite s;
			s.Load( ID );
			m_iNumTilesWide = 2+int(SCREEN_WIDTH /s.GetUnzoomedWidth());
			m_iNumTilesWide = min( m_iNumTilesWide, MAX_TILES_WIDE );
			m_iNumTilesHigh = 2+int(SCREEN_HEIGHT/s.GetUnzoomedHeight());
			m_iNumTilesHigh = min( m_iNumTilesHigh, MAX_TILES_HIGH );
			m_fTilesStartX = s.GetUnzoomedWidth() / 2;
			m_fTilesStartY = s.GetUnzoomedHeight() / 2;
			m_fTilesSpacingX = s.GetUnzoomedWidth();
			m_fTilesSpacingY = s.GetUnzoomedHeight();
			for( int x=0; x<m_iNumTilesWide; x++ )
			{
				for( int y=0; y<m_iNumTilesHigh; y++ )
				{
					Sprite* pSprite = new Sprite;
					this->AddChild( pSprite );
					pSprite->Load( ID );
					pSprite->SetTextureWrapping( true );	// gets rid of some "cracks"

					switch( effect )
					{
					case EFFECT_TILE_STILL:
						break;
					case EFFECT_TILE_SCROLL_LEFT:
						m_fTileVelocityX = -PARTICLE_SPEED;
						break;
					case EFFECT_TILE_SCROLL_RIGHT:
						m_fTileVelocityX = +PARTICLE_SPEED;
						break;
					case EFFECT_TILE_SCROLL_UP:
						m_fTileVelocityY = -PARTICLE_SPEED;
						break;
					case EFFECT_TILE_SCROLL_DOWN:
						m_fTileVelocityY = +PARTICLE_SPEED;
						break;
					case EFFECT_TILE_FLIP_X:
						pSprite->SetEffectSpin( RageVector3(2,0,0) );
						break;
					case EFFECT_TILE_FLIP_Y:
						pSprite->SetEffectSpin( RageVector3(0,2,0) );
						break;
					case EFFECT_TILE_PULSE:
						pSprite->SetEffectPulse( 1, 0.3f, 1.f );
						break;
					default:
						ASSERT(0);
					}
				}
			}
		}
		break;
	default:
		ASSERT(0);
	}


	CString sHint = sPath;
	sHint.MakeLower();

	if( sHint.Find("cyclecolor") != -1 )
		for( unsigned i=0; i<m_SubActors.size(); i++ )
			m_SubActors[i]->SetEffectRainbow( 5 );

	if( sHint.Find("cyclealpha") != -1 )
		for( unsigned i=0; i<m_SubActors.size(); i++ )
			m_SubActors[i]->SetEffectDiffuseShift( 2, RageColor(1,1,1,1), RageColor(1,1,1,0) );

	if( sHint.Find("startonrandomframe") != -1 )
		for( unsigned i=0; i<m_SubActors.size(); i++ )
			m_SubActors[i]->SetState( rand()%m_SubActors[i]->GetNumStates() );

	if( sHint.Find("dontanimate") != -1 )
		for( unsigned i=0; i<m_SubActors.size(); i++ )
			m_SubActors[i]->StopAnimating();

	if( sHint.Find("add") != -1 )
		for( unsigned i=0; i<m_SubActors.size(); i++ )
			m_SubActors[i]->SetBlendMode( BLEND_ADD );
}

void BGAnimationLayer::LoadFromNode( const CString& sAniDir_, const XNode& layer )
{
	CString sAniDir = sAniDir_;

	Init();
	if( sAniDir.Right(1) != "/" )
		sAniDir += "/";

	DEBUG_ASSERT( IsADirectory(sAniDir) );

	CHECKPOINT_M( ssprintf( "BGAnimationLayer::LoadFromIni \"%s\" %s",
		sAniDir.c_str(), m_bGeneric? "(generic) ":"" ) );

	{
		CString sPlayer;
		if( layer.GetAttrValue("Player", sPlayer) )
			ASSERT_M( 0, "The BGAnimation parameter 'Player' is deprecated.  Please use 'Condition=IsPlayerEnabled(p)'." );
	}

	{
		CString expr;
		if( layer.GetAttrValue("Cond",expr) || layer.GetAttrValue("Condition",expr) )
		{
			if( !Lua::RunExpressionB( expr ) )
				return;
		}
	}

	bool bStretch = false;
	{
		CString type = "sprite";
		layer.GetAttrValue( "Type", type );
		type.MakeLower();

		/* The preferred way of stretching a sprite to fit the screen is "Type=sprite"
		 * and "stretch=1".  "type=1" is for backwards-compatibility. */
		layer.GetAttrValue( "Stretch", bStretch );

		// Check for string match first, then do integer match.
		// "if(atoi(type)==0)" was matching against all string matches.
		// -Chris
		if( stricmp(type,"sprite")==0 )
		{
			m_Type = TYPE_SPRITE;
		}
		else if( stricmp(type,"particles")==0 )
		{
			m_Type = TYPE_PARTICLES;
		}
		else if( stricmp(type,"tiles")==0 )
		{
			m_Type = TYPE_TILES;
		}
		else if( atoi(type) == 1 )
		{
			m_Type = TYPE_SPRITE; 
			bStretch = true; 
		}
		else if( atoi(type) == 2 )
		{
			m_Type = TYPE_PARTICLES; 
		}
		else if( atoi(type) == 3 )
		{
			m_Type = TYPE_TILES; 
		}
		else
		{
			m_Type = TYPE_SPRITE;
		}
	}

	layer.GetAttrValue( "FOV", m_fFOV );
	layer.GetAttrValue( "Lighting", m_bLighting );
	layer.GetAttrValue( "TexCoordVelocityX", m_fTexCoordVelocityX );
	layer.GetAttrValue( "TexCoordVelocityY", m_fTexCoordVelocityY );
	layer.GetAttrValue( "DrawCond", m_sDrawCond );

	// compat:
	layer.GetAttrValue( "StretchTexCoordVelocityX", m_fTexCoordVelocityX );
	layer.GetAttrValue( "StretchTexCoordVelocityY", m_fTexCoordVelocityY );

	// particle and tile stuff
	float fZoomMin = 1;
	float fZoomMax = 1;
	layer.GetAttrValue( "ZoomMin", fZoomMin );
	layer.GetAttrValue( "ZoomMax", fZoomMax );

	float fVelocityXMin = 10, fVelocityXMax = 10;
	float fVelocityYMin = 0, fVelocityYMax = 0;
	float fVelocityZMin = 0, fVelocityZMax = 0;
	float fOverrideSpeed = 0;		// 0 means don't override speed
	layer.GetAttrValue( "VelocityXMin", fVelocityXMin );
	layer.GetAttrValue( "VelocityXMax", fVelocityXMax );
	layer.GetAttrValue( "VelocityYMin", fVelocityYMin );
	layer.GetAttrValue( "VelocityYMax", fVelocityYMax );
	layer.GetAttrValue( "VelocityZMin", fVelocityZMin );
	layer.GetAttrValue( "VelocityZMax", fVelocityZMax );
	layer.GetAttrValue( "OverrideSpeed", fOverrideSpeed );

	int iNumParticles = 10;
	layer.GetAttrValue( "NumParticles", iNumParticles );

	layer.GetAttrValue( "ParticlesBounce", m_bParticlesBounce );
	layer.GetAttrValue( "TilesStartX", m_fTilesStartX );
	layer.GetAttrValue( "TilesStartY", m_fTilesStartY );
	layer.GetAttrValue( "TilesSpacingX", m_fTilesSpacingX );
	layer.GetAttrValue( "TilesSpacingY", m_fTilesSpacingY );
	layer.GetAttrValue( "TileVelocityX", m_fTileVelocityX );
	layer.GetAttrValue( "TileVelocityY", m_fTileVelocityY );


	bool NeedTextureStretch = false;
	if( m_fTexCoordVelocityX != 0 ||
		m_fTexCoordVelocityY != 0 )
		NeedTextureStretch = true;

	switch( m_Type )
	{
	case TYPE_SPRITE:
		{
			Actor* pActor = LoadFromActorFile( sAniDir, layer );
			this->AddChild( pActor );
			if( bStretch )
				pActor->StretchTo( FullScreenRectF );

			/* Annoying: old BGAnimations (those in theme BGAnimations/ and, more importantly,
			 * those included with songs) expect a 640x480 coordinate system with a default
			 * of 320x240.  We can't just move the whole layer to the center (that'll move the
			 * whole coordinate system, not just the default), we have to move the actor. */
			if( !m_bGeneric )
				pActor->SetXY( SCREEN_CENTER_X, SCREEN_CENTER_Y );
		}
		break;
	case TYPE_PARTICLES:
		{
			CString sFile;
			layer.GetAttrValue( "File", sFile );
			FixSlashesInPlace( sFile );
			
			CString sPath = sAniDir+sFile;
			CollapsePath( sPath );


			ASSERT( !m_bGeneric );
			for( int i=0; i<iNumParticles; i++ )
			{
				Actor* pActor = MakeActor( sPath );
				this->AddChild( pActor );
				pActor->SetXY( randomf(float(FullScreenRectF.left),float(FullScreenRectF.right)),
							   randomf(float(FullScreenRectF.top),float(FullScreenRectF.bottom)) );
				pActor->SetZoom( randomf(fZoomMin,fZoomMax) );
				m_vParticleVelocity.push_back( RageVector3( 
					randomf(fVelocityXMin,fVelocityXMax),
					randomf(fVelocityYMin,fVelocityYMax),
					randomf(fVelocityZMin,fVelocityZMax) ) );
				if( fOverrideSpeed != 0 )
				{
					RageVec3Normalize( &m_vParticleVelocity[i], &m_vParticleVelocity[i] );
					m_vParticleVelocity[i] *= fOverrideSpeed;
				}
			}
		}
		break;
	case TYPE_TILES:
		{
			CString sFile;
			layer.GetAttrValue( "File", sFile );
			FixSlashesInPlace( sFile );
			
			CString sPath = sAniDir+sFile;
			CollapsePath( sPath );

			ASSERT( !m_bGeneric );
			Sprite s;
			RageTextureID ID(sPath);
			ID.bStretch = true;
			s.Load( ID );
			if( m_fTilesSpacingX == -1 )
				m_fTilesSpacingX = s.GetUnzoomedWidth();
			if( m_fTilesSpacingY == -1 )
				m_fTilesSpacingY = s.GetUnzoomedHeight();
			m_iNumTilesWide = 2+(int)(SCREEN_WIDTH /m_fTilesSpacingX);
			m_iNumTilesHigh = 2+(int)(SCREEN_HEIGHT/m_fTilesSpacingY);
			unsigned NumSprites = m_iNumTilesWide * m_iNumTilesHigh;
			for( unsigned i=0; i<NumSprites; i++ )
			{
				Sprite* pSprite = new Sprite;
				this->AddChild( pSprite );
				pSprite->Load( ID );
				pSprite->SetTextureWrapping( true );		// gets rid of some "cracks"
				pSprite->SetZoom( randomf(fZoomMin,fZoomMax) );
			}
		}
		break;
	default:
		ASSERT(0);
	}

	bool bStartOnRandomFrame = false;
	layer.GetAttrValue( "StartOnRandomFrame", bStartOnRandomFrame );
	if( bStartOnRandomFrame )
	{
		for( unsigned i=0; i<m_SubActors.size(); i++ )
			m_SubActors[i]->SetState( rand()%m_SubActors[i]->GetNumStates() );
	}
}

void BGAnimationLayer::Update( float fDeltaTime )
{
	if( m_fHibernateSecondsLeft > 0 )
		return;

	fDeltaTime *= m_fUpdateRate;

	for( unsigned i=0; i<m_SubActors.size(); i++ )
		m_SubActors[i]->Update( fDeltaTime );


	switch( m_Type )
	{
	case TYPE_SPRITE:
		if( m_fTexCoordVelocityX || m_fTexCoordVelocityY )
		{
			for( unsigned i=0; i<m_SubActors.size(); i++ )
			{
				/* XXX: there's no longer any guarantee that this is a Sprite */
				Sprite *pSprite = (Sprite*)m_SubActors[i];
				pSprite->StretchTexCoords(
					fDeltaTime*m_fTexCoordVelocityX,
					fDeltaTime*m_fTexCoordVelocityY );
			}
		}
		break;
	case TYPE_PARTICLES:
		for( unsigned i=0; i<m_SubActors.size(); i++ )
		{
			Actor* pActor = m_SubActors[i];
			RageVector3 &vel = m_vParticleVelocity[i];

			m_SubActors[i]->SetX( pActor->GetX() + fDeltaTime*vel.x );
			m_SubActors[i]->SetY( pActor->GetY() + fDeltaTime*vel.y );
			pActor->SetZ( pActor->GetZ() + fDeltaTime*vel.z  );
			if( m_bParticlesBounce )
			{
				if( HitGuardRailLeft(pActor) )	
				{
					vel.x *= -1;
					pActor->SetX( GetGuardRailLeft(pActor) );
				}
				if( HitGuardRailRight(pActor) )	
				{
					vel.x *= -1;
					pActor->SetX( GetGuardRailRight(pActor) );
				}
				if( HitGuardRailTop(pActor) )	
				{
					vel.y *= -1;
					pActor->SetY( GetGuardRailTop(pActor) );
				}
				if( HitGuardRailBottom(pActor) )	
				{
					vel.y *= -1;
					pActor->SetY( GetGuardRailBottom(pActor) );
				}
			}
			else // !m_bParticlesBounce 
			{
				if( vel.x<0  &&  IsOffScreenLeft(pActor) )
					pActor->SetX( GetOffScreenRight(pActor) );
				if( vel.x>0  &&  IsOffScreenRight(pActor) )
					pActor->SetX( GetOffScreenLeft(pActor) );
				if( vel.y<0  &&  IsOffScreenTop(pActor) )
					pActor->SetY( GetOffScreenBottom(pActor) );
				if( vel.y>0  &&  IsOffScreenBottom(pActor) )
					pActor->SetY( GetOffScreenTop(pActor) );
			}
		}
		break;
	case TYPE_TILES:
		{
			float fSecs = RageTimer::GetTimeSinceStart();
			float fTotalWidth = m_iNumTilesWide * m_fTilesSpacingX;
			float fTotalHeight = m_iNumTilesHigh * m_fTilesSpacingY;
			
			ASSERT( int(m_SubActors.size()) == m_iNumTilesWide * m_iNumTilesHigh );

			for( int x=0; x<m_iNumTilesWide; x++ )
			{
				for( int y=0; y<m_iNumTilesHigh; y++ )
				{
					int i = y*m_iNumTilesWide + x;

					float fX = m_fTilesStartX + m_fTilesSpacingX * x + fSecs * m_fTileVelocityX;
					float fY = m_fTilesStartY + m_fTilesSpacingY * y + fSecs * m_fTileVelocityY;

					fX += m_fTilesSpacingX/2;
					fY += m_fTilesSpacingY/2;

					fX = fmodf( fX, fTotalWidth );
					fY = fmodf( fY, fTotalHeight );

					if( fX < 0 )	fX += fTotalWidth;
					if( fY < 0 )	fY += fTotalHeight;

					fX -= m_fTilesSpacingX/2;
					fY -= m_fTilesSpacingY/2;
					
					m_SubActors[i]->SetX( fX );
					m_SubActors[i]->SetY( fY );
				}
			}
		}
		break;
	default:
		ASSERT(0);
	}
}

bool BGAnimationLayer::EarlyAbortDraw()
{
	if( m_sDrawCond.empty() )
		return false;

	if( !Lua::RunExpressionB( m_sDrawCond ) )
		return true;

	return false;
}

void BGAnimationLayer::DrawPrimitives()
{
	if( m_fFOV != -1 )
	{
		DISPLAY->CameraPushMatrix();
		DISPLAY->LoadMenuPerspective( m_fFOV, SCREEN_CENTER_X, SCREEN_CENTER_Y );
	}

	if( m_bLighting )
	{
		DISPLAY->SetLighting( true );
		DISPLAY->SetLightDirectional( 
			0, 
			RageColor(1,1,1,1), 
			RageColor(1,1,1,1),
			RageColor(1,1,1,1),
			RageVector3(0,0,1) );
	}

	ActorFrame::DrawPrimitives();
	
	if( m_fFOV != -1 )
	{
		DISPLAY->CameraPopMatrix();
	}

	if( m_bLighting )
	{
		DISPLAY->SetLightOff( 0 );
		DISPLAY->SetLighting( false );
	}
}

void BGAnimationLayer::GainFocus( float fRate, bool bRewindMovie, bool bLoop )
{
	m_fUpdateRate = fRate;

	ActorFrame::GainFocus( fRate, bRewindMovie, bLoop );
}


/*
 * (c) 2001-2004 Ben Nordstrom, Chris Danford, Glenn Maynard
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
