#include "global.h"
#include "ScreenOptionsManageCourses.h"
#include "ScreenManager.h"
#include "RageLog.h"
#include "GameState.h"
#include "SongManager.h"
#include "CommonMetrics.h"
#include "ScreenTextEntry.h"
#include "ScreenPrompt.h"
#include "ScreenMiniMenu.h"
#include "GameManager.h"
#include "Difficulty.h"
#include "CourseUtil.h"
#include "LocalizedString.h"
#include "OptionRowHandler.h"
#include "ProfileManager.h"
#include "Profile.h"
#include "CourseWriterCRS.h"
#include "RageFileManager.h"
#include "PrefsManager.h"

AutoScreenMessage( SM_BackFromRename )
AutoScreenMessage( SM_BackFromDelete )
AutoScreenMessage( SM_BackFromContextMenu )

enum CourseAction
{
	CourseAction_Edit,
	CourseAction_Rename,
	CourseAction_Delete,
	NUM_CourseAction
};
static const char *CourseActionNames[] = {
	"Edit",
	"Rename",
	"Delete",
};
XToString( CourseAction );
#define FOREACH_CourseAction( i ) FOREACH_ENUM( CourseAction, i )

static MenuDef g_TempMenu(
	"ScreenMiniMenuContext"
);

REGISTER_SCREEN_CLASS( ScreenOptionsManageCourses );


struct StepsTypeAndDifficulty
{
	StepsType st;
	Difficulty cd;

	StepsTypeAndDifficulty( const StepsType &s, const Difficulty &d ) : st( s ), cd( d ) { }
	bool operator==( const StepsTypeAndDifficulty &stad ) const { return st == stad.st && cd == stad.cd; }
};
static void SetNextCombination()
{
	vector<StepsTypeAndDifficulty> v;
	{
		FOREACH_CONST( StepsType, CommonMetrics::STEPS_TYPES_TO_SHOW.GetValue(), st )
		{
			FOREACH_CONST( CourseDifficulty, CommonMetrics::COURSE_DIFFICULTIES_TO_SHOW.GetValue(), cd )
				v.push_back( StepsTypeAndDifficulty(*st, *cd) );
		}
	}

	StepsTypeAndDifficulty curVal( GAMESTATE->m_stEdit, GAMESTATE->m_cdEdit );
	vector<StepsTypeAndDifficulty>::const_iterator iter = find( v.begin(), v.end(), curVal );
	if( iter == v.end() || ++iter == v.end() )
		iter = v.begin();

	curVal = *iter;

	GAMESTATE->m_stEdit.Set( curVal.st );
	GAMESTATE->m_cdEdit.Set( curVal.cd );
	// XXX Testing.
	SCREENMAN->SystemMessage( ssprintf("%s, %s", GameManager::GetStepsTypeInfo(curVal.st).szName, DifficultyToString(curVal.cd).c_str()) );

	EditCourseUtil::UpdateAndSetTrail();
}

void ScreenOptionsManageCourses::Init()
{
	if( PREFSMAN->m_iArcadeOptionsNavigation )
		SetNavigation( NAV_THREE_KEY_MENU );

	ScreenOptions::Init();

	m_soundDifficultyChanged.Load( THEME->GetPathS("ScreenEditCourseSubmenu", "difficulty changed") );
	EDIT_MODE.Load( m_sName,"EditMode" );
	CREATE_NEW_SCREEN.Load( m_sName, "CreateNewScreen" );
}

void ScreenOptionsManageCourses::BeginScreen()
{
	if( GAMESTATE->m_stEdit == StepsType_Invalid  ||
	    GAMESTATE->m_cdEdit == Difficulty_Invalid )
	{
		SetNextCombination();
	}
	
	vector<const Style*> vpStyles;
	GameManager::GetStylesForGame( GAMESTATE->m_pCurGame, vpStyles );
	const Style *pStyle = vpStyles[0];
	GAMESTATE->SetCurrentStyle( pStyle );


	// Remember the current course.  All Course pointers will be invalidated when we load the machine profile below.
	CourseID cidLast;
	cidLast.FromCourse( GAMESTATE->m_pCurCourse );

	vector<OptionRowHandler*> vHands;
	
	int iIndex = 0;
	
	{
		vHands.push_back( OptionRowHandlerUtil::MakeNull() );
		OptionRowDefinition &def = vHands.back()->m_Def;
		def.m_layoutType = LAYOUT_SHOW_ONE_IN_ROW;
		def.m_bOneChoiceForAllPlayers = true;
		def.m_sName = "Create New Course";
		def.m_sExplanationName = "Create New Course";
		def.m_vsChoices.clear();
		def.m_vsChoices.push_back( "" );
		iIndex++;
	}	

	m_vpCourses.clear();
	FlushDirCache();
	PROFILEMAN->LoadMachineProfileEdits();
	
	switch( EDIT_MODE.GetValue() )
	{
	DEFAULT_FAIL( EDIT_MODE.GetValue() );
	case EditMode_Home:
		EditCourseUtil::GetAllEditCourses( m_vpCourses );
		break;
	case EditMode_Practice:
	case EditMode_Full:
		SONGMAN->GetAllCourses( m_vpCourses, false );
		break;
	}
	
	FOREACH_CONST( Course*, m_vpCourses, p )
	{
		vHands.push_back( OptionRowHandlerUtil::MakeNull() );
		OptionRowDefinition &def = vHands.back()->m_Def;
		
		def.m_sName = (*p)->GetDisplayFullTitle();
		def.m_bAllowThemeTitle = false;	// not themable
		def.m_sExplanationName = "Select Course";
		def.m_vsChoices.clear();
		def.m_vsChoices.push_back( "" );
		def.m_bAllowThemeItems = false;	// already themed
		iIndex++;
	}
	
	ScreenOptions::InitMenu( vHands );
	
	ScreenOptions::BeginScreen();
	
	// select the last chosen course
	GAMESTATE->m_pCurCourse.Set( cidLast.ToCourse() );
	if( GAMESTATE->m_pCurCourse )
	{
		EditCourseUtil::UpdateAndSetTrail();
		vector<Course*>::const_iterator iter = find( m_vpCourses.begin(), m_vpCourses.end(), GAMESTATE->m_pCurCourse );
		if( iter != m_vpCourses.end() )
		{
			int iIndex = iter - m_vpCourses.begin();
			this->MoveRowAbsolute( GAMESTATE->m_MasterPlayerNumber, 1 + iIndex );
		}
	}

	AfterChangeRow( GAMESTATE->m_MasterPlayerNumber );
}

static LocalizedString ERROR_RENAMING		("ScreenOptionsManageCourses", "Error renaming file.");
static LocalizedString ERROR_DELETING_FILE	("ScreenOptionsManageCourses", "Error deleting the file '%s'.");
static LocalizedString COURSE_WILL_BE_LOST	( "ScreenOptionsManageCourses", "This course will be lost permanently." );
static LocalizedString CONTINUE_WITH_DELETE	( "ScreenOptionsManageCourses", "Continue with delete?" );
static LocalizedString ENTER_COURSE_NAME	( "ScreenOptionsManageCourses", "Enter a name for the course." );
void ScreenOptionsManageCourses::HandleScreenMessage( const ScreenMessage SM )
{
	if( SM == SM_GoToNextScreen )
	{
		int iCurRow = m_iCurrentRow[GAMESTATE->m_MasterPlayerNumber];

		if( iCurRow == 0 )	// "create new"
		{
			/* Allocate the Course now, but don't save the file until the user explicitly chooses Save */
			Course *pCourse = new Course;
			EditCourseUtil::LoadDefaults( *pCourse );
			pCourse->m_LoadedFromProfile = ProfileSlot_Machine;
			SONGMAN->AddCourse( pCourse );
			GAMESTATE->m_pCurCourse.Set( pCourse );
			EditCourseUtil::s_bNewCourseNeedsName = true;
			EditCourseUtil::UpdateAndSetTrail();

			SCREENMAN->SetNewScreen( CREATE_NEW_SCREEN );
			return;	// don't call base
		}
		else if( m_pRows[iCurRow]->GetRowType() == OptionRow::RowType_Exit )
		{
			this->HandleScreenMessage( SM_GoToPrevScreen );
			return;	// don't call base
		}
		else
		{
			// do base behavior
		}
	}
	else if( SM == SM_BackFromRename )
	{
		if( !ScreenTextEntry::s_bCancelledLast )
		{
			ASSERT( ScreenTextEntry::s_sLastAnswer != "" );	// validate should have assured this

			if( !EditCourseUtil::RenameAndSave(GAMESTATE->m_pCurCourse, ScreenTextEntry::s_sLastAnswer) )
			{
				ScreenPrompt::Prompt( SM_None, ERROR_RENAMING );
				return;
			}

			SCREENMAN->SetNewScreen( this->m_sName ); // reload
		}
	}
	else if( SM == SM_BackFromDelete )
	{
		if( ScreenPrompt::s_LastAnswer == ANSWER_YES )
		{
			Course *pCourse = GetCourseWithFocus();
			if( !EditCourseUtil::RemoveAndDeleteFile( pCourse ) )
			{
				ScreenPrompt::Prompt( SM_None, ssprintf(ERROR_DELETING_FILE.GetValue(),pCourse->m_sPath.c_str()) );
				return;
			}

			GAMESTATE->m_pCurCourse.Set( NULL );
			GAMESTATE->m_pCurTrail[PLAYER_1].Set( NULL );
			SCREENMAN->SetNewScreen( this->m_sName ); // reload
		}
	}
	else if( SM == SM_BackFromContextMenu )
	{
		if( !ScreenMiniMenu::s_bCancelled )
		{
			switch( ScreenMiniMenu::s_iLastRowCode )
			{
			case CourseAction_Edit:
				{
					GAMESTATE->m_pCurCourse.Set( GetCourseWithFocus() );
					EditCourseUtil::UpdateAndSetTrail();
					EditCourseUtil::s_bNewCourseNeedsName = false;
					ScreenOptions::BeginFadingOut();
					break;
				}
			case CourseAction_Rename:
				ScreenTextEntry::TextEntry( 
					SM_BackFromRename, 
					ENTER_COURSE_NAME, 
					GAMESTATE->m_pCurCourse->GetDisplayFullTitle(), 
					EditCourseUtil::MAX_NAME_LENGTH, 
					EditCourseUtil::ValidateEditCourseName );
			break;
			case CourseAction_Delete:
				ScreenPrompt::Prompt( SM_None, COURSE_WILL_BE_LOST.GetValue()+"\n\n"+CONTINUE_WITH_DELETE.GetValue(), PROMPT_YES_NO, ANSWER_NO );
				break;
			}
		}
	}
	else if( SM == SM_LoseFocus )
	{
		this->PlayCommand( "ScreenLoseFocus" );
	}
	else if( SM == SM_GainFocus )
	{
		this->PlayCommand( "ScreenGainFocus" );
	}

	ScreenOptions::HandleScreenMessage( SM );
}
	
void ScreenOptionsManageCourses::AfterChangeRow( PlayerNumber pn )
{
	Course *pCourse = GetCourseWithFocus();
	Trail *pTrail = pCourse ? pCourse->GetTrail( GAMESTATE->m_stEdit, GAMESTATE->m_cdEdit ) : NULL;
	
	GAMESTATE->m_pCurCourse.Set( pCourse );
	GAMESTATE->m_pCurTrail[PLAYER_1].Set( pTrail );

	ScreenOptions::AfterChangeRow( pn );
}

void ScreenOptionsManageCourses::MenuSelect( const InputEventPlus &input )
{
	if( input.type != IET_FIRST_PRESS )
		return;
	SetNextCombination();
	m_soundDifficultyChanged.Play();

}

static LocalizedString YOU_HAVE_MAX( "ScreenOptionsManageCourses", "You have %d, the maximum number allowed." );
static LocalizedString YOU_MUST_DELETE( "ScreenOptionsManageCourses", "You must delete an existing before creating a new." );
void ScreenOptionsManageCourses::ProcessMenuStart( const InputEventPlus &input )
{
	if( IsTransitioning() )
		return;

	int iCurRow = m_iCurrentRow[GAMESTATE->m_MasterPlayerNumber];

	if( iCurRow == 0 )	// "create new"
	{
		vector<Course*> vpCourses;
		EditCourseUtil::GetAllEditCourses( vpCourses );
		if( vpCourses.size() >= (size_t)EditCourseUtil::MAX_PER_PROFILE )
		{
			RString s = ssprintf( YOU_HAVE_MAX.GetValue()+"\n\n"+YOU_MUST_DELETE.GetValue(), EditCourseUtil::MAX_PER_PROFILE );
			ScreenPrompt::Prompt( SM_None, s );
			return;
		}
		SCREENMAN->PlayStartSound();
		this->BeginFadingOut();
	}
	else if( m_pRows[iCurRow]->GetRowType() == OptionRow::RowType_Exit )
	{
		SCREENMAN->PlayStartSound();
		this->BeginFadingOut();
	}
	else	// a course
	{
		g_TempMenu.rows.clear();
		FOREACH_CourseAction( i )
		{
			MenuRowDef mrd( i, CourseActionToString(i), true, EditMode_Home, true, true, 0, "" );
			g_TempMenu.rows.push_back( mrd );
		}

		int iWidth, iX, iY;
		this->GetWidthXY( GAMESTATE->m_MasterPlayerNumber, iCurRow, 0, iWidth, iX, iY );
		ScreenMiniMenu::MiniMenu( &g_TempMenu, SM_BackFromContextMenu, SM_BackFromContextMenu, (float)iX, (float)iY );
	}
}

void ScreenOptionsManageCourses::ImportOptions( int iRow, const vector<PlayerNumber> &vpns )
{

}

void ScreenOptionsManageCourses::ExportOptions( int iRow, const vector<PlayerNumber> &vpns )
{

}

Course *ScreenOptionsManageCourses::GetCourseWithFocus() const
{
	int iCurRow = m_iCurrentRow[GAMESTATE->m_MasterPlayerNumber];
	if( iCurRow == 0 )
		return NULL;
	else if( m_pRows[iCurRow]->GetRowType() == OptionRow::RowType_Exit )
		return NULL;
	
	// a course
	int index = iCurRow - 1;
	return m_vpCourses[index];
}

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
