#ifndef DARWIN_CRASH_H
#define DARWIN_CRASH_H
/*
 *  Crash.h
 *  stepmania
 *
 *  Created by Steve Checkoway on Thu Jul 31 2003.
 *  Copyright (c) 2003 Steve Checkoway. All rights reserved.
 *
 */

#include <MachineExceptions.h>

OSStatus HandleException(ExceptionInformation *theException);
void *GetCrashedFramePtr();
void InformUserOfCrash( const CString &sPath );

#endif /* DARWIN_CRASH_H */
