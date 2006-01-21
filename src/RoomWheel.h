/* RoomWheel - A wheel containing data about rooms. */

#ifndef ROOMWHEEL_H
#define ROOMWHEEL_H

#include "Quad.h"
#include "WheelBase.h"
#include "WheelItemBase.h"
#include "ThemeMetric.h"

struct RoomWheelData : public WheelItemBaseData
{
	RoomWheelData() { WheelItemBaseData::WheelItemBaseData(); }
	RoomWheelData( WheelItemType wit, CString title, CString SubTitle, RageColor color );

	CString			m_sDesc;
	WheelNotifyIcon::Flags  m_Flags;
};

class RoomWheelItem : public WheelItemBase
{
public:
	RoomWheelItem(CString sType = "RoomWheelItem");
	RoomWheelData* data;
	void Load( CString sType );
	virtual void LoadFromWheelItemBaseData(WheelItemBaseData* pWID);

	BitmapText m_Desc;

private:
	ThemeMetric<float>				DESC_X;
	ThemeMetric<float>				DESC_Y;
	ThemeMetric<float>				DESC_WIDTH;
	ThemeMetric<apActorCommands>	DESC_ON_COMMAND;
};

struct RoomInfo
{
	CString songTitle;
	CString songSubTitle;
	CString songArtist;
	int numPlayers;
	int maxPlayers;
	vector<CString> players;
};

class RoomInfoDisplay : public ActorFrame
{
public:
	~RoomInfoDisplay();
	virtual void Load( CString sType );
	virtual void Update( float fDeltaTime );
	void SetRoom( const RoomWheelData* roomData );
	void SetRoomInfo( const RoomInfo& info);
	void DeployInfoBox();
	void RetractInfoBox();
private:
	void RequestRoomInfo();
	enum RoomInfoDisplayState
	{
		OPEN = 0,
		CLOSED,
		LOCKED
	};

	RoomInfoDisplayState m_state;
	Quad m_bg;
	BitmapText m_Title;
	BitmapText m_Desc;

	BitmapText m_lastRound;
	BitmapText m_songTitle;
	BitmapText m_songSub;
	BitmapText m_songArtist;

	int m_numPlayers;
	int m_maxPlayers;
	BitmapText m_players;
	vector<BitmapText*> m_playersList;

	RageTimer m_deployDelay;

	ThemeMetric<float>	X;
	ThemeMetric<float>	Y;
	ThemeMetric<float>	DEPLOY_DELAY;
	ThemeMetric<float>	RETRACT_DELAY;
	ThemeMetric<float>	PLAYERLISTX;
	ThemeMetric<float>	PLAYERLISTY;
	ThemeMetric<float>	PLAYERLISTOFFSETX;
	ThemeMetric<float>	PLAYERLISTOFFSETY;
};

class RoomWheel : public WheelBase {
public:
	virtual void Load( CString sType );
	virtual void BuildWheelItemsData( vector<WheelItemBaseData*> &arrayWheelItemDatas );
	virtual unsigned int GetNumItems() const;
	virtual void RemoveItem( int index );
	virtual bool Select();
	virtual void Update( float fDeltaTime );
	virtual void Move(int n);

	inline RoomWheelData* GetItem(unsigned int i) { return (RoomWheelData*)WheelBase::GetItem(i + m_offset); }
	void AddPerminateItem(RoomWheelData* itemdata);
	int GetCurrentIndex() const { return m_iSelection; }
private:
	int m_offset;

	RoomInfoDisplay m_roomInfo;
};

#endif

/*
 * (c) 2004 Josh Allen
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
