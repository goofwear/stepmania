/* OptionRowHandler - Shows PlayerOptions and SongOptions in icon form. */

#ifndef OptionRowHandler_H
#define OptionRowHandler_H

#include "OptionRow.h"
#include "GameCommand.h"
#include "LuaReference.h"
#include "RageUtil.h"
struct MenuRowDef;

struct ConfOption;

class OptionRowHandler
{
public:
	Commands m_cmds;
	OptionRowDefinition m_Def;
	vector<RString> m_vsReloadRowMessages;	// refresh this row on on these messages
	
	OptionRowHandler() { Init(); }
	virtual ~OptionRowHandler() { }
	virtual void Init()
	{
		m_cmds.v.clear();
		m_Def.Init();
		m_vsReloadRowMessages.clear();
	}
	void Load( const Commands &cmds )
	{
		Init();
		m_cmds = cmds;
		this->LoadInternal( cmds );
	}
	virtual void LoadInternal( const Commands &cmds ) = 0;

	/*
	 * We may re-use OptionRowHandlers.  This is called before each
	 * use.  If the contents of the row are dependent on external
	 * state (for example, the current song), clear the row contents
	 * and reinitialize them.  As an optimization, rows which do not
	 * change can be initialized just once and left alone.
	 *
	 * If the row has been reinitialized, return RELOAD_CHANGED_ALL, and the
	 * graphic elements will also be reinitialized.  If only m_vEnabledForPlayers
	 * has been changed, return RELOAD_CHANGED_ENABLED.  If the row is static, and
	 * nothing has changed, return RELOAD_CHANGED_NONE.
	 */
	enum ReloadChanged { RELOAD_CHANGED_NONE, RELOAD_CHANGED_ENABLED, RELOAD_CHANGED_ALL };
	virtual ReloadChanged Reload() { return RELOAD_CHANGED_NONE; }

	virtual void ImportOption( const vector<PlayerNumber> &vpns, vector<bool> vbSelectedOut[NUM_PLAYERS] ) const = 0;
	/* Returns an OPT mask. */
	virtual int ExportOption( const vector<PlayerNumber> &vpns, const vector<bool> vbSelected[NUM_PLAYERS] ) const = 0;
	virtual void GetIconTextAndGameCommand( int iFirstSelection, RString &sIconTextOut, GameCommand &gcOut ) const;
	virtual RString GetScreen( int iChoice ) const { return RString(); }
};


namespace OptionRowHandlerUtil
{
	OptionRowHandler* Make( const Commands &cmds );
	OptionRowHandler* MakeNull();
	OptionRowHandler* MakeSimple( const MenuRowDef &mrd );

	void SelectExactlyOne( int iSelection, vector<bool> &vbSelectedOut );
	int GetOneSelection( const vector<bool> &vbSelected );
}

inline void VerifySelected( SelectType st, const vector<bool> &vbSelected, const RString &sName )
{
	int iNumSelected = 0;
	if( st == SELECT_ONE )
	{
		ASSERT_M( vbSelected.size() > 0, ssprintf("%s: %i/%i", sName.c_str(), iNumSelected, int(vbSelected.size())) );
		for( unsigned e = 0; e < vbSelected.size(); ++e )
			if( vbSelected[e] )
				iNumSelected++;
		ASSERT_M( iNumSelected == 1, ssprintf("%s: %i/%i", sName.c_str(), iNumSelected, int(vbSelected.size())) );
	}
}

#endif

/*
 * (c) 2002-2004 Chris Danford
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
