#include "global.h"
#include "ScreenDebugOverlay.h"
#include "ScreenDimensions.h"
#include "ScreenManager.h"
#include "PrefsManager.h"
#include "GamePreferences.h"
#include "RageLog.h"
#include "GameState.h"
#include "PlayerState.h"
#include "StepMania.h"
#include "GameCommand.h"
#include "ScreenGameplay.h"
#include "RageSoundManager.h"
#include "GameSoundManager.h"
#include "RageTextureManager.h"
#include "MemoryCardManager.h"
#include "NoteSkinManager.h"
#include "Bookkeeper.h"
#include "ProfileManager.h"
#include "CodeDetector.h"
#include "RageInput.h"
#include "RageDisplay.h"
#include "InputEventPlus.h"
#include "LocalizedString.h"
#include "Profile.h"
#include "SongManager.h"
#include "GameLoop.h"
#include "song.h"

static bool g_bIsDisplayed = false;
static bool g_bIsSlow = false;
static bool g_bIsHalt = false;
static RageTimer g_HaltTimer(RageZeroTimer);
static float g_fImageScaleCurrent = 1;
static float g_fImageScaleDestination = 1;

//
// self-registering debug lines
// We don't use SubscriptionManager, because we want to keep the line order.
//
static LocalizedString ON			( "ScreenDebugOverlay", "on" );
static LocalizedString OFF			( "ScreenDebugOverlay", "off" );

class IDebugLine;
static vector<IDebugLine*> *g_pvpSubscribers = NULL;
class IDebugLine
{
public:
	IDebugLine()
	{ 
		if( g_pvpSubscribers == NULL )
			g_pvpSubscribers = new vector<IDebugLine*>;
		g_pvpSubscribers->push_back( this );
	}
	virtual ~IDebugLine() { }
	enum Type { all_screens, gameplay_only };
	virtual Type GetType() const { return all_screens; }
	virtual RString GetDisplayTitle() = 0;
	virtual RString GetDisplayValue() { return IsEnabled() ? ON.GetValue():OFF.GetValue(); }
	virtual RString GetPageName() const { return "Main"; }
	virtual bool ForceOffAfterUse() const { return false; }
	virtual bool IsEnabled() = 0;
	virtual void DoAndMakeSystemMessage( RString &sMessageOut )
	{
		RString s1 = GetDisplayTitle();
		RString s2 = GetDisplayValue();
		if( !s2.empty() )
			s1 += " - ";
		sMessageOut = s1 + s2;
	};

	DeviceInput m_Button;
};

static bool IsGameplay()
{
	return SCREENMAN && SCREENMAN->GetTopScreen() && SCREENMAN->GetTopScreen()->GetScreenType() == gameplay;
}


REGISTER_SCREEN_CLASS( ScreenDebugOverlay );

ScreenDebugOverlay::~ScreenDebugOverlay()
{
	this->RemoveAllChildren();

	FOREACH( BitmapText*, m_vptextButton, p )
		SAFE_DELETE( *p );
	m_vptextButton.clear();
	FOREACH( BitmapText*, m_vptextFunction, p )
		SAFE_DELETE( *p );
	m_vptextFunction.clear();
}

const int MAX_DEBUG_LINES = 30;
struct MapDebugToDI
{
	DeviceInput holdForDebug1;
	DeviceInput holdForDebug2;
	DeviceInput holdForSlow;
	DeviceInput holdForFast;
	DeviceInput debugButton[MAX_DEBUG_LINES];
	DeviceInput gameplayButton[MAX_DEBUG_LINES];
	map<DeviceInput, int> pageButton;
	void Clear()
	{
		holdForDebug1.MakeInvalid();
		holdForDebug2.MakeInvalid();
		holdForSlow.MakeInvalid();
		holdForFast.MakeInvalid();
		for( int i=0; i<MAX_DEBUG_LINES; i++ )
		{
			debugButton[i].MakeInvalid();
			gameplayButton[i].MakeInvalid();
		}
	}
};
static MapDebugToDI g_Mappings;

static LocalizedString IN_GAMEPLAY( "ScreenDebugOverlay", "%s in gameplay" );
static LocalizedString OR( "ScreenDebugOverlay", "or" );
static RString GetDebugButtonName( const IDebugLine *pLine )
{
	RString s = INPUTMAN->GetDeviceSpecificInputString(pLine->m_Button);
	switch( pLine->GetType() )
	{
	case IDebugLine::all_screens:
		return s;
	case IDebugLine::gameplay_only:
		return ssprintf( IN_GAMEPLAY.GetValue(), s.c_str() );
	default:
		ASSERT(0);
	}
}

static LocalizedString DEBUG_MENU( "ScreenDebugOverlay", "Debug Menu" );
void ScreenDebugOverlay::Init()
{
	Screen::Init();
	
	// Init debug mappings
	// TODO: Arch-specific?
	{
		g_Mappings.Clear();

		g_Mappings.holdForDebug1 = DeviceInput(DEVICE_KEYBOARD, KEY_F3);
		g_Mappings.holdForDebug2.MakeInvalid();
		g_Mappings.holdForSlow = DeviceInput(DEVICE_KEYBOARD, KEY_ACCENT);
		g_Mappings.holdForFast = DeviceInput(DEVICE_KEYBOARD, KEY_TAB);


		int i=0;
		g_Mappings.gameplayButton[i++]	= DeviceInput(DEVICE_KEYBOARD, KEY_F8);
		g_Mappings.gameplayButton[i++]	= DeviceInput(DEVICE_KEYBOARD, KEY_F7);
		g_Mappings.gameplayButton[i++]	= DeviceInput(DEVICE_KEYBOARD, KEY_F6);
		i=0;
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_C1);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_C2);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_C3);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_C4);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_C5);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_C6);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_C7);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_C8);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_C9);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_C0);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_Cq);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_Cw);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_Ce);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_Cr);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_Ct);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_Cy);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_Cu);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_Ci);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_Co);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_Cp);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_UP);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_DOWN);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_BACK);
		g_Mappings.pageButton[DeviceInput(DEVICE_KEYBOARD, KEY_F5)] = 0;
		g_Mappings.pageButton[DeviceInput(DEVICE_KEYBOARD, KEY_F6)] = 1;
		g_Mappings.pageButton[DeviceInput(DEVICE_KEYBOARD, KEY_F7)] = 2;
		g_Mappings.pageButton[DeviceInput(DEVICE_KEYBOARD, KEY_F8)] = 3;
	}

	map<RString,int> iNextDebugButton;
	int iNextGameplayButton = 0;
	FOREACH( IDebugLine*, *g_pvpSubscribers, p )
	{
		RString sPageName = (*p)->GetPageName();
		
		DeviceInput di;
		switch( (*p)->GetType() )
		{
		case IDebugLine::all_screens:
			di = g_Mappings.debugButton[iNextDebugButton[sPageName]++];
			break;
		case IDebugLine::gameplay_only:
			di = g_Mappings.gameplayButton[iNextGameplayButton++];
			break;
		}
		(*p)->m_Button = di;

		if( find(m_asPages.begin(), m_asPages.end(), sPageName) == m_asPages.end() )
			m_asPages.push_back( sPageName );
	}

	m_iCurrentPage = 0;
	m_bForcedHidden = false;

	m_Quad.StretchTo( RectF( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT ) );
	m_Quad.SetDiffuse( RageColor(0, 0, 0, 0.5f) );
	this->AddChild( &m_Quad );
	
	m_textHeader.LoadFromFont( THEME->GetPathF("Common", "normal") );
	m_textHeader.SetHorizAlign( align_left );
	m_textHeader.SetX( SCREEN_LEFT+20 );
	m_textHeader.SetY( SCREEN_TOP+20 );
	m_textHeader.SetZoom( 1.0f );
	m_textHeader.SetText( DEBUG_MENU );
	this->AddChild( &m_textHeader );

	FOREACH_CONST( IDebugLine*, *g_pvpSubscribers, p )
	{
		{
			BitmapText *pT1 = new BitmapText;
			pT1->LoadFromFont( THEME->GetPathF("Common", "normal") );
			pT1->SetHorizAlign( align_right );
			pT1->SetText( "blah" );
			pT1->SetShadowLength( 2 );
			m_vptextButton.push_back( pT1 );
			this->AddChild( pT1 );
		}
		{
			BitmapText *pT2 = new BitmapText;
			pT2->LoadFromFont( THEME->GetPathF("Common", "normal") );
			pT2->SetHorizAlign( align_left );
			pT2->SetText( "blah" );
			pT2->SetShadowLength( 2 );
			m_vptextFunction.push_back( pT2 );
			this->AddChild( pT2 );
		}
	}

	this->SetVisible( false );
}

void ScreenDebugOverlay::Update( float fDeltaTime )
{
	{
		float fRate = 1;
		if( INPUTFILTER->IsBeingPressed(g_Mappings.holdForFast) )
		{
			if( INPUTFILTER->IsBeingPressed(g_Mappings.holdForSlow) )
				fRate = 0; /* both; stop time */
			else
				fRate *= 4;
		}
		else if( INPUTFILTER->IsBeingPressed(g_Mappings.holdForSlow) )
		{
			fRate /= 4;
		}

		if( g_bIsHalt )
			fRate = 0;
		else if( g_bIsSlow )
			fRate /= 4;

		GameLoop::SetUpdateRate( fRate );
	}

	bool bCenteringNeedsUpdate = g_fImageScaleCurrent != g_fImageScaleDestination;
	fapproach( g_fImageScaleCurrent, g_fImageScaleDestination, fDeltaTime );
	if( bCenteringNeedsUpdate )
	{
		DISPLAY->ChangeCentering(
			PREFSMAN->m_iCenterImageTranslateX, 
			PREFSMAN->m_iCenterImageTranslateY,
			PREFSMAN->m_fCenterImageAddWidth - (int)SCREEN_WIDTH + (int)(g_fImageScaleCurrent*SCREEN_WIDTH),
			PREFSMAN->m_fCenterImageAddHeight - (int)SCREEN_HEIGHT + (int)(g_fImageScaleCurrent*SCREEN_HEIGHT) );
	}

	Screen::Update(fDeltaTime);

	this->SetVisible( g_bIsDisplayed && !m_bForcedHidden );
	if( !g_bIsDisplayed )
		return;

	UpdateText();
}

void ScreenDebugOverlay::UpdateText()
{
	m_textHeader.SetText( ssprintf("%s: %s", DEBUG_MENU.GetValue().c_str(), GetCurrentPageName().c_str()) );

	/* Highlight options that aren't the default. */
	const RageColor off(0.6f, 0.6f, 0.6f, 1.0f);
	const RageColor on(1, 1, 1, 1);
	
	int iOffset = 0;
	FOREACH_CONST( IDebugLine*, *g_pvpSubscribers, p )
	{
		RString sPageName = (*p)->GetPageName();

		int i = p-g_pvpSubscribers->begin();

		float fY = SCREEN_TOP+50 + iOffset * 16;

		BitmapText &txt1 = *m_vptextButton[i];
		BitmapText &txt2 = *m_vptextFunction[i];
		if( sPageName != GetCurrentPageName() )
		{
			txt1.SetVisible( false );
			txt2.SetVisible( false );
			continue;
		}
		txt1.SetVisible( true );
		txt2.SetVisible( true );
		++iOffset;

		txt1.SetX( SCREEN_CENTER_X-50 );
		txt1.SetY( fY );
		txt1.SetZoom( 0.7f );

		txt2.SetX( SCREEN_CENTER_X-30 );
		txt2.SetY( fY );
		txt2.SetZoom( 0.7f );

		RString s1 = (*p)->GetDisplayTitle();
		RString s2 = (*p)->GetDisplayValue();

		bool bOn = (*p)->IsEnabled();

		txt1.SetDiffuse( bOn ? on:off );
		txt2.SetDiffuse( bOn ? on:off );

		RString sButton = GetDebugButtonName( *p );
		if( !sButton.empty() )
			sButton += ": ";
		txt1.SetText( sButton );
		if( !s2.empty() )
			s1 += " - ";
		txt2.SetText( s1 + s2 );
	}
	
	if( g_bIsHalt )
	{
		/* More than once I've paused the game accidentally and wasted time
		 * figuring out why, so warn. */
		if( g_HaltTimer.Ago() >= 5.0f )
		{
			g_HaltTimer.Touch();
			LOG->Warn( "Game halted" );
		}
	}
}

template<typename U, typename V>
bool GetValueFromMap( const map<U, V> &m, const U &key, V &val )
{
	typename map<U, V>::const_iterator it = m.find(key);
	if( it == m.end() )
		return false;
	val = it->second;
	return true;
}

bool ScreenDebugOverlay::OverlayInput( const InputEventPlus &input )
{
	if( input.DeviceI == g_Mappings.holdForDebug1 || 
		input.DeviceI == g_Mappings.holdForDebug2 )
	{
		bool bHoldingNeither =
			(!g_Mappings.holdForDebug1.IsValid() || !INPUTFILTER->IsBeingPressed(g_Mappings.holdForDebug1)) &&
			(!g_Mappings.holdForDebug2.IsValid() || !INPUTFILTER->IsBeingPressed(g_Mappings.holdForDebug2));
		bool bHoldingBoth =
			(!g_Mappings.holdForDebug1.IsValid() || INPUTFILTER->IsBeingPressed(g_Mappings.holdForDebug1)) &&
			(!g_Mappings.holdForDebug2.IsValid() || INPUTFILTER->IsBeingPressed(g_Mappings.holdForDebug2));
		if( bHoldingNeither )
			m_bForcedHidden = false;
			
		if( bHoldingBoth )
			g_bIsDisplayed = true;
		else
			g_bIsDisplayed = false;
	}

	int iPage;
	if( g_bIsDisplayed && GetValueFromMap(g_Mappings.pageButton, input.DeviceI, iPage) )
	{
		if( input.type != IET_FIRST_PRESS )
			return true; /* eat the input but do nothing */
		m_iCurrentPage = iPage;
		CLAMP( m_iCurrentPage, 0, (int) m_asPages.size()-1 );
		return true;
	}

	FOREACH_CONST( IDebugLine*, *g_pvpSubscribers, p )
	{
		RString sPageName = (*p)->GetPageName();

		int i = p-g_pvpSubscribers->begin();

		// Gameplay buttons are available only in gameplay.  Non-gameplay buttons are
		// only available when the screen is displayed.
		switch( (*p)->GetType() )
		{
		case IDebugLine::all_screens:
			if( !g_bIsDisplayed )
				continue;
			if( sPageName != GetCurrentPageName() )
				continue;
			break;
		case IDebugLine::gameplay_only:
			if( !IsGameplay() )
				continue;
			break;
		default:
			ASSERT(0);
		}

		if( input.DeviceI == (*p)->m_Button )
		{
			if( input.type != IET_FIRST_PRESS )
				return true; /* eat the input but do nothing */

			BitmapText &txt1 = *m_vptextButton[i];
			txt1.FinishTweening();
			float fZoom = txt1.GetZoom();
			txt1.SetZoom( fZoom * 1.2f );
			txt1.BeginTweening( 0.2f, TWEEN_LINEAR );
			txt1.SetZoom( fZoom );

			RString sMessage;
			(*p)->DoAndMakeSystemMessage( sMessage );
			if( !sMessage.empty() )
				SCREENMAN->SystemMessage( sMessage );
			if( (*p)->ForceOffAfterUse() )
				m_bForcedHidden = true;

			UpdateText();
			return true;
		}
	}

	return false;
}


//
// DebugLine helpers
//
static void SetSpeed()
{
	// PauseMusic( g_bIsHalt );
}

void ChangeVolume( float fDelta )
{
	Preference<float> *pRet = Preference<float>::GetPreferenceByName("SoundVolume");

	float fVol = pRet->Get();
	fVol += fDelta;
	CLAMP( fVol, 0.0f, 1.0f );
	pRet->Set( fVol );
	SOUNDMAN->SetMixVolume();
}



//
// DebugLines
//
#define DECLARE_ONE( x ) static x g_##x


static LocalizedString AUTO_PLAY		( "ScreenDebugOverlay", "AutoPlay" );
static LocalizedString ASSIST			( "ScreenDebugOverlay", "Assist" );
static LocalizedString AUTOSYNC			( "ScreenDebugOverlay", "Autosync" );
static LocalizedString COIN_MODE		( "ScreenDebugOverlay", "CoinMode" );
static LocalizedString HALT			( "ScreenDebugOverlay", "Halt" );
static LocalizedString LIGHTS_DEBUG		( "ScreenDebugOverlay", "Lights Debug" );
static LocalizedString MONKEY_INPUT		( "ScreenDebugOverlay", "Monkey Input" );
static LocalizedString RENDERING_STATS		( "ScreenDebugOverlay", "Rendering Stats" );
static LocalizedString VSYNC			( "ScreenDebugOverlay", "Vsync" );
static LocalizedString MULTITEXTURE		( "ScreenDebugOverlay", "Multitexture" );
static LocalizedString SCREEN_TEST_MODE		( "ScreenDebugOverlay", "Screen Test Mode" );
static LocalizedString SCREEN_SHOW_MASKS	( "ScreenDebugOverlay", "Show Masks" );
static LocalizedString PROFILE			( "ScreenDebugOverlay", "Profile" );
static LocalizedString CLEAR_PROFILE_STATS	( "ScreenDebugOverlay", "Clear Profile Stats" );
static LocalizedString FILL_PROFILE_STATS	( "ScreenDebugOverlay", "Fill Profile Stats" );
static LocalizedString SEND_NOTES_ENDED		( "ScreenDebugOverlay", "Send Notes Ended" );
static LocalizedString RELOAD			( "ScreenDebugOverlay", "Reload" );
static LocalizedString RESTART			( "ScreenDebugOverlay", "Restart" );
static LocalizedString SCREEN_ON		( "ScreenDebugOverlay", "Send On To Screen" );
static LocalizedString SCREEN_OFF		( "ScreenDebugOverlay", "Send Off To Screen" );
static LocalizedString RELOAD_THEME_AND_TEXTURES( "ScreenDebugOverlay", "Reload Theme and Textures" );
static LocalizedString WRITE_PROFILES		( "ScreenDebugOverlay", "Write Profiles" );
static LocalizedString WRITE_PREFERENCES	( "ScreenDebugOverlay", "Write Preferences" );
static LocalizedString MENU_TIMER		( "ScreenDebugOverlay", "Menu Timer" );
static LocalizedString FLUSH_LOG		( "ScreenDebugOverlay", "Flush Log" );
static LocalizedString PULL_BACK_CAMERA		( "ScreenDebugOverlay", "Pull Back Camera" );
static LocalizedString VOLUME_UP		( "ScreenDebugOverlay", "Volume Up" );
static LocalizedString VOLUME_DOWN		( "ScreenDebugOverlay", "Volume Down" );
static LocalizedString UPTIME			( "ScreenDebugOverlay", "Uptime" );
static LocalizedString FORCE_CRASH		( "ScreenDebugOverlay", "Force Crash" );
static LocalizedString SLOW			( "ScreenDebugOverlay", "Slow" );
static LocalizedString CPU			( "ScreenDebugOverlay", "CPU" );
static LocalizedString SONG			( "ScreenDebugOverlay", "Song" );
static LocalizedString MACHINE			( "ScreenDebugOverlay", "Machine" );
static LocalizedString SYNC_TEMPO   ( "ScreenDebugOverlay", "Tempo" );


class DebugLineAutoplay : public IDebugLine
{
	virtual RString GetDisplayTitle() { return AUTO_PLAY.GetValue(); }
	virtual RString GetDisplayValue()
	{
		switch( GamePreferences::m_AutoPlay.Get() )
		{
		case PC_HUMAN:		return OFF.GetValue();	break;
		case PC_AUTOPLAY:	return ON.GetValue();	break;
		case PC_CPU:		return CPU.GetValue();	break;
		default:	ASSERT(0);	return RString();
		}
	}
	virtual Type GetType() const { return IDebugLine::gameplay_only; }
	virtual bool IsEnabled() { return GamePreferences::m_AutoPlay.Get() != PC_HUMAN; }
	virtual void DoAndMakeSystemMessage( RString &sMessageOut )
	{
		ASSERT( GAMESTATE->m_MasterPlayerNumber != PLAYER_INVALID );
		PlayerController pc = GAMESTATE->m_pPlayerState[GAMESTATE->m_MasterPlayerNumber]->m_PlayerController;
		bool bHoldingShift = INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT) );
		if( bHoldingShift )
			pc = (pc==PC_CPU) ? PC_HUMAN : PC_CPU;
		else
			pc = (pc==PC_AUTOPLAY) ? PC_HUMAN : PC_AUTOPLAY;
		GamePreferences::m_AutoPlay.Set( pc );
		FOREACH_HumanPlayer(p)
			GAMESTATE->m_pPlayerState[p]->m_PlayerController = GamePreferences::m_AutoPlay;
		FOREACH_MultiPlayer(p)
			GAMESTATE->m_pMultiPlayerState[p]->m_PlayerController = GamePreferences::m_AutoPlay;
		IDebugLine::DoAndMakeSystemMessage( sMessageOut );
	}
};

class DebugLineAssist : public IDebugLine
{
	virtual RString GetDisplayTitle() { return ASSIST.GetValue(); }
	virtual Type GetType() const { return gameplay_only; }
	virtual RString GetDisplayValue() { 
		SongOptions so;
		so.m_bAssistClap = GAMESTATE->m_SongOptions.GetSong().m_bAssistClap;
		so.m_bAssistMetronome = GAMESTATE->m_SongOptions.GetSong().m_bAssistMetronome;
		if( so.m_bAssistClap || so.m_bAssistMetronome )
			return so.GetLocalizedString();
		else
			return OFF.GetValue();
	}
	virtual bool IsEnabled() { return GAMESTATE->m_SongOptions.GetSong().m_bAssistClap || GAMESTATE->m_SongOptions.GetSong().m_bAssistMetronome; }
	virtual void DoAndMakeSystemMessage( RString &sMessageOut )
	{
		ASSERT( GAMESTATE->m_MasterPlayerNumber != PLAYER_INVALID );
		bool bHoldingShift = INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT) );
		bool b;
		if( bHoldingShift )
			b = !GAMESTATE->m_SongOptions.GetSong().m_bAssistMetronome;
		else
			b = !GAMESTATE->m_SongOptions.GetSong().m_bAssistClap;
		if( bHoldingShift )
			SO_GROUP_ASSIGN( GAMESTATE->m_SongOptions, ModsLevel_Song, m_bAssistMetronome, b );
		else
			SO_GROUP_ASSIGN( GAMESTATE->m_SongOptions, ModsLevel_Song, m_bAssistClap, b );

		IDebugLine::DoAndMakeSystemMessage( sMessageOut );
	}
};

class DebugLineAutosync : public IDebugLine
{
	virtual RString GetDisplayTitle() { return AUTOSYNC.GetValue(); }
	virtual RString GetDisplayValue()
	{ 
		switch( GAMESTATE->m_SongOptions.GetSong().m_AutosyncType )
		{
		case SongOptions::AUTOSYNC_OFF: 	return OFF.GetValue();  		break;
		case SongOptions::AUTOSYNC_SONG:	return SONG.GetValue(); 		break;
		case SongOptions::AUTOSYNC_MACHINE:	return MACHINE.GetValue(); 		break;
		case SongOptions::AUTOSYNC_TEMPO:	return SYNC_TEMPO.GetValue();		break;
		default:	ASSERT(0);
		}
	}
	virtual Type GetType() const { return IDebugLine::gameplay_only; }
	virtual bool IsEnabled() { return GAMESTATE->m_SongOptions.GetSong().m_AutosyncType!=SongOptions::AUTOSYNC_OFF; }
	virtual void DoAndMakeSystemMessage( RString &sMessageOut )
	{
		int as = GAMESTATE->m_SongOptions.GetSong().m_AutosyncType + 1;
		bool bAllowSongAutosync = !GAMESTATE->IsCourseMode();
		if( !bAllowSongAutosync  && 
		  ( as == SongOptions::AUTOSYNC_SONG || as == SongOptions::AUTOSYNC_TEMPO ) )
			as = SongOptions::AUTOSYNC_MACHINE;
		wrap( as, SongOptions::NUM_AUTOSYNC_TYPES );
		SO_GROUP_ASSIGN( GAMESTATE->m_SongOptions, ModsLevel_Song, m_AutosyncType, SongOptions::AutosyncType(as) );
		MESSAGEMAN->Broadcast( Message_AutosyncChanged );
		IDebugLine::DoAndMakeSystemMessage( sMessageOut );
	}
};

class DebugLineCoinMode : public IDebugLine
{
	virtual RString GetDisplayTitle() { return COIN_MODE.GetValue(); }
	virtual RString GetDisplayValue() { return CoinModeToString(GamePreferences::m_CoinMode); }
	virtual bool IsEnabled() { return true; }
	virtual void DoAndMakeSystemMessage( RString &sMessageOut )
	{
		int cm = GamePreferences::m_CoinMode+1;
		wrap( cm, NUM_CoinMode );
		GamePreferences::m_CoinMode.Set( CoinMode(cm) );
		SCREENMAN->RefreshCreditsMessages();
		IDebugLine::DoAndMakeSystemMessage( sMessageOut );
	}
};

class DebugLineSlow : public IDebugLine
{
	virtual RString GetDisplayTitle() { return SLOW.GetValue(); }
	virtual bool IsEnabled() { return g_bIsSlow; }
	virtual void DoAndMakeSystemMessage( RString &sMessageOut )
	{
		g_bIsSlow = !g_bIsSlow;
		SetSpeed();
		IDebugLine::DoAndMakeSystemMessage( sMessageOut );
	}
};

class DebugLineHalt : public IDebugLine
{
	virtual RString GetDisplayTitle() { return HALT.GetValue(); }
	virtual bool IsEnabled() { return g_bIsHalt; }
	virtual void DoAndMakeSystemMessage( RString &sMessageOut )
	{
		g_bIsHalt = !g_bIsHalt;
		g_HaltTimer.Touch();
		SetSpeed();
		IDebugLine::DoAndMakeSystemMessage( sMessageOut );
	}
};

class DebugLineLightsDebug : public IDebugLine
{
	virtual RString GetDisplayTitle() { return LIGHTS_DEBUG.GetValue(); }
	virtual bool IsEnabled() { return PREFSMAN->m_bDebugLights.Get(); }
	virtual void DoAndMakeSystemMessage( RString &sMessageOut )
	{
		PREFSMAN->m_bDebugLights.Set( !PREFSMAN->m_bDebugLights );
		IDebugLine::DoAndMakeSystemMessage( sMessageOut );
	}
};

class DebugLineMonkeyInput : public IDebugLine
{
	virtual RString GetDisplayTitle() { return MONKEY_INPUT.GetValue(); }
	virtual bool IsEnabled() { return PREFSMAN->m_bMonkeyInput.Get(); }
	virtual void DoAndMakeSystemMessage( RString &sMessageOut )
	{
		PREFSMAN->m_bMonkeyInput.Set( !PREFSMAN->m_bMonkeyInput );
		IDebugLine::DoAndMakeSystemMessage( sMessageOut );
	}
};

class DebugLineStats : public IDebugLine
{
	virtual RString GetDisplayTitle() { return RENDERING_STATS.GetValue(); }
	virtual bool IsEnabled() { return PREFSMAN->m_bShowStats.Get(); }
	virtual void DoAndMakeSystemMessage( RString &sMessageOut )
	{
		PREFSMAN->m_bShowStats.Set( !PREFSMAN->m_bShowStats );
		IDebugLine::DoAndMakeSystemMessage( sMessageOut );
	}
};

class DebugLineVsync : public IDebugLine
{
	virtual RString GetDisplayTitle() { return VSYNC.GetValue(); }
	virtual bool IsEnabled() { return PREFSMAN->m_bVsync.Get(); }
	virtual void DoAndMakeSystemMessage( RString &sMessageOut )
	{
		PREFSMAN->m_bVsync.Set( !PREFSMAN->m_bVsync );
		StepMania::ApplyGraphicOptions();
		IDebugLine::DoAndMakeSystemMessage( sMessageOut );
	}
};

class DebugLineAllowMultitexture : public IDebugLine
{
	virtual RString GetDisplayTitle() { return MULTITEXTURE.GetValue(); }
	virtual bool IsEnabled() { return PREFSMAN->m_bAllowMultitexture.Get(); }
	virtual void DoAndMakeSystemMessage( RString &sMessageOut )
	{
		PREFSMAN->m_bAllowMultitexture.Set( !PREFSMAN->m_bAllowMultitexture );
		IDebugLine::DoAndMakeSystemMessage( sMessageOut );
	}
};

class DebugLineScreenTestMode : public IDebugLine
{
	virtual RString GetDisplayTitle() { return SCREEN_TEST_MODE.GetValue(); }
	virtual bool IsEnabled() { return PREFSMAN->m_bScreenTestMode.Get(); }
	virtual RString GetPageName() const { return "Theme"; }
	virtual void DoAndMakeSystemMessage( RString &sMessageOut )
	{
		PREFSMAN->m_bScreenTestMode.Set( !PREFSMAN->m_bScreenTestMode );
		IDebugLine::DoAndMakeSystemMessage( sMessageOut );
	}
};

class DebugLineShowMasks : public IDebugLine
{
	virtual RString GetDisplayTitle() { return SCREEN_SHOW_MASKS.GetValue(); }
	virtual bool IsEnabled() { return GetPref()->Get(); }
	virtual RString GetPageName() const { return "Theme"; }
	virtual void DoAndMakeSystemMessage( RString &sMessageOut )
	{
		GetPref()->Set( !GetPref()->Get() );
		IDebugLine::DoAndMakeSystemMessage( sMessageOut );
	}

	Preference<bool> *GetPref()
	{
		return Preference<bool>::GetPreferenceByName("ShowMasks");
	}
};

static ProfileSlot g_ProfileSlot = ProfileSlot_Machine;
static bool IsSelectProfilePersistent()
{
	if( g_ProfileSlot == ProfileSlot_Machine )
		return true;
	return PROFILEMAN->IsPersistentProfile( (PlayerNumber) g_ProfileSlot );
}

class DebugLineProfileSlot : public IDebugLine
{
	virtual RString GetDisplayTitle() { return PROFILE.GetValue(); }
	virtual RString GetDisplayValue()
	{
		switch( g_ProfileSlot )
		{
		case ProfileSlot_Machine: return "Machine";
		case ProfileSlot_Player1: return "Player 1";
		case ProfileSlot_Player2: return "Player 2";
		}

		return RString();
	}
	virtual bool IsEnabled() { return IsSelectProfilePersistent(); }
	virtual RString GetPageName() const { return "Profiles"; }
	virtual void DoAndMakeSystemMessage( RString &sMessageOut )
	{
		enum_add( g_ProfileSlot, +1 );
		if( g_ProfileSlot == NUM_ProfileSlot )
			g_ProfileSlot = ProfileSlot_Player1;

		IDebugLine::DoAndMakeSystemMessage( sMessageOut );
	}
};


class DebugLineClearProfileStats : public IDebugLine
{
	virtual RString GetDisplayTitle() { return CLEAR_PROFILE_STATS.GetValue(); }
	virtual RString GetDisplayValue() { return RString(); }
	virtual bool IsEnabled() { return IsSelectProfilePersistent(); }
	virtual RString GetPageName() const { return "Profiles"; }
	virtual void DoAndMakeSystemMessage( RString &sMessageOut )
	{
		Profile *pProfile = PROFILEMAN->GetProfile( g_ProfileSlot );
		pProfile->ClearStats();
		IDebugLine::DoAndMakeSystemMessage( sMessageOut );
	}
};

static HighScore MakeRandomHighScore( float fPercentDP )
{
	HighScore hs;
	hs.SetName( "FAKE" );
	hs.SetGrade( (Grade)SCALE( RandomInt(5), 0, 4, Grade_Tier01, Grade_Tier05 ) );
	hs.SetScore( RandomInt(100*1000) );
	hs.SetPercentDP( fPercentDP );
	hs.SetSurviveSeconds( randomf(30.0f, 100.0f) );
	PlayerOptions po;
	po.ChooseRandomModifiers();
	hs.SetModifiers( po.GetString() );
	hs.SetDateTime( DateTime::GetNowDateTime() );
	hs.SetPlayerGuid( Profile::MakeGuid() );
	hs.SetMachineGuid( Profile::MakeGuid() );
	hs.SetProductID( RandomInt(10) );
	FOREACH_ENUM( TapNoteScore, tns )
		hs.SetTapNoteScore( tns, RandomInt(100) );
	FOREACH_ENUM( HoldNoteScore, hns )
		hs.SetHoldNoteScore( hns, RandomInt(100) );
	RadarValues rv;
	FOREACH_ENUM( RadarCategory, rc )
		rv.m_Values.f[rc] = randomf( 0, 1 );
	hs.SetRadarValues( rv );

	return hs;
}

static void FillProfileStats( Profile *pProfile )
{
	pProfile->InitSongScores(); 
	pProfile->InitCourseScores();

	static int s_iCount = 0;
	// Choose a percent for all scores.  This is useful for testing unlocks
	// where some elements are unlocked at a certain percent complete
	float fPercentDP = s_iCount ? randomf( 0.6f, 1.0f ) : 1.0f;
	s_iCount = (s_iCount+1)%2;


	int iCount = pProfile->IsMachine()? 
		PREFSMAN->m_iMaxHighScoresPerListForMachine.Get():
		PREFSMAN->m_iMaxHighScoresPerListForPlayer.Get();

	vector<Song*> vpAllSongs = SONGMAN->GetSongs();
	FOREACH( Song*, vpAllSongs, pSong )
	{
		vector<Steps*> vpAllSteps = (*pSong)->GetAllSteps();
		FOREACH( Steps*, vpAllSteps, pSteps )
		{
			pProfile->IncrementStepsPlayCount( *pSong, *pSteps );
			for( int i=0; i<iCount; i++ )
			{
				int iIndex = 0;
				pProfile->AddStepsHighScore( *pSong, *pSteps, MakeRandomHighScore(fPercentDP), iIndex );
			}
		}
	}
	
	vector<Course*> vpAllCourses;
	SONGMAN->GetAllCourses( vpAllCourses, true );
	FOREACH( Course*, vpAllCourses, pCourse )
	{
		vector<Trail*> vpAllTrails;
		(*pCourse)->GetAllTrails( vpAllTrails );
		FOREACH( Trail*, vpAllTrails, pTrail )
		{
			pProfile->IncrementCoursePlayCount( *pCourse, *pTrail );
			for( int i=0; i<iCount; i++ )
			{
				int iIndex = 0;
				pProfile->AddCourseHighScore( *pCourse, *pTrail, MakeRandomHighScore(fPercentDP), iIndex );
			}
		}
	}
}

class DebugLineFillProfileStats : public IDebugLine
{
	virtual RString GetDisplayTitle() { return FILL_PROFILE_STATS.GetValue(); }
	virtual RString GetDisplayValue() { return RString(); }
	virtual bool IsEnabled() { return IsSelectProfilePersistent(); }
	virtual RString GetPageName() const { return "Profiles"; }
	virtual void DoAndMakeSystemMessage( RString &sMessageOut )
	{
		Profile* pProfile = PROFILEMAN->GetProfile( g_ProfileSlot );
		FillProfileStats( pProfile );
		IDebugLine::DoAndMakeSystemMessage( sMessageOut );
	}
};

class DebugLineSendNotesEnded : public IDebugLine
{
	virtual RString GetDisplayTitle() { return SEND_NOTES_ENDED.GetValue(); }
	virtual RString GetDisplayValue() { return RString(); }
	virtual bool IsEnabled() { return true; }
	virtual void DoAndMakeSystemMessage( RString &sMessageOut )
	{
		SCREENMAN->PostMessageToTopScreen( SM_NotesEnded, 0 );
		IDebugLine::DoAndMakeSystemMessage( sMessageOut );
	}
};

class DebugLineReloadCurrentScreen : public IDebugLine
{
	virtual RString GetDisplayTitle() { return RELOAD.GetValue(); }
	virtual RString GetDisplayValue() { return SCREENMAN && SCREENMAN->GetTopScreen()? SCREENMAN->GetTopScreen()->GetName() : RString(); }
	virtual bool IsEnabled() { return true; }
	virtual RString GetPageName() const { return "Theme"; }
	virtual void DoAndMakeSystemMessage( RString &sMessageOut )
	{
		RString sScreenName = SCREENMAN->GetScreen(0)->GetName();
		SCREENMAN->PopAllScreens();

		SOUND->StopMusic();
		StepMania::ResetGame();

		SCREENMAN->SetNewScreen( sScreenName );
		IDebugLine::DoAndMakeSystemMessage( sMessageOut );
		sMessageOut = "";
	}
};

class DebugLineRestartCurrentScreen : public IDebugLine
{
	virtual RString GetDisplayTitle() { return RESTART.GetValue(); }
	virtual RString GetDisplayValue() { return SCREENMAN && SCREENMAN->GetTopScreen()? SCREENMAN->GetTopScreen()->GetName() : RString(); }
	virtual bool IsEnabled() { return true; }
	virtual bool ForceOffAfterUse() const { return true; }
	virtual RString GetPageName() const { return "Theme"; }
	virtual void DoAndMakeSystemMessage( RString &sMessageOut )
	{
		SCREENMAN->GetTopScreen()->BeginScreen();
		IDebugLine::DoAndMakeSystemMessage( sMessageOut );
		sMessageOut = "";
	}
};

class DebugLineCurrentScreenOn : public IDebugLine
{
	virtual RString GetDisplayTitle() { return SCREEN_ON.GetValue(); }
	virtual RString GetDisplayValue() { return SCREENMAN && SCREENMAN->GetTopScreen()? SCREENMAN->GetTopScreen()->GetName() : RString(); }
	virtual bool IsEnabled() { return true; }
	virtual bool ForceOffAfterUse() const { return true; }
	virtual RString GetPageName() const { return "Theme"; }
	virtual void DoAndMakeSystemMessage( RString &sMessageOut )
	{
		SCREENMAN->GetTopScreen()->PlayCommand("On");
		IDebugLine::DoAndMakeSystemMessage( sMessageOut );
		sMessageOut = "";
	}
};

class DebugLineCurrentScreenOff : public IDebugLine
{
	virtual RString GetDisplayTitle() { return SCREEN_OFF.GetValue(); }
	virtual RString GetDisplayValue() { return SCREENMAN && SCREENMAN->GetTopScreen()? SCREENMAN->GetTopScreen()->GetName() : RString(); }
	virtual bool IsEnabled() { return true; }
	virtual bool ForceOffAfterUse() const { return true; }
	virtual RString GetPageName() const { return "Theme"; }
	virtual void DoAndMakeSystemMessage( RString &sMessageOut )
	{
		SCREENMAN->GetTopScreen()->PlayCommand("Off");
		IDebugLine::DoAndMakeSystemMessage( sMessageOut );
		sMessageOut = "";
	}
};

class DebugLineReloadTheme : public IDebugLine
{
	virtual RString GetDisplayTitle() { return RELOAD_THEME_AND_TEXTURES.GetValue(); }
	virtual RString GetDisplayValue() { return RString(); }
	virtual bool IsEnabled() { return true; }
	virtual RString GetPageName() const { return "Theme"; }
	virtual void DoAndMakeSystemMessage( RString &sMessageOut )
	{
		THEME->ReloadMetrics();
		TEXTUREMAN->ReloadAll();
		NOTESKIN->RefreshNoteSkinData( GAMESTATE->m_pCurGame );
		CodeDetector::RefreshCacheItems();
		// HACK: Don't update text below.  Return immediately because this screen
		// was just destroyed as part of the theme reload.
		IDebugLine::DoAndMakeSystemMessage( sMessageOut );
	}
};

class DebugLineWriteProfiles : public IDebugLine
{
	virtual RString GetDisplayTitle() { return WRITE_PROFILES.GetValue(); }
	virtual RString GetDisplayValue() { return RString(); }
	virtual bool IsEnabled() { return IsSelectProfilePersistent(); }
	virtual RString GetPageName() const { return "Profiles"; }
	virtual void DoAndMakeSystemMessage( RString &sMessageOut )
	{
		// Also save bookkeeping and profile info for debugging
		// so we don't have to play through a whole song to get new output.
		if( g_ProfileSlot == ProfileSlot_Machine )
			GAMESTATE->SaveLocalData();
		else
			GAMESTATE->SaveProfile( (PlayerNumber) g_ProfileSlot );
		IDebugLine::DoAndMakeSystemMessage( sMessageOut );
	}
};

class DebugLineWritePreferences : public IDebugLine
{
	virtual RString GetDisplayTitle() { return WRITE_PREFERENCES.GetValue(); }
	virtual RString GetDisplayValue() { return RString(); }
	virtual bool IsEnabled() { return true; }
	virtual void DoAndMakeSystemMessage( RString &sMessageOut )
	{
		PREFSMAN->SavePrefsToDisk();
		IDebugLine::DoAndMakeSystemMessage( sMessageOut );
	}
};

class DebugLineMenuTimer : public IDebugLine
{
	virtual RString GetDisplayTitle() { return MENU_TIMER.GetValue(); }
	virtual RString GetDisplayValue() { return RString(); }
	virtual bool IsEnabled() { return PREFSMAN->m_bMenuTimer.Get(); }
	virtual void DoAndMakeSystemMessage( RString &sMessageOut )
	{
		PREFSMAN->m_bMenuTimer.Set( !PREFSMAN->m_bMenuTimer );
		IDebugLine::DoAndMakeSystemMessage( sMessageOut );
	}
};

class DebugLineFlushLog : public IDebugLine
{
	virtual RString GetDisplayTitle() { return FLUSH_LOG.GetValue(); }
	virtual RString GetDisplayValue() { return RString(); }
	virtual bool IsEnabled() { return true; }
	virtual void DoAndMakeSystemMessage( RString &sMessageOut )
	{
		LOG->Flush();
		IDebugLine::DoAndMakeSystemMessage( sMessageOut );
	}
};

class DebugLinePullBackCamera : public IDebugLine
{
	virtual RString GetDisplayTitle() { return PULL_BACK_CAMERA.GetValue(); }
	virtual RString GetDisplayValue() { return RString(); }
	virtual bool IsEnabled() { return g_fImageScaleDestination != 1; }
	virtual void DoAndMakeSystemMessage( RString &sMessageOut )
	{
		if( g_fImageScaleDestination == 1 )
			g_fImageScaleDestination = 0.5f;
		else
			g_fImageScaleDestination = 1;
		IDebugLine::DoAndMakeSystemMessage( sMessageOut );
	}
};

class DebugLineVolumeUp : public IDebugLine
{
	virtual RString GetDisplayTitle() { return VOLUME_UP.GetValue(); }
	virtual RString GetDisplayValue() { return ssprintf("%.0f%%", GetPref()->Get()*100); }
	virtual bool IsEnabled() { return true; }
	virtual void DoAndMakeSystemMessage( RString &sMessageOut )
	{
		ChangeVolume( +0.1f );
		IDebugLine::DoAndMakeSystemMessage( sMessageOut );
	}
	Preference<float> *GetPref()
	{
		return Preference<float>::GetPreferenceByName("SoundVolume");
	}
};

class DebugLineVolumeDown : public IDebugLine
{
	virtual RString GetDisplayTitle() { return VOLUME_DOWN.GetValue(); }
	virtual RString GetDisplayValue() { return RString(); }
	virtual bool IsEnabled() { return true; }
	virtual void DoAndMakeSystemMessage( RString &sMessageOut )
	{
		ChangeVolume( -0.1f );
		IDebugLine::DoAndMakeSystemMessage( sMessageOut );
		sMessageOut += " - " + ssprintf("%.0f%%",GetPref()->Get()*100);
	}
	Preference<float> *GetPref()
	{
		return Preference<float>::GetPreferenceByName("SoundVolume");
	}
};

class DebugLineForceCrash : public IDebugLine
{
	virtual RString GetDisplayTitle() { return FORCE_CRASH.GetValue(); }
	virtual RString GetDisplayValue() { return RString(); }
	virtual bool IsEnabled() { return false; }
	virtual void DoAndMakeSystemMessage( RString &sMessageOut ) { FAIL_M("DebugLineCrash"); }
};

class DebugLineUptime : public IDebugLine
{
	virtual RString GetDisplayTitle() { return UPTIME.GetValue(); }
	virtual RString GetDisplayValue() { return SecondsToMMSSMsMsMs(RageTimer::GetTimeSinceStart()); }
	virtual bool IsEnabled() { return false; }
	virtual void DoAndMakeSystemMessage( RString &sMessageOut ) {}
};

/* #ifdef out the lines below if you don't want them to appear on certain
 * platforms.  This is easier than #ifdefing the whole DebugLine definitions
 * that can span pages.
 */

DECLARE_ONE( DebugLineAutoplay );
DECLARE_ONE( DebugLineAssist );
DECLARE_ONE( DebugLineAutosync );
DECLARE_ONE( DebugLineCoinMode );
DECLARE_ONE( DebugLineSlow );
DECLARE_ONE( DebugLineHalt );
DECLARE_ONE( DebugLineLightsDebug );
DECLARE_ONE( DebugLineMonkeyInput );
DECLARE_ONE( DebugLineStats );
DECLARE_ONE( DebugLineVsync );
DECLARE_ONE( DebugLineAllowMultitexture );
DECLARE_ONE( DebugLineScreenTestMode );
DECLARE_ONE( DebugLineShowMasks );
DECLARE_ONE( DebugLineProfileSlot );
DECLARE_ONE( DebugLineClearProfileStats );
DECLARE_ONE( DebugLineFillProfileStats );
DECLARE_ONE( DebugLineSendNotesEnded );
DECLARE_ONE( DebugLineReloadCurrentScreen );
DECLARE_ONE( DebugLineRestartCurrentScreen );
DECLARE_ONE( DebugLineCurrentScreenOn );
DECLARE_ONE( DebugLineCurrentScreenOff );
DECLARE_ONE( DebugLineReloadTheme );
DECLARE_ONE( DebugLineWriteProfiles );
DECLARE_ONE( DebugLineWritePreferences );
DECLARE_ONE( DebugLineMenuTimer );
DECLARE_ONE( DebugLineFlushLog );
DECLARE_ONE( DebugLinePullBackCamera );
DECLARE_ONE( DebugLineVolumeUp );
DECLARE_ONE( DebugLineVolumeDown );
DECLARE_ONE( DebugLineForceCrash );
DECLARE_ONE( DebugLineUptime );


/*
 * (c) 2001-2005 Chris Danford, Glenn Maynard
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
