#include "global.h"

#define __glext_h_

#if defined(WIN32)
#include <windows.h>
#endif

#if !defined(DARWIN)
# include <GL/gl.h>
# include <GL/glu.h>
#else
# include <OpenGL/gl.h>
# include <OpenGL/glu.h>
#endif

#undef __glext_h_
#include "glext.h"

#include "RageDisplay_OGL_Extensions.h"
#include "RageLog.h"
#include "arch/LowLevelWindow/LowLevelWindow.h"

GLExt_t GLExt;

bool HasExtension( CString ext );

#define F(n)  { (void **) &GLExt.n , #n }

struct func_t
{
	void **p;
	const char *name;
};

static bool LoadAllOrNothing( struct func_t *funcs, LowLevelWindow *pWind )
{
	bool bGotAll = true;
	for( unsigned i = 0; funcs[i].p != NULL; ++i )
	{
		*funcs[i].p = pWind->GetProcAddress( funcs[i].name );
		if( *funcs[i].p == NULL )
		{
			bGotAll = false;
			break;
		}
	}

	if( bGotAll )
		return true;

	/* If any function in the array wasn't found, clear them all. */
	for( unsigned i = 0; funcs[i].p != NULL; ++i )
		*funcs[i].p = NULL;

	return false;
}

void GLExt_t::Load( LowLevelWindow *pWind )
{
	memset( this, 0, sizeof(*this) );

	m_bEXT_texture_env_combine = HasExtension("GL_EXT_texture_env_combine");
	m_bGL_EXT_bgra = HasExtension("GL_EXT_bgra");

#if defined(WIN32)
	if( HasExtension("WGL_EXT_swap_control") )
		wglSwapIntervalEXT = (PWSWAPINTERVALEXTPROC) pWind->GetProcAddress("wglSwapIntervalEXT");
#elif defined(DARWIN)
	wglSwapIntervalEXT = wglSwapIntervalEXT;
#endif

	if( HasExtension("GL_EXT_paletted_texture") )
	{
		glColorTableEXT = (PFNGLCOLORTABLEPROC) pWind->GetProcAddress("glColorTableEXT");
		glGetColorTableParameterivEXT = (PFNGLCOLORTABLEPARAMETERIVPROC) pWind->GetProcAddress("glGetColorTableParameterivEXT");
	}

	if( HasExtension("GL_ARB_multitexture") )
		glActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC) pWind->GetProcAddress("glActiveTextureARB");

	/*
	 * Find extension functions.
	 *
	 * X11R6.7.0 (or possibly ATI's drivers) seem to be returning bogus values for glBindBufferARB
	 * if we don't actually check for GL_ARB_vertex_buffer_object. 
	 * https://sf.net/tracker/download.php?group_id=37892&atid=421366&file_id=88086&aid=958820
	 * https://sf.net/tracker/download.php?group_id=37892&atid=421366&file_id=85542&aid=944836
	 *
	 * Let's check them all, to be safe.
	 */
	if( HasExtension("GL_ARB_vertex_buffer_object") )
	{
		func_t funcs[] = {
			F( glGenBuffersARB ),
			F( glBindBufferARB ),
			F( glBufferDataARB ),
			F( glBufferSubDataARB ),
			F( glDeleteBuffersARB ),
			{ NULL, NULL },
		};

		LoadAllOrNothing( funcs, pWind );
	}

	if( HasExtension("GL_EXT_draw_range_elements") )
		GLExt.glDrawRangeElements = (PFNGLDRAWRANGEELEMENTSPROC) pWind->GetProcAddress("glDrawRangeElements");

	m_bGL_ARB_shader_objects = HasExtension("GL_ARB_shader_objects");
	if( m_bGL_ARB_shader_objects )
	{
		func_t funcs[] = {
			F( glCreateShaderObjectARB ),
			F( glCreateShaderObjectARB ),
			F( glCreateProgramObjectARB ),
			F( glShaderSourceARB ),
			F( glCompileShaderARB ),
			F( glGetObjectParameterfvARB ),
			F( glGetObjectParameterivARB ),
			F( glGetInfoLogARB ),
			F( glAttachObjectARB ),
			F( glDeleteObjectARB ),
			F( glLinkProgramARB ),
			F( glUseProgramObjectARB ),
			F( glVertexAttrib2fARB ),
			F( glVertexAttrib3fARB ),
			F( glVertexAttrib4fARB ),
			F( glEnableVertexAttribArrayARB ),
			F( glDisableVertexAttribArrayARB ),
			F( glVertexAttribPointerARB ),
			{ NULL, NULL }
		};

		if( !LoadAllOrNothing(funcs, pWind) )
			m_bGL_ARB_shader_objects = false;
	}

	m_bGL_ARB_vertex_shader = m_bGL_ARB_shader_objects && HasExtension("GL_ARB_vertex_shader");
	if( m_bGL_ARB_vertex_shader )
	{
		func_t funcs[] =
		{
			F( glBindAttribLocationARB ),
			{ NULL, NULL }
		};
		if( !LoadAllOrNothing(funcs, pWind) )
			m_bGL_ARB_vertex_shader = false;
	}

	m_bGL_ARB_shading_language_100 = HasExtension("GL_ARB_shading_language_100");
	if( m_bGL_ARB_shading_language_100 )
	{
		while( glGetError() != GL_NO_ERROR )
			;
		const char *pzVersion = (const char *) glGetString( GL_SHADING_LANGUAGE_VERSION );

		GLenum glError = glGetError();
		if( glError == GL_INVALID_ENUM )
		{
			LOG->Info( "No GL_SHADING_LANGUAGE_VERSION; assuming 1.0" );
			m_iShadingLanguageVersion = 100;
		}
		else
		{
			const float fVersion = strtof( pzVersion, NULL );
			m_iShadingLanguageVersion = int(roundf(fVersion * 100));
			/* The version string may contain extra information beyond the version number. */
			LOG->Info( "OpenGL shading language: %s", pzVersion );
		}
	}
}
