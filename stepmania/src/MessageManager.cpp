#include "global.h"
#include "MessageManager.h"
#include "Foreach.h"
#include "Actor.h"
#include "RageUtil.h"

MessageManager*	MESSAGEMAN = NULL;	// global and accessable from anywhere in our program


MessageManager::MessageManager()
{
}

MessageManager::~MessageManager()
{
}

void MessageManager::Subscribe( Actor* pActor, const CString& sMessage )
{
	SubscribersSet& subs = m_MessageToSubscribers[sMessage];
#if _DEBUG
	SubscribersSet::iterator iter = subs.find(pActor);
	ASSERT_M( iter == subs.end(), "already subscribed" );
#endif
	subs.insert( pActor );
}
void MessageManager::Unsubscribe( Actor* pActor, const CString& sMessage )
{
	SubscribersSet& subs = m_MessageToSubscribers[sMessage];
	SubscribersSet::iterator iter = subs.find(pActor);
	ASSERT( iter != subs.end() );
	subs.erase( iter );
}

void MessageManager::Broadcast( const CString& sMessage )
{
	SubscribersSet& subs = m_MessageToSubscribers[sMessage];
	FOREACHS_CONST( Actor*, subs, p )
	{
		(*p)->PlayCommand( sMessage );
	}
}

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
