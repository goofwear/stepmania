/* Manages our X connection and window. */
#ifndef X11_HELPER_H
#define X11_HELPER_H

#include <X11/Xlib.h>		// Window

#include "RageDisplay.h"	// RageDisplay

namespace X11Helper
{
	// Create the connection, if necessary; otherwise do some important
	// internal session-tracking stuff (so you should call this anyway).
	// True on success, false failure.
	bool Go();

	// Get the current Display (connection). Behavior is undefined if you
	// didn't call Go() with a successful result first.
	Display *Dpy();

	// (Re)create the window on the screen of this number with this depth,
	// this visual type, this width (optional -- you can resize the window
	// in your callback later), and this height (optional). Returns true on
	// success, false on failure.
	bool MakeWindow(int screenNum, int depth, Visual *visual, int width=64,
								int height=64);

	// Callback type.
	typedef void (*Callback_t)(Window*);

	//  TODO: We need to manage event masks here, to keep the InputHandler
	// and LowLevelWindow code separate.
	
	// Register a callback for new windows (including the initial window).
	// This callback will be called with 0 if I try to create a new window,
	// but fail. Returns false if for some really messed up reason it fails
	// (shouldn't happen), false otherwise
	bool Callback(Callback_t cb);

	// Destroy the connection, if appropriate; otherwise do some important
	// internal session-tracking stuff (so you should call it anyway).
	// True success, false failure.
	void Stop();
}

#endif

/*
 * (c) 2005 Ben Anderson
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
