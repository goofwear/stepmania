#include "global.h"

#if !defined(WITHOUT_NETWORKING)
#include "ScreenNetSelectBase.h"
#include "ScreenManager.h"
#include "ThemeManager.h"
#include "RageTimer.h"
#include "ActorUtil.h"
#include "Actor.h"
#include "GameSoundManager.h"
#include "MenuTimer.h"
#include "NetworkSyncManager.h"
#include "RageUtil.h"

#define CHATINPUT_WIDTH				THEME->GetMetricF(m_sName,"ChatInputBoxWidth")
#define CHATINPUT_HEIGHT			THEME->GetMetricF(m_sName,"ChatInputBoxHeight")
#define CHATINPUT_COLOR				THEME->GetMetricC(m_sName,"ChatInputBoxColor")
#define CHATOUTPUT_WIDTH			THEME->GetMetricF(m_sName,"ChatOutputBoxWidth")
#define CHATOUTPUT_HEIGHT			THEME->GetMetricF(m_sName,"ChatOutputBoxHeight")
#define CHATOUTPUT_COLOR			THEME->GetMetricC(m_sName,"ChatOutputBoxColor")
#define SHOW_CHAT_LINES				THEME->GetMetricI(m_sName,"ChatOutputLines")

#define USERSALT_Y					THEME->GetMetricF(m_sName,"UsersAY")
#define USERSDELT_X					THEME->GetMetricF(m_sName,"UsersDX")
#define USERS_Y						THEME->GetMetricF(m_sName,"UsersY")
#define USERS_X						THEME->GetMetricF(m_sName,"UsersX")

const ScreenMessage	SM_AddToChat	= ScreenMessage(SM_User+4);
const ScreenMessage SM_UsersUpdate	= ScreenMessage(SM_User+7);
const ScreenMessage SM_SMOnlinePack	= ScreenMessage(SM_User+8);	//Unused, but should be known

ScreenNetSelectBase::ScreenNetSelectBase( const CString& sName ) : ScreenWithMenuElements( sName )
{
	//ChatBox
	m_rectChatInputBox.SetDiffuse( CHATINPUT_COLOR ); 
	m_rectChatInputBox.SetName( "ChatInputBox" );
	m_rectChatInputBox.SetWidth( CHATINPUT_WIDTH );
	m_rectChatInputBox.SetHeight( CHATINPUT_HEIGHT );
	SET_XY_AND_ON_COMMAND( m_rectChatInputBox );
	this->AddChild( &m_rectChatInputBox );

	m_rectChatOutputBox.SetDiffuse( CHATOUTPUT_COLOR );
	m_rectChatOutputBox.SetName( "ChatOutputBox" );
	m_rectChatOutputBox.SetWidth( CHATOUTPUT_WIDTH );
	m_rectChatOutputBox.SetHeight( CHATOUTPUT_HEIGHT );
	SET_XY_AND_ON_COMMAND( m_rectChatOutputBox );
	this->AddChild( &m_rectChatOutputBox );

	m_textChatInput.LoadFromFont( THEME->GetPathF(m_sName,"chat") );
	m_textChatInput.SetHorizAlign( align_left );
	m_textChatInput.SetVertAlign( align_top );
	m_textChatInput.SetShadowLength( 0 );
	m_textChatInput.SetName( "ChatInput" );
	m_textChatInput.SetWrapWidthPixels( (int)(CHATINPUT_WIDTH * 2) );
	SET_XY_AND_ON_COMMAND( m_textChatInput );
	this->AddChild( &m_textChatInput );

	m_textOutHidden.LoadFromFont( THEME->GetPathF(m_sName,"chat") );
	m_textOutHidden.SetWrapWidthPixels( (int)(CHATOUTPUT_WIDTH * 2) );

	m_textChatOutput.LoadFromFont( THEME->GetPathF(m_sName,"chat") );
	m_textChatOutput.SetHorizAlign( align_left );
	m_textChatOutput.SetVertAlign( align_bottom );
	m_textChatOutput.SetShadowLength( 0 );
	m_textChatOutput.SetName( "ChatOutput" );
	SET_XY_AND_ON_COMMAND( m_textChatOutput );
	this->AddChild( &m_textChatOutput );

	//Display updated chat (maybe this should be a function)?
	m_textOutHidden.SetText( NSMAN->m_sChatText );
	vector <wstring> wLines;
	m_textOutHidden.GetLines( wLines );
	m_actualText = "";
	for (unsigned i = max(int(wLines.size()) - SHOW_CHAT_LINES, 0 ) ; i < wLines.size() ; ++i)
		m_actualText += WStringToCString( wLines[i] )+'\n';
	m_textChatOutput.SetText( m_actualText );

	//Users' Background
	
	m_rectUsersBG.SetName( "UsersBG" );
	SET_QUAD_INIT( m_rectUsersBG );
	this->AddChild( &m_rectUsersBG );

	//Display users list
	UpdateUsers();

	return;
}

void ScreenNetSelectBase::Input( const DeviceInput& DeviceI, const InputEventType type,
								  const GameInput& GameI, const MenuInput& MenuI,
								  const StyleInput& StyleI )
{
	if( m_In.IsTransitioning() || m_Out.IsTransitioning() )
		return;

	if( (type != IET_FIRST_PRESS) && (type != IET_SLOW_REPEAT) && (type != IET_FAST_REPEAT ) )
		return;

	bool bHoldingShift = 
		INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT)) ||
		INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_RSHIFT));

	bool bHoldingCtrl = 
		INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_LCTRL)) ||
		INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_RCTRL)) ||
		(!NSMAN->useSMserver);	//If we are disconnected, assume no chatting

	switch( DeviceI.button )
	{
	case KEY_ENTER:
	case KEY_KP_ENTER:
		if (!bHoldingCtrl)
		{
			if ( m_sTextInput != "" )
				NSMAN->SendChat( m_sTextInput );
			m_sTextInput="";
			UpdateTextInput();
			return;
		}
		break;
	case KEY_BACK:
		if(!m_sTextInput.empty())
			m_sTextInput = m_sTextInput.erase( m_sTextInput.size()-1 );
		UpdateTextInput();
		break;
	default:
		char c;
		c = DeviceI.ToChar();

		if( bHoldingShift && !bHoldingCtrl )
		{
			c = (char)toupper(c);

			switch( c )
			{
			case '`':	c='~';	break;
			case '1':	c='!';	break;
			case '2':	c='@';	break;
			case '3':	c='#';	break;
			case '4':	c='$';	break;
			case '5':	c='%';	break;
			case '6':	c='^';	break;
			case '7':	c='&';	break;
			case '8':	c='*';	break;
			case '9':	c='(';	break;
			case '0':	c=')';	break;
			case '-':	c='_';	break;
			case '=':	c='+';	break;
			case '[':	c='{';	break;
			case ']':	c='}';	break;
			case '\'':	c='"';	break;
			case '\\':	c='|';	break;
			case ';':	c=':';	break;
			case ',':	c='<';	break;
			case '.':	c='>';	break;
			case '/':	c='?';	break;
			}
		}
		if( (c >= ' ') && (!bHoldingCtrl) )
		{
			m_sTextInput += c;
			UpdateTextInput();
		}
		break;
	}
	Screen::Input( DeviceI, type, GameI, MenuI, StyleI );	// default input handler
}

void ScreenNetSelectBase::HandleScreenMessage( const ScreenMessage SM )
{
	Screen::HandleScreenMessage( SM );
	switch( SM )
	{
	case SM_GoToPrevScreen:
		SCREENMAN->SetNewScreen( THEME->GetMetric (m_sName, "PrevScreen") );
		break;
	case SM_GoToNextScreen:
		SOUND->StopMusic();
		SCREENMAN->SetNewScreen( THEME->GetMetric (m_sName, "NextScreen") );
		break;
	case SM_AddToChat:
		{
			m_textOutHidden.SetText( NSMAN->m_sChatText );
			vector <wstring> wLines;
			m_textOutHidden.GetLines( wLines );
			m_actualText = "";
			for (unsigned i = max(int(wLines.size()) - SHOW_CHAT_LINES, 0 ) ; i < wLines.size() ; ++i)
				m_actualText += WStringToCString( wLines[i] )+'\n';
			m_textChatOutput.SetText( m_actualText );
			break;
		}
	case SM_UsersUpdate:
		UpdateUsers();
		break;

	}

}

void ScreenNetSelectBase::TweenOffScreen()
{
	OFF_COMMAND( m_rectChatInputBox );
	OFF_COMMAND( m_rectChatOutputBox );
	OFF_COMMAND( m_textChatInput );
	OFF_COMMAND( m_textChatOutput );

	for( unsigned i=0; i<m_textUsers.size(); i++ )
		OFF_COMMAND( m_textUsers[i] );

	OFF_COMMAND( m_rectUsersBG );
}

void ScreenNetSelectBase::UpdateTextInput()
{
	m_textChatInput.SetText( m_sTextInput );  
}

void ScreenNetSelectBase::UpdateUsers()
{
	float tX = USERS_X - USERSDELT_X;
	float tY = USERS_Y;

	for( unsigned i=0; i< m_textUsers.size(); i++)
		this->RemoveChild( &m_textUsers[i] );

	unsigned oldUsers = m_textUsers.size();

	m_textUsers.clear();

	m_textUsers.resize( NSMAN->m_ActivePlayer.size() );

	for( unsigned i=0; i < NSMAN->m_ActivePlayer.size(); i++)
	{
		m_textUsers[i].LoadFromFont( THEME->GetPathF(m_sName,"chat") );
		m_textUsers[i].SetHorizAlign( align_center );
		m_textUsers[i].SetVertAlign( align_top );
		m_textUsers[i].SetShadowLength( 0 );
		m_textUsers[i].SetName( "Users" );
		
		tX += USERSDELT_X;

		if ( (i % 2) == 1)
			tY = USERSALT_Y + USERS_Y;
		else
			tY = USERS_Y;
		m_textUsers[i].SetXY( tX, tY );

		if ( i > oldUsers )
			ON_COMMAND( m_textUsers[i] );
	
		m_textUsers[i].SetText( NSMAN->m_PlayerNames[NSMAN->m_ActivePlayer[i]] );
		m_textUsers[i].SetDiffuseColor ( THEME->GetMetricC( m_sName,
			ssprintf("Users%dColor", NSMAN->m_PlayerStatus[NSMAN->m_ActivePlayer[i]] ) ) );

		this->AddChild( &m_textUsers[i] );
	}
}

void UtilSetQuadInit( Actor& actor, const CString &sClassName )
{
	UtilSetXYAndOnCommand( actor, sClassName );
	actor.SetDiffuse( THEME->GetMetricC( sClassName, actor.m_sName + "Color" ) );
	actor.SetWidth( THEME->GetMetricF( sClassName, actor.m_sName + "Width" ) );
	actor.SetHeight( THEME->GetMetricF( sClassName, actor.m_sName + "Height" ) );
}

#endif

/*
 * (c) 2004 Charles Lohr
 * All rights reserved.
 *      Elements from ScreenTextEntry
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
