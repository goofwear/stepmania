#include "global.h"
#include "LuaManager.h"
#include "LuaFunctions.h"
#include "LuaReference.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "RageFile.h"
#include "arch/Dialog/Dialog.h"
#include "Foreach.h"

#include <csetjmp>
#include <cassert>

LuaManager *LUA = NULL;
static LuaFunctionList *g_LuaFunctions = NULL;

#if defined(_WINDOWS)
	#pragma comment(lib, "lua-5.0/lib/LibLua.lib")
	#pragma comment(lib, "lua-5.0/lib/LibLuaLib.lib")
#endif
#if defined(_XBOX)
	#pragma comment(lib, "lua-5.0/lib/LibLuaXbox.lib")
	#pragma comment(lib, "lua-5.0/lib/LibLuaLibXbox.lib")
#endif
#if defined(_WINDOWS) || defined (_XBOX)
	/* "interaction between '_setjmp' and C++ object destruction is non-portable"
	 * We don't care; we'll throw a fatal exception immediately anyway. */
	#pragma warning (disable : 4611)
#endif
#if defined (DARWIN)
	extern void NORETURN longjmp(jmp_buf env, int val);
#endif



struct ChunkReaderData
{
	const CString *buf;
	bool done;
	ChunkReaderData() { buf = NULL; done = false; }
};

const char *ChunkReaderString( lua_State *L, void *ptr, size_t *size )
{
	ChunkReaderData *data = (ChunkReaderData *) ptr;
	if( data->done )
		return NULL;

	data->done = true;

	*size = data->buf->size();
	const char *ret = data->buf->data();
	
	return ret;
}

void LuaManager::PushStackNil()
{
	lua_pushnil( L );
}

void LuaManager::PushStack( int out, lua_State *L )
{
	if( L == NULL )
		L = LUA->L;

	/* XXX: stack bounds */
	lua_pushnumber( L, out );
}

void LuaManager::PushStack( bool out, lua_State *L )
{
	if( L == NULL )
		L = LUA->L;

	/* XXX: stack bounds */
	lua_pushboolean( L, out );
}

void LuaManager::PushStack( float val, lua_State *L )
{
	if( L == NULL )
		L = LUA->L;

	/* XXX: stack bounds */
	lua_pushnumber( L, val );
}

void LuaManager::PushStack( void *out, lua_State *L )
{
	if( L == NULL )
		L = LUA->L;

	if( out )
		lua_pushlightuserdata( L, out );
	else
		lua_pushnil( L );
}

void LuaManager::PushStack( const CString &out, lua_State *L )
{
	if( L == NULL )
		L = LUA->L;
	lua_pushstring( L, out );
}

void LuaManager::CreateTableFromArray( const vector<bool> &aIn, lua_State *L )
{
	if( L == NULL )
		L = LUA->L;

	lua_newtable( L );
	for( unsigned i = 0; i < aIn.size(); ++i )
	{
		lua_pushboolean( L, aIn[i] );
		lua_rawseti( L, -2, i+1 );
	}
}

void LuaManager::ReadArrayFromTable( vector<bool> &aOut, lua_State *L )
{
	if( L == NULL )
		L = LUA->L;

	luaL_checktype( L, -1, LUA_TTABLE );

	for( unsigned i = 0; i < aOut.size(); ++i )
	{
		lua_rawgeti( L, -1, i+1 );
		bool bOn = !!lua_toboolean( L, -1 );
		aOut[i] = bOn;
		lua_pop( L, 1 );
	}
}

void LuaManager::PopStack( CString &out )
{
	/* There must be at least one entry on the stack. */
	ASSERT( lua_gettop(L) > 0 );
	ASSERT( lua_isstring(L, -1) );

	out = lua_tostring( L, -1 );
	lua_pop( L, -1 );
}

bool LuaManager::GetStack( int pos, int &out )
{
	if( pos < 0 )
		pos = lua_gettop(L) - pos - 1;
	if( pos < 1 )
		return false;
	
	out = (int) lua_tonumber( L, pos );
	return true;
}

void LuaManager::SetGlobal( const CString &sName )
{
	lua_setglobal( L, sName );
}


static int LuaPanic( lua_State *L )
{
	CString sErr;
	LUA->PopStack( sErr );

	RageException::Throw( "%s", sErr.c_str() );
}



// Actor registration
static vector<RegisterWithLuaFn>	*g_vRegisterActorTypes = NULL;

void LuaManager::Register( RegisterWithLuaFn pfn )
{
	if( g_vRegisterActorTypes == NULL )
		g_vRegisterActorTypes = new vector<RegisterWithLuaFn>;

	g_vRegisterActorTypes->push_back( pfn );
}





LuaManager::LuaManager()
{
	LUA = this;	// so that LUA is available when we call the Register functions

	L = NULL;

	ResetState();
}

LuaManager::~LuaManager()
{
	lua_close( L );
}

void LuaManager::RegisterTypes()
{
	if( g_vRegisterActorTypes )
	{
		for( unsigned i=0; i<g_vRegisterActorTypes->size(); i++ )
		{
			RegisterWithLuaFn fn = (*g_vRegisterActorTypes)[i];
			fn( L );
		}
	}
}

void LuaManager::ResetState()
{
	if( L != NULL )
	{
		LuaReference::BeforeResetAll();

		lua_close( L );
	}

	L = lua_open();
	ASSERT( L );

	lua_atpanic( L, LuaPanic );
	
	luaopen_base( L );
	luaopen_math( L );
	luaopen_string( L );
	luaopen_table( L );
	lua_settop(L, 0); // luaopen_* pushes stuff onto the stack that we don't need

	for( const LuaFunctionList *p = g_LuaFunctions; p; p=p->next )
		lua_register( L, p->name, p->func );
	
	RegisterTypes();

	LuaReference::AfterResetAll();
}

void LuaManager::PrepareExpression( CString &sInOut )
{
	// HACK: Many metrics have "//" comments that Lua fails to parse.
	// Replace them with Lua-style comments.
	sInOut.Replace( "//", "--" );
	
	// comment out HTML style color values
	sInOut.Replace( "#", "--" );
	
	// Remove leading +, eg. "+50"; Lua doesn't handle that.
	if( sInOut.size() >= 1 && sInOut[0] == '+' )
		sInOut.erase( 0, 1 );
}

bool LuaManager::RunScriptFile( const CString &sFile )
{
	RageFile f;
	if( !f.Open( sFile ) )
	{
		CString sError = ssprintf( "Couldn't open Lua script \"%s\": %s", sFile.c_str(), f.GetError().c_str() );
		Dialog::OK( sError, "LUA_ERROR" );
		return false;
	}

	CString sScript;
	if( f.Read( sScript ) == -1 )
	{
		CString sError = ssprintf( "Error reading Lua script \"%s\": %s", sFile.c_str(), f.GetError().c_str() );
		Dialog::OK( sError, "LUA_ERROR" );
		return false;
	}

	CString sError;
	if( !RunScript( sScript, sFile, sError ) )
	{
		sError = ssprintf( "Lua runtime error: %s", sError.c_str() );
		Dialog::OK( sError, "LUA_ERROR" );
		return false;
	}
	
	return true;
}

bool LuaManager::RunScript( const CString &sScript, const CString &sName, CString &sError, int iReturnValues )
{
	// load string
	{
		ChunkReaderData data;
		data.buf = &sScript;
		int ret = lua_load( L, ChunkReaderString, &data, sName );

		if( ret )
		{
			LuaManager::PopStack( sError );
			return false;
		}

		ASSERT_M( lua_gettop(L) == 1, ssprintf("%i", lua_gettop(L)) );
	}

	// evaluate
	{
		int ret = lua_pcall( L, 0, iReturnValues, 0 );
		if( ret )
		{
			LuaManager::PopStack( sError );
			return false;
		}
	}

	return true;
}


bool LuaManager::RunScript( const CString &sExpression, const CString &sName )
{
	CString sError;
	if( !RunScript( sExpression, sName.size()? sName:"in", sError, 1 ) )
	{
		sError = ssprintf( "Lua runtime error parsing \"%s\": %s", sName.size()? sName.c_str():sExpression.c_str(), sError.c_str() );
		Dialog::OK( sError, "LUA_ERROR" );
		return false;
	}

	return true;
}

bool LuaManager::RunExpressionB( const CString &str )
{
	if( !RunScript( "return " + str ) )
		return false;

	/* Don't accept a function as a return value. */
	if( lua_isfunction( L, -1 ) )
		RageException::Throw( "result is a function; did you forget \"()\"?" );

	bool result = !!lua_toboolean( L, -1 );
	lua_pop( L, -1 );

	return result;
}

float LuaManager::RunExpressionF( const CString &str )
{
	if( !RunScript( "return " + str ) )
		return 0;

	/* Don't accept a function as a return value. */
	if( lua_isfunction( L, -1 ) )
		RageException::Throw( "result is a function; did you forget \"()\"?" );

	float result = (float) lua_tonumber( L, -1 );
	lua_pop( L, -1 );

	return result;
}

int LuaManager::RunExpressionI( const CString &str )
{
	return (int)RunExpressionF(str);
}

bool LuaManager::RunExpressionS( const CString &str, CString &sOut )
{
	if( !RunScript( "return " + str ) )
		return false;

	/* Don't accept a function as a return value. */
	if( lua_isfunction( L, -1 ) )
		RageException::Throw( "result is a function; did you forget \"()\"?" );

	sOut = lua_tostring( L, -1 );
	lua_pop( L, -1 );

	return true;
}

bool LuaManager::RunAtExpressionS( CString &sStr )
{
	if( sStr.size() == 0 || sStr[0] != '@' )
		return false;

	/* Erase "@". */
	sStr.erase( 0, 1 );

	CString sOut;
	RunExpressionS( sStr, sOut );
	sStr = sOut;
	return true;
}

float LuaManager::RunAtExpressionF( const CString &_sStr )
{
	if( _sStr.size() == 0 || _sStr[0] != '@' )
		return 0;

	CString sStr = _sStr;

	/* Erase "@". */
	sStr.erase( 0, 1 );

	CString sOut;
	RunExpressionS( sStr, sOut );
	return strtof( sOut, NULL );
}

void LuaManager::Fail( const CString &err )
{
	lua_pushstring( L, err );
	lua_error( L );
}


LuaFunctionList::LuaFunctionList( CString name_, lua_CFunction func_ )
{
	name = name_;
	func = func_;
	next = g_LuaFunctions;
	g_LuaFunctions = this;
}


LuaFunction_NoArgs( MonthOfYear, GetLocalTime().tm_mon+1 );
LuaFunction_NoArgs( DayOfMonth, GetLocalTime().tm_mday );
LuaFunction_NoArgs( Hour, GetLocalTime().tm_hour );
LuaFunction_NoArgs( Minute, GetLocalTime().tm_min );
LuaFunction_NoArgs( Second, GetLocalTime().tm_sec );
LuaFunction_NoArgs( Year, GetLocalTime().tm_year+1900 );
LuaFunction_NoArgs( Weekday, GetLocalTime().tm_wday );
LuaFunction_NoArgs( DayOfYear, GetLocalTime().tm_yday );

LuaFunction_Str( Trace, (LOG->Trace("%s", str.c_str()),true) );

/*
 * (c) 2004 Glenn Maynard
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
