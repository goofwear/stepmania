#include "global.h"
/*
-----------------------------------------------------------------------------
 Class: ProfileHtml

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "ProfileHtml.h"
#include "ThemeManager.h"
#include "RageFile.h"
#include "RageLog.h"
#include "SongManager.h"
#include "song.h"
#include "Steps.h"
#include <ctime>
#include "GameManager.h"
#include "Course.h"
#include "Bookkeeper.h"
#include "PrefsManager.h"
#include "CryptManager.h"
#include "UnlockSystem.h"
#include "RageUtil.h"
#include "SongUtil.h"
#include "StepsUtil.h"
#include "CourseUtil.h"
#include "GameState.h"


const CString STATS_HTML	= "Stats.html";
const CString STYLE_CSS		= "Style.css";

#define TITLE					THEME->GetMetric("ProfileHtml","Title")
#define FOOTER					THEME->GetMetric("ProfileHtml","Footer")
#define VERIFICATION_TEXT		THEME->GetMetric("ProfileHtml","VerificationText")
#define SHOW_PLAY_MODE(pm)		THEME->GetMetricB("ProfileHtml","ShowPlayMode"+PlayModeToString(pm))
#define SHOW_RADAR_CATEGORY(rc)	THEME->GetMetricB("ProfileHtml","ShowRadarCategory"+RadarCategoryToString(rc))
#define SHOW_STYLE(s)			THEME->GetMetricB("ProfileHtml","ShowStyle"+Capitalize(GAMEMAN->GetStyleDefForStyle(s)->m_szName))
#define SHOW_DIFFICULTY(dc)		THEME->GetMetricB("ProfileHtml","ShowDifficulty"+DifficultyToString(dc))
#define SHOW_STEPS_TYPE(st)		THEME->GetMetricB("ProfileHtml","ShowStepsType"+Capitalize(GAMEMAN->NotesTypeToString(st)))
#define SHOW_HIGH_SCORE_SCORE	THEME->GetMetricB("ProfileHtml","ShowHighScoreScore")
#define SHOW_HIGH_SCORE_GRADE	THEME->GetMetricB("ProfileHtml","ShowHighScoreGrade")
#define SHOW_HIGH_SCORE_PERCENT	THEME->GetMetricB("ProfileHtml","ShowHighScorePercent")

#define NEWLINE "\r\n"

CString HighScoreToString( const HighScore& hs )
{
	CStringArray asTokens;
	asTokens.push_back( hs.sName.empty() ? "????" : hs.sName );
	if( SHOW_HIGH_SCORE_GRADE )
		asTokens.push_back( GradeToThemedString(hs.grade) );
	if( SHOW_HIGH_SCORE_SCORE )
		asTokens.push_back( ssprintf("%i", hs.iScore) );
	if( SHOW_HIGH_SCORE_PERCENT )
		asTokens.push_back( ssprintf("%05.2f%%", hs.fPercentDP*100) );
	
	return join(", ",asTokens);
}

void TranslatedWrite( RageFile &f, CString s )
{
	s.Replace("\n",NEWLINE);
	f.Write( s );
}

static int g_Level = 1;

inline CString MakeUniqueId()												{ CString s="id"+ssprintf("%d%d%d",rand(),rand(),rand()); return s; }
inline void PRINT_OPEN(RageFile &f,CString sName,bool bExpanded,CString sID){ g_Level++; ASSERT(g_Level>0 && g_Level<6); TranslatedWrite(f,ssprintf("<div class='section%d'>\n" "<h%d onClick='expandIt(%s); return false' CLASS='outline'>%s</h%d>\n" "<DIV ID='%s' CLASS='%s'>\n", g_Level, g_Level, sID.c_str(), sName.c_str(), g_Level, sID.c_str(), bExpanded?"visibletext":"hiddentext") ); }
inline void PRINT_OPEN(RageFile &f,CString sName,bool bExpanded=false)		{ PRINT_OPEN(f,sName,bExpanded,MakeUniqueId()); }
inline void PRINT_CLOSE(RageFile &f)										{ TranslatedWrite(f, "</div>\n" "</div>\n" ); g_Level--; ASSERT(g_Level>=0); }


struct Table
{
	Table() {}

	struct Line
	{
		Line() {}
		Line(CString n)							{ sName = n; }
		Line(CString n,CString v)				{ sName = n; sValue = v; }
		Line(CString n,bool v)					{ sName = n; sValue = ssprintf("%s",v?"yes":"no"); }
		Line(CString n,int v)					{ sName = n; sValue = Commify(v); }
		Line(int r,CString n,int v)				{ sRank = ssprintf("%d",r); sName = n; sValue = ssprintf("%d",v); }
		Line(int r,CString n,CString sn,int v)	{ sRank = ssprintf("%d",r); sName = n; sSubName = sn; sValue = ssprintf("%d",v); }
		Line(int r,CString n,CString sn,CString ssn,int v)	{ sRank = ssprintf("%d",r); sName = n; sSubName = sn; sSubSubName = ssn; sValue = ssprintf("%d",v); }
		Line(int r,CString n,CString v)				{ sRank = ssprintf("%d",r); sName = n; sValue = v; }
		Line(int r,CString n,CString sn,CString v)	{ sRank = ssprintf("%d",r); sName = n; sSubName = sn; sValue = v; }
		Line(int r,CString n,CString sn,CString ssn,CString v)	{ sRank = ssprintf("%d",r); sName = n; sSubName = sn; sSubSubName = ssn; sValue = v; }

		CString sRank;
		CString sName;
		CString sSubName;
		CString sSubSubName;
		CString sValue;
	};

	int iNumCols;
	vector<Line> vLines;
};

#define BEGIN_TABLE(cols)			{ Table table; table.iNumCols=cols;
#define TABLE_LINE1(p1)				table.vLines.push_back( Table::Line(p1) );
#define TABLE_LINE2(p1,p2)			table.vLines.push_back( Table::Line(p1,p2) );
#define TABLE_LINE3(p1,p2,p3)		table.vLines.push_back( Table::Line(p1,p2,p3) );
#define TABLE_LINE4(p1,p2,p3,p4)	table.vLines.push_back( Table::Line(p1,p2,p3,p4) );
#define TABLE_LINE5(p1,p2,p3,p4,p5)	table.vLines.push_back( Table::Line(p1,p2,p3,p4,p5) );
#define END_TABLE					PrintTable( f, table ); }

inline void PrintTable(RageFile &f,Table &table)
{
	const vector<Table::Line> &vLines = table.vLines;
	int &iNumCols = table.iNumCols;

	ASSERT( iNumCols > 0 );

	if( vLines.empty() )
		return;

	bool bPrintRank = !vLines.empty() && !vLines[0].sRank.empty();
	bool bPrintInstructions = !vLines.empty() && vLines[0].sRank.empty() && vLines[0].sSubName.empty() && vLines[0].sSubSubName.empty() && vLines[0].sValue.empty();

	int iMaxItemsPerCol = (vLines.size()+iNumCols-1) / iNumCols;
	iNumCols = (vLines.size()+iMaxItemsPerCol-1) / iMaxItemsPerCol;	// round up
	TranslatedWrite(f,ssprintf("<table class='%s'><tr>\n",bPrintInstructions?"instructions":"group"));
	if( iNumCols == 0 )
	{
		TranslatedWrite(f,"<td>empty</td>\n");
	}
	for( int col=0; col<iNumCols; col++ )
	{
		TranslatedWrite(f,"<td>\n");

		int iStartItem = col*iMaxItemsPerCol;

		TranslatedWrite(f,"<table class='column'>\n");
		for( int i=iStartItem; i<iStartItem+iMaxItemsPerCol; i++ )
		{
			TranslatedWrite(f,"<tr>");

			const Table::Line& line = (i<(int)vLines.size()) ? vLines[i] : Table::Line();
			if( bPrintRank )
			{
				TranslatedWrite(f,"<td class='rank'>");
				TranslatedWrite(f, line.sRank );
				TranslatedWrite(f,"</td>");
				TranslatedWrite(f,"<td>&nbsp;</td>");
			}
			if( bPrintRank )
			{
				TranslatedWrite(f,"<td>");
				TranslatedWrite(f,"<p class='songtitle'>");
				TranslatedWrite(f, line.sName );
				TranslatedWrite(f,"</p>");
				if( !line.sSubName.empty() )
				{
					TranslatedWrite(f,"<p class='songsubtitle'>");
					TranslatedWrite(f, line.sSubName );
					TranslatedWrite(f,"</p>");
				}
				TranslatedWrite(f,"</td>");
			}
			else if( line.sValue.empty() )
			{
				TranslatedWrite(f,"<td>");
				TranslatedWrite(f, line.sName );
				TranslatedWrite(f,"</td>");
			}
			else
			{
				TranslatedWrite(f,"<td class='name'>");
				TranslatedWrite(f, line.sName );
				TranslatedWrite(f,"</td>");
			}

			if( !line.sSubSubName.empty() )
			{
				TranslatedWrite(f,"<td>&nbsp;</td>");
				TranslatedWrite(f,"<td>");
				TranslatedWrite(f,"<p class='stepsdescription'>");
				TranslatedWrite(f, line.sSubSubName );
				TranslatedWrite(f,"</p>");
				TranslatedWrite(f,"</td>");
			}

			if( !line.sValue.empty() )
			{
				TranslatedWrite(f,"<td>&nbsp;</td>");
				TranslatedWrite(f,"<td class='value'>");
				TranslatedWrite(f, line.sValue );
				TranslatedWrite(f,"</td>");
			}
			else
			{
				TranslatedWrite(f,"<td>&nbsp;</td>");
				TranslatedWrite(f,"<td>&nbsp;</td>");
			}

			TranslatedWrite(f, "\n" );

			TranslatedWrite(f,"</tr>\n");
		}
		TranslatedWrite(f,"</table>\n");

		TranslatedWrite(f,"</td>\n");
	}
	TranslatedWrite(f,"</tr>\n</table>\n");
} 

void PrintEmptyTable( RageFile &f )
{
	BEGIN_TABLE(1);
	TABLE_LINE1("empty");
	END_TABLE;
}

#define STRING_AS_LINK(s) CString("<a href='"+s+"'>"+s+"</a>")

void PrintInstructions( RageFile &f, const Profile *pProfile, CString sTitle )
{
	PRINT_OPEN(f,sTitle);
	{
		PRINT_OPEN(f,"Overview",true);
		{
			BEGIN_TABLE(1);
			TABLE_LINE1("<p>This directory contains all your game profile data.  Please read these instructions before modifying or moving any files on your memory card.  Modifying files may result in irreversible loss of your data.</p>\n");
			END_TABLE;
		}
		PRINT_CLOSE(f);

		PRINT_OPEN(f,"Description of Files");
		{
			BEGIN_TABLE(1);
			TABLE_LINE2(STRING_AS_LINK(EDITS_SUBDIR),		CString("Place edit step files in this directory.  See the Edits section below for more details."));
			TABLE_LINE2(STRING_AS_LINK(SCREENSHOTS_SUBDIR),	CString("All screenshots that you take are saved in this directory."));
			TABLE_LINE2(DONT_SHARE_SIG,						CString("This is a secret file that you should never share with anyone else.  See the Sharing Your Data section below for more details."));
			TABLE_LINE2(STRING_AS_LINK(EDITABLE_INI),		CString("Holds preferences that you can edit offline using your home computer.  This file is not digitally signed."));
			TABLE_LINE2(STATS_HTML,							CString("You're looking at this file now.  It contains a formatted view of all your saved data, plus some data from the last machine you played on."));
			TABLE_LINE2(STATS_HTML+SIGNATURE_APPEND,		CString("The signature file for "+STATS_HTML+"."));
			TABLE_LINE2(STRING_AS_LINK(STATS_XML),			CString("This is the primary data file.  It contains all the score data and statistics, and is read by the game when you join."));
			TABLE_LINE2(STATS_XML+SIGNATURE_APPEND,			CString("The signature file for "+STATS_XML+"."));
			TABLE_LINE2(STYLE_CSS,							CString("Contains style data used by "+STATS_HTML+"."));
			END_TABLE;
		}
		PRINT_CLOSE(f);

		PRINT_OPEN(f,"Digital Signatures");
		{
			BEGIN_TABLE(1);
			TABLE_LINE1("<p>Some files on your memory card have a corresponding digital signature.  Digital signatures are used to verify that your files haven't been modified outside of the game.  This prevents cheaters from changing their score data and then passing it off as real.</p>\n");
			TABLE_LINE1("<p>Before the game reads your memory card data, it verifies that your data and digital signatures match.  If the data and signatures don't match, then your data has been modified outside of the game.  When the game detects this condition, it will completely ignore your tampered data.  It is very important that you -do not- modify any file that has a digital signature because this will cause your data to be permanently unusable.</p>\n");
			TABLE_LINE1("<p>If someone else shares their profile data with you, you can verify their score data using digital signatures.  To verify their data, you'll need 3 things:</p>\n");
			TABLE_LINE1("<p>- the "+STATS_XML+" data file</p>\n");
			TABLE_LINE1("<p>- the digital signature file "+STATS_XML+SIGNATURE_APPEND+"</p>\n");
			TABLE_LINE1("<p>- a <a href='http://www.stepmania.com/stepmania/digitalsignatures/'>small utility</a> that will check data against a signature</p>\n");
			END_TABLE;
		}
		PRINT_CLOSE(f);

		PRINT_OPEN(f,"About Editable Preferences");
		{
			BEGIN_TABLE(1);
			TABLE_LINE1("<p>The file "+STRING_AS_LINK(EDITABLE_INI)+" contains settings that you can modify using your home computer.  If you're using a Windows PC, you can click <a href='"+EDITABLE_INI+"'>here</a> to open the file for editing.  This file is not digitally signed and the game will import any changes you make to this file.</p>\n");
			END_TABLE;
		}
		PRINT_CLOSE(f);

		PRINT_OPEN(f,"About Screenshots");
		{
			BEGIN_TABLE(1);
			TABLE_LINE1("<p>The "+STRING_AS_LINK(SCREENSHOTS_SUBDIR)+" directory contains all screenshots that you've captured while playing the game.  See the Screenshots section later on this page to see thumbnails and more information captured at the time of the screenshot.  The Screenshots section also lists an MD5 hash of the screenshot file.  You can use the MD5 has to verify that the screenshot has not been modified since it was first saved.</p>\n");
			TABLE_LINE1("<p>If your memory card is full, you may delete files from this directory or the move files to another disk.  If you move a screenshot to another disk, you can still verify the screenshot file using the MD5 hash.</p>\n");
			END_TABLE;
		}
		PRINT_CLOSE(f);

		PRINT_OPEN(f,"About Edits");
		{
			BEGIN_TABLE(1);
			TABLE_LINE1("<p>The "+STRING_AS_LINK(EDITS_SUBDIR)+" directory contains edit step files that you've created yourself or downloaded from the internet.  See <a href='http://www.stepmania.com/stepmania/edits'>here</a> for more information about edit files.</p>\n");
			END_TABLE;
		}
		PRINT_CLOSE(f);

		PRINT_OPEN(f,"Sharing Your Data");
		{
			BEGIN_TABLE(1);
			TABLE_LINE1("<p>You can share your score data with other players or submit it to a web site for an internet ranking contest.  When sharing your data though, do -not- share the file "+DONT_SHARE_SIG+".  "+DONT_SHARE_SIG+" is private digital signature required by the game before loading memory card data.  Without "+DONT_SHARE_SIG+", the person you're sharing data with can verify that your data is original, but can't load your data using their memory card or pass your scores off as their own.</p>\n");
			TABLE_LINE1("<p>If you do share your "+DONT_SHARE_SIG+" with someone, then they can completely replicate your memory card and pass your scores off as their own.</p>\n");
			END_TABLE;
		}
		PRINT_CLOSE(f);

		PRINT_OPEN(f,"Backing Up/Moving Your Data");
		{
			BEGIN_TABLE(1);
			TABLE_LINE1("<p>To make a backup of your data, copy the entire "+PREFSMAN->m_sMemoryCardProfileSubdir+"/ directory on the root of your memory card to your local hard drive.</p>\n");
			TABLE_LINE1("<p>To move your data from the current memory card to a new memory card, move the entire "+PREFSMAN->m_sMemoryCardProfileSubdir+"/ directory on your current memory card to the root directory on the new memory card.</p>\n");
			END_TABLE;
		}
		PRINT_CLOSE(f);
	}
	PRINT_CLOSE(f);
}

void PrintStatistics( RageFile &f, const Profile *pProfile, CString sTitle, vector<Song*> &vpSongs, vector<Steps*> &vpAllSteps, vector<StepsType> &vStepsTypesToShow, map<Steps*,Song*> mapStepsToSong, vector<Course*> vpCourses )
{
	PRINT_OPEN(f,sTitle,true);
	{
		PRINT_OPEN(f,"General Info",true);
		{
			BEGIN_TABLE(2);
			TABLE_LINE2( "DisplayName",						pProfile->m_sDisplayName.empty() ? "(empty)" : pProfile->m_sDisplayName );
			TABLE_LINE2( "ID",								pProfile->m_sGuid );
			TABLE_LINE2( "LastUsedHighScoreName",			pProfile->m_sLastUsedHighScoreName );
			TABLE_LINE2( "LastPlayedMachineID",				pProfile->m_sLastPlayedMachineGuid );
			TABLE_LINE2( "UsingProfileDefaultModifiers",	pProfile->m_bUsingProfileDefaultModifiers );
			PlayerOptions po;
			po.FromString( pProfile->m_sDefaultModifiers );
			TABLE_LINE2( "DefaultModifiers",				po.GetThemedString() );
			TABLE_LINE2( "TotalPlays",						pProfile->m_iTotalPlays );
			TABLE_LINE2( "TotalPlay",						SecondsToHHMMSS( (float) pProfile->m_iTotalPlaySeconds) );
			TABLE_LINE2( "TotalGameplay",					SecondsToHHMMSS( (float) pProfile->m_iTotalGameplaySeconds) );
			TABLE_LINE2( "CurrentCombo",					pProfile->m_iCurrentCombo );
			TABLE_LINE2( "TotalCaloriesBurned",				pProfile->GetDisplayTotalCaloriesBurned() );
			TABLE_LINE2( "TotalTapsAndHolds",				pProfile->m_iTotalTapsAndHolds );
			TABLE_LINE2( "TotalJumps",						pProfile->m_iTotalJumps );
			TABLE_LINE2( "TotalHolds",						pProfile->m_iTotalHolds );
			TABLE_LINE2( "TotalMines",						pProfile->m_iTotalMines );
			TABLE_LINE2( "TotalHands",						pProfile->m_iTotalHands );
			END_TABLE;
		}
		PRINT_CLOSE(f);
		

		PRINT_OPEN(f,"Num Songs Played by PlayMode");
		{
			BEGIN_TABLE(4);
			FOREACH_PlayMode( pm )
			{
				if( !SHOW_PLAY_MODE(pm) )
					continue;	// skip
				TABLE_LINE2( PlayModeToThemedString(pm), pProfile->m_iNumSongsPlayedByPlayMode[pm] );
			}
			END_TABLE;
		}
		PRINT_CLOSE(f);


		PRINT_OPEN(f,"Num Songs Played by Style");
		{
			BEGIN_TABLE(4);
			for( Style style = (Style)0; style<NUM_STYLES; ((int&)style)++ )
			{
				const StyleDef* pStyleDef = GAMEMAN->GetStyleDefForStyle(style);
				StepsType st = pStyleDef->m_StepsType;
				if( !pStyleDef->m_bUsedForGameplay )
					continue;	// skip
				// only show if this style plays a StepsType that we're showing
				if( find(vStepsTypesToShow.begin(),vStepsTypesToShow.end(),st) == vStepsTypesToShow.end() )
					continue;	// skip
				if( !SHOW_STYLE(style) )
					continue;
				CString s = GAMEMAN->StyleToThemedString(style);
				TABLE_LINE2( s, pProfile->m_iNumSongsPlayedByStyle[style] );
			}
			END_TABLE;
		}
		PRINT_CLOSE(f);


		PRINT_OPEN(f,"Num Songs Played by Difficulty");
		{
			BEGIN_TABLE(4);
			FOREACH_Difficulty( dc )
			{
				if( !SHOW_DIFFICULTY(dc) )
					continue;
				TABLE_LINE2( DifficultyToThemedString(dc), pProfile->m_iNumSongsPlayedByDifficulty[dc] );
			}
			END_TABLE;
		}
		PRINT_CLOSE(f);

		PRINT_OPEN(f,"Num Songs Played by Meter");
		{
			BEGIN_TABLE(4);
			for( int i=MIN_METER; i<=MAX_METER; i++ )
				TABLE_LINE2( ssprintf("Meter %d",i), pProfile->m_iNumSongsPlayedByMeter[i] );
			END_TABLE;
		}
		PRINT_CLOSE(f);

		PRINT_OPEN(f,"Grade Count");
		{
			int iGradeCount[NUM_GRADES];
			ZERO( iGradeCount );

			for( unsigned i=0; i<vpSongs.size(); i++ )
			{
				Song* pSong = vpSongs[i];
				const vector<Steps*> vpSteps = pSong->GetAllSteps();

				for( unsigned j=0; j<vpSteps.size(); j++ )
				{
					const Steps* pSteps = vpSteps[j];

					const HighScoreList &hsl = pProfile->GetStepsHighScoreList(pSong,pSteps);
					if( hsl.vHighScores.empty() )
						continue;	// no data, skip this one
					Grade g = hsl.GetTopScore().grade;
					ASSERT( g != GRADE_NO_DATA );
					ASSERT( g < NUM_GRADES );
					ASSERT( g >= 0 );
					iGradeCount[g] ++;
				}
			}

			BEGIN_TABLE(6);
			for( int g=0; g<PREFSMAN->m_iNumGradeTiersUsed; g++ )
				TABLE_LINE2( GradeToThemedString((Grade)g), iGradeCount[g] );
			TABLE_LINE2( GradeToThemedString(GRADE_FAILED), iGradeCount[GRADE_FAILED] );
			END_TABLE;
		}
		PRINT_CLOSE(f);

	}
	PRINT_CLOSE(f);
}

void PrintPopularity( RageFile &f, const Profile *pProfile, CString sTitle, vector<Song*> &vpSongs, vector<Steps*> &vpAllSteps, vector<StepsType> &vStepsTypesToShow, map<Steps*,Song*> mapStepsToSong, vector<Course*> vpCourses )
{
	PRINT_OPEN(f, sTitle );
	if( vpSongs.size() )
	{
		SongUtil::SortSongPointerArrayByNumPlays( vpSongs, pProfile, true );
		Song* pSongPopularThreshold = vpSongs[ vpSongs.size()*2/3 ];
		int iPopularNumPlaysThreshold = pProfile->GetSongNumTimesPlayed(pSongPopularThreshold);
		
		// unplayed songs are always considered unpopular
		if( iPopularNumPlaysThreshold == 0 )
			iPopularNumPlaysThreshold = 1;

		unsigned uMaxToShow = min( vpSongs.size(), (unsigned)100 );

		// compute total plays
		int iTotalPlays = 0;
		for( unsigned i=0; i<vpSongs.size(); i++ )
			iTotalPlays += pProfile->GetSongNumTimesPlayed( vpSongs[i] );


		{
			PRINT_OPEN(f, "Most Popular Songs" );
			{
				BEGIN_TABLE(1);
				unsigned i;
				for( i=0; i<uMaxToShow; i++ )
				{
					Song* pSong = vpSongs[i];
					int iNumTimesPlayed = pProfile->GetSongNumTimesPlayed(pSong);
					if( iNumTimesPlayed == 0 || iNumTimesPlayed < iPopularNumPlaysThreshold )	// not popular
						break;	// done searching
					TABLE_LINE4(i+1, pSong->GetDisplayMainTitle(), pSong->GetDisplaySubTitle(), PrettyPercent(iNumTimesPlayed,iTotalPlays) );
				}
				if( i == 0 )
					TABLE_LINE1("empty");
				END_TABLE;
			}
			PRINT_CLOSE(f);
		}

		{
			SongUtil::SortSongPointerArrayByNumPlays( vpSongs, pProfile, false );
			PRINT_OPEN(f, "Least Popular Songs" );
			{
				BEGIN_TABLE(1);
				unsigned i;
				for( i=0; i<uMaxToShow; i++ )
				{
					Song* pSong = vpSongs[i];
					int iNumTimesPlayed = pProfile->GetSongNumTimesPlayed(pSong);
					if( iNumTimesPlayed >= iPopularNumPlaysThreshold )	// not unpopular
						break;	// done searching
					TABLE_LINE4(i+1, pSong->GetDisplayMainTitle(), pSong->GetDisplaySubTitle(), PrettyPercent(iNumTimesPlayed,iTotalPlays) );
				}
				if( i == 0 )
					TABLE_LINE1("empty");
				END_TABLE;
			}
			PRINT_CLOSE(f);
		}

		{
			unsigned uNumToShow = min( vpAllSteps.size(), (unsigned)100 );

			StepsUtil::SortStepsPointerArrayByNumPlays( vpAllSteps, pProfile, true );
			PRINT_OPEN(f, "Most Popular Steps" );
			{
				BEGIN_TABLE(1);
				unsigned i;
				for( i=0; i<uNumToShow; i++ )
				{
					Steps* pSteps = vpAllSteps[i];
					Song* pSong = mapStepsToSong[pSteps];
					int iNumTimesPlayed = pProfile->GetStepsNumTimesPlayed(pSong,pSteps);
					if( iNumTimesPlayed==0 )
						continue;	// skip
					CString s;
					s += GAMEMAN->NotesTypeToThemedString(pSteps->m_StepsType);
					s += " ";
					s += DifficultyToThemedString(pSteps->GetDifficulty());
					TABLE_LINE5(i+1, pSong->GetDisplayMainTitle(), pSong->GetDisplaySubTitle(), s, PrettyPercent(iNumTimesPlayed,iTotalPlays) );
				}
				if( i == 0 )
					TABLE_LINE1("empty");
				END_TABLE;
			}
			PRINT_CLOSE(f);
		}

		{
			unsigned uNumToShow = min( vpCourses.size(), (unsigned)100 );

			CourseUtil::SortCoursePointerArrayByNumPlays( vpCourses, pProfile, true );
			PRINT_OPEN(f, "Most Popular Courses" );
			{
				BEGIN_TABLE(2);
				unsigned i;
				for( i=0; i<uNumToShow; i++ )
				{
					Course* pCourse = vpCourses[i];
					int iNumTimesPlayed = pProfile->GetCourseNumTimesPlayed(pCourse);
					TABLE_LINE3(i+1, pCourse->m_sName, PrettyPercent(iNumTimesPlayed,iTotalPlays) );
				}
				if( i == 0 )
					TABLE_LINE1("empty");
				END_TABLE;
			}
			PRINT_CLOSE(f);
		}
	}
	PRINT_CLOSE(f);
}


// return true if anything was printed
typedef bool (*FnPrintSong)(RageFile &f, const Profile *pProfile, Song* pSong );
typedef bool (*FnPrintGroup)(RageFile &f, const Profile *pProfile, CString sGroup );
typedef bool (*FnPrintStepsType)(RageFile &f, const Profile *pProfile, StepsType st );
typedef bool (*FnPrintCourse)(RageFile &f, const Profile *pProfile, Course* pCourse );


bool PrintSongsInGroup( RageFile &f, const Profile *pProfile, CString sGroup, FnPrintSong pFn )
{
	vector<Song*> vpSongs;
	SONGMAN->GetSongs( vpSongs, sGroup );

	if( vpSongs.empty() )
		return false;

	PRINT_OPEN(f, sGroup );
	{
		bool bPrintedAny = false;
		for( unsigned i=0; i<vpSongs.size(); i++ )
		{
			Song* pSong = vpSongs[i];
			bPrintedAny |= pFn( f, pProfile, pSong );
		}

		if( !bPrintedAny )
			PrintEmptyTable(f);
	}
	PRINT_CLOSE(f);

	return true;
}

bool PrintGroups( RageFile &f, const Profile *pProfile, CString sTitle, FnPrintGroup pFn )
{
	CStringArray asGroups;
	SONGMAN->GetGroupNames( asGroups );
	
	if( asGroups.empty() )
		return false;

	PRINT_OPEN(f, sTitle );
	{
		bool bPrintedAny = false;

		for( unsigned g=0; g<asGroups.size(); g++ )
		{
			CString sGroup = asGroups[g];
			bPrintedAny |= pFn( f, pProfile, sGroup );
		}

		if( !bPrintedAny )
			PrintEmptyTable(f);
	}
	PRINT_CLOSE(f);

	return true;
}

bool PrintCourses( RageFile &f, const Profile *pProfile, CString sTitle, FnPrintCourse pFn )
{
	vector<Course*> vpCourses;
	SONGMAN->GetAllCourses( vpCourses, true );
	
	if( vpCourses.empty() )
		return false;

	PRINT_OPEN(f, sTitle );
	{
		bool bPrintedAny = false;

		for( unsigned c=0; c<vpCourses.size(); c++ )
		{
			Course* pCourse = vpCourses[c];
			bPrintedAny |= pFn( f, pProfile, pCourse );
		}

		if( !bPrintedAny )
			PrintEmptyTable(f);
	}
	PRINT_CLOSE(f);

	return true;
}

bool PrintStepsTypes( RageFile &f, const Profile *pProfile, CString sTitle, vector<StepsType> vStepsTypesToShow, FnPrintStepsType pFn )
{
	PRINT_OPEN(f, sTitle );
	{
		for( unsigned s=0; s<vStepsTypesToShow.size(); s++ )
		{
			StepsType st = vStepsTypesToShow[s];

			pFn( f, pProfile, st );
		}
	}
	PRINT_CLOSE(f);

	return true;
}

void PrintHighScoreListTable( RageFile &f, const HighScoreList& hsl )
{
	BEGIN_TABLE(2);
	for( unsigned i=0; i<hsl.vHighScores.size(); i++ )
	{
		const HighScore &hs = hsl.vHighScores[i];
		CString sName = ssprintf("#%d",i+1);

		CStringArray asTokens;
		asTokens.push_back( hs.sName.empty() ? "????" : hs.sName );
		
		TABLE_LINE2( sName, HighScoreToString(hs) );
	}
	END_TABLE;
}

bool PrintHighScoresForSong( RageFile &f, const Profile *pProfile, Song* pSong )
{
	if( pSong->NeverDisplayed() || UNLOCKMAN->SongIsLocked(pSong) )
		return false;	// skip
	int iNumTimesPlayed = pProfile->GetSongNumTimesPlayed(pSong);
	if( iNumTimesPlayed == 0 )
		return false;	// skip

	vector<Steps*> vpSteps = pSong->GetAllSteps();

	bool bPrintedOpen = false;

	{
		//
		// Print Steps list
		//
		for( unsigned j=0; j<vpSteps.size(); j++ )
		{
			Steps* pSteps = vpSteps[j];
			if( pSteps->IsAutogen() )
				continue;	// skip autogen
			if( pProfile->GetStepsNumTimesPlayed(pSong,pSteps)==0 )
				continue;	// skip
			const HighScoreList &hsl = pProfile->GetStepsHighScoreList( pSong,pSteps );
			if( hsl.vHighScores.empty() )
				continue;

			if( !bPrintedOpen )
			{
				PRINT_OPEN(f, pSong->GetFullDisplayTitle() );
				bPrintedOpen = true;
			}

			CString s = 
				GAMEMAN->NotesTypeToThemedString(pSteps->m_StepsType) + 
				" - " +
				DifficultyToThemedString(pSteps->GetDifficulty());
			PRINT_OPEN(f, s, true);
			{
				PrintHighScoreListTable( f, hsl );
			}
			PRINT_CLOSE(f);
		}
	}
	if( bPrintedOpen )
		PRINT_CLOSE(f);

	return bPrintedOpen;
}

bool PrintHighScoresForCourse( RageFile &f, const Profile *pProfile, Course* pCourse )
{
	bool bPrintedAny = false;

	for( StepsType st=(StepsType)0; st<NUM_STEPS_TYPES; st=(StepsType)(st+1) )
	{
		FOREACH_CourseDifficulty( cd )
		{
			const HighScoreList &hsl = pProfile->GetCourseHighScoreList( pCourse, st, cd );
			if( hsl.vHighScores.empty() )
				continue;

			bPrintedAny = true;

			PRINT_OPEN(f, GAMEMAN->NotesTypeToThemedString(st)+" - "+CourseDifficultyToThemedString(cd) );
			{
				PrintHighScoreListTable( f, hsl );
			}
			PRINT_CLOSE(f);
		}
	}

	return bPrintedAny;
}

bool PrintHighScoresForGroup(RageFile &f, const Profile *pProfile, CString sGroup )
{
	return PrintSongsInGroup( f, pProfile, sGroup, PrintHighScoresForSong );
}

void PrintSongHighScores( RageFile &f, const Profile *pProfile, CString sTitle, vector<Song*> &vpSongs, vector<Steps*> &vpAllSteps, vector<StepsType> &vStepsTypesToShow, map<Steps*,Song*> mapStepsToSong, vector<Course*> vpCourses )
{
	PrintGroups( f, pProfile, sTitle, PrintHighScoresForGroup );
}

void PrintCourseHighScores( RageFile &f, const Profile *pProfile, CString sTitle, vector<Song*> &vpSongs, vector<Steps*> &vpAllSteps, vector<StepsType> &vStepsTypesToShow, map<Steps*,Song*> mapStepsToSong, vector<Course*> vpCourses )
{
	PrintCourses( f, pProfile, sTitle, PrintHighScoresForCourse );
}

bool PrintPercentCompleteForStepsType( RageFile &f, const Profile *pProfile, StepsType st )
{
	unsigned i;
	const vector<Song*> &vpSongs = SONGMAN->GetAllSongs();
	vector<Course*> vpCourses;
	SONGMAN->GetAllCourses( vpCourses, true );

 	PRINT_OPEN(f, GAMEMAN->NotesTypeToThemedString(st) );
	{
		BEGIN_TABLE(1);
		TABLE_LINE2( "Percent Complete", ssprintf("%06.3f%%",pProfile->GetPercentCompleteForStepsType(st)*100) );
		TABLE_LINE2( "Actual Song Points", pProfile->GetActualSongDancePointsForStepsType(st) );
		TABLE_LINE2( "Actual Course Points", pProfile->GetActualCourseDancePointsForStepsType(st) );
		TABLE_LINE2( "Possible Song Points", pProfile->GetPossibleSongDancePointsForStepsType(st) );
		TABLE_LINE2( "Possible Course Points", pProfile->GetPossibleCourseDancePointsForStepsType(st) );
		END_TABLE;
	
		PRINT_OPEN(f, "Songs" );
		{
			TranslatedWrite(f, "<table class='difficulty'>\n" );

			// table header row
			TranslatedWrite(f, "<tr><td>&nbsp;</td>" );
			FOREACH_Difficulty( dc )
			{
				if( dc == DIFFICULTY_EDIT )
					continue;	// skip
				if( !SHOW_DIFFICULTY(dc) )
					continue;
				TranslatedWrite(f, ssprintf("<td>%s</td>", DifficultyToThemedString(dc).c_str()) );
			}
			TranslatedWrite(f, "</tr>\n" );

			// table body rows
			for( i=0; i<vpSongs.size(); i++ )
			{
				Song* pSong = vpSongs[i];

				if( pSong->m_SelectionDisplay == Song::SHOW_NEVER )
					continue;	// skip
				if( UNLOCKMAN->SongIsLocked(pSong) )
					continue;

				TranslatedWrite(f, "<tr>" );
				
				TranslatedWrite(f, "<td>" );
				TranslatedWrite(f, ssprintf("<p class='songtitle'>%s</p>", pSong->GetDisplayMainTitle().c_str()) );
				TranslatedWrite(f, ssprintf("<p class='songsubtitle'>%s</p>", pSong->GetDisplaySubTitle().c_str()) );
				TranslatedWrite(f, "</td>" );

				FOREACH_Difficulty( dc )
				{
					if( dc == DIFFICULTY_EDIT )
						continue;	// skip
					if( !SHOW_DIFFICULTY(dc) )
						continue;
					Steps* pSteps = pSong->GetStepsByDifficulty( st, dc, false );
					if( pSteps && !pSteps->IsAutogen() )
					{
						TranslatedWrite(f,"<td>");
						TranslatedWrite(f, ssprintf("(%d)",pSteps->GetMeter()) );
						HighScore hs = pProfile->GetStepsHighScoreList( pSong,pSteps ).GetTopScore();
						Grade grade = hs.grade;
						if( grade != GRADE_NO_DATA )
						{
							TranslatedWrite(f, ssprintf("&nbsp;%s %05.2f%%",GradeToThemedString(grade).c_str(), hs.fPercentDP*100) );
						}
						TranslatedWrite(f,"</td>");
					}
					else
					{
						TranslatedWrite(f, "<td>&nbsp;</td>" );
					}
				}

				TranslatedWrite(f, "</tr>\n" );
			}

			TranslatedWrite(f, "</table>\n" );
		}
		PRINT_CLOSE(f);
	
		PRINT_OPEN(f, "Courses" );
		{
			TranslatedWrite(f, "<table class='difficulty'>\n" );

			// table header row
			TranslatedWrite(f, "<tr><td>&nbsp;</td>" );
			FOREACH_CourseDifficulty( cd )
			{
				TranslatedWrite(f, ssprintf("<td>%s</td>", CourseDifficultyToThemedString(cd).c_str()) );
			}
			TranslatedWrite(f, "</tr>\n" );

			// table body rows
			for( i=0; i<vpCourses.size(); i++ )
			{
				Course* pCourse = vpCourses[i];
				
				if( !pCourse->AllSongsAreFixed() )
					continue;

				TranslatedWrite(f, "<tr>" );
				
				TranslatedWrite(f, "<td>" );
				TranslatedWrite(f, ssprintf("<p class='songtitle'>%s</p>", pCourse->m_sName.c_str()) );
				TranslatedWrite(f, "</td>" );

				FOREACH_CourseDifficulty( cd )
				{
					if( pCourse->HasCourseDifficulty(st,cd) )
					{
						TranslatedWrite(f,"<td>");
						/* HACK: Course::GetMeter() requires that a style be set, since
						 * a course can have different meter values depending on which
						 * style is set. */
						if( GAMESTATE->m_CurStyle == STYLE_INVALID )
							GAMESTATE->m_CurStyle = STYLE_DANCE_SINGLE;
						TranslatedWrite(f, ssprintf("(%.2f)",pCourse->GetMeter(cd)) );
						HighScore hs = pProfile->GetCourseHighScoreList(pCourse, st, cd).GetTopScore();
						Grade grade = hs.grade;
						if( grade != GRADE_NO_DATA )
						{
							TranslatedWrite(f, ssprintf("&nbsp;%s %05.2f%%",GradeToThemedString(grade).c_str(), hs.fPercentDP*100) );
						}
						TranslatedWrite(f,"</td>");
					}
					else
					{
						TranslatedWrite(f, "<td>&nbsp;</td>" );
					}
				}

				TranslatedWrite(f, "</tr>\n" );
			}

			TranslatedWrite(f, "</table>\n" );
		}
		PRINT_CLOSE(f);
	}
	PRINT_CLOSE(f);

	return true;
}

void PrintPercentComplete( RageFile &f, const Profile *pProfile, CString sTitle, vector<Song*> &vpSongs, vector<Steps*> &vpAllSteps, vector<StepsType> &vStepsTypesToShow, map<Steps*,Song*> mapStepsToSong, vector<Course*> vpCourses )
{
	PrintStepsTypes( f, pProfile, sTitle, vStepsTypesToShow, PrintPercentCompleteForStepsType );
}

bool PrintRecentScores( RageFile &f, const Profile *pProfile, CString sTitle, vector<Song*> &vpSongs, vector<Steps*> &vpAllSteps, vector<StepsType> &vStepsTypesToShow, map<Steps*,Song*> mapStepsToSong, vector<Course*> vpCourses )
{
	PRINT_OPEN(f, sTitle );
	{
		PRINT_OPEN(f, "Songs" );
		{
			for( unsigned i=0; i<pProfile->m_vRecentStepsScores.size(); i++ )
			{
				const Profile::HighScoreForASongAndSteps hsfas = pProfile->m_vRecentStepsScores[i];

				BEGIN_TABLE(1);
				TABLE_LINE2( "Song", hsfas.songID.ToString() );
				TABLE_LINE2( "Steps", hsfas.stepsID.ToString() );
				TABLE_LINE2( "Results", HighScoreToString(hsfas.hs) );
				END_TABLE;
			}
			if( pProfile->m_vRecentStepsScores.empty() )
				PrintEmptyTable(f);
		}
		PRINT_CLOSE(f);

		PRINT_OPEN(f, "Courses" );
		{
			for( unsigned i=0; i<pProfile->m_vRecentCourseScores.size(); i++ )
			{
				const Profile::HighScoreForACourse hsfac = pProfile->m_vRecentCourseScores[i];

				BEGIN_TABLE(1);
				TABLE_LINE2( "Course", hsfac.courseID.ToString() );
				TABLE_LINE2( "Results", HighScoreToString(hsfac.hs) );
				END_TABLE;
			}
			if( pProfile->m_vRecentCourseScores.empty() )
				PrintEmptyTable(f);
		}
		PRINT_CLOSE(f);
	}
	PRINT_CLOSE(f);

	return true;
}

bool PrintCatalogForSong( RageFile &f, const Profile *pProfile, Song* pSong )
{
	if( pSong->NeverDisplayed() || UNLOCKMAN->SongIsLocked(pSong) )
		return false;	// skip

	vector<Steps*> vpSteps = pSong->GetAllSteps();

	bool bShowRadarCategory[NUM_RADAR_CATEGORIES];
	CString sThemedRadarCategory[NUM_RADAR_CATEGORIES];
	FOREACH_RadarCategory(rc)
	{
		bShowRadarCategory[rc] = SHOW_RADAR_CATEGORY(rc);
		sThemedRadarCategory[rc] = RadarCategoryToThemedString(rc);
	}

	PRINT_OPEN(f, pSong->GetFullDisplayTitle() );
	{
		BEGIN_TABLE(2);
		TABLE_LINE2( "Artist", pSong->GetDisplayArtist() );
		TABLE_LINE2( "GroupName", pSong->m_sGroupName );
		float fMinBPM, fMaxBPM;
		pSong->GetDisplayBPM( fMinBPM, fMaxBPM );
		CString sBPM = (fMinBPM==fMaxBPM) ? ssprintf("%.1f",fMinBPM) : ssprintf("%.1f - %.1f",fMinBPM,fMaxBPM);
		TABLE_LINE2( "BPM", sBPM );
		TABLE_LINE2( "Credit", pSong->m_sCredit );
		TABLE_LINE2( "MusicLength", SecondsToMMSSMsMs(pSong->m_fMusicLengthSeconds) );
		TABLE_LINE2( "Lyrics", pSong->HasLyrics() );
		TABLE_LINE2( "NumTimesPlayed", pProfile->GetSongNumTimesPlayed(pSong) );
		END_TABLE;

		//
		// Print Steps list
		//
		for( unsigned j=0; j<vpSteps.size(); j++ )
		{
			Steps* pSteps = vpSteps[j];
			if( pSteps->IsAutogen() )
				continue;	// skip autogen
			CString s = 
				GAMEMAN->NotesTypeToThemedString(pSteps->m_StepsType) + 
				" - " +
				DifficultyToThemedString(pSteps->GetDifficulty());
			PRINT_OPEN(f, s, true);	// use poister value as the hash
			{
				BEGIN_TABLE(3);
				TABLE_LINE2( "Meter", pSteps->GetMeter() );
				TABLE_LINE2( "Description", pSteps->GetDescription() );
				TABLE_LINE2( "NumTimesPlayed", pProfile->GetStepsNumTimesPlayed(pSong,pSteps) );
				END_TABLE;

				BEGIN_TABLE(2);

				FOREACH_RadarCategory( cat )
				{
					if( !bShowRadarCategory[cat] )
						continue;	// skip

					const CString &sCat = sThemedRadarCategory[cat];
					float fVal = pSteps->GetRadarValues()[cat];
					CString sVal = ssprintf( "%05.2f", fVal );
					TABLE_LINE2( sCat, sVal );
				}
				END_TABLE;
			}
			PRINT_CLOSE(f);
		}
	}
	PRINT_CLOSE(f);

	return true;
}

bool PrintCatalogForGroup( RageFile &f, const Profile *pProfile, CString sGroup )
{
	return PrintSongsInGroup( f, pProfile, sGroup, PrintCatalogForSong );
}

void PrintCatalogList( RageFile &f, const Profile *pProfile, CString sTitle, vector<Song*> &vpSongs, vector<Steps*> &vpAllSteps, vector<StepsType> &vStepsTypesToShow, map<Steps*,Song*> mapStepsToSong, vector<Course*> vpCourses )
{
	PrintGroups( f, pProfile, sTitle, PrintCatalogForGroup );
}

void PrintBookkeeping( RageFile &f, const Profile *pProfile, CString sTitle, vector<Song*> &vpSongs, vector<Steps*> &vpAllSteps, vector<StepsType> &vStepsTypesToShow, map<Steps*,Song*> mapStepsToSong, vector<Course*> vpCourses)
{
	PRINT_OPEN(f, sTitle );
	{
		// GetCoinsLastDays
		{
			int coins[NUM_LAST_DAYS];
			BOOKKEEPER->GetCoinsLastDays( coins );
			PRINT_OPEN(f, ssprintf("Coins for Last %d Days",NUM_LAST_DAYS), true );
			{
				BEGIN_TABLE(4);
				for( int i=0; i<NUM_LAST_DAYS; i++ )
				{
					CString sDay = (i==0) ? "Today" : ssprintf("%d day(s) ago",i);
					TABLE_LINE2( sDay, coins[i] );
				}
				END_TABLE;
			}
			PRINT_CLOSE(f);
		}

		// GetCoinsLastWeeks
		{
			int coins[NUM_LAST_WEEKS];
			BOOKKEEPER->GetCoinsLastWeeks( coins );
			PRINT_OPEN(f, ssprintf("Coins for Last %d Weeks",NUM_LAST_WEEKS), true );
			{
				BEGIN_TABLE(4);
				for( int i=0; i<NUM_LAST_WEEKS; i++ )
				{
					CString sWeek = LastWeekToString( i );
					TABLE_LINE2( sWeek, coins[i] );
				}
				END_TABLE;
			}
			PRINT_CLOSE(f);
		}

		// GetCoinsByDayOfWeek
		{
			int coins[DAYS_IN_WEEK];
			BOOKKEEPER->GetCoinsByDayOfWeek( coins );
			PRINT_OPEN(f, "Coins by Day of Week", true );
			{
				BEGIN_TABLE(4);
				for( int i=0; i<DAYS_IN_WEEK; i++ )
				{
					CString sDay = DayOfWeekToString(i);
					TABLE_LINE2( sDay, coins[i] );
				}
				END_TABLE;
			}
			PRINT_CLOSE(f);
		}

		// GetCoinsByHour
		{
			int coins[HOURS_IN_DAY];
			BOOKKEEPER->GetCoinsByHour( coins );
			PRINT_OPEN(f, ssprintf("Coins for Last %d Hours",HOURS_IN_DAY), true );
			{
				BEGIN_TABLE(4);
				for( int i=0; i<HOURS_IN_DAY; i++ )
				{
					CString sHour = ssprintf("hour %d",i);
					TABLE_LINE2( sHour, coins[i] );
				}
				END_TABLE;
			}
			PRINT_CLOSE(f);
		}
	}
	PRINT_CLOSE(f);
}

void PrintScreenshot( RageFile &f, const Profile::Screenshot &ss )
{
	CString sImagePath = SCREENSHOTS_SUBDIR+ss.sFileName;
	CString sImgHtml = ssprintf("<a href='%s' target='_new'><img class='screenshot' src='%s' width='160' height='120'></a>", sImagePath.c_str(), sImagePath.c_str() );
	
	
	TranslatedWrite(f,"<table>\n");
	TranslatedWrite(f,"<tr>\n");
	TranslatedWrite(f,"<td>"+sImgHtml+"</td>\n");
	TranslatedWrite(f,"<td>\n");

	BEGIN_TABLE(1);

	TABLE_LINE2( "File", ss.sFileName );
	TABLE_LINE2( "MD5", ss.sMD5 );
	TABLE_LINE2( "Time", (CString)ctime(&ss.time) );
	TABLE_LINE2( "Machine", ss.sMachineGuid );

	END_TABLE;

	TranslatedWrite(f,"</td>\n");

	TranslatedWrite(f,"</table>\n");
}

void PrintScreenshots( RageFile &f, const Profile *pProfile, CString sTitle, CString sProfileDir )
{
	PRINT_OPEN(f, sTitle );
	{
		CString sCurrentMonth;
		bool bFirstMonth = true;

		for( int i = (int)pProfile->m_vScreenshots.size()-1; i >= 0; i-- )
		{
			Profile::Screenshot ss = pProfile->m_vScreenshots[i];
			tm new_time;
			localtime_r( &ss.time, &new_time );
			int iYear = new_time.tm_year+1900;
			int iMonth = new_time.tm_mon+1;
			CString sNewMonth = ssprintf("%02d/%d", iMonth, iYear );
			
			if( sNewMonth != sCurrentMonth )
			{
				if( !bFirstMonth )
					PRINT_CLOSE(f);
				PRINT_OPEN(f, sNewMonth, bFirstMonth);
			}
			PrintScreenshot( f, ss );

			sCurrentMonth = sNewMonth;
			bFirstMonth = false;
		}

		if( pProfile->m_vScreenshots.empty() )
		{
			PrintEmptyTable(f);
		}
		else
		{
			PRINT_CLOSE(f);
		}
	}
	PRINT_CLOSE(f);
}

void PrintCaloriesBurned( RageFile &f, const Profile *pProfile, CString sTitle, CString sProfileDir )
{
	PRINT_OPEN(f, sTitle );
	{
		PRINT_OPEN(f, "Totals", true);
		{
			BEGIN_TABLE(1);

			TABLE_LINE2("All Time", pProfile->GetDisplayTotalCaloriesBurned() );
			TABLE_LINE2("Per Song", ssprintf("%.2f",pProfile->m_fTotalCaloriesBurned/pProfile->GetTotalNumSongsPlayed()) );
			TABLE_LINE2("Per Second of Gameplay", ssprintf("%.2f",pProfile->m_fTotalCaloriesBurned/(float)pProfile->m_iTotalGameplaySeconds) );
		
			END_TABLE;
		}
		PRINT_CLOSE(f);

		PRINT_OPEN(f, "By Week", false);
		{
			BEGIN_TABLE(4);

			// row for each week
			tm when = GetLocalTime();
			when = GetNextSunday( when );
			when = AddDays( when, -DAYS_IN_WEEK );
			{
				for( int w=0; w<NUM_LAST_WEEKS; w++ )
				{
					float fWeekCals = 0;

					for( int d=0; d<DAYS_IN_WEEK; d++ )
					{
						Profile::Day day = { when.tm_yday, when.tm_year+1900 };
						float fDayCals = pProfile->GetCaloriesBurnedForDay( day );
						fWeekCals += fDayCals;
						when = AddDays( when, +1 );
					}

					TABLE_LINE2(LastWeekToString(w), Commify((int)fWeekCals) );

					when = AddDays( when, -DAYS_IN_WEEK*2 );
				}
			}
			
			END_TABLE;
		}
		PRINT_CLOSE(f);

		PRINT_OPEN(f, "By Day of Week", false);
		{
			float fCaloriesByDay[DAYS_IN_WEEK];
			ZERO( fCaloriesByDay );

			// row for each week
			tm when = GetLocalTime();
			when = GetNextSunday( when );
			when = AddDays( when, -DAYS_IN_WEEK );
			for( int w=0; w<NUM_LAST_WEEKS; w++ )
			{
				for( int d=0; d<DAYS_IN_WEEK; d++ )
				{
					Profile::Day day = { when.tm_yday, when.tm_year+1900 };
					float fDayCals = pProfile->GetCaloriesBurnedForDay( day );
					fCaloriesByDay[d] += fDayCals;
					when = AddDays( when, +1 );
				}

				when = AddDays( when, -DAYS_IN_WEEK*2 );
			}
			
			BEGIN_TABLE(2);

			for( int d=0; d<DAYS_IN_WEEK; d++ )
			{
				TABLE_LINE2(DayOfWeekToString(d), Commify((int)fCaloriesByDay[d]) );
			}

			END_TABLE;
		}
		PRINT_CLOSE(f);
	}
	PRINT_CLOSE(f);
}


void SaveStatsWebPage( 
	CString sDir, 
	const Profile *pProfile, 	
	const Profile *pMachineProfile, 	
	HtmlType htmlType
	)
{
	CString fn = sDir + STATS_HTML;

	LOG->Trace( "Writing %s ...", fn.c_str() );

	//
	// Open file
	//
	RageFile f;
	if( !f.Open( fn, RageFile::WRITE ) )
	{
		LOG->Warn( "Couldn't open file \"%s\": %s", fn.c_str(), f.GetError().c_str() );
		return;
	}

	//
	// Gather data
	//
	vector<Song*> vpSongs = SONGMAN->GetAllSongs();
	vector<Steps*> vpAllSteps;
	map<Steps*,Song*> mapStepsToSong;
	for( unsigned i=0; i<vpSongs.size(); i++ )
	{
		Song* pSong = vpSongs[i];
		vector<Steps*> vpSteps = pSong->GetAllSteps();
		for( unsigned j=0; j<vpSteps.size(); j++ )
		{
			Steps* pSteps = vpSteps[j];
			if( pSteps->IsAutogen() )
				continue;	// skip
			vpAllSteps.push_back( pSteps );
			mapStepsToSong[pSteps] = pSong;
		}
	}
	vector<Course*> vpCourses;
	SONGMAN->GetAllCourses( vpCourses, false );

	//
	// Calculate which StepTypes to show
	//
	vector<StepsType> vStepsTypesToShow;
	for( StepsType st=(StepsType)0; st<NUM_STEPS_TYPES; st=(StepsType)(st+1) )
	{
		// don't show if there are no Steps of this StepsType 
		bool bOneSongHasStepsForThisStepsType = false;
		for( unsigned i=0; i<vpSongs.size(); i++ )
		{
			Song* pSong = vpSongs[i];
			vector<Steps*> vpSteps;
			pSong->GetSteps( vpSteps, st, DIFFICULTY_INVALID, -1, -1, "", false );
			if( !vpSteps.empty() )
			{
				bOneSongHasStepsForThisStepsType = true;
				break;
			}
		}
		if( !bOneSongHasStepsForThisStepsType )
			continue;
		if( !SHOW_STEPS_TYPE(st) )
			continue;
		vStepsTypesToShow.push_back( st );
	}

	//
	// Print HTML headers
	//
	{
		TranslatedWrite(f, ssprintf("\
<html>\n\
<head>\n\
<META HTTP-EQUIV=\"Content-Type\" Content=\"text/html; charset=UTF-8\">\n\
<title>%s</title>\n\
<SCRIPT LANGUAGE=\"JavaScript\" TYPE=\"text/javascript\">\n\
<!-- // hide from old browsers\n\
\n\
// hide text from MSIE browsers\n\
\n\
with (document)\n\
{\n\
	write(\"<STYLE TYPE='text/css'>\");\n\
	if (navigator.appName == 'Microsoft Internet Explorer')\n\
		{\n\
		write(\".hiddentext {display:none} .visibletext {display:block} .outline {cursor:hand; text-decoration:underline}\");\n\
		}\n\
	write(\"</STYLE>\");\n\
}\n\
\n\
// show text on click for MSIE browsers\n\
\n\
function expandIt(whichEl)\n\
{\n\
	if (navigator.appName == 'Microsoft Internet Explorer')\n\
		{\n\
		whichEl.style.display = (whichEl.style.display == \"block\" ) ? \"none\" : \"block\";\n\
		}\n\
	else return;\n\
}\n\
// end hiding from old browsers -->\n\
</SCRIPT>\n\
<link rel='stylesheet' type='text/css' href='%s'>\n\
</head>\n\
<body>",
TITLE.c_str(), STYLE_CSS.c_str() ) );
	}

	CString sName = pProfile->GetDisplayName();
	time_t ltime = time( NULL );
	CString sTime = ctime( &ltime );

	CString sNameCell;
	switch( htmlType )
	{
	case HTML_TYPE_PLAYER:
		sNameCell = ssprintf(
			"%s<br>\n"
			"%s\n",
			sName.c_str(), sTime.c_str() );
		break;
	case HTML_TYPE_MACHINE:
		sNameCell = ssprintf(
			"Machine: %s<br>\n"
			"%s\n",
			sName.c_str(), sTime.c_str() );
		break;
	default:
		ASSERT(0);
	}

	TranslatedWrite(f, ssprintf(
		"<table border='0' cellpadding='0' cellspacing='0' width='100%%' cellspacing='5'><tr><td><h1>%s</h1></td><td>%s</td></tr>\n</table>\n",
		TITLE.c_str(), sNameCell.c_str() ) );

	switch( htmlType )
	{
	case HTML_TYPE_PLAYER:
		PrintInstructions(		f, pProfile,	"Instructions" );
		PrintStatistics(		f, pProfile,	"My Statistics",					vpSongs, vpAllSteps, vStepsTypesToShow, mapStepsToSong, vpCourses );
		PrintPopularity(		f, pProfile,	"My Popularity",					vpSongs, vpAllSteps, vStepsTypesToShow, mapStepsToSong, vpCourses );
		PrintSongHighScores(	f, pProfile,	"My Song High Scores",				vpSongs, vpAllSteps, vStepsTypesToShow, mapStepsToSong, vpCourses );
		PrintCourseHighScores(	f, pProfile,	"My Course High Scores",			vpSongs, vpAllSteps, vStepsTypesToShow, mapStepsToSong, vpCourses );
		PrintScreenshots(		f, pProfile,	"My Screenshots",					sDir );
		PrintCaloriesBurned(	f, pProfile,	"My Calories Burned",				sDir );
		PrintPercentComplete(	f, pProfile,	"My Percent Complete",				vpSongs, vpAllSteps, vStepsTypesToShow, mapStepsToSong, vpCourses );
		PrintRecentScores(		f, pProfile,	"My Recent Scores",					vpSongs, vpAllSteps, vStepsTypesToShow, mapStepsToSong, vpCourses );
		PrintPopularity(		f, pProfile,	"Last Machine's Popularity",		vpSongs, vpAllSteps, vStepsTypesToShow, mapStepsToSong, vpCourses );
		PrintSongHighScores(	f, pProfile,	"Last Machine's Song High Scores",	vpSongs, vpAllSteps, vStepsTypesToShow, mapStepsToSong, vpCourses );
		PrintCourseHighScores(	f, pProfile,	"Last Machine's Course High Scores",vpSongs, vpAllSteps, vStepsTypesToShow, mapStepsToSong, vpCourses );
		break;
	case HTML_TYPE_MACHINE:
		PrintStatistics(		f, pProfile,	"Statistics",			vpSongs, vpAllSteps, vStepsTypesToShow, mapStepsToSong, vpCourses );
		PrintPopularity(		f, pProfile,	"Popularity",			vpSongs, vpAllSteps, vStepsTypesToShow, mapStepsToSong, vpCourses );
		PrintSongHighScores(	f, pProfile,	"Song High Scores",		vpSongs, vpAllSteps, vStepsTypesToShow, mapStepsToSong, vpCourses );
		PrintCourseHighScores(	f, pProfile,	"Course High Scores",	vpSongs, vpAllSteps, vStepsTypesToShow, mapStepsToSong, vpCourses );
		PrintScreenshots(		f, pProfile,	"Screenshots",			sDir );
		PrintCaloriesBurned(	f, pProfile,	"Calories Burned",		sDir );
		PrintPercentComplete(	f, pProfile,	"Percent Complete",		vpSongs, vpAllSteps, vStepsTypesToShow, mapStepsToSong, vpCourses );
		PrintRecentScores(		f, pProfile,	"Most Scores",			vpSongs, vpAllSteps, vStepsTypesToShow, mapStepsToSong, vpCourses );
		PrintCatalogList(		f, pProfile,	"Song Information",		vpSongs, vpAllSteps, vStepsTypesToShow, mapStepsToSong, vpCourses );
		PrintBookkeeping(		f, pProfile,	"Bookkeeping",			vpSongs, vpAllSteps, vStepsTypesToShow, mapStepsToSong, vpCourses );	
		break;
	default:
		ASSERT(0);
	}

	TranslatedWrite(f, ssprintf("<p class='footer'>%s</p>\n", FOOTER.c_str()) );

	TranslatedWrite(f, "</body>\n" );
	TranslatedWrite(f, "</html>\n" );
	f.Close();

	// Don't sign Stats.html.  The html data is redundant to the xml data, and 
	// signing is slow.
	//if( PREFSMAN->m_bSignProfileData )
	//	CryptManager::SignFileToFile(fn);
	
	//
	// Copy CSS file from theme.  If the copy fails, oh well...
	// 
	CString sStyleFile = THEME->GetPathToO("ProfileManager style.css");
	FileCopy( sStyleFile, sDir+STYLE_CSS );
	LOG->Trace( "Done." );		

}


