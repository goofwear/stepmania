#include "global.h"
#include "ThemeManager.h"
#include "RageLog.h"
#include "PrefsManager.h"
#include "RageException.h"
#include "RageTimer.h"
#include "RageUtil.h"
#include "IniFile.h"
#include "RageTimer.h"
#include "FontCharAliases.h"
#include "arch/Dialog/Dialog.h"
#include "RageFile.h"
#include "ScreenManager.h"
#include "StepMania.h"
#include "Foreach.h"
#include "ThemeMetric.h"
#include "LuaManager.h"
#include "ScreenDimensions.h"
#include "Command.h"


ThemeManager*	THEME = NULL;	// global object accessable from anywhere in the program

static const CString ElementCategoryNames[] = {
	"BGAnimations",
	"Fonts",
	"Graphics",
	"Sounds",
	"Other"
};
XToString( ElementCategory, NUM_ElementCategory );
StringToX( ElementCategory );


const CString BASE_THEME_NAME = "default";
const CString LANGUAGES_SUBDIR = "Languages/";
const CString BASE_LANGUAGE = "english";
const CString THEMES_DIR  = "Themes/";
const CString METRICS_FILE = "metrics.ini";


struct Theme
{
	CString sThemeName;
	IniFile *iniMetrics;	// pointer because the copy constructor isn't a deep copy
};
// When looking for a metric or an element, search these from head to tail.
static deque<Theme> g_vThemes;


//
// For self-registering metrics
//
#include "SubscriptionManager.h"
template<>
set<IThemeMetric*>* SubscriptionManager<IThemeMetric>::s_pSubscribers = NULL;

void ThemeManager::Subscribe( IThemeMetric *p )
{
	SubscriptionManager<IThemeMetric>::Subscribe( p );

	// It's ThemeManager's responsibility to make sure all of it's subscribers
	// are updated with current data.  If a metric is created after 
	// a theme is loaded, ThemeManager should update it right away (not just
	// when the theme changes).
	if( THEME && THEME->GetCurThemeName().size() )
		p->Read();
}

void ThemeManager::Unsubscribe( IThemeMetric *p )
{
	SubscriptionManager<IThemeMetric>::Unsubscribe( p );
}


/* We spend a lot of time doing redundant theme path lookups.  Cache results. */
static map<CString, CString> g_ThemePathCache[NUM_ElementCategory];
void ThemeManager::ClearThemePathCache()
{
	for( int i = 0; i < NUM_ElementCategory; ++i )
		g_ThemePathCache[i].clear();
}

static void FileNameToClassAndElement( const CString &sFileName, CString &sClassNameOut, CString &sElementOut )
{
	// split into class name and file name
	unsigned iIndexOfFirstSpace = sFileName.find(" ");
	if( iIndexOfFirstSpace == string::npos )	// no space
	{
		sClassNameOut = "";
		sElementOut = sFileName;
	}
	else
	{
		sClassNameOut = sFileName.Left( iIndexOfFirstSpace );
		sElementOut = sFileName.Right( sFileName.size() - iIndexOfFirstSpace - 1 );
	}
}


static CString ClassAndElementToFileName( const CString &sClassName, const CString &sElement )
{
	if( sClassName.empty() )
		return sElement;
	else
		return sClassName + " " + sElement;
}

ThemeManager::ThemeManager()
{
	THEME = this;	// so that we can Register THEME on construction

	/* We don't have any theme loaded until SwitchThemeAndLanguage is called. */
	m_sCurThemeName = "";
	
	CStringArray arrayThemeNames;
	GetThemeNames( arrayThemeNames );
}

ThemeManager::~ThemeManager()
{
	for( unsigned i = 0; i < g_vThemes.size(); ++i )
		delete g_vThemes[i].iniMetrics;
}

void ThemeManager::GetThemeNames( CStringArray& AddTo )
{
	GetDirListing( THEMES_DIR + "*", AddTo, true );
	StripCvs( AddTo );
}

bool ThemeManager::DoesThemeExist( const CString &sThemeName )
{
	CStringArray asThemeNames;	
	GetThemeNames( asThemeNames );
	for( unsigned i=0; i<asThemeNames.size(); i++ )
	{
		if( !sThemeName.CompareNoCase(asThemeNames[i]) )
			return true;
	}
	return false;
}

void ThemeManager::GetLanguages( CStringArray& AddTo )
{
	AddTo.clear();
	CStringArray asTemp;

	/* XXX: this should use FallbackTheme */
	GetLanguagesForTheme( m_sCurThemeName, AddTo );
	GetLanguagesForTheme( BASE_THEME_NAME, asTemp );
	AddTo.insert( AddTo.begin(), asTemp.begin(), asTemp.end() );

	// remove dupes
	sort( AddTo.begin(), AddTo.end() );
	CStringArray::iterator it = unique( AddTo.begin(), AddTo.end() );
	AddTo.erase(it, AddTo.end());
}

bool ThemeManager::DoesLanguageExist( const CString &sLanguage )
{
	CStringArray asLanguages;
	GetLanguages( asLanguages );

	for( unsigned i=0; i<asLanguages.size(); i++ )
		if( sLanguage.CompareNoCase(asLanguages[i])==0 )
			return true;
	return false;
}

void ThemeManager::LoadThemeRecursive( deque<Theme> &theme, const CString &sThemeName )
{
	static int depth = 0;
	static bool loaded_base = false;
	depth++;
	ASSERT_M( depth < 20, "Circular theme fallback references detected." );

	if( !sThemeName.CompareNoCase(BASE_THEME_NAME) )
		loaded_base = true;

	Theme t;
	t.iniMetrics = new IniFile;
	t.sThemeName = sThemeName;
	t.iniMetrics->ReadFile( GetMetricsIniPath(sThemeName) );
	t.iniMetrics->ReadFile( GetLanguageIniPath(sThemeName,BASE_LANGUAGE) );
	if( m_sCurLanguage.CompareNoCase(BASE_LANGUAGE) )
		t.iniMetrics->ReadFile( GetLanguageIniPath(sThemeName,m_sCurLanguage) );

	/* Read the fallback theme.  If no fallback theme is specified, and we havn't
	 * already loaded it, fall back on BASE_THEME_NAME.  That way, default theme
	 * fallbacks can be disabled with "FallbackTheme=". */
	CString sFallback;
	if( !t.iniMetrics->GetValue("Global","FallbackTheme",sFallback) )
	{
		if( sThemeName.CompareNoCase( BASE_THEME_NAME ) && !loaded_base )
			sFallback = BASE_THEME_NAME;
	}
	if( !sFallback.empty() )
		LoadThemeRecursive( theme, sFallback );

	g_vThemes.push_front( t );

	if( !sThemeName.CompareNoCase(sThemeName) )
		loaded_base = false;

	depth--;
}

void ThemeManager::SwitchThemeAndLanguage( const CString &sThemeName, const CString &sLanguage )
{
	CString sTheme = sThemeName;
	CString sLang = sLanguage;
	if( !DoesThemeExist(sTheme) )
		sTheme = BASE_THEME_NAME;
	if( !DoesLanguageExist(sLang) )
		sLang = BASE_LANGUAGE;
	LOG->Trace("ThemeManager::SwitchThemeAndLanguage: \"%s\", \"%s\"",
		sTheme.c_str(), sLang.c_str() );

	if( sTheme == m_sCurThemeName && sLang == m_sCurLanguage )
		return;

	m_sCurThemeName = sTheme;
	m_sCurLanguage = sLang;

	// clear theme path cache
	for( int i = 0; i < NUM_ElementCategory; ++i )
		g_ThemePathCache[i].clear();

	for( unsigned i = 0; i < g_vThemes.size(); ++i )
		delete g_vThemes[i].iniMetrics;
	g_vThemes.clear();

	// load current theme
	LoadThemeRecursive( g_vThemes, m_sCurThemeName );

	CString sMetric;
	for( int i = 0; GetCommandlineArgument( "metric", &sMetric, i ); ++i )
	{
		/* sMetric must be "foo::bar=baz".  "foo" and "bar" never contain "=", so in
		 * "foo::bar=1+1=2", "baz" is always "1+1=2".  Neither foo nor bar may be
		 * empty, but baz may be. */
		Regex re( "^([^=]+)::([^=]+)=(.*)$" );
		vector<CString> sBits;
		if( !re.Compare( sMetric, sBits ) )
			RageException::Throw( "Invalid argument \"--metric=%s\"", sMetric.c_str() );

		g_vThemes.front().iniMetrics->SetValue( sBits[0], sBits[1], sBits[2] );
	}
	
	LOG->MapLog("theme", "Theme: %s", sTheme.c_str());
	LOG->MapLog("language", "Language: %s", sLang.c_str());

	// reload common sounds
	if ( SCREENMAN != NULL )
		SCREENMAN->ThemeChanged();

	/* Lua globals can use metrics which are cached, and vice versa.  Update Lua
	 * globals first; it's Lua's job to explicitly update cached metrics that it
	 * uses. */
	UpdateLuaGlobals();

	// reload subscribers
	FOREACHS_CONST( IThemeMetric*, *SubscriptionManager<IThemeMetric>::s_pSubscribers, p )
		(*p)->Read();
}

void ThemeManager::RunLuaScripts( const CString &sMask )
{
	/* Run all script files with the given mask in Lua for all themes.  Start
	 * from the deepest fallback theme and work outwards. */
	deque<Theme>::const_iterator iter = g_vThemes.end();
	do
	{
		--iter;
		const CString &sThemeDir = GetThemeDirFromName( iter->sThemeName );
		CStringArray asElementPaths;
		GetDirListing( sThemeDir + "Scripts/" + sMask, asElementPaths, false, true );
		for( unsigned i = 0; i < asElementPaths.size(); ++i )
		{
			const CString &sPath = asElementPaths[i];
			LOG->Trace( "Loading \"%s\" ...", sPath.c_str() );
			LuaHelpers::RunScriptFile( sPath );
		}
	}
	while( iter != g_vThemes.begin() );
}

void ThemeManager::UpdateLuaGlobals()
{
	/* Ugly: we need Serialize.lua to be loaded in order to be able to ResetState,
	 * but everything else should be able to depend on globals being set. */
	RunLuaScripts( "Serialize.lua" );
	LUA->ResetState();

	/* Important: explicitly refresh cached metrics that we use. */
	THEME_SCREEN_WIDTH.Read();
	THEME_SCREEN_HEIGHT.Read();

	LUA->SetGlobal( "SCREEN_WIDTH", (int) SCREEN_WIDTH );
	LUA->SetGlobal( "SCREEN_HEIGHT", (int) SCREEN_HEIGHT );
	LUA->SetGlobal( "SCREEN_LEFT", (int) SCREEN_LEFT );
	LUA->SetGlobal( "SCREEN_RIGHT", (int) SCREEN_RIGHT );
	LUA->SetGlobal( "SCREEN_TOP", (int) SCREEN_TOP );
	LUA->SetGlobal( "SCREEN_BOTTOM", (int) SCREEN_BOTTOM );
	LUA->SetGlobal( "SCREEN_CENTER_X", (int) SCREEN_CENTER_X );
	LUA->SetGlobal( "SCREEN_CENTER_Y", (int) SCREEN_CENTER_Y );

	RunLuaScripts( "*.lua" );
}

CString ThemeManager::GetThemeDirFromName( const CString &sThemeName )
{
	return THEMES_DIR + sThemeName + "/";
}

CString ThemeManager::GetPathToRaw( const CString &sThemeName_, ElementCategory category, const CString &sClassName_, const CString &sElement_ ) 
{
	/* Ugly: the parameters to this function may be a reference into g_vThemes, or something
	 * else that might suddenly go away when we call ReloadMetrics. */
	const CString sThemeName = sThemeName_;
	const CString sClassName = sClassName_;
	const CString sElement = sElement_;

try_element_again:

	const CString sThemeDir = GetThemeDirFromName( sThemeName );
	const CString &sCategory = ElementCategoryToString(category);

	CStringArray asElementPaths;

	// If sFileName already has an extension, we're looking for a specific file
	bool bLookingForSpecificFile = sElement.find_last_of('.') != sElement.npos;

	if( bLookingForSpecificFile )
	{
		GetDirListing( sThemeDir + sCategory+"/"+ClassAndElementToFileName(sClassName,sElement), asElementPaths, false, true );
	}
	else	// look for all files starting with sFileName that have types we can use
	{
		CStringArray asPaths;
		GetDirListing( sThemeDir + sCategory + "/" + ClassAndElementToFileName(sClassName,sElement) + "*",
						asPaths, false, true );

		for( unsigned p = 0; p < asPaths.size(); ++p )
		{
			static const char *masks[NUM_ElementCategory][14] = {
				{ "redir", "", "xml", NULL },
				{ "redir", "ini", NULL },
				{ "redir", "xml", "png", "jpg", "bmp", "gif","avi", "mpg", "mpeg", "txt", "", NULL},
				{ "redir", "mp3", "ogg", "wav", NULL },
				{ "*", NULL },
			};		
			const char **asset_masks = masks[category];

			const CString ext = GetExtension( asPaths[p] );

			for( int i = 0; asset_masks[i]; ++i )
			{
				/* No extension means directories. */
				if( asset_masks[i][0] == 0 && !IsADirectory(asPaths[p]) )
					continue;

				if( ext == asset_masks[i] || !strcmp(asset_masks[i], "*") )
				{
					asElementPaths.push_back( asPaths[p] );
					break;
				}
			}
		}
	}
	

	if( asElementPaths.size() == 0 )
		return NULL;	// This isn't fatal.

	if( asElementPaths.size() > 1 )
	{
		FlushDirCache();
		g_ThemePathCache[category].clear();

		CString message = ssprintf( 
			"ThemeManager:  There is more than one theme element element that matches "
			"'%s/%s/%s'.  Please remove all but one of these matches.",
			sThemeName.c_str(), sCategory.c_str(), ClassAndElementToFileName(sClassName,sElement).c_str() );

		switch( Dialog::AbortRetryIgnore(message) )
		{
		case Dialog::abort:
			RageException::Throw( message ); 
			break;
		case Dialog::retry:
			FlushDirCache();
			ReloadMetrics();
			goto try_element_again;
		case Dialog::ignore:
			break;
		}
	}


	CString sPath = asElementPaths[0];
	bool bIsARedirect = GetExtension(sPath).CompareNoCase("redir")==0;

	if( !bIsARedirect )
		return sPath;

	CString sNewFileName;
	GetFileContents( sPath, sNewFileName, true );

	CString sNewClassName, sNewFile;
	FileNameToClassAndElement(sNewFileName, sNewClassName, sNewFile);
	
	/* Search again.  For example, themes/default/Fonts/foo might redir
	 * to "Hello"; but "Hello" might be overridden in themes/hot pink/Fonts/Hello. */
	/* Important: We need to do a full search.  For example, BG redirs in
	 * the default theme point to "_shared background", and themes override
	 * just "_shared background"; the redirs in the default theme should end
	 * up resolving to the overridden background. */
	/* Use GetPathToOptional because we don't want report that there's an element
	 * missing.  Instead we want to report that the redirect is invalid. */
	CString sNewPath = GetPath(category, sNewClassName, sNewFile, true);

	if( !sNewPath.empty() )
	{
		return sNewPath;
	}
	else
	{
		CString message = ssprintf(
				"ThemeManager:  The redirect '%s' points to the file '%s', which does not exist. "
				"Verify that this redirect is correct.",
				sPath.c_str(), sNewFileName.c_str());

		if( Dialog::AbortRetryIgnore(message) == Dialog::retry )
		{
			FlushDirCache();
			ReloadMetrics();
			goto try_element_again;
		}

		RageException::Throw( "%s", message.c_str() ); 
	}
}

CString ThemeManager::GetPathToAndFallback( ElementCategory category, const CString &sClassName_, const CString &sElement ) 
{
	CString sClassName( sClassName_ );

	int n = 100;
	while( n-- )
	{
		FOREACHD_CONST( Theme, g_vThemes, iter )
		{
			// search with requested name
			CString sRet = GetPathToRaw( iter->sThemeName, category, sClassName, sElement );
			if( !sRet.empty() )
				return sRet;
		}

		if( sClassName.empty() )
			return CString();

		// search fallback name (if any)
		CString sFallback;
		if( !GetMetricRawRecursive(sClassName, "Fallback", sFallback) )
			return CString();
		sClassName = sFallback;
	}

	RageException::Throw( "Infinite recursion looking up theme element \"%s\"",
		ClassAndElementToFileName(sClassName, sElement).c_str() );
}

CString ThemeManager::GetPath( ElementCategory category, const CString &sClassName_, const CString &sElement_, bool bOptional ) 
{
	/* Ugly: the parameters to this function may be a reference into g_vThemes, or something
	 * else that might suddenly go away when we call ReloadMetrics. */
	const CString sClassName = sClassName_;
	const CString sElement = sElement_;

	CString sFileName = ClassAndElementToFileName( sClassName, sElement );

	map<CString, CString> &Cache = g_ThemePathCache[category];
	{
		map<CString, CString>::const_iterator i;
		
		i = Cache.find( sFileName );
		if( i != Cache.end() )
			return i->second;
	}
	
try_element_again:
	
	// search the current theme
	CString ret = GetPathToAndFallback( category, sClassName, sElement );
	if( !ret.empty() )	// we found something
	{
		Cache[sFileName] = ret;
		return ret;
	}

	if( bOptional )
	{
		Cache[sFileName] = "";
		return NULL;
	}

	const CString &sCategory = ElementCategoryToString(category);

	/* We can't fall back on _missing in Other: the file types are unknown. */
	CString sMessage = "The theme element \"" + sCategory + "/" + sFileName +"\" is missing.";
	Dialog::Result res;
	if( category != EC_OTHER )
		res = Dialog::AbortRetryIgnore(sMessage, "MissingThemeElement");
	else
		res = Dialog::AbortRetry(sMessage, "MissingThemeElement");
	switch( res )
	{
	case Dialog::retry:
		FlushDirCache();
		ReloadMetrics();
		goto try_element_again;
	case Dialog::ignore:
		LOG->Warn( 
			"Theme element '%s/%s' could not be found in '%s' or '%s'.", 
			sCategory.c_str(),
			sFileName.c_str(), 
			GetThemeDirFromName(m_sCurThemeName).c_str(), 
			GetThemeDirFromName(BASE_THEME_NAME).c_str() );

		/* Err? */
		if(sFileName == "_missing")
			RageException::Throw("'_missing' isn't present in '%s%s'", GetThemeDirFromName(BASE_THEME_NAME).c_str(), sCategory.c_str() );

		Cache[sFileName] = GetPath( category, "", "_missing" );
		return Cache[sFileName];
	case Dialog::abort:
		RageException::Throw( "Theme element '%s/%s' could not be found in '%s' or '%s'.", 
			sCategory.c_str(),
			sFileName.c_str(), 
			GetThemeDirFromName(m_sCurThemeName).c_str(), 
			GetThemeDirFromName(BASE_THEME_NAME).c_str() );
	default:
		ASSERT(0);
		return NULL;
	}
}


CString ThemeManager::GetMetricsIniPath( const CString &sThemeName )
{
	return GetThemeDirFromName( sThemeName ) + METRICS_FILE;
}

bool ThemeManager::HasMetric( const CString &sClassName, const CString &sValueName )
{
	CString sThrowAway;
	return GetMetricRawRecursive( sClassName, sValueName, sThrowAway );
}

void ThemeManager::ReloadMetrics()
{
	// force a reload of the metrics cache
	const CString sThemeName = m_sCurThemeName;
	const CString sCurLanguage = m_sCurLanguage;
	m_sCurThemeName = "";
	m_sCurLanguage = "";

	SwitchThemeAndLanguage( sThemeName, sCurLanguage );
	if( SCREENMAN )
		SCREENMAN->SystemMessage( "Reloaded metrics" );

	//
	// clear theme path cache
	//
	for( int i = 0; i < NUM_ElementCategory; ++i )
		g_ThemePathCache[i].clear();
}


bool ThemeManager::GetMetricRawRecursive( const CString &sClassName_, const CString &sValueName, CString &sOut )
{
	CString sClassName( sClassName_ );

	int n = 100;
	while( n-- )
	{
		FOREACHD_CONST( Theme, g_vThemes, iter )
		{
			if( iter->iniMetrics->GetValue(sClassName,sValueName,sOut) )
				return true;
		}

		if( !sValueName.compare("Fallback") )
			return false;

		CString sFallback;
		if( !GetMetricRawRecursive(sClassName,"Fallback",sFallback) )
			return false;

		sClassName = sFallback;
	}

	RageException::Throw( "Infinite recursion looking up theme metric \"%s::%s\"", sClassName.c_str(), sValueName.c_str() );
}

CString ThemeManager::GetMetricRaw( const CString &sClassName_, const CString &sValueName_ )
{
	/* Ugly: the parameters to this function may be a reference into g_vThemes, or something
	 * else that might suddenly go away when we call ReloadMetrics. */
	const CString sClassName = sClassName_;
	const CString sValueName = sValueName_;

try_metric_again:

	CString ret;
	if( ThemeManager::GetMetricRawRecursive( sClassName, sValueName, ret ) )
		return ret;


	CString sMessage = ssprintf( "The theme metric '%s-%s' is missing.  Correct this and click Retry, or Cancel to break.",sClassName.c_str(),sValueName.c_str() );
	switch( Dialog::AbortRetryIgnore(sMessage) )
	{
	case Dialog::abort:
		break;	// fall through
	case Dialog::retry:
		FlushDirCache();
		ReloadMetrics();
		goto try_metric_again;
	case Dialog::ignore:
		return NULL;
	default:
		ASSERT(0);
	}

	CString sCurMetricPath = GetMetricsIniPath(m_sCurThemeName);
	CString sDefaultMetricPath = GetMetricsIniPath(BASE_THEME_NAME);

	CString sError = ssprintf( "Theme metric '%s : %s' could not be found in '%s' or '%s'.", 
		sClassName.c_str(),
		sValueName.c_str(),
		sCurMetricPath.c_str(), 
		sDefaultMetricPath.c_str()
		);
	RageException::Throw( sError );
}

/* Get a string metric. */
CString ThemeManager::GetMetric( const CString &sClassName, const CString &sValueName )
{
	CString sValue = GetMetricRaw(sClassName,sValueName);

	EvaluateString( sValue );

	return sValue;
}

void ThemeManager::EvaluateString( CString &sText )
{
	/* If the string begins with an @, then this is a Lua expression
	 * that should be evaluated immediately.
	 * Still do font aliases on the resulting string. */
	LuaHelpers::RunAtExpressionS( sText );

	// "::" means newline since you can't use line breaks in an ini file.
	// XXX: this makes it impossible to put a colon at the end of a line, eg: "Color:\nRed"
	sText.Replace("::","\n");

	FontCharAliases::ReplaceMarkers( sText );
}

int ThemeManager::GetMetricI( const CString &sClassName, const CString &sValueName )
{
	CString sValue = GetMetric( sClassName, sValueName );	// Use non-raw so that Lua expressions are allowed

	LuaHelpers::PrepareExpression( sValue );

	return LuaHelpers::RunExpressionI( sValue );
}

float ThemeManager::GetMetricF( const CString &sClassName, const CString &sValueName )
{
	CString sValue = GetMetric( sClassName, sValueName );	// Use non-raw so that Lua expressions are allowed

	LuaHelpers::PrepareExpression( sValue );

	return LuaHelpers::RunExpressionF( sValue );
}

// #include "LuaManager.h"
bool ThemeManager::GetMetricB( const CString &sClassName, const CString &sValueName )
{
	CString sValue = GetMetric( sClassName, sValueName );	// Use non-raw so that Lua expressions are allowed

	/* Watch out: "0" and "1" are not false and true in Lua (all string values are
	 * true).  Make sure that we catch all values that are supposed to be simple
	 * booleans. */
	TrimLeft( sValue );
	TrimRight( sValue );
	if( sValue.Left(1) == "0" )
		return false; /* optimization */
	if( sValue.Left(1) == "1" )
		return true; /* optimization */

	LuaHelpers::PrepareExpression( sValue );
	
	return LuaHelpers::RunExpressionB( sValue );
}

RageColor ThemeManager::GetMetricC( const CString &sClassName, const CString &sValueName )
{
	CString sValue = GetMetric( sClassName, sValueName );	// Use non-raw so that Lua expressions are allowed

	RageColor ret(1,1,1,1);
	if( !ret.FromString(sValue) )
		LOG->Warn( "The color value '%s' for metric '%s : %s' is invalid.", sValue.c_str(), sClassName.c_str(), sValueName.c_str() );
	return ret;
}

Commands ThemeManager::GetMetricM( const CString &sClassName, const CString &sValueName )
{
	CString sValue = GetMetric( sClassName, sValueName );	// Use non-raw so that Lua expressions are allowed
	return ParseCommands( sValue );
}

apActorCommands ThemeManager::GetMetricA( const CString &sClassName, const CString &sValueName )
{
	CString sValue = GetMetricRaw( sClassName, sValueName );
	return apActorCommands( new ActorCommands( sValue ) );
}

void ThemeManager::NextTheme()
{
	CStringArray as;
	GetThemeNames( as );
	unsigned i;
	for( i=0; i<as.size(); i++ )
		if( as[i].CompareNoCase(m_sCurThemeName)==0 )
			break;
	int iNewIndex = (i+1)%as.size();
	SwitchThemeAndLanguage( as[iNewIndex], m_sCurLanguage );
}

void ThemeManager::GetLanguagesForTheme( const CString &sThemeName, CStringArray& asLanguagesOut )
{
	CString sLanguageDir = GetThemeDirFromName(sThemeName) + LANGUAGES_SUBDIR;
	CStringArray as;
	GetDirListing( sLanguageDir + "*.ini", as );
	
	// stip out metrics.ini
	for( int i=as.size()-1; i>=0; i-- )
	{
		if( as[i].CompareNoCase(METRICS_FILE)==0 )
			as.erase( as.begin()+i );
		// strip ".ini"
		as[i] = as[i].Left( as[i].size()-4 );
	}

	asLanguagesOut = as;
}

CString ThemeManager::GetLanguageIniPath( const CString &sThemeName, const CString &sLanguage )
{
	return GetThemeDirFromName(sThemeName) + LANGUAGES_SUBDIR + sLanguage + ".ini";
}

void ThemeManager::GetModifierNames( vector<CString>& AddTo )
{
	for( deque<Theme>::const_iterator iter = g_vThemes.begin();
		iter != g_vThemes.end();
		++iter )
	{
		const XNode *cur = iter->iniMetrics->GetChild( "OptionNames" );
		if( cur )
		{
			FOREACH_CONST_Attr( cur, p )
				AddTo.push_back( p->m_sName );
		}
	}
}

void ThemeManager::GetMetric( const CString &sClassName, const CString &sValueName, apActorCommands &valueOut )
{
	valueOut = GetMetricA( sClassName, sValueName ); 
}

void ThemeManager::GetMetric( const CString &sClassName, const CString &sValueName, LuaExpression &valueOut )
{
	CString sValue = GetMetricRaw( sClassName, sValueName );
	valueOut.SetFromExpression( "function(self) " + sValue + "end" );
}

void ThemeManager::GetMetricsThatBeginWith( const CString &sClassName_, const CString &sValueName, set<CString> &vsValueNamesOut )
{
	CString sClassName( sClassName_ );
	while( !sClassName.empty() )
	{
		// Iterate over themes in the chain.
		FOREACHD_CONST( Theme, g_vThemes, i )
		{
			const XNode *cur = i->iniMetrics->GetChild( sClassName );
			if( cur == NULL )
				continue;

			// Iterate over all metrics that match.
			for( XAttrs::const_iterator j = cur->m_attrs.lower_bound( sValueName ); j != cur->m_attrs.end(); ++j )
			{
				const CString &sv = j->first;
				if( sv.Left(sValueName.size()) == sValueName )
					vsValueNamesOut.insert( sv );
				else	// we passed the last metric that matched sValueName
					break;
			}
		}

		// put the fallback (if any) in sClassName
		CString sFallback;
		if( GetMetricRawRecursive( sClassName, "Fallback", sFallback ) )
			sClassName = sFallback;
		else
			sClassName = "";
	}
}

void ThemeManager::LoadPreferencesFromSection( const CString &sClassName )
{
	set<CString> asNames;

	GetMetricsThatBeginWith( sClassName, "", asNames );

	/* If "Theme" isn't set, we're not changing the theme.  Break out and
		* load other preferences. */
	if( asNames.find("Theme") != asNames.end() )
	{
		/* Change the theme.  Only do this once. */
		CString sTheme;
		GetMetric( sClassName, "Theme", sTheme );
		THEME->SwitchThemeAndLanguage( sTheme, PREFSMAN->m_sLanguage );

		asNames.erase( "Theme" );
	}

	FOREACHS( CString, asNames, sName )
	{
		if( !sName->CompareNoCase("Fallback") )
			continue;

		IPreference *pPref = PREFSMAN->GetPreferenceByName( *sName );
		if( pPref == NULL )
		{
			LOG->Warn( "Unknown preference in [%s]: %s", sClassName.c_str(), sName->c_str() );
			continue;
		}

		CString sVal;
		GetMetric( sClassName, *sName, sVal );

		pPref->FromString( sVal );
	}
}

CString ThemeManager::GetBlankGraphicPath()
{
	return THEMES_DIR + BASE_THEME_NAME + "/" + ElementCategoryToString(EC_GRAPHICS) + "/_blank.png";
}

// lua start
#include "LuaBinding.h"

class LunaThemeManager: public Luna<ThemeManager>
{
public:
	LunaThemeManager() { LUA->Register( Register ); }

	static int GetMetric( T* p, lua_State *L )			{ lua_pushstring(L, p->GetMetric(SArg(1),SArg(2)) ); return 1; }
	static int GetPathG( T* p, lua_State *L )			{ lua_pushstring(L, p->GetPathG(SArg(1),SArg(2)) ); return 1; }
	static int GetPathB( T* p, lua_State *L )			{ lua_pushstring(L, p->GetPathB(SArg(1),SArg(2)) ); return 1; }
	static int GetPathS( T* p, lua_State *L )			{ lua_pushstring(L, p->GetPathS(SArg(1),SArg(2)) ); return 1; }

	static void Register(lua_State *L)
	{
		ADD_METHOD( GetMetric );
		ADD_METHOD( GetPathG );
		ADD_METHOD( GetPathB );
		ADD_METHOD( GetPathS );

		Luna<T>::Register( L );

		// Add global singleton if constructed already.  If it's not constructed yet,
		// then we'll register it later when we reinit Lua just before 
		// initializing the display.
		if( THEME )
		{
			lua_pushstring(L, "THEME");
			THEME->PushSelf( L );
			lua_settable(L, LUA_GLOBALSINDEX);
		}
	}
};

LUA_REGISTER_CLASS( ThemeManager )
// lua end


/*
 * (c) 2001-2004 Chris Danford
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
