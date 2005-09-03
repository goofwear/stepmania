#include "global.h"
#include "ActorUtil.h"
#include "Sprite.h"
#include "Model.h"
#include "BGAnimation.h"
#include "ThemeManager.h"
#include "RageFileManager.h"
#include "RageLog.h"
#include "EnumHelper.h"
#include "XmlFile.h"
#include "LuaManager.h"
#include "Foreach.h"

#include "arch/Dialog/Dialog.h"


// Actor registration
static map<CString,CreateActorFn>	*g_pmapRegistrees = NULL;

static bool IsRegistered( const CString& sClassName )
{
	return g_pmapRegistrees->find( sClassName ) != g_pmapRegistrees->end();
}

void ActorUtil::Register( const CString& sClassName, CreateActorFn pfn )
{
	if( g_pmapRegistrees == NULL )
		g_pmapRegistrees = new map<CString,CreateActorFn>;

	map<CString,CreateActorFn>::iterator iter = g_pmapRegistrees->find( sClassName );
	ASSERT_M( iter == g_pmapRegistrees->end(), ssprintf("Actor class '%s' already registered.", sClassName.c_str()) );

	(*g_pmapRegistrees)[sClassName] = pfn;
}

Actor* ActorUtil::Create( const CString& sClassName, const CString& sDir, const XNode* pNode )
{
	map<CString,CreateActorFn>::iterator iter = g_pmapRegistrees->find( sClassName );
	ASSERT_M( iter != g_pmapRegistrees->end(), ssprintf("Actor '%s' is not registered.",sClassName.c_str()) )

	CreateActorFn pfn = iter->second;
	return (*pfn)( sDir, pNode );
}

/* Return false to retry. */
void ActorUtil::ResolvePath( CString &sPath, const CString &sName )
{
	const CString sOriginalPath( sPath );

retry:
	CollapsePath( sPath );

	// If we know this is an exact match, don't bother with the GetDirListing,
	// so "foo" doesn't partial match "foobar" if "foo" exists.
	RageFileManager::FileType ft = FILEMAN->GetFileType( sPath );
	if( ft != RageFileManager::TYPE_FILE && ft != RageFileManager::TYPE_DIR )
	{
		CStringArray asPaths;
		GetDirListing( sPath + "*", asPaths, false, true );	// return path too

		if( asPaths.empty() )
		{
			CString sError = ssprintf( "A file in '%s' references a file '%s' which doesn't exist.", sName.c_str(), sPath.c_str() );
			switch( Dialog::AbortRetryIgnore( sError, "BROKEN_FILE_REFERENCE" ) )
			{
			case Dialog::abort:
				RageException::Throw( sError ); 
				break;
			case Dialog::retry:
				FlushDirCache();
				goto retry;
			case Dialog::ignore:
				asPaths.push_back( sPath );
				if( GetExtension(asPaths[0]) == "" )
					asPaths[0] = SetExtension( asPaths[0], "png" );
				break;
			default:
				ASSERT(0);
			}
		}
		else if( asPaths.size() > 1 )
		{
			CString sError = ssprintf( "A file in '%s' references a file '%s' which has multiple matches.", sName.c_str(), sPath.c_str() );
			switch( Dialog::AbortRetryIgnore( sError, "BROKEN_FILE_REFERENCE" ) )
			{
			case Dialog::abort:
				RageException::Throw( sError ); 
				break;
			case Dialog::retry:
				FlushDirCache();
				goto retry;
			case Dialog::ignore:
				asPaths.erase( asPaths.begin()+1, asPaths.end() );
				break;
			default:
				ASSERT(0);
			}
		}

		sPath = asPaths[0];
	}

	sPath = DerefRedir( sPath );
}


Actor* ActorUtil::LoadFromActorFile( const CString& sDir, const XNode* pNode )
{
	ASSERT( pNode );

	{
		CString expr;
		if( pNode->GetAttrValue("Condition",expr) )
		{
			LuaHelpers::RunAtExpressionS( expr );
			if( !LuaHelpers::RunExpressionB(expr) )
				return NULL;
		}
	}

	// Load Params
	{
		FOREACH_CONST_Child( pNode, pChild )
		{
			if( pChild->m_sName == "Param" )
			{
				CString sName;
				if( !pChild->GetAttrValue( "Name", sName ) )
				{
					RageException::Throw( ssprintf("Param node in '%s' is missing the attribute 'Name'", sDir.c_str()) );
				}

				THEME->EvaluateString( sName );

				CString s;
				if( pChild->GetAttrValue( "Function", s ) )
				{
					LuaExpression expr;
					THEME->EvaluateString( s );
					expr.SetFromExpression( s );
					Lua *L = LUA->Get();
					expr.PushSelf( L );
					ASSERT( !lua_isnil(L, -1) );
					lua_call( L, 0, 1 ); // 0 args, 1 results
					lua_setglobal( L, sName );
					LUA->Release(L);
				}
				else if( pChild->GetAttrValue( "Value", s ) )
				{
					THEME->EvaluateString( s );
					LUA->SetGlobalFromExpression( sName, s );
				}
				else
				{
					RageException::Throw( ssprintf("Param node in '%s' is missing the attribute 'Function' or 'Value'", sDir.c_str()) );
				}
			}
		}
	}


	// Element name is the type in XML.
	// Type= is the name in INI.
	CString sClass = pNode->m_sName;
	bool bHasClass = pNode->GetAttrValue( "Class", sClass );
	if( !bHasClass )
		bHasClass = pNode->GetAttrValue( "Type", sClass );	// for backward compatibility

	// backward compat hack
	if( !bHasClass )
	{
		CString sText;
		if( pNode->GetAttrValue( "Text", sText ) )
			sClass = "BitmapText";
	}

	Actor *pReturn = NULL;

	if( IsRegistered(sClass) )
	{
		pReturn = ActorUtil::Create( sClass, sDir, pNode );
	}
	else // sClass is empty or garbage (e.g. "1" // 0==Sprite")
	{
		// automatically figure out the type
		CString sFile;
		pNode->GetAttrValue( "File", sFile );

		LuaHelpers::RunAtExpressionS( sFile );
		bool bIsAbsolutePath = sFile.Left(1) == "/";
		FixSlashesInPlace( sFile );

		/* Be careful: if sFile is "", and we don't check it, then we can end up recursively
		 * loading the layer we're in. */
		if( sFile == "" )
		{
			CString sError = ssprintf( "An xml file in '%s' is missing the File attribute or has an invalid Class \"%s\"",
				sDir.c_str(), sClass.c_str() );
			Dialog::OK( sError );
			pReturn = new Actor;	// Return a dummy object so that we don't crash in AutoActor later.
			goto all_done;
		}

		CString sNewPath = bIsAbsolutePath ? sFile : sDir+sFile;

		ActorUtil::ResolvePath( sNewPath, sDir );

		pReturn = ActorUtil::MakeActor( sNewPath );
		if( pReturn == NULL )
			goto all_done;
	 	pReturn->LoadFromNode( sDir, pNode );
	}

all_done:

	// Unload Params
	{
		FOREACH_CONST_Child( pNode, pChild )
		{
			if( pChild->m_sName == "Param" )
			{
				CString sName;
				if( !pChild->GetAttrValue( "Name", sName ) )
				{
					RageException::Throw( ssprintf("Param node in '%s' is missing the attribute 'Name'", sDir.c_str()) );
				}

				LUA->UnsetGlobal( sName );
			}
		}
	}

	return pReturn;
}

Actor* ActorUtil::MakeActor( const RageTextureID &ID )
{
	FileType ft = GetFileType(ID.filename);
	switch( ft )
	{
	case FT_Xml:
		{
			XNode xml;
			if( !xml.LoadFromFile(ID.filename) )
			{
				// XNode will warn about the error
				return new Actor;
			}
			CString sDir = Dirname( ID.filename );
			return LoadFromActorFile( sDir, &xml );
		}
	case FT_Directory:
		{
			CString sDir = ID.filename;
			if( sDir.Right(1) != "/" )
				sDir += '/';

			CString sXml = sDir + "default.xml";
			if( DoesFileExist(sXml) )
			{
				XNode xml;
				if( !xml.LoadFromFile(sXml) )
				{
					// XNode will warn about the error
					return new Actor;
				}
				return LoadFromActorFile( sDir, &xml );
			}
			else
			{
				BGAnimation *pBGA = new BGAnimation;
				pBGA->LoadFromAniDir( sDir );
				return pBGA;
			}
		}
	case FT_Bitmap:
	case FT_Movie:
		{
			Sprite* pSprite = new Sprite;
			pSprite->Load( ID );
			return pSprite;
		}
	case FT_Model:
		{
			Model* pModel = new Model;
			pModel->Load( ID.filename );
			return pModel;
		}
	default:
		RageException::Throw("File \"%s\" has unknown type, \"%s\"",
			ID.filename.c_str(), FileTypeToString(ft).c_str() );
	}
}

void ActorUtil::SetXY( Actor& actor, const CString &sType )
{
	ASSERT( !actor.GetName().empty() );

	/*
	 * Hack: We normally SET_XY in Init(), and run ON_COMMAND in BeginScreen.  We
	 * want to load the actor's commands in Init(), since that takes long enough
	 * to skip.  So, run LoadAllCommands here if it hasn't been run yet.
	 */
	if( !actor.HasCommand("On") )	// this actor hasn't loaded commands yet
		LoadAllCommands( actor, sType );

	actor.SetXY( THEME->GetMetricF(sType,actor.GetName()+"X"), THEME->GetMetricF(sType,actor.GetName()+"Y") );
}

void ActorUtil::LoadCommand( Actor& actor, const CString &sType, const CString &sCommandName )
{
	actor.AddCommand( sCommandName, THEME->GetMetricA(sType,actor.GetName()+sCommandName+"Command") );
}

void ActorUtil::LoadAndPlayCommand( Actor& actor, const CString &sType, const CString &sCommandName, Actor* pParent )
{
	// HACK:  It's very often that we command things to TweenOffScreen 
	// that we aren't drawing.  We know that an Actor is not being
	// used if its name is blank.  So, do nothing on Actors with a blank name.
	// (Do "playcommand" anyway; BGAs often have no name.)
	if( sCommandName=="Off" && actor.GetName().empty() )
		return;

	ASSERT_M( 
		!actor.GetName().empty(), 
		ssprintf("!actor.GetName().empty() ('%s', '%s')", sType.c_str(), sCommandName.c_str()) 
		);

	if( !actor.HasCommand(sCommandName ) )	// this actor hasn't loaded commands yet
		LoadAllCommands( actor, sType );

	// If we didn't load the command in LoadAllCommands, load the requested command 
	// explicitly.  The metric is missing, and ThemeManager will prompt.
	if( !actor.HasCommand(sCommandName) )
	{
		// If this metric exists and we didn't load it in LoadAllCommands, then 
		// LoadAllCommands has a bug.
		DEBUG_ASSERT( !THEME->HasMetric(sType,actor.GetName()+sCommandName+"Command") );
		
		LoadCommand( actor, sType, sCommandName );
	}

	actor.PlayCommand( sCommandName, pParent );
}

void ActorUtil::LoadAllCommands( Actor& actor, const CString &sType )
{
	set<CString> vsValueNames;
	THEME->GetMetricsThatBeginWith( sType, actor.GetName(), vsValueNames );

	FOREACHS_CONST( CString, vsValueNames, v )
	{
		const CString &sv = *v;
		if( sv.Right(7) == "Command" )
		{
			CString sCommandName( sv.begin()+actor.GetName().size(), sv.end()-7 );
			LoadCommand( actor, sType, sCommandName );
		}
	}
}

static bool CompareActorsByZPosition(const Actor *p1, const Actor *p2)
{
	return p1->GetZ() < p2->GetZ();
}

void ActorUtil::SortByZPosition( vector<Actor*> &vActors )
{
	// Preserve ordering of Actors with equal Z positions.
	stable_sort( vActors.begin(), vActors.end(), CompareActorsByZPosition );
}

static const CString FileTypeNames[] = {
	"Bitmap", 
	"Movie", 
	"Directory", 
	"Xml", 
	"Model", 
};
XToString( FileType, NUM_FileType );

FileType ActorUtil::GetFileType( const CString &sPath )
{
	CString sExt = GetExtension( sPath );
	sExt.MakeLower();
	
	if( sExt=="xml" )		return FT_Xml;
	else if( 
		sExt=="png" ||
		sExt=="jpg" || 
		sExt=="gif" || 
		sExt=="bmp" )		return FT_Bitmap;
	else if( 
		sExt=="avi" || 
		sExt=="mpeg" || 
		sExt=="mpg" )		return FT_Movie;
	else if( 
		sExt=="txt" )		return FT_Model;
	/* Do this last, to avoid the IsADirectory in most cases. */
	else if( IsADirectory(sPath)  )	return FT_Directory;
	else					return FT_Invalid;
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
