#ifndef ERROR_DIALOG_WIN32_H
#define ERROR_DIALOG_WIN32_H

#include "ErrorDialog.h"

class ErrorDialog_Win32: public ErrorDialog
{
public:
	void ShowError( const CString &error );
};

#undef ARCH_ERROR_DIALOG
#define ARCH_ERROR_DIALOG ErrorDialog_Win32

#endif

/*
 * Copyright (c) 2002 by the person(s) listed below.  All rights reserved.
 *
 * Glenn Maynard
 */
