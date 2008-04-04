#include "global.h"
#include "GradeDisplay.h"
#include "RageUtil.h"
#include "arch/Dialog/Dialog.h"
#include "RageLog.h"
#include "ActorUtil.h"
#include "ThemeManager.h"

REGISTER_ACTOR_CLASS( GradeDisplay )

void GradeDisplay::Load( RString sMetricsGroup )
{
	ASSERT( m_vSpr.empty() );
	FOREACH_UsedGrade( g )
	{
		m_vSpr.resize( m_vSpr.size()+1 );
		m_vSpr.back().Load( THEME->GetPathG(sMetricsGroup,GradeToString(g)) );
		m_vSpr.back()->SetVisible( false );
	}
}

void GradeDisplay::SetGrade( Grade grade )
{
	int i = 0;
	FOREACH_UsedGrade( g )
	{
		m_vSpr[i]->SetVisible( g == grade );
		i++;
	}
}

// lua start
#include "LuaBinding.h"

class LunaGradeDisplay: public Luna<GradeDisplay>
{
public:
	static int Load( T* p, lua_State *L )
	{
		p->Load( SArg(1) );
		return 0;
	}
	static int SetGrade( T* p, lua_State *L )
	{
		Grade g = Enum::Check<Grade>(L, 2);
		p->SetGrade( g );
		return 0;
	}

	LunaGradeDisplay()
	{
		ADD_METHOD( Load );
		ADD_METHOD( SetGrade );
	}
};

LUA_REGISTER_DERIVED_CLASS( GradeDisplay, Sprite )
// lua end

/*
 * (c) 2001-2002 Chris Danford
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
