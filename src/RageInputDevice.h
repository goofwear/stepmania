#ifndef RAGEINPUTDEVICE_H
#define RAGEINPUTDEVICE_H 1

#include "SDL_keysym.h"
#include "RageTimer.h"

const int NUM_KEYBOARD_BUTTONS = SDLK_LAST;
const int NUM_JOYSTICKS = 6;
const int NUM_JOYSTICK_HATS = 1;
const int NUM_PUMPS = 2;
const int NUM_PARAS = 2;

enum InputDevice {
	DEVICE_KEYBOARD = 0,
	DEVICE_JOY1,
	DEVICE_JOY2,
	DEVICE_JOY3,
	DEVICE_JOY4,
	DEVICE_JOY5,
	DEVICE_JOY6,
	DEVICE_PUMP1,
	DEVICE_PUMP2,
	DEVICE_PARA,
	NUM_INPUT_DEVICES,	// leave this at the end
	DEVICE_NONE			// means this is NULL
};

// button byte codes for directional pad
enum JoystickButton {
	JOY_LEFT = 0, JOY_RIGHT, 
	JOY_UP, JOY_DOWN,
	JOY_Z_UP, JOY_Z_DOWN,
	JOY_ROT_UP, JOY_ROT_DOWN, JOY_ROT_LEFT, JOY_ROT_RIGHT, JOY_ROT_Z_UP, JOY_ROT_Z_DOWN,
	JOY_HAT_LEFT, JOY_HAT_RIGHT, JOY_HAT_UP, JOY_HAT_DOWN, 
	JOY_AUX_1, JOY_AUX_2, JOY_AUX_3, JOY_AUX_4,
	JOY_1,	JOY_2,	JOY_3,	JOY_4,	JOY_5,	JOY_6,	JOY_7,	JOY_8,	JOY_9,	JOY_10,
	JOY_11,	JOY_12,	JOY_13,	JOY_14,	JOY_15,	JOY_16,	JOY_17,	JOY_18,	JOY_19,	JOY_20,
	JOY_21,	JOY_22,	JOY_23,	JOY_24, JOY_25, JOY_26, JOY_27, JOY_28, JOY_29, JOY_30,
	JOY_31,	JOY_32,
	NUM_JOYSTICK_BUTTONS	// leave this at the end
};

enum PumpButton {
	PUMP_UL,
	PUMP_UR,
	PUMP_MID,
	PUMP_DL,
	PUMP_DR,
	PUMP_ESCAPE,

	/* These buttons are for slave pads, attached to the first pad; they don't have
	 * their own USB device, and they have no escape button. */
	PUMP_2P_UL,
	PUMP_2P_UR,
	PUMP_2P_MID,
	PUMP_2P_DL,
	PUMP_2P_DR,
	NUM_PUMP_PAD_BUTTONS	// leave this at the end
};

enum ParaButton {
	PARA_L,
	PARA_UL,
	PARA_U,
	PARA_UR,
	PARA_R,
	PARA_SELECT,
	PARA_START,
	PARA_MENU_LEFT,
	PARA_MENU_RIGHT,
	NUM_PARA_PAD_BUTTONS	// leave this at the end
};


const int NUM_DEVICE_BUTTONS[NUM_INPUT_DEVICES] =
{
	NUM_KEYBOARD_BUTTONS, // DEVICE_KEYBOARD
	NUM_JOYSTICK_BUTTONS, // DEVICE_JOY1
	NUM_JOYSTICK_BUTTONS, // DEVICE_JOY2
	NUM_JOYSTICK_BUTTONS, // DEVICE_JOY3
	NUM_JOYSTICK_BUTTONS, // DEVICE_JOY4
	NUM_JOYSTICK_BUTTONS, // DEVICE_JOY5
	NUM_JOYSTICK_BUTTONS, // DEVICE_JOY6
	NUM_PUMP_PAD_BUTTONS, // DEVICE_PUMP1
	NUM_PUMP_PAD_BUTTONS, // DEVICE_PUMP2
	NUM_PARA_PAD_BUTTONS, // DEVICE_PARA
};

const int MAX_DEVICE_BUTTONS = NUM_KEYBOARD_BUTTONS;

struct DeviceInput
{
public:
	InputDevice device;
	int button;
	RageTimer ts;

	DeviceInput(): device(DEVICE_NONE), button(-1), ts(RageZeroTimer) { }
	DeviceInput( InputDevice d, int b ): device(d), button(b), ts(RageZeroTimer) { }
	DeviceInput( InputDevice d, int b, const RageTimer &t ):
		device(d), button(b), ts(t) { }

	bool operator==( const DeviceInput &other ) 
	{ 
		return device == other.device  &&  button == other.button;
	};

	CString GetDescription();
	
	CString toString();
	bool fromString( const CString &s );

	bool IsValid() const { return device != DEVICE_NONE; };
	void MakeInvalid() { device = DEVICE_NONE; };

	char ToChar() const;

	bool IsJoystick() const { return DEVICE_JOY1 <= device && device < DEVICE_JOY1+NUM_JOYSTICKS; }

	static int NumButtons(InputDevice device);
};

#endif
/*
 * Copyright (c) 2001-2002 Chris Danford
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
