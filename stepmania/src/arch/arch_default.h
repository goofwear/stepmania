#ifndef SDL_ARCH_H
#define SDL_ARCH_H

/* All driver types should have a default, portable implementation, provided
 * here. */
/* None of these SDL implementations run on Xbox, so I've commented 
 * out use of this header and added the #includes to each arch_*.h */
/* 
 * The basic design principle of arch is that a new platform can be up and running
 * quickly using only arch_default, with OpenGL graphics and SDL input, and things can
 * be overloaded later.  That way, people don't have to learn how arch works
 * until after they have the basic stuff running.
 *
 * For Xbox, let's leave this file alone, and special case stuff in arch_xbox.h.
 *
 * For LowLevelWindow, just undefine SUPPORT_OPENGL and don't link any LLW at
 * all.

 * For InputHandler, undef SUPPORT_SDL_INPUT and we won't create the SDL input
 * handler.  (We'll still include the header here, but it'll never be used.)
 */

/* We can have any number of input drivers.  By default, use SDL for keyboards
 * and joysticks.  If you write your own handler for these devices types, or if
 * SDL input doesn't work on your platform, undef SUPPORT_SDL_INPUT in your platform
 * header. */
#define SUPPORT_SDL_INPUT
#include "InputHandler/InputHandler_SDL.h"

/* Load default fallback drivers; some of these may be overridden by arch-specific
 * drivers.  These are all singleton drivers--we never use more than one. */
/* XXX: null, none, Null--pick one */
#include "LoadingWindow/LoadingWindow_Null.h"
#include "ArchHooks/ArchHooks_none.h"
#include "Sound/RageSoundDriver_Null.h"
#include "Lights/LightsDriver_Null.h"

#if defined(SUPPORT_OPENGL)
#include "LowLevelWindow/LowLevelWindow_SDL.h"
#endif

#endif

/*
 * Copyright (c) 2002 by the person(s) listed below.  All rights reserved.
 *
 * Glenn Maynard
 */
