/* ProductInfo - Branding strings. */

#ifndef PRODUCT_INFO_H
#define PRODUCT_INFO_H

// Don't forget to also change ProductInfo.inc!

// Change these three.
#define PRODUCT_FAMILY_BARE StepMania
#define PRODUCT_ID_BARE StepMania4
// String used for the install directory and registry locations
#define PRODUCT_VER_BARE 4.0.b6

// These cannot be #undef'd so make them unlikely to conflict with anything
#define PRODUCT_STRINGIFY(x) #x
#define PRODUCT_XSTRINGIFY(x) PRODUCT_STRINGIFY(x)

#define PRODUCT_FAMILY		PRODUCT_XSTRINGIFY(PRODUCT_FAMILY_BARE)
#define PRODUCT_ID		PRODUCT_XSTRINGIFY(PRODUCT_ID_BARE)
#define PRODUCT_VER		PRODUCT_XSTRINGIFY(PRODUCT_VER_BARE)
#define PRODUCT_ID_VER		PRODUCT_VER
#define PRODUCT_DISPLAY		PRODUCT_FAMILY " " PRODUCT_VER

#define VIDEO_TROUBLESHOOTING_URL "http://www.stepmania.com/wiki/Video_Driver_Troubleshooting"
#define REPORT_BUG_URL "http://sourceforge.net/tracker/?func=add&group_id=37892&atid=421366"

#define CAN_INSTALL_PACKAGES true

#endif

/*
 * (c) 2003-2005 Chris Danford
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

