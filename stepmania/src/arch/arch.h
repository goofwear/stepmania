#ifndef ARCH_H
#define ARCH_H

/* Include this file if you need to create an instance of a driver object.  */
class LoadingWindow;
class RageSoundDriver;
class ArchHooks;
class InputHandler;
class LowLevelWindow;
class LightsDriver;

LoadingWindow *MakeLoadingWindow();
ArchHooks *MakeArchHooks();
LowLevelWindow *MakeLowLevelWindow();

void MakeInputHandlers(vector<InputHandler *> &Add);
RageSoundDriver *MakeRageSoundDriver(CString drivers);
LightsDriver *MakeLightsDriver(CString driver);

/* These definitions are in here, instead of in arch_*.h, because they
 * need to be available to other modules.  It'd be overkill to create separate
 * "config" and "driver" headers for each arch. */

/* Define the default list of sound drivers for each arch.  It's
 * OK to list drivers that may not be available. */
#if defined(LINUX)
	#define DEFAULT_SOUND_DRIVER_LIST "ALSA9,ALSA9-sw,OSS"
#elif defined(DARWIN)
	#define DEFAULT_SOUND_DRIVER_LIST "CoreAudio,QT,QT1"
#elif defined(_WINDOWS)
	#define DEFAULT_SOUND_DRIVER_LIST "DirectSound,DirectSound-sw,WaveOut"
#elif defined(_XBOX)
	#define DEFAULT_SOUND_DRIVER_LIST "DirectSound"
#else
	#define DEFAULT_SOUND_DRIVER_LIST "Null"
#endif

#define DEFAULT_MOVIE_DRIVER_LIST "FFMpeg,DShow,Null"

/* Choose your renderers. */
#if defined(_WINDOWS)
#define SUPPORT_OPENGL
#define SUPPORT_D3D
#elif defined(_XBOX)
#define SUPPORT_D3D
#else
#define SUPPORT_OPENGL
#endif

/* Hack for Xbox: All paths must be absolute. */
#if defined(_XBOX)
	#define SYS_BASE_PATH "D:\\"
#else
	#define SYS_BASE_PATH ""
#endif
#define BASE_PATH "" /* XXX remove */

#endif

/*
 * Copyright (c) 2002-2003 by the person(s) listed below.  All rights reserved.
 *
 * Glenn Maynard
 */
