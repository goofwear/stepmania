#include "global.h"

/* This input handler checks for SDL device input messages and sends them off to
 * InputFilter.  If we add mouse support, it should go here, too.
 *
 * Note that the SDL video event systems are interlinked--you can't use events
 * until video is initialized.  So, if we aren't using SDL for video, we can't
 * use SDL events at all, and so we lose input support, too.  In that case, you'll
 * need to write other input handlers for those cases and load them instead of this.
 * (Part of this is probably because, in Windows, you need a window to get input.)
 */
#include "InputHandler_SDL.h"
#include "SDL_utils.h"
#include "RageLog.h"
#include "RageDisplay.h"
#include "PrefsManager.h"

static const Sint8 Handled_SDL_Events[] = {
	SDL_KEYDOWN, SDL_KEYUP, SDL_JOYBUTTONDOWN, SDL_JOYBUTTONUP,
	SDL_JOYAXISMOTION, SDL_JOYHATMOTION, -1
};
static int SDL_EventMask;

InputHandler_SDL::InputHandler_SDL()
{
	SDL_InitSubSystem( SDL_INIT_JOYSTICK );

#ifdef _XBOX
	//strange hardware timing issue with 3rd party controllers
	Sleep(750);
#endif

	SDL_EnableKeyRepeat( 0, 0 );

	/* We can do this to get Unicode values in the key struct, which (with
	 * a little more work) will make us work better on international keyboards.
	 * Not all archs support this well. */
	// SDL_EnableUNICODE( 1 );

	//
	// Init joysticks
	//
	int iNumJoySticks = min( SDL_NumJoysticks(), NUM_JOYSTICKS );
	LOG->Info( "Found %d joysticks", iNumJoySticks );
	int i;
	for( i=0; i<iNumJoySticks; i++ )
	{
		SDL_Joystick *pJoystick = SDL_JoystickOpen( i );

		if(pJoystick == NULL) {
			LOG->Info("   %d: '%s' Error opening: %s",
				i, SDL_JoystickName(i), SDL_GetError());
			continue;
		}

		LOG->Info( "   %d: '%s' axes: %d, hats: %d, buttons: %d",
			i,
			SDL_JoystickName(i),
			SDL_JoystickNumAxes(pJoystick),
			SDL_JoystickNumHats(pJoystick),
			SDL_JoystickNumButtons(pJoystick) );

		/* For some weird reason, we won't get any joystick events at all
		 * if we don't keep the joystick open.  (Why?  The joystick event
		 * API is completely separate from the SDL_Joystick polling API ...) */
		Joysticks.push_back(pJoystick);
	}

	for(i = 0; Handled_SDL_Events[i] != -1; ++i)
	{
		mySDL_EventState(Handled_SDL_Events[i], SDL_ENABLE);
		SDL_EventMask |= SDL_EVENTMASK(Handled_SDL_Events[i]);
	}
}

InputHandler_SDL::~InputHandler_SDL()
{
	unsigned i;
	for(i = 0; i < Joysticks.size(); ++i)
		SDL_JoystickClose(Joysticks[i]);

	SDL_QuitSubSystem( SDL_INIT_JOYSTICK );

	for(i = 0; Handled_SDL_Events[i] != -1; ++i)
		mySDL_EventState(Handled_SDL_Events[i], SDL_IGNORE);
}

void InputHandler_SDL::Update(float fDeltaTime)
{
#ifdef _XBOX
	static int lastValX1 = 0;
	static int lastValY1 = 0;
	static int lastValX2 = 0;
	static int lastValY2 = 0;
	static int resolutionChanged = 0;
#endif

	SDL_Event event;
	while(SDL_GetEvent(event, SDL_EventMask))
	{
		switch(event.type)
		{
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			{
			DeviceInput di(DEVICE_KEYBOARD, event.key.keysym.sym);

			/* SDL is inconsistent about this key: */
			if( di.button == SDLK_SYSREQ )
				di.button = SDLK_PRINT;

			ButtonPressed(di, event.key.state == SDL_PRESSED);
			}
			continue;

		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
		{
			InputDevice i = InputDevice(DEVICE_JOY1 + event.jbutton.which);
			JoystickButton Button = JoystickButton(JOY_1 + event.jbutton.button);
			if(Button >= NUM_JOYSTICK_BUTTONS)
			{
				LOG->Warn("Ignored joystick event (button too high)");
				continue;
			}
			DeviceInput di(i, Button);
			ButtonPressed(di, event.jbutton.state == SDL_PRESSED);
			continue;
		}
		
		case SDL_JOYAXISMOTION:
		{
#ifdef _XBOX
			/* XXX: If you want to adjust the screen, use the menu.  Screen
			 * adjustment code doesn't belong in the input driver. */
			resolutionChanged = 1;
			if ( abs(event.jaxis.value) > 15600 )
			{
				switch ( event.jaxis.axis )
				{
				case 0: lastValX1 = event.jaxis.value; break;
				case 1: lastValY1 = event.jaxis.value; break;
				case 2: lastValX2 = event.jaxis.value; break;
				case 3: lastValY2 = event.jaxis.value; break;
				}
			}
			else
			{
				switch ( event.jaxis.axis )
				{
				case 0: lastValX1 = 0; break;
				case 1: lastValY1 = 0; break;
				case 2: lastValX2 = 0; break;
				case 3: lastValY2 = 0; break;
				}
			}
#else
			InputDevice i = InputDevice(DEVICE_JOY1 + event.jaxis.which);
			JoystickButton neg = (JoystickButton)(JOY_LEFT+2*event.jaxis.axis);
			JoystickButton pos = (JoystickButton)(JOY_RIGHT+2*event.jaxis.axis);
			ButtonPressed(DeviceInput(i, neg), event.jaxis.value < -16000);
			ButtonPressed(DeviceInput(i, pos), event.jaxis.value > +16000);
#endif
			continue;
		}
		
		case SDL_JOYHATMOTION:
		{
			InputDevice i = InputDevice(DEVICE_JOY1 + event.jhat.which);
			ButtonPressed(DeviceInput(i, JOY_HAT_UP), !!(event.jhat.value & SDL_HAT_UP));
			ButtonPressed(DeviceInput(i, JOY_HAT_DOWN), !!(event.jhat.value & SDL_HAT_DOWN));
			ButtonPressed(DeviceInput(i, JOY_HAT_LEFT), !!(event.jhat.value & SDL_HAT_LEFT));
			ButtonPressed(DeviceInput(i, JOY_HAT_RIGHT), !!(event.jhat.value & SDL_HAT_RIGHT));
			continue;
		}
		}
	}

#ifdef _XBOX

	if ( resolutionChanged )
	{
		if ( lastValX1 || lastValY1 || lastValX2 || lastValY2 )
		{
			PREFSMAN->m_fScreenPosX += ((float)lastValX1/32767.0f);
			if ( PREFSMAN->m_fScreenPosX < 0)
				PREFSMAN->m_fScreenPosX  = 0;

			PREFSMAN->m_fScreenPosY -= ((float)lastValY1/32767.0f);
			if ( PREFSMAN->m_fScreenPosY < 0)
				PREFSMAN->m_fScreenPosY  = 0;

			PREFSMAN->m_fScreenWidth += ((float)lastValX2/32767.0f);
			PREFSMAN->m_fScreenHeight -= ((float)lastValY2/32767.0f);

			DISPLAY->ResolutionChanged();
		}
		else
		{
			resolutionChanged = 0;
		}
	}
#endif

	InputHandler::UpdateTimer();
}


void InputHandler_SDL::GetDevicesAndDescriptions(vector<InputDevice>& vDevicesOut, vector<CString>& vDescriptionsOut)
{
	vDevicesOut.push_back( DEVICE_KEYBOARD );
	vDescriptionsOut.push_back( "Keyboard" );

	for( int i=0; i<SDL_NumJoysticks(); i++ )
	{
		if( SDL_JoystickOpened(i) )
		{
			vDevicesOut.push_back( InputDevice(DEVICE_JOY1+i) );
			vDescriptionsOut.push_back( SDL_JoystickName(i) );
		}
	}
}

/*
 * (c) 2001-2004 Chris Danford, Glenn Maynard
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
