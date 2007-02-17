#ifndef RAGE_DISPLAY_OGL_HELPERS_H
#define RAGE_DISPLAY_OGL_HELPERS_H

/* ours may be more up-to-date */
#define __glext_h_

#if defined(WIN32)
#include <windows.h>
#endif

#if !defined(MACOSX)
# include <GL/gl.h>
# include <GL/glu.h>
#else
# include <OpenGL/gl.h>
# include <OpenGL/glu.h>
#endif

#undef __glext_h_
#include "glext.h"

/* Import RageDisplay, for types.  Do not include RageDisplay_OGL.h. */
#include "RageDisplay.h"

/* Windows defines GL_EXT_paletted_texture incompletely: */
#ifndef GL_TEXTURE_INDEX_SIZE_EXT
#define GL_TEXTURE_INDEX_SIZE_EXT         0x80ED
#endif

/* Not in glext.h: */
typedef bool (APIENTRY * PWSWAPINTERVALEXTPROC) (int interval);

namespace RageDisplay_OGL_Helpers
{
	void Init();
	RString GLToString( GLenum e );
};

class RenderTarget
{
public:
	virtual ~RenderTarget() { }
	virtual void Create( const RenderTargetParam &param, int &iTextureWidthOut, int &iTextureHeightOut ) = 0;

	virtual unsigned GetTexture() const = 0;

	/* Render to this RenderTarget. */
	virtual void StartRenderingTo() = 0;

	/* Stop rendering to this RenderTarget.  Update the texture, if necessary, and
	 * make it available. */
	virtual void FinishRenderingTo() = 0;

	virtual bool InvertY() const { return false; }

	const RenderTargetParam &GetParam() const { return m_Param; }

protected:
	RenderTargetParam m_Param;
};

class LowLevelWindow;
struct GLExt_t
{
	bool m_bARB_texture_env_combine;
	bool m_bEXT_texture_env_combine;
	bool m_bGL_EXT_bgra;

	PWSWAPINTERVALEXTPROC wglSwapIntervalEXT;
	PFNGLCOLORTABLEPROC glColorTableEXT;
	PFNGLBLENDEQUATIONPROC glBlendEquation;
	PFNGLCOLORTABLEPARAMETERIVPROC glGetColorTableParameterivEXT;
	PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;
	PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTextureARB;
	PFNGLGENBUFFERSARBPROC glGenBuffersARB;
	PFNGLBINDBUFFERARBPROC glBindBufferARB;
	PFNGLBUFFERDATAARBPROC glBufferDataARB;
	PFNGLBUFFERSUBDATAARBPROC glBufferSubDataARB;
	PFNGLDELETEBUFFERSARBPROC glDeleteBuffersARB;
	PFNGLDRAWRANGEELEMENTSPROC glDrawRangeElements;

	// GL_ARB_shader_objects:
	bool m_bGL_ARB_shader_objects;
	PFNGLCREATESHADEROBJECTARBPROC glCreateShaderObjectARB;
	PFNGLCREATEPROGRAMOBJECTARBPROC glCreateProgramObjectARB;
	PFNGLSHADERSOURCEARBPROC glShaderSourceARB;
	PFNGLCOMPILESHADERARBPROC glCompileShaderARB;
	PFNGLGETOBJECTPARAMETERFVARBPROC glGetObjectParameterfvARB;
	PFNGLGETOBJECTPARAMETERIVARBPROC glGetObjectParameterivARB;
	PFNGLGETINFOLOGARBPROC glGetInfoLogARB;
	PFNGLATTACHOBJECTARBPROC glAttachObjectARB;
	PFNGLDELETEOBJECTARBPROC glDeleteObjectARB;
	PFNGLLINKPROGRAMARBPROC glLinkProgramARB;
	PFNGLUSEPROGRAMOBJECTARBPROC glUseProgramObjectARB;
	PFNGLVERTEXATTRIB2FARBPROC glVertexAttrib2fARB;
	PFNGLVERTEXATTRIB3FARBPROC glVertexAttrib3fARB;
	PFNGLVERTEXATTRIB4FARBPROC glVertexAttrib4fARB;
	PFNGLENABLEVERTEXATTRIBARRAYARBPROC glEnableVertexAttribArrayARB;
	PFNGLDISABLEVERTEXATTRIBARRAYARBPROC glDisableVertexAttribArrayARB;
	PFNGLVERTEXATTRIBPOINTERARBPROC glVertexAttribPointerARB;
	PFNGLGETUNIFORMLOCATIONARBPROC glGetUniformLocationARB;
	PFNGLUNIFORM1FARBPROC glUniform1fARB;
	PFNGLUNIFORM2FARBPROC glUniform2fARB;
	PFNGLUNIFORM3FARBPROC glUniform3fARB;
	PFNGLUNIFORM4FARBPROC glUniform4fARB;
	PFNGLUNIFORM1IARBPROC glUniform1iARB;
	PFNGLUNIFORM2IARBPROC glUniform2iARB;
	PFNGLUNIFORM3IARBPROC glUniform3iARB;
	PFNGLUNIFORM4IARBPROC glUniform4iARB;
	PFNGLUNIFORM1FVARBPROC glUniform1fvARB;
	PFNGLUNIFORM2FVARBPROC glUniform2fvARB;
	PFNGLUNIFORM3FVARBPROC glUniform3fvARB;
	PFNGLUNIFORM4FVARBPROC glUniform4fvARB;
	PFNGLUNIFORM1IVARBPROC glUniform1ivARB;
	PFNGLUNIFORM2IVARBPROC glUniform2ivARB;
	PFNGLUNIFORM3IVARBPROC glUniform3ivARB;
	PFNGLUNIFORM4IVARBPROC glUniform4ivARB;
	PFNGLUNIFORMMATRIX2FVARBPROC glUniformMatrix2fvARB;
	PFNGLUNIFORMMATRIX3FVARBPROC glUniformMatrix3fvARB;
	PFNGLUNIFORMMATRIX4FVARBPROC glUniformMatrix4fvARB;

	// GL_ARB_vertex_shader and GL_ARB_fragment_shader:
	bool m_bGL_ARB_vertex_shader;
	bool m_bGL_ARB_fragment_shader;
	PFNGLBINDATTRIBLOCATIONARBPROC glBindAttribLocationARB;

	bool m_bGL_ARB_shading_language_100;
	int m_iShadingLanguageVersion; /* * 100 */

	// GL_EXT_framebuffer_object:
	bool m_bGL_EXT_framebuffer_object;
	PFNGLISRENDERBUFFEREXTPROC glIsRenderbufferEXT;
	PFNGLBINDRENDERBUFFEREXTPROC glBindRenderbufferEXT;
	PFNGLDELETERENDERBUFFERSEXTPROC glDeleteRenderbuffersEXT;
	PFNGLGENRENDERBUFFERSEXTPROC glGenRenderbuffersEXT;
	PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorageEXT;
	PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC glGetRenderbufferParameterivEXT;
	PFNGLISFRAMEBUFFEREXTPROC glIsFramebufferEXT;
	PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT;
	PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT;
	PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT;
	PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT;
	PFNGLFRAMEBUFFERTEXTURE1DEXTPROC glFramebufferTexture1DEXT;
	PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT;
	PFNGLFRAMEBUFFERTEXTURE3DEXTPROC glFramebufferTexture3DEXT;
	PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbufferEXT;
	PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glGetFramebufferAttachmentParameterivEXT;
	PFNGLGENERATEMIPMAPEXTPROC glGenerateMipmapEXT;

	void Load( LowLevelWindow *pWind );

	bool HasExtension( const RString &sExt ) const;
};

extern GLExt_t GLExt;

#endif

/*
 * Copyright (c) 2001-2005 Chris Danford, Glenn Maynard
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
