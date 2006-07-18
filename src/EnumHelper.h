#ifndef EnumHelper_H
#define EnumHelper_H

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

#define FOREACH_ENUM( e, max, var )	for( e var=(e)0; var<max; enum_add<e>( var, +1 ) )
#define FOREACH_ENUM2( e, var )	for( e var=(e)0; var<NUM_##e; enum_add<e>( var, +1 ) )


static const RString EMPTY_STRING;

// TypeName[] must be an array of const char *, not RString, to
// avoid initialization order problems when calling XToString.
// Use Check##X##ToStringParamType to enforce that.
#define XToString(X, CNT)	\
	static void Check##X##ToStringParamType( const char **p ) { } \
	const RString& X##ToString( X x ) \
	{	\
		Check##X##ToStringParamType( X##Names ); \
		static auto_ptr<RString> as_##X##Name[CNT]; \
		if( as_##X##Name[0].get() == NULL ) { \
			for( unsigned i = 0; i < CNT; ++i ) \
			{ \
				auto_ptr<RString> ap( new RString( X##Names[i] ) ); \
				as_##X##Name[i] = ap; \
			} \
		} \
		ASSERT( CNT == ARRAYSIZE(X##Names) );	\
		if( x == CNT+1 ) 	\
			return EMPTY_STRING;	\
		ASSERT( x < CNT );	\
		return *as_##X##Name[x];	\
	}

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
		for( i = 0; i < ARRAYSIZE(X##Names); ++i )	\
			if( !s2.CompareNoCase(X##Names[i]) )	\
				return (X)i;	\
		return (X)(i+1); /*invalid*/	\
	} \
	template<> void StringTo<X>( const RString& s, X &out ) { out = StringTo##X( s ); }
// VC6 hack: even templated functions can't differ by only return value.  Move return value to a reference parameter.
template<class T> void StringTo( const RString& s, T &out );

#define LuaXToString(X)	\
LuaFunction( X##ToString, X##ToString( (X) IArg(1) ) );

#define LuaStringToX(X)	\
LuaFunction( StringTo##X, (X) StringTo##X( SArg(1) ) );

#define LuaXType(X, CNT, Prefix, bCapitalize)	\
static void Lua##X(lua_State* L) \
{ \
	FOREACH_ENUM( X, CNT, i ) \
	{ \
		RString s = X##Names[i]; \
		if( bCapitalize ) \
			s.MakeUpper(); \
		LUA->SetGlobal( Prefix+s, i ); \
	} \
	{ \
		RString sNum = "NUM" #X "S" ; \
		if( bCapitalize ) \
			sNum.MakeUpper(); \
		LUA->SetGlobal( sNum, CNT ); \
	} \
	{ \
		RString sNum = "NUM_" #X; \
		LUA->SetGlobal( sNum, CNT ); \
	} \
	LUA->SetGlobal( Prefix "INVALID", CNT+1 ); \
} \
REGISTER_WITH_LUA_FUNCTION( Lua##X );

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
