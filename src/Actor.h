#ifndef ACTOR_H
#define ACTOR_H
/*
-----------------------------------------------------------------------------
 Class: Actor

 Desc: Base class for all objects that appear on the screen.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "RageTypes.h"

class Actor
{
public:
	Actor();
	virtual ~Actor() { }
	void Reset();

	enum TweenType { 
		TWEEN_LINEAR, 
		TWEEN_ACCELERATE, 
		TWEEN_DECELERATE, 
		TWEEN_BOUNCE_BEGIN, 
		TWEEN_BOUNCE_END,
		TWEEN_SPRING,
	};
	enum Effect { no_effect,
				diffuse_blink,	diffuse_shift,
				glow_blink,		glow_shift,
				rainbow,
				wag,	bounce,		bob,	pulse,
				spin,	vibrate
				};

	struct TweenState
	{
		// start and end position for tweening
		RageVector3 pos;
		RageVector3 rotation;
		RageVector4 quat;
		RageVector3 scale;
		RectF		crop;	// 0 = no cropping, 1 = fully cropped
		RectF		fade;	// 0 = no fade
		RageColor	fadecolor;
		RageColor   diffuse[4];
		RageColor   glow;
		GlowMode	glowmode;

		void Init();
		static void MakeWeightedAverage( TweenState& average_out, const TweenState& ts1, const TweenState& ts2, float fPercentBetween );
	};

	enum EffectClock { CLOCK_TIMER, CLOCK_BGM };

	// let subclasses override
	virtual void Draw();		// calls, BeginDraw, DrawPrimitives, EndDraw
	virtual void BeginDraw();	// pushes transform onto world matrix stack
	virtual void SetRenderStates();	// Actor should call at beginning of their DrawPrimitives()
	virtual void DrawPrimitives() {};	// Derivitives should override
	virtual void EndDraw();		// pops transform from world matrix stack
	bool IsFirstUpdate();
	virtual void Update( float fDeltaTime );
	virtual void UpdateTweening( float fDeltaTime );
	virtual void CopyTweening( const Actor &from );


	CString m_sName, m_sID;

	CString GetName() const		{ return m_sName; };
	CString GetID() const		{ return m_sID; };
	/* m_sName is the name actors use to look up internal metrics for themselves, and for
	 * filenames.  m_sID is the name parents use to look up their own metrics for an actor
	 * (usually via ActorUtil).  (This is experimental; see DifficultyMeter.cpp for more
	 * information.) */
	void SetName( const CString &sName, const CString &sID = "" ) { m_sName = sName; m_sID = (sID.size()? sID:sName);  };



	/* Do subclasses really need to override tweening?  Tween data should
	 * probably be private ... - glenn */
	/* Things like a "FocusingSprite" might, but probably not.  -Chris */

	/* Return the current coordinates, not the destination coordinates;
	 * that's what the old behavior was, at least, and it's what ScreenMusicScroll
	 * expects.  I could see uses for knowing the destination coords, though,
	 * especially now that setting parameters when tweening and when not tweening
	 * is somewhat abstracted.  Hmmm. -glenn */
	/* Things like the cursor on ScreenSelectDifficutly need to know the dest coordinates.
	 * -Chris */
	virtual float GetX() const				{ return m_current.pos.x; };
	virtual float GetY() const				{ return m_current.pos.y; };
	virtual float GetZ() const				{ return m_current.pos.z; };
	virtual float GetDestX()				{ return DestTweenState().pos.x; };
	virtual float GetDestY()				{ return DestTweenState().pos.y; };
	virtual float GetDestZ()				{ return DestTweenState().pos.z; };
	virtual void  SetX( float x )			{ DestTweenState().pos.x = x; };
	virtual void  SetY( float y )			{ DestTweenState().pos.y = y; };
	virtual void  SetZ( float z )			{ DestTweenState().pos.z = z; };
	virtual void  SetXY( float x, float y )	{ DestTweenState().pos.x = x; DestTweenState().pos.y = y; };

	// height and width vary depending on zoom
	virtual float GetUnzoomedWidth()		{ return m_size.x; }
	virtual float GetUnzoomedHeight()		{ return m_size.y; }
	virtual float GetZoomedWidth()			{ return m_size.x * DestTweenState().scale.x; }
	virtual float GetZoomedHeight()			{ return m_size.y * DestTweenState().scale.y; }
	virtual void  SetWidth( float width )	{ m_size.x = width; }
	virtual void  SetHeight( float height )	{ m_size.y = height; }

	void  SetBaseZoomX( float zoom )	{ m_baseScale.x = zoom;	}
	void  SetBaseZoomY( float zoom )	{ m_baseScale.y = zoom; }
	void  SetBaseZoomZ( float zoom )	{ m_baseScale.z = zoom; }
	virtual void  SetBaseRotationX( float rot )	{ m_baseRotation.x = rot; }
	virtual void  SetBaseRotationY( float rot )	{ m_baseRotation.y = rot; }
	virtual void  SetBaseRotationZ( float rot )	{ m_baseRotation.z = rot; }

	virtual float GetZoom()					{ return DestTweenState().scale.x; }	// not accurate in some cases
	virtual float GetZoomX()				{ return DestTweenState().scale.x; }
	virtual float GetZoomY()				{ return DestTweenState().scale.y; }
	virtual float GetZoomZ()				{ return DestTweenState().scale.z; }
	virtual void  SetZoom( float zoom )		{ DestTweenState().scale.x = zoom;	DestTweenState().scale.y = zoom; }
	virtual void  SetZoomX( float zoom )	{ DestTweenState().scale.x = zoom;	}
	virtual void  SetZoomY( float zoom )	{ DestTweenState().scale.y = zoom; }
	virtual void  SetZoomZ( float zoom )	{ DestTweenState().scale.z = zoom; }
	virtual void  ZoomToWidth( float zoom )	{ SetZoomX( zoom / GetUnzoomedWidth() ); }
	virtual void  ZoomToHeight( float zoom ){ SetZoomY( zoom / GetUnzoomedHeight() ); }

	virtual float GetRotationX()			{ return DestTweenState().rotation.x; }
	virtual float GetRotationY()			{ return DestTweenState().rotation.y; }
	virtual float GetRotationZ()			{ return DestTweenState().rotation.z; }
	virtual void  SetRotationX( float rot )	{ DestTweenState().rotation.x = rot; }
	virtual void  SetRotationY( float rot )	{ DestTweenState().rotation.y = rot; }
	virtual void  SetRotationZ( float rot )	{ DestTweenState().rotation.z = rot; }
	virtual void  AddRotationH( float rot );
	virtual void  AddRotationP( float rot );
	virtual void  AddRotationR( float rot );

	virtual float GetCropLeft()					{ return DestTweenState().crop.left; }
	virtual float GetCropTop()					{ return DestTweenState().crop.top;	}
	virtual float GetCropRight()				{ return DestTweenState().crop.right;}
	virtual float GetCropBottom()				{ return DestTweenState().crop.bottom;}
	virtual void  SetCropLeft( float percent )	{ DestTweenState().crop.left = percent; }
	virtual void  SetCropTop( float percent )	{ DestTweenState().crop.top = percent;	}
	virtual void  SetCropRight( float percent )	{ DestTweenState().crop.right = percent;}
	virtual void  SetCropBottom( float percent ){ DestTweenState().crop.bottom = percent;}

	virtual void  SetFadeLeft( float percent )	{ DestTweenState().fade.left = percent; }
	virtual void  SetFadeTop( float percent )	{ DestTweenState().fade.top = percent;	}
	virtual void  SetFadeRight( float percent )	{ DestTweenState().fade.right = percent;}
	virtual void  SetFadeBottom( float percent ){ DestTweenState().fade.bottom = percent;}
	virtual void  SetFadeDiffuseColor( const RageColor &c ) { DestTweenState().fadecolor = c; }

	virtual void SetGlobalDiffuseColor( RageColor c );
	virtual void SetGlobalX( float x );

	virtual void SetDiffuse( RageColor c ) { for(int i=0; i<4; i++) DestTweenState().diffuse[i] = c; };
	virtual void SetDiffuseAlpha( float f ) { for(int i = 0; i < 4; ++i) { RageColor c = GetDiffuses( i ); c.a = f; SetDiffuses( i, c ); } }
	virtual void SetDiffuseColor( RageColor c );
	virtual void SetDiffuses( int i, RageColor c )		{ DestTweenState().diffuse[i] = c; };
	virtual void SetDiffuseUpperLeft( RageColor c )		{ DestTweenState().diffuse[0] = c; };
	virtual void SetDiffuseUpperRight( RageColor c )	{ DestTweenState().diffuse[1] = c; };
	virtual void SetDiffuseLowerLeft( RageColor c )		{ DestTweenState().diffuse[2] = c; };
	virtual void SetDiffuseLowerRight( RageColor c )	{ DestTweenState().diffuse[3] = c; };
	virtual void SetDiffuseTopEdge( RageColor c )		{ DestTweenState().diffuse[0] = DestTweenState().diffuse[1] = c; };
	virtual void SetDiffuseRightEdge( RageColor c )		{ DestTweenState().diffuse[1] = DestTweenState().diffuse[3] = c; };
	virtual void SetDiffuseBottomEdge( RageColor c )	{ DestTweenState().diffuse[2] = DestTweenState().diffuse[3] = c; };
	virtual void SetDiffuseLeftEdge( RageColor c )		{ DestTweenState().diffuse[0] = DestTweenState().diffuse[2] = c; };
	virtual RageColor GetDiffuse()						{ return DestTweenState().diffuse[0]; };
	virtual RageColor GetDiffuses( int i )				{ return DestTweenState().diffuse[i]; };
	virtual void SetGlow( RageColor c )					{ DestTweenState().glow = c; };
	virtual RageColor GetGlow()							{ return DestTweenState().glow; };
	virtual void SetGlowMode( GlowMode m )				{ DestTweenState().glowmode = m; };
	virtual GlowMode GetGlowMode()						{ return DestTweenState().glowmode; };


	virtual void BeginTweening( float time, TweenType tt = TWEEN_LINEAR );
	virtual void StopTweening();
	virtual void FinishTweening();
	virtual void HurryTweening( float factor );
	virtual float GetTweenTimeLeft() const;	// Amount of time until all tweens have stopped
	virtual TweenState& DestTweenState()	// where Actor will end when its tween finish
	{
		if( m_TweenStates.empty() )	// not tweening
			return m_current;
		else
			return LatestTween();
	}
	virtual void SetLatestTween( TweenState ts )	{ LatestTween() = ts; }

	
	enum StretchType { fit_inside, cover };

	void ScaleToCover( const RectI &rect )		{ ScaleTo( rect, cover ); }
	void ScaleToFitInside( const RectI &rect )	{ ScaleTo( rect, fit_inside); };
	void ScaleTo( const RectI &rect, StretchType st );

	void StretchTo( const RectI &rect );
	void StretchTo( const RectF &rect );




	enum HorizAlign { align_left, align_center, align_right };
	virtual void SetHorizAlign( HorizAlign ha ) { m_HorizAlign = ha; }
	virtual void SetHorizAlign( CString s );

	enum VertAlign { align_top, align_middle, align_bottom };
	virtual void SetVertAlign( VertAlign va ) { m_VertAlign = va; }
	virtual void SetVertAlign( CString s );


	// effects
	void SetEffectNone()						{ m_Effect = no_effect; }
	Effect GetEffect()							{ return m_Effect; }
	void SetEffectColor1( RageColor c )			{ m_effectColor1 = c; }
	void SetEffectColor2( RageColor c )			{ m_effectColor2 = c; }
	void SetEffectPeriod( float fSecs )			{ m_fEffectPeriodSeconds = fSecs; } 
	void SetEffectOffset( float fPercent )		{ m_fEffectPerfectOffset = fPercent; }
	void SetEffectClock( EffectClock c )		{ m_EffectClock = c; }
	void SetEffectClock( CString s );

	void SetEffectMagnitude( RageVector3 vec )	{ m_vEffectMagnitude = vec; }

	void SetEffectDiffuseBlink( 
		float fEffectPeriodSeconds = 1.0f,
		RageColor c1 = RageColor(0.5f,0.5f,0.5f,1), 
		RageColor c2 = RageColor(1,1,1,1) );
	void SetEffectDiffuseShift( float fEffectPeriodSeconds = 1.f,
		RageColor c1 = RageColor(0,0,0,1), 
		RageColor c2 = RageColor(1,1,1,1) );
	void SetEffectGlowBlink( float fEffectPeriodSeconds = 1.f,
		RageColor c1 = RageColor(1,1,1,0.2f),
		RageColor c2 = RageColor(1,1,1,0.8f) );
	void SetEffectGlowShift( 
		float fEffectPeriodSeconds = 1.0f,
		RageColor c1 = RageColor(1,1,1,0.2f),
		RageColor c2 = RageColor(1,1,1,0.8f) );
	void SetEffectRainbow( 
		float fEffectPeriodSeconds = 2.0f );
	void SetEffectWag( 
		float fPeriod = 2.f, 
		RageVector3 vect = RageVector3(0,0,0.2f) );
	void SetEffectBounce( 
		float fPeriod = 2.f, 
		RageVector3 vect = RageVector3(0,0,20) );
	void SetEffectBob( 
		float fPeriod = 2.f, 
		RageVector3 vect = RageVector3(0,0,20) );
	void SetEffectPulse( 
		float fPeriod = 2.f,
		float fMinZoom = 0.5f,
		float fMaxZoom = 1.f );
	void SetEffectSpin( 
		RageVector3 vect = RageVector3(0,0,180) );
	void SetEffectVibrate( 
		RageVector3 vect = RageVector3(10,10,10) );


	//
	// other properties
	//
	void SetHidden( bool b )	{ m_bHidden = b; }
	void SetShadowLength( float fLength );
	void EnableShadow( bool b )	{ m_bShadow = b; }

	virtual void EnableAnimation( bool b ) 		{ m_bIsAnimating = b; }
	virtual void StartAnimating()	{ this->EnableAnimation(true); };
	virtual void StopAnimating()	{ this->EnableAnimation(false); };


	//
	// render states
	//
	virtual void SetBlendMode( BlendMode mode ) { m_BlendMode = mode; } 
	virtual void SetBlendMode( CString );
	virtual void SetTextureWrapping( bool b ) 	{ m_bTextureWrapping = b; } 
	virtual void SetClearZBuffer( bool b ) 		{ m_bClearZBuffer = b; } 
	virtual void SetUseZBuffer( bool b ) 		{ SetZTest(b); SetZWrite(b); } 
	virtual void SetZTest( bool b ) 			{ m_bZTest = b; } 
	virtual void SetZWrite( bool b ) 			{ m_bZWrite = b; } 
	virtual void SetUseBackfaceCull( bool b ) 	{ m_bUseBackfaceCull = b; } 

	//
	// fade command
	//
	void Fade( float fSleepSeconds, CString sFadeString, float fFadeSeconds, bool bFadingOff );
	void FadeOn( float fSleepSeconds, CString sFadeString, float fFadeSeconds )	{ Fade(fSleepSeconds,sFadeString,fFadeSeconds,false); };
	void FadeOff( float fSleepSeconds, CString sFadeString, float fFadeSeconds )	{ Fade(fSleepSeconds,sFadeString,fFadeSeconds,true); };

	float Command( CString sCommandString );	// return length in seconds to execute command
	virtual void HandleCommand( const CStringArray &asTokens );	// derivable
	static float GetCommandLength( CString command );

	virtual void SetState( int iNewState ) {};
	virtual void SetSecondsIntoAnimation( float fSeconds ) {};
	virtual int GetNumStates() { return 1; };

protected:

	struct TweenInfo
	{
		// counters for tweening
		TweenType	m_TweenType;
		float		m_fTimeLeftInTween;	// how far into the tween are we?
		float		m_fTweenTime;		// seconds between Start and End positions/zooms
	};


	RageVector3	m_baseRotation;
	RageVector3	m_baseScale;


	RageVector2	m_size;
	TweenState	m_current;
	TweenState	m_start;
	vector<TweenState>	m_TweenStates;
	vector<TweenInfo>	m_TweenInfo;

	TweenState& LatestTween() { ASSERT(m_TweenStates.size()>0);	return m_TweenStates.back(); };

	//
	// Temporary variables that are filled just before drawing
	//
	TweenState m_tempState;
	TweenState *m_pTempState;

	bool	m_bFirstUpdate;

	//
	// Stuff for alignment
	//
	HorizAlign	m_HorizAlign;
	VertAlign	m_VertAlign;


	//
	// Stuff for effects
	//
	Effect m_Effect;
	float m_fSecsIntoEffect;
	float m_fEffectPeriodSeconds;
	float m_fEffectPerfectOffset;
	EffectClock m_EffectClock;

	RageColor   m_effectColor1;
	RageColor   m_effectColor2;
	RageVector3 m_vEffectMagnitude;


	//
	// other properties
	//
	bool	m_bHidden;
	bool	m_bShadow;
	float	m_fShadowLength;
	bool	m_bIsAnimating;

	//
	// render states
	//
	bool	m_bTextureWrapping;
	BlendMode	m_BlendMode;
	bool	m_bClearZBuffer;
	bool	m_bZTest, m_bZWrite;
	bool	m_bUseBackfaceCull;
};

#endif
