#import <Cocoa/Cocoa.h>
#import "ProductInfo.h"
#import "archutils/Darwin/SMMainThread.h"

static NSWindow *g_window = nil;
static NSTextView *g_text = nil;

void MakeNewCocoaWindow( const void *data, unsigned length )
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSImage *image = nil;
	
	if( length )
	{
		NSData *d = [NSData dataWithBytes:data length:length];
		
		image = [[[NSImage alloc] initWithData:d] autorelease];
	}
	if( !image )
		return;
	
	NSView *view;
	NSSize size = [image size];
	NSRect viewRect, windowRect;
	float height = 0.0f;

	NSFont *font = [NSFont systemFontOfSize:0.0f];
	NSRect textRect;
	// Just give it a size until it is created.
	textRect = NSMakeRect( 0, 0, size.width, size.height );
	g_text = [[[NSTextView alloc] initWithFrame:textRect] autorelease];
	[g_text setFont:font];
	height = [[g_text layoutManager] defaultLineHeightForFont:font]*3 + 4;
	textRect = NSMakeRect( 0, 0, size.width, height );
	
	[g_text setFrame:textRect];
	[g_text setEditable:NO];
	[g_text setSelectable:NO];
	[g_text setDrawsBackground:YES];
	[g_text setBackgroundColor:[NSColor lightGrayColor]];
	[g_text setAlignment:NSCenterTextAlignment];
	[g_text setString:@"Initializing Hardware..."];

	viewRect = NSMakeRect(0, height, size.width, size.height);
	NSImageView *iView = [[[NSImageView alloc] initWithFrame:viewRect] autorelease];
	[iView setImage:image];
	[iView setImageFrameStyle:NSImageFrameNone];

	windowRect = NSMakeRect( 0, 0, size.width, size.height + height );
	g_window = [[NSWindow alloc] initWithContentRect:windowRect
					     styleMask:NSTitledWindowMask
					       backing:NSBackingStoreBuffered
						 defer:YES];
	
	SMMainThread *mt = [[SMMainThread alloc] init];
	view = [g_window contentView];

	// Set some properties.
	ADD_ACTIONb( mt, g_window, setOneShot:, YES );
	ADD_ACTIONb( mt, g_window, setExcludedFromWindowsMenu:, YES );
	ADD_ACTIONb( mt, g_window, useOptimizedDrawing:, YES );
	ADD_ACTION1( mt, g_window, setTitle:, @PRODUCT_FAMILY );
	ADD_ACTION0( mt, g_window, center );
	// Set subviews.
	ADD_ACTION1( mt, view, addSubview:, g_text );
	ADD_ACTION1( mt, view, addSubview:, iView );
	// Make key and order front.
	ADD_ACTION1( mt, g_window, makeKeyAndOrderFront:, nil );
	
	// Perform all of the actions in order on the main thread.
	[mt performOnMainThread];
	[mt release];
	
	[pool release];
}

void DisposeOfCocoaWindow()
{
	// Just in case performSelectorOnMainThread:withObject:waitUntilDone: needs an autorelease pool.
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	 // Released when closed as controlled by setReleasedWhenClosed.
	[g_window performSelectorOnMainThread:@selector(close) withObject:nil waitUntilDone:NO];
	[pool release];
}

void SetCocoaWindowText(const char *s)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSString *str = [NSString stringWithUTF8String:s];
	[g_text performSelectorOnMainThread:@selector(setString:) withObject:(str ? str : @"") waitUntilDone:NO];
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
