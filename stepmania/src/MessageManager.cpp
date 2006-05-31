#include "global.h"
#include "MessageManager.h"
#include "Foreach.h"
#include "RageUtil.h"
#include "RageThreads.h"
#include "EnumHelper.h"

#include <set>
#include <map>

MessageManager*	MESSAGEMAN = NULL;	// global and accessable from anywhere in our program


static const char *MessageNames[] = {
	"CurrentGameChanged",
	"CurrentStyleChanged",
	"PlayModeChanged",
	"CurrentSongChanged",
	"CurrentStepsP1Changed",
	"CurrentStepsP2Changed",
	"CurrentCourseChanged",
	"CurrentTrailP1Changed",
	"CurrentTrailP2Changed",
	"GameplayLeadInChanged",
	"EditStepsTypeChanged",
	"EditCourseDifficultyChanged",
	"EditSourceStepsChanged",
	"EditSourceStepsTypeChanged",
	"PreferredDifficutyP1Changed",
	"PreferredDifficutyP2Changed",
	"PreferredCourseDifficutyP1Changed",
	"PreferredCourseDifficutyP2Changed",
	"EditCourseEntryIndexChanged",
	"EditLocalProfileIDChanged",
	"GoalCompleteP1",
	"GoalCompleteP2",
	"NoteCrossed",
	"NoteWillCrossIn400Ms",
	"NoteWillCrossIn800Ms",
	"NoteWillCrossIn1200Ms",
	"CardRemovedP1",
	"CardRemovedP2",
	"BeatCrossed",
	"MenuUpP1",
	"MenuUpP2",
	"MenuDownP1",
	"MenuDownP2",
	"MenuLeftP1",
	"MenuLeftP2",
	"MenuRightP1",
	"MenuRightP2",
	"MenuSelectionChanged",
	"CoinInserted",
	"SideJoinedP1",
	"SideJoinedP2",
	"PlayersFinalized",
	"AssistTickChanged",
	"AutosyncChanged",
	"PreferredSongGroupChanged",
	"PreferredCourseGroupChanged",
	"SortOrderChanged",
	"LessonTry1",
	"LessonTry2",
	"LessonTry3",
	"LessonCleared",
	"LessonFailed",
	"StorageDevicesChanged",
	"AutoJoyMappingApplied",
	"ScreenChanged",
	"ShowJudgmentMuliPlayerP1",
	"ShowJudgmentMuliPlayerP2",
	"ShowJudgmentMuliPlayerP3",
	"ShowJudgmentMuliPlayerP4",
	"ShowJudgmentMuliPlayerP5",
	"ShowJudgmentMuliPlayerP6",
	"ShowJudgmentMuliPlayerP7",
	"ShowJudgmentMuliPlayerP8",
	"ShowJudgmentMuliPlayerP9",
	"ShowJudgmentMuliPlayerP10",
	"ShowJudgmentMuliPlayerP11",
	"ShowJudgmentMuliPlayerP12",
	"ShowJudgmentMuliPlayerP13",
	"ShowJudgmentMuliPlayerP14",
	"ShowJudgmentMuliPlayerP15",
	"ShowJudgmentMuliPlayerP16",
	"ShowJudgmentMuliPlayerP17",
	"ShowJudgmentMuliPlayerP18",
	"ShowJudgmentMuliPlayerP19",
	"ShowJudgmentMuliPlayerP20",
	"ShowJudgmentMuliPlayerP21",
	"ShowJudgmentMuliPlayerP22",
	"ShowJudgmentMuliPlayerP23",
	"ShowJudgmentMuliPlayerP24",
	"ShowJudgmentMuliPlayerP25",
	"ShowJudgmentMuliPlayerP26",
	"ShowJudgmentMuliPlayerP27",
	"ShowJudgmentMuliPlayerP28",
	"ShowJudgmentMuliPlayerP29",
	"ShowJudgmentMuliPlayerP30",
	"ShowJudgmentMuliPlayerP31",
	"ShowJudgmentMuliPlayerP32",
	"ShowHoldJudgmentMuliPlayerP1",
	"ShowHoldJudgmentMuliPlayerP2",
	"ShowHoldJudgmentMuliPlayerP3",
	"ShowHoldJudgmentMuliPlayerP4",
	"ShowHoldJudgmentMuliPlayerP5",
	"ShowHoldJudgmentMuliPlayerP6",
	"ShowHoldJudgmentMuliPlayerP7",
	"ShowHoldJudgmentMuliPlayerP8",
	"ShowHoldJudgmentMuliPlayerP9",
	"ShowHoldJudgmentMuliPlayerP10",
	"ShowHoldJudgmentMuliPlayerP11",
	"ShowHoldJudgmentMuliPlayerP12",
	"ShowHoldJudgmentMuliPlayerP13",
	"ShowHoldJudgmentMuliPlayerP14",
	"ShowHoldJudgmentMuliPlayerP15",
	"ShowHoldJudgmentMuliPlayerP16",
	"ShowHoldJudgmentMuliPlayerP17",
	"ShowHoldJudgmentMuliPlayerP18",
	"ShowHoldJudgmentMuliPlayerP19",
	"ShowHoldJudgmentMuliPlayerP20",
	"ShowHoldJudgmentMuliPlayerP21",
	"ShowHoldJudgmentMuliPlayerP22",
	"ShowHoldJudgmentMuliPlayerP23",
	"ShowHoldJudgmentMuliPlayerP24",
	"ShowHoldJudgmentMuliPlayerP25",
	"ShowHoldJudgmentMuliPlayerP26",
	"ShowHoldJudgmentMuliPlayerP27",
	"ShowHoldJudgmentMuliPlayerP28",
	"ShowHoldJudgmentMuliPlayerP29",
	"ShowHoldJudgmentMuliPlayerP30",
	"ShowHoldJudgmentMuliPlayerP31",
	"ShowHoldJudgmentMuliPlayerP32",
};
XToString( Message, NUM_Message );

static RageMutex g_Mutex( "MessageManager" );

typedef set<IMessageSubscriber*> SubscribersSet;
static map<RString,SubscribersSet> g_MessageToSubscribers;

MessageManager::MessageManager()
{
}

MessageManager::~MessageManager()
{
}

void MessageManager::Subscribe( IMessageSubscriber* pSubscriber, const RString& sMessage )
{
	LockMut(g_Mutex);

	SubscribersSet& subs = g_MessageToSubscribers[sMessage];
#if _DEBUG
	SubscribersSet::iterator iter = subs.find(pSubscriber);
	ASSERT_M( iter == subs.end(), "already subscribed" );
#endif
	subs.insert( pSubscriber );
}

void MessageManager::Subscribe( IMessageSubscriber* pSubscriber, Message m )
{
	Subscribe( pSubscriber, MessageToString(m) );
}

void MessageManager::Unsubscribe( IMessageSubscriber* pSubscriber, const RString& sMessage )
{
	LockMut(g_Mutex);

	SubscribersSet& subs = g_MessageToSubscribers[sMessage];
	SubscribersSet::iterator iter = subs.find(pSubscriber);
	ASSERT( iter != subs.end() );
	subs.erase( iter );
}

void MessageManager::Unsubscribe( IMessageSubscriber* pSubscriber, Message m )
{
	Unsubscribe( pSubscriber, MessageToString(m) );
}

void MessageManager::Broadcast( const RString& sMessage ) const
{
	ASSERT( !sMessage.empty() );

	LockMut(g_Mutex);

	map<RString,SubscribersSet>::const_iterator iter = g_MessageToSubscribers.find( sMessage );
	if( iter == g_MessageToSubscribers.end() )
		return;

	FOREACHS_CONST( IMessageSubscriber*, iter->second, p )
	{
		IMessageSubscriber *pSub = *p;
		pSub->HandleMessageInternal( sMessage );
	}
}

void MessageManager::Broadcast( Message m ) const
{
	Broadcast( MessageToString(m) );
}

void IMessageSubscriber::ClearMessages( const RString sMessage )
{
	LockMut(g_Mutex);

	if( sMessage.empty() )
	{
		m_aMessages.clear();
		return;
	}

	for( int i=m_aMessages.size()-1; i>=0; i-- )
		if( m_aMessages[i].sMessage == sMessage )
			m_aMessages.erase( m_aMessages.begin()+i ); 
}

void IMessageSubscriber::HandleMessageInternal( const RString& sMessage )
{
	QueuedMessage QM;
	QM.sMessage = sMessage;
	QM.fDelayRemaining = 0;

	g_Mutex.Lock();
	m_aMessages.push_back( QM );
	g_Mutex.Unlock();
}

void IMessageSubscriber::ProcessMessages( float fDeltaTime )
{
	/* Important optimization for the vast majority of cases: don't lock the
	 * mutex if we have no messages. */
	if( m_aMessages.empty() )
		return;

	g_Mutex.Lock();
	for( unsigned i=0; i<m_aMessages.size(); i++ )
		m_aMessages[i].fDelayRemaining -= fDeltaTime;

	for( unsigned i = 0; i < m_aMessages.size(); ++i )
	{
		/* Remove the message from the list. */
		const RString sMessage = m_aMessages[i].sMessage;
		m_aMessages.erase( m_aMessages.begin()+i );
		--i;

		unsigned iSize = m_aMessages.size();

		g_Mutex.Unlock();
		HandleMessage( sMessage );
		g_Mutex.Lock();

		/* If the size changed, start over. */
		if( iSize != m_aMessages.size() )
			i = 0;
	}
	g_Mutex.Unlock();
}

MessageSubscriber::MessageSubscriber( const MessageSubscriber &cpy ):
	IMessageSubscriber(cpy)
{
	m_vsSubscribedTo = cpy.m_vsSubscribedTo;
}

void MessageSubscriber::SubscribeToMessage( const RString &sMessageName )
{
	MESSAGEMAN->Subscribe( this, sMessageName );
	m_vsSubscribedTo.push_back( sMessageName );
}

void MessageSubscriber::SubscribeToMessage( Message message )
{
	MESSAGEMAN->Subscribe( this, message );
	m_vsSubscribedTo.push_back( MessageToString(message) );
}

void MessageSubscriber::UnsubscribeAll()
{
	FOREACH_CONST( RString, m_vsSubscribedTo, s )
		MESSAGEMAN->Unsubscribe( this, *s );
	m_vsSubscribedTo.clear();
}


// lua start
#include "LuaBinding.h"

class LunaMessageManager: public Luna<MessageManager>
{
public:
	LunaMessageManager() { LUA->Register( Register ); }

	static int Broadcast( T* p, lua_State *L )
	{
		p->Broadcast( SArg(1) );
		return 0;
	}

	static void Register(lua_State *L)
	{
		ADD_METHOD( Broadcast );

		Luna<T>::Register( L );

		// Add global singleton if constructed already.  If it's not constructed yet,
		// then we'll register it later when we reinit Lua just before 
		// initializing the display.
		if( MESSAGEMAN )
		{
			lua_pushstring(L, "MESSAGEMAN");
			MESSAGEMAN->PushSelf( L );
			lua_settable(L, LUA_GLOBALSINDEX);
		}
	}
};

LUA_REGISTER_CLASS( MessageManager )
// lua end

/*
 * (c) 2003-2004 Chris Danford
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
