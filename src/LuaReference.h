/* LuaReference - a self-cleaning Lua reference. */

#ifndef LUA_REFERENCE_H
#define LUA_REFERENCE_H

struct lua_State;

class LuaReference
{
public:
	LuaReference();
	virtual ~LuaReference();

	/* Copying a reference makes a new reference pointing to the same object. */
	LuaReference( const LuaReference &cpy );
	LuaReference &operator=( const LuaReference &cpy );

	/* Create a reference pointing to the item at the top of the stack, and pop
	 * the stack. */
	void SetFromStack();
	void SetFromNil();

	/* Push the referenced object onto the stack.  If not set (or set to nil), push nil. */
	virtual void PushSelf( lua_State *L ) const;

	/* Return true if set.  (SetFromNil() counts as being set.) */
	bool IsSet() const;

	static void ReRegisterAll();	// call this after resetting Lua

protected:
	/* If this object is able to recreate itself, overload this, and ReRegisterAll
	 * will work; the reference will still exist after a theme change.  If not
	 * implemented, the reference will be unset after ReRegister. */
	virtual void Register();

private:
	void Unregister();
	void ReRegister();
	int m_iReference;
};

#endif

/*
 * (c) 2005 Glenn Maynard, Chris Danford
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
