#include "ProductInfo.h"

void InformUserOfCrash( const char *sPath )
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSString *sQuit = NSLocalizedString( @"Quit " PRODUCT_FAMILY, @"Button name" );
	NSString *s = @PRODUCT_FAMILY " has crashed. Debugging information has been output to\n\n%s\n\n"
	@"Please file a bug report at\n\n" REPORT_BUG_URL;
	
	int ret = NSRunCriticalAlertPanel( @PRODUCT_FAMILY " has crashed", s, @"File Bug Report",
					   sQuit, @"Open crashinfo.txt", sPath );
	
	NSWorkspace *ws = [NSWorkspace sharedWorkspace];
	
	switch( ret )
	{
	case NSAlertDefaultReturn:
		[ws openURL:[NSURL URLWithString:@REPORT_BUG_URL]]; // fall through
	case NSAlertOtherReturn:
		[ws openFile:[NSString stringWithUTF8String:sPath]];
	}
	[pool release];
}

/*
 * (c) 2003-2006 Steve Checkoway
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
