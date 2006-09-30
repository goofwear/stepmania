#ifndef ENUM_HELPER_H
#define ENUM_HELPER_H

#include "LuaReference.h"
#include "RageUtil.h"

extern "C"
{
#include "lua-5.1/src/lua.h"
}

/*
 * Safely add an integer to an enum.
 *
 * This is illegal:
 *
 *  ((int&)val) += iAmt;
 *
 * It breaks aliasing rules; the compiler is allowed to assume that "val" doesn't
 * change (unless it's declared volatile), and in some cases, you'll end up getting
 * old values for "val" following the add.  (What's probably really happening is
 * that the memory location is being added to, but the value is stored in a register,
 * and breaking aliasing rules means the compiler doesn't know that the register
 * value is invalid.)
 *
 * Always do these conversions through a union.
 */
template<typename T>
static inline void enum_add( T &val, int iAmt )
{
	union conv
	{
		T value;
		int i;
	};

	conv c;
	c.value = val;
	c.i += iAmt;
	val = c.value;
}

template<typename T>
static inline T enum_add2( T val, int iAmt )
{
	enum_add( val, iAmt );
	return val;
}

#define FOREACH_ENUM_N( e, max, var )	for( e var=(e)0; var<max; enum_add<e>( var, +1 ) )
#define FOREACH_ENUM( e, var )	for( e var=(e)0; var<NUM_##e; enum_add<e>( var, +1 ) )
#define FOREACH_ENUM2 FOREACH_ENUM

int CheckEnum( lua_State *L, LuaReference &table, int iPos, int iInvalid, const char *szType );

template<typename T>
struct EnumTraits
{
	static LuaReference StringToEnum;
	static LuaReference EnumToString;
	static T Invalid;
	static const char *szName;
};

namespace Enum
{
	template<typename T>
	static T Check( lua_State *L, int iPos )
	{
		return (T) CheckEnum( L, EnumTraits<T>::StringToEnum, iPos, EnumTraits<T>::Invalid, EnumTraits<T>::szName );
	}
	template<typename T>
	static void Push( lua_State *L, T iVal )
	{
		/* Enum_Invalid values are nil in Lua. */
		if( iVal == EnumTraits<T>::Invalid )
		{
			lua_pushnil( L );
			return;
		}

		/* Look up the string value. */
		EnumTraits<T>::EnumToString.PushSelf( L );
		lua_rawgeti( L, -1, iVal + 1 );
		lua_replace( L, -2 );
	}
};

static const RString EMPTY_STRING;
const RString &EnumToString( int iVal, int iMax, const char **szNameArray, auto_ptr<RString> *pNameCache ); // XToString helper

#define XToString2(X) \
	COMPILE_ASSERT( NUM_##X == ARRAYLEN(X##Names) ); \
	const RString& X##ToString( X x ) \
	{	\
		static auto_ptr<RString> as_##X##Name[NUM_##X+2]; \
		return EnumToString( x, NUM_##X, X##Names, as_##X##Name ); \
	}
#define XToString(X, CNT) XToString2(X)

#define XToLocalizedString(X)      \
	const RString &X##ToLocalizedString( X x ) \
	{       \
		static auto_ptr<LocalizedString> g_##X##Name[NUM_##X]; \
		if( g_##X##Name[0].get() == NULL ) { \
			for( unsigned i = 0; i < NUM_##X; ++i ) \
			{ \
				auto_ptr<LocalizedString> ap( new LocalizedString(#X, X##ToString((X)i)) ); \
				g_##X##Name[i] = ap; \
			} \
		} \
		return g_##X##Name[x]->GetValue();  \
	}

#define StringToX(X)	\
	X StringTo##X( const RString& s ) \
	{	\
		RString s2 = s;	\
		s2.MakeLower();	\
		unsigned i; \
		for( i = 0; i < ARRAYLEN(X##Names); ++i )	\
			if( !s2.CompareNoCase(X##Names[i]) )	\
				return (X)i;	\
		return (X)(i+1); /*invalid*/	\
	}

#define LuaDeclareType(X) \
namespace LuaHelpers { bool FromStack( lua_State *L, X &Object, int iOffset ); } \
namespace LuaHelpers { void Push( lua_State *L, const X &Object ); }

#define LuaXType(X)	\
static void Lua##X(lua_State* L) \
{ \
	/* Create the EnumToString table: { "UnlockEntry_ArcadePoints", "UnlockEntry_DancePoints" } */ \
	lua_newtable( L ); \
	FOREACH_ENUM2( X, i ) \
	{ \
		RString s = X##ToString( i ); \
		lua_pushstring( L, (#X "_")+s ); \
		lua_rawseti( L, -2, i+1 ); /* 1-based */ \
	} \
	EnumTraits<X>::EnumToString.SetFromStack( L ); \
	EnumTraits<X>::EnumToString.PushSelf( L ); \
	lua_setglobal( L, #X ); \
	/* Create the StringToEnum table: { "UnlockEntry_ArcadePoints" = 0, "UnlockEntry_DancePoints" = 1 } */ \
	lua_newtable( L ); \
	FOREACH_ENUM2( X, i ) \
	{ \
		RString s = X##ToString( i ); \
		lua_pushstring( L, (#X "_")+s ); \
		lua_pushnumber( L, i ); /* 0-based */ \
		lua_rawset( L, -3 ); \
	} \
	EnumTraits<X>::StringToEnum.SetFromStack( L ); \
	EnumTraits<X>::StringToEnum.PushSelf( L ); \
	lua_setglobal( L, #X "Index" ); \
} \
REGISTER_WITH_LUA_FUNCTION( Lua##X ); \
template<> LuaReference EnumTraits<X>::StringToEnum; \
template<> LuaReference EnumTraits<X>::EnumToString; \
template<> X EnumTraits<X>::Invalid = enum_add2( NUM_##X, 1 ); \
template<> const char *EnumTraits<X>::szName = #X; \
namespace LuaHelpers { bool FromStack( lua_State *L, X &Object, int iOffset ) { Object = Enum::Check<X>( L, iOffset ); return true; } } \
namespace LuaHelpers { void Push( lua_State *L, const X &Object ) { Enum::Push<X>( L, Object ); } }

#endif

/*
 * (c) 2004 Chris Danford
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
