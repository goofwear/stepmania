#ifndef INPUTFILTER_H
#define INPUTFILTER_H
/*
-----------------------------------------------------------------------------
 Class: InputFilter

 Desc: Checks RageInput and generates a list of InputEvents, which can 
	represent when a button is first pressed, when a button is released,
	or when an repeating input fires.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "RageInputDevice.h"

const float TIME_BEFORE_SLOW_REPEATS = 0.25f;
const float TIME_BEFORE_FAST_REPEATS = 1.5f;

//const float TIME_BETWEEN_SLOW_REPEATS = 0.25f;
const float TIME_BETWEEN_SLOW_REPEATS = 0.125f;
const float TIME_BETWEEN_FAST_REPEATS = 0.125f;

enum InputEventType { IET_FIRST_PRESS, IET_SLOW_REPEAT, IET_FAST_REPEAT, IET_RELEASE };

struct InputEvent : public DeviceInput
{
	InputEvent() { type=IET_FIRST_PRESS; };
	InputEvent( InputDevice d, int b, InputEventType t ): DeviceInput(d, b) { type=t; };
	InputEvent( DeviceInput di, InputEventType t ): DeviceInput(di) { type=t; };

	InputEventType type;
};

typedef vector<InputEvent> InputEventArray;

class RageMutex;
class InputFilter
{
	bool m_BeingHeld[NUM_INPUT_DEVICES][NUM_DEVICE_BUTTONS];
	bool m_BeingForced[NUM_INPUT_DEVICES][NUM_DEVICE_BUTTONS];
	float m_fSecsHeld[NUM_INPUT_DEVICES][NUM_DEVICE_BUTTONS];

	/* If > 0, then when it reaches 0, stop forcing. */
	float m_fSecsToForce[NUM_INPUT_DEVICES][NUM_DEVICE_BUTTONS];
	InputEventArray queue;
	RageMutex *queuemutex;

	void ForceKey( DeviceInput di, float duration );
	void StopForcingKey( DeviceInput di );

public:
	void ButtonPressed( DeviceInput di, bool Down );
	void ResetDevice( InputDevice dev );

	InputFilter();
	~InputFilter();
	void Reset();
	void Update(float fDeltaTime);

	bool IsBeingPressed( DeviceInput di );
	float GetSecsHeld( DeviceInput di );
	
	void GetInputEvents( InputEventArray &array );
};


extern InputFilter*	INPUTFILTER;	// global and accessable from anywhere in our program



#endif
