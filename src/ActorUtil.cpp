#include "global.h"
#include "ActorUtil.h"
#include "Sprite.h"
#include "Model.h"
#include "ThemeManager.h"
#include "RageFileManager.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "EnumHelper.h"
#include "XmlFile.h"
#include "XmlFileUtil.h"
#include "LuaManager.h"
#include "Foreach.h"

#include "arch/Dialog/Dialog.h"


// Actor registration
static map<RString,CreateActorFn>	*g_pmapRegistrees = NULL;

static bool IsRegistered( const RString& sClassName )
{
	return g_pmapRegistrees->find( sClassName ) != g_pmapRegistrees->end();
}

void ActorUtil::Register( const RString& sClassName, CreateActorFn pfn )
{
	if( g_pmapRegistrees == NULL )
		g_pmapRegistrees = new map<RString,CreateActorFn>;

	map<RString,CreateActorFn>::iterator iter = g_pmapRegistrees->find( sClassName );
	ASSERT_M( iter == g_pmapRegistrees->end(), ssprintf("Actor class '%s' already registered.", sClassName.c_str()) );

	(*g_pmapRegistrees)[sClassName] = pfn;
}

Actor* ActorUtil::Create( const RString& sClassName, const XNode* pNode, Actor *pParentActor )
{
	map<RString,CreateActorFn>::iterator iter = g_pmapRegistrees->find( sClassName );
	ASSERT_M( iter != g_pmapRegistrees->end(), ssprintf("Actor '%s' is not registered.",sClassName.c_str()) );

	const CreateActorFn &pfn = iter->second;
	Actor *pRet = pfn();

	if( pParentActor )
		pRet->SetParent( pParentActor );

	pRet->LoadFromNode( pNode );
	return pRet;
}

void ActorUtil::ResolvePath( RString &sPath, const RString &sName )
{
	const RString sOriginalPath( sPath );

retry:
	CollapsePath( sPath );

	// If we know this is an exact match, don't bother with the GetDirListing,
	// so "foo" doesn't partial match "foobar" if "foo" exists.
	RageFileManager::FileType ft = FILEMAN->GetFileType( sPath );
	if( ft != RageFileManager::TYPE_FILE && ft != RageFileManager::TYPE_DIR )
	{
		vector<RString> asPaths;
		GetDirListing( sPath + "*", asPaths, false, true );	// return path too

		if( asPaths.empty() )
		{
			RString sError = ssprintf( "A file in '%s' references a file '%s' which doesn't exist.", sName.c_str(), sPath.c_str() );
			switch( Dialog::AbortRetryIgnore( sError, "BROKEN_FILE_REFERENCE" ) )
			{
			case Dialog::abort:
				RageException::Throw( "%s", sError.c_str() ); 
				break;
			case Dialog::retry:
				// XXX: Do we need to flush everything?
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

		THEME->FilterFileLanguages( asPaths );

		if( asPaths.size() > 1 )
		{
			RString sError = ssprintf( "A file in '%s' references a file '%s' which has multiple matches.", sName.c_str(), sPath.c_str() );
			sError += "\n" + join( "\n", asPaths );
			switch( Dialog::AbortRetryIgnore( sError, "BROKEN_FILE_REFERENCE" ) )
			{
			case Dialog::abort:
				RageException::Throw( "%s", sError.c_str() ); 
				break;
			case Dialog::retry:
				// XXX: Do we need to flush everything?
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

static void PushParamsTable( Lua *L )
{
	lua_pushstring( L, "P" );
	lua_rawget( L, LUA_GLOBALSINDEX );
	if( lua_isnil(L, -1) )
	{
		lua_pop( L, 1 );
		lua_newtable( L );
		lua_pushstring( L, "P" );
		lua_pushvalue( L, -2 );
		lua_rawset( L, LUA_GLOBALSINDEX );
	}
}

/* Set an input parameter to the first value on the stack.  If pOld is non-NULL,
 * set it to the old value.  The value used on the stack will be removed. */
void ActorUtil::SetParamFromStack( Lua *L, RString sName, LuaReference *pOld )
{
	int iValue = lua_gettop(L);

	PushParamsTable( L );
	int iParams = lua_gettop(L);

	/* Save the old value. */
	if( pOld != NULL )
	{
		lua_getfield( L, iParams, sName );
		pOld->SetFromStack( L );
	}

	/* Set the value in the table. */
	lua_pushvalue( L, iValue );
	lua_setfield( L, iParams, sName );

	lua_settop( L, iValue-1 );
}

/* Look up a param set with SetParamFromStack, and push it on the stack. */
void ActorUtil::GetParam( Lua *L, const RString &sName )
{
	/* Search the params table. */
	PushParamsTable( L );
	LuaHelpers::Push( L, sName );
	lua_rawget( L, -2 );
	lua_remove( L, -2 );
}

Actor* ActorUtil::LoadFromNode( const XNode* pNode, Actor *pParentActor )
{
	ASSERT( pNode );

	{
		bool bCond;
		if( pNode->GetAttrValue("Condition", bCond) && !bCond )
			return NULL;
	}

	// Load Params
	map<RString, LuaReference> setOldParams;
	{
		FOREACH_CONST_Child( pNode, pChild )
		{
			if( pChild->GetName() == "Param" )
			{
				RString sName;
				if( !pChild->GetAttrValue( "Name", sName ) )
					Dialog::OK( ssprintf("%s: Param: missing the attribute \"Name\"", ActorUtil::GetWhere(pNode).c_str()), "MISSING_ATTRIBUTE" );

				Lua *L = LUA->Get();
				if( !pChild->PushAttrValue( L, "Value" ) )
					Dialog::OK( ssprintf("%s: Param: missing the attribute \"Value\"", ActorUtil::GetWhere(pNode).c_str()), "MISSING_ATTRIBUTE" );

				SetParamFromStack( L, sName, &setOldParams[sName] );
				LUA->Release(L);
			}
		}
	}


	// Element name is the type in XML.
	// Type= is the name in INI.
	// TODO: Remove the backward compat fallback
	RString sClass = pNode->GetName();
	bool bHasClass = pNode->GetAttrValue( "Class", sClass );
	if( !bHasClass )
		bHasClass = pNode->GetAttrValue( "Type", sClass );	// for backward compatibility

	// backward compat hack
	if( !bHasClass )
	{
		RString sText;
		if( pNode->GetAttrValue( "Text", sText ) )
			sClass = "BitmapText";
	}

	Actor *pReturn = NULL;

	if( IsRegistered(sClass) )
	{
		pReturn = ActorUtil::Create( sClass, pNode, pParentActor );
	}
	else // sClass is empty or garbage (e.g. "1" or "0 // 0==Sprite")
	{
		// automatically figure out the type
		RString sFile;
		ActorUtil::GetAttrPath( pNode, "File", sFile );

		FixSlashesInPlace( sFile );

		/* Be careful: if sFile is "", and we don't check it, then we can end up recursively
		 * loading the layer we're in. */
		if( sFile == "" )
		{
			RString sError = ssprintf( "%s: missing the File attribute or has an invalid Class \"%s\"",
				ActorUtil::GetWhere(pNode).c_str(), sClass.c_str() );
			Dialog::OK( sError );
			pReturn = new Actor;	// Return a dummy object so that we don't crash in AutoActor later.
			goto all_done;
		}

		ActorUtil::ResolvePath( sFile, GetSourcePath(pNode) );

		pReturn = ActorUtil::MakeActor( sFile, pNode, pParentActor );
		if( pReturn == NULL )
			goto all_done;
	}

all_done:

	// Unload Params
	{
		Lua *L = LUA->Get();
		FOREACHM( RString, LuaReference, setOldParams, old )
		{
			old->second.PushSelf( L );
			SetParamFromStack( L, old->first, NULL );
		}
		LUA->Release(L);
	}

	return pReturn;
}

/*
 * Merge commands from a parent into a child.  For example,
 *
 * parent.xml: <Layer File="child" OnCommand="x,10" />
 * child.xml:  <Layer1 File="image" OffCommand="y,10" />
 *
 * results in:
 *
 * <Layer1 File="image" OnCommand="x,10" OffCommand="y,10" />
 *
 * Warn about duplicate commands.
 */
static void MergeActorXML( XNode *pChild, const XNode *pParent )
{
	FOREACH_CONST_Child( pParent, p )
		pChild->AppendChild( new XNode(*p) );

	FOREACH_CONST_Attr( pParent, p )
	{
		const RString &sName = p->first;
		if( !EndsWith(sName, "Command") )
			continue;

		if( pChild->GetAttr(p->first) != NULL )
		{
			RString sWarning = 
				ssprintf( "%s: overriding \"%s\" in %s node \"%s\"",
				ActorUtil::GetWhere(pParent).c_str(),
				sName.c_str(),
				ActorUtil::GetWhere(pChild).c_str(),
				pParent->GetName().c_str() );
			Dialog::OK( sWarning, "XML_ATTRIB_OVERRIDE" );
		}
		pChild->AppendAttrFrom( p->first, p->second->Copy() );
	}
}

namespace
{
	void AnnotateXMLTree( XNode *pNode, const RString &sFile )
	{
		RString sDir = Dirname( sFile );

		vector<XNode *> queue;
		queue.push_back( pNode );
		while( !queue.empty() )
		{
			pNode = queue.back();
			queue.pop_back();
			FOREACH_Child( pNode, pChild )
				queue.push_back( pChild );

			/* Source file, for error messages: */
			pNode->AppendAttr( "_Source", sFile );

			/* Directory of caller, for relative paths: */
			pNode->AppendAttr( "_Dir", sDir );
		}

	}

	XNode *LoadXNodeFromLuaShowErrors( const RString &sFile )
	{
		RString sScript;
		if( !GetFileContents(sFile, sScript) )
			return NULL;

		Lua *L = LUA->Get();

		RString sError;
		if( !LuaHelpers::RunScript(L, sScript, "@" + sFile, sError, 1) )
		{
			lua_pop( L, 1 );
			LUA->Release( L );
			sError = ssprintf( "Lua runtime error: %s", sError.c_str() );
			Dialog::OK( sError, "LUA_ERROR" );
			return NULL;
		}

		if( lua_type(L, -1) != LUA_TTABLE )
		{
			lua_pop( L, 1 );
			LUA->Release( L );
			sError = ssprintf( "%s: must return a table", sFile.c_str() );
			Dialog::OK( sError, "LUA_ERROR" );
			return NULL;
		}

		XNode *pRet = XmlFileUtil::XNodeFromTable( L );

		LUA->Release( L );
		return pRet;
	}
}

/*
 * If pParent is non-NULL, it's the parent node when nesting XML, which is
 * used only by ActorUtil::LoadFromNode.
 */
Actor* ActorUtil::MakeActor( const RString &sPath_, const XNode *pParent, Actor *pParentActor )
{
	static const XNode dummy;
	if( pParent == NULL )
		pParent = &dummy;

	RString sPath( sPath_ );

	/* If @ expressions are allowed through this path, we've already
	 * evaluated them.  Make sure we don't allow double-evaluation. */
	ASSERT_M( sPath.empty() || sPath[0] != '@', sPath );

	FileType ft = GetFileType( sPath );
	if( ft == FT_Directory && sPath.Right(1) != "/" )
		sPath += '/';

	if( ft == FT_Directory )
	{
		RString sXMLPath = sPath + "default.xml";
		if( DoesFileExist(sXMLPath) )
		{
			sPath = sXMLPath;
			ft = FT_Xml;
		}
	}

	if( ft == FT_Directory )
	{
		RString sLuaPath = sPath + "default.lua";
		if( DoesFileExist(sLuaPath) )
		{
			sPath = sLuaPath;
			ft = FT_Lua;
		}
	}

	RString sDir = Dirname( sPath );
	switch( ft )
	{
	case FT_Xml:
		{
			XNode xml;
			if( !XmlFileUtil::LoadFromFileShowErrors(xml, sPath) )
			{
				// XNode will warn about the error
				return new Actor;
			}

			XmlFileUtil::CompileXNodeTree( &xml, sPath );
			AnnotateXMLTree( &xml, sPath );
			MergeActorXML( &xml, pParent );
			return ActorUtil::LoadFromNode( &xml );
		}
	case FT_Lua:
		{
			auto_ptr<XNode> pNode( LoadXNodeFromLuaShowErrors(sPath) );
			if( pNode.get() == NULL )
			{
				// XNode will warn about the error
				return new Actor;
			}

			MergeActorXML( pNode.get(), pParent );
			Actor *pRet = ActorUtil::LoadFromNode( pNode.get() );
			return pRet;
		}
	case FT_Directory:
		{
			XNode xml;
			xml.AppendAttr( "AniDir", sPath );

			return ActorUtil::Create( "BGAnimation", &xml, pParentActor );
		}
	case FT_Bitmap:
	case FT_Movie:
		{
			XNode xml( *pParent );
			xml.AppendAttr( "Texture", sPath );

			return ActorUtil::Create( "Sprite", &xml, pParentActor );
		}
	case FT_Model:
		{
			XNode xml( *pParent );
			xml.AppendAttr( "Meshes", sPath );
			xml.AppendAttr( "Materials", sPath );
			xml.AppendAttr( "Bones", sPath );

			return ActorUtil::Create( "Model", &xml, pParentActor );
		}
	default:
		RageException::Throw("File \"%s\" has unknown type, \"%s\".",
				     sPath.c_str(), FileTypeToString(ft).c_str() );
	}
}

RString ActorUtil::GetSourcePath( const XNode *pNode )
{
	RString sRet;
	pNode->GetAttrValue( "_Source", sRet );
	if( sRet.substr(0, 1) == "@" )
		sRet.erase( 0, 1 );

	return sRet;
}

RString ActorUtil::GetWhere( const XNode *pNode )
{
	RString sPath = GetSourcePath( pNode );

	int iLine;
	if( pNode->GetAttrValue("_Line", iLine) )
		sPath += ssprintf( ":%i", iLine );
	return sPath;
}

bool ActorUtil::GetAttrPath( const XNode *pNode, const RString &sName, RString &sOut )
{
	if( !pNode->GetAttrValue(sName, sOut) )
		return false;

	bool bIsRelativePath = sOut.Left(1) != "/";
	if( bIsRelativePath )
	{
		RString sDir;
		if( !pNode->GetAttrValue("_Dir", sDir) )
		{
			LOG->Warn( "Relative path \"%s\", but path is unknown", sOut.c_str() );
			return false;
		}
		sOut = sDir+sOut;
	}

	return true;
}

apActorCommands ActorUtil::ParseActorCommands( const RString &sCommands, const RString &sName )
{
	Lua *L = LUA->Get();
	LuaHelpers::ParseCommandList( L, sCommands, sName );
	LuaReference *pRet = new LuaReference;
	pRet->SetFromStack( L );
	LUA->Release( L );

	return apActorCommands( pRet );
}

void ActorUtil::SetXY( Actor& actor, const RString &sType )
{
	ASSERT( !actor.GetName().empty() );

	/*
	 * Hack: We normally SET_XY in Init(), and run ON_COMMAND in BeginScreen.  We
	 * want to load the actor's commands in Init(), since that takes long enough
	 * to skip.  So, run LoadAllCommands here if it hasn't been run yet.
	 */
	if( !actor.HasCommand("On") )	// this actor hasn't loaded commands yet
		LoadAllCommands( actor, sType );

	/*
	 * Hack: This is run after InitCommand, and InitCommand might set X/Y.  If
	 * these are both 0, leave the actor where it is.  If InitCommand doesn't,
	 * then 0,0 is the default, anyway.
	 */
	float fX = THEME->GetMetricF(sType,actor.GetName()+"X");
	float fY = THEME->GetMetricF(sType,actor.GetName()+"Y");
	if( fX != 0 || fY != 0 )
		actor.SetXY( fX, fY );
}

void ActorUtil::LoadCommand( Actor& actor, const RString &sType, const RString &sCommandName )
{
	ActorUtil::LoadCommandFromName( actor, sType, sCommandName, actor.GetName() );
}

void ActorUtil::LoadCommandFromName( Actor& actor, const RString &sType, const RString &sCommandName, const RString &sName )
{
	actor.AddCommand( sCommandName, THEME->GetMetricA(sType,sName+sCommandName+"Command") );
}

void ActorUtil::LoadAndPlayCommand( Actor& actor, const RString &sType, const RString &sCommandName )
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

	actor.PlayCommand( sCommandName );
}

void ActorUtil::LoadAllCommands( Actor& actor, const RString &sType )
{
	LoadAllCommandsFromName( actor, sType, actor.GetName() );
}

void ActorUtil::LoadAllCommandsFromName( Actor& actor, const RString &sType, const RString &sName )
{
	set<RString> vsValueNames;
	THEME->GetMetricsThatBeginWith( sType, sName, vsValueNames );

	FOREACHS_CONST( RString, vsValueNames, v )
	{
		const RString &sv = *v;
		static RString sEnding = "Command"; 
		if( EndsWith(sv,sEnding) )
		{
			RString sCommandName( sv.begin()+sName.size(), sv.end()-sEnding.size() );
			LoadCommandFromName( actor, sType, sCommandName, sName );
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

static const char *FileTypeNames[] = {
	"Bitmap", 
	"Movie", 
	"Directory", 
	"Xml", 
	"Lua", 
	"Model", 
};
XToString( FileType, NUM_FileType );

FileType ActorUtil::GetFileType( const RString &sPath )
{
	RString sExt = GetExtension( sPath );
	sExt.MakeLower();
	
	if( sExt=="xml" )		return FT_Xml;
	else if( sExt=="lua" )		return FT_Lua;
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
	else if( sPath.size() > 0 && sPath[sPath.size()-1] == '/' )
					return FT_Directory;
	/* Do this last, to avoid the IsADirectory in most cases. */
	else if( IsADirectory(sPath)  )	return FT_Directory;
	else				return FileType_Invalid;
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
