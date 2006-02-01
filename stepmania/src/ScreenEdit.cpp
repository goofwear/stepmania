#include "global.h"
#include "ScreenEdit.h"
#include "PrefsManager.h"
#include "SongManager.h"
#include "ScreenManager.h"
#include "ScreenSaveSync.h"
#include "GameConstantsAndTypes.h"
#include "GameManager.h"
#include "RageLog.h"
#include "GameSoundManager.h"
#include "GameState.h"
#include "InputMapper.h"
#include "RageLog.h"
#include "ThemeManager.h"
#include "NoteSkinManager.h"
#include "Steps.h"
#include "NoteDataUtil.h"
#include "SongUtil.h"
#include "StepsUtil.h"
#include "Foreach.h"
#include "ScreenDimensions.h"
#include "ThemeMetric.h"
#include "PlayerState.h"
#include "ScreenTextEntry.h"
#include "ScreenMiniMenu.h"
#include "ScreenPrompt.h"
#include "Style.h"
#include "ActorUtil.h"
#include "CommonMetrics.h"
#include "BackgroundUtil.h"
#include <utility>
#include <float.h>
#include "InputEventPlus.h"
#include "NotesWriterSM.h"
#include "LocalizedString.h"

static Preference<float> g_iDefaultRecordLength( "DefaultRecordLength", 4 );
static Preference<bool> g_bEditorShowBGChangesPlay( "EditorShowBGChangesPlay", false );

//
// Defines specific to ScreenEdit
//
const float RECORD_HOLD_SECONDS = 0.3f;

#define PLAYER_X			(SCREEN_CENTER_X)
#define PLAYER_Y			(SCREEN_CENTER_Y)
#define PLAYER_HEIGHT		(360)
#define PLAYER_Y_STANDARD	(PLAYER_Y-PLAYER_HEIGHT/2)

#define EDIT_X				(SCREEN_CENTER_X)
#define EDIT_Y				(PLAYER_Y)

#define RECORD_X			(SCREEN_CENTER_X)
#define RECORD_Y			(SCREEN_CENTER_Y)

#define PLAY_RECORD_HELP_TEXT	THEME->GetString(m_sName,"PlayRecordHelpText")

AutoScreenMessage( SM_UpdateTextInfo )
AutoScreenMessage( SM_BackFromMainMenu )
AutoScreenMessage( SM_BackFromAreaMenu )
AutoScreenMessage( SM_BackFromStepsInformation )
AutoScreenMessage( SM_BackFromOptions )
AutoScreenMessage( SM_BackFromSongInformation )
AutoScreenMessage( SM_BackFromBGChange )
AutoScreenMessage( SM_BackFromInsertTapAttack )
AutoScreenMessage( SM_BackFromInsertTapAttackPlayerOptions )
AutoScreenMessage( SM_BackFromInsertCourseAttack )
AutoScreenMessage( SM_BackFromInsertCourseAttackPlayerOptions )
AutoScreenMessage( SM_BackFromCourseModeMenu )
AutoScreenMessage( SM_DoRevertToLastSave )
AutoScreenMessage( SM_DoRevertFromDisk )
AutoScreenMessage( SM_BackFromBPMChange )
AutoScreenMessage( SM_BackFromStopChange )
AutoScreenMessage( SM_DoSaveAndExit )
AutoScreenMessage( SM_DoExit )
AutoScreenMessage( SM_SaveSuccessful );
AutoScreenMessage( SM_SaveFailed );

static const char *EditStateNames[] = {
	"Edit",
	"Record",
	"RecordPaused",
	"Playing"
};
XToString( EditState, NUM_EDIT_STATES );

const RString INPUT_TIPS_TEXT = 
#if defined(XBOX)
	"Up/Down:\n     change beat\n"
	"Left/Right:\n     change snap\n"
	"A/B/X/Y:\n     add/remove\n     tap note\n"
	"Create hold note:\n     Hold a button\n     while moving\n     Up or Down\n"
	"White:\n     Set area\n     marker\n"
	"Start:\n     Area Menu\n"
	"Select:\n     Main Menu\n"
	"Black:\n     Show\n     shortcuts\n";
#else
	"Up/Down:\n     change beat\n"
	"Left/Right:\n     change snap\n"
	"Number keys:\n     add/remove\n     tap note\n"
	"Create hold note:\n     Hold a number\n     while moving\n     Up or Down\n"
	"Space bar:\n     Set area\n     marker\n"
	"Enter:\n     Area Menu\n"
	"Escape:\n     Main Menu\n"
	"F1:\n     Show help\n";
#endif

#if defined(XBOX)
void ScreenEdit::InitEditMappings()
{
	/* XXX: fill this in */
}
#else
void ScreenEdit::InitEditMappings()
{
	m_EditMappings.Clear();

	// Global mappings:
	m_EditMappings.button[EDIT_BUTTON_SCROLL_UP_LINE][0] = DeviceInput(DEVICE_KEYBOARD, KEY_UP);
	m_EditMappings.button[EDIT_BUTTON_SCROLL_UP_PAGE][0] = DeviceInput(DEVICE_KEYBOARD, KEY_PGUP);
	m_EditMappings.button[EDIT_BUTTON_SCROLL_DOWN_LINE][0] = DeviceInput(DEVICE_KEYBOARD, KEY_DOWN);
	m_EditMappings.button[EDIT_BUTTON_SCROLL_DOWN_PAGE][0] = DeviceInput(DEVICE_KEYBOARD, KEY_PGDN);
	m_EditMappings.button[EDIT_BUTTON_SCROLL_HOME][0] = DeviceInput(DEVICE_KEYBOARD, KEY_HOME);
	m_EditMappings.button[EDIT_BUTTON_SCROLL_END][0] = DeviceInput(DEVICE_KEYBOARD, KEY_END);
	m_EditMappings.button[EDIT_BUTTON_SCROLL_NEXT][0] = DeviceInput(DEVICE_KEYBOARD, KEY_PERIOD);
	m_EditMappings.button[EDIT_BUTTON_SCROLL_PREV][0] = DeviceInput(DEVICE_KEYBOARD, KEY_COMMA);

	m_EditMappings.button[EDIT_BUTTON_SCROLL_SELECT][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT);
	m_EditMappings.button[EDIT_BUTTON_SCROLL_SELECT][1] = DeviceInput(DEVICE_KEYBOARD, KEY_RSHIFT);

	switch( EDIT_MODE.GetValue() )
	{
	case EditMode_Practice:
		// Left = Zoom out (1x, 2x, 4x)
		// Right = Zoom in
		m_EditMappings.button   [EDIT_BUTTON_SCROLL_SPEED_DOWN][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LEFT);
		m_EditMappings.button   [EDIT_BUTTON_SCROLL_SPEED_UP][0]   = DeviceInput(DEVICE_KEYBOARD, KEY_RIGHT);

		// Enter = Play selection
		// P = Play current beat to end
		m_EditMappings.button   [EDIT_BUTTON_PLAY_SELECTION][0]    = DeviceInput(DEVICE_KEYBOARD, KEY_ENTER);
		m_EditMappings.button   [EDIT_BUTTON_PLAY_FROM_CURSOR][0]  = DeviceInput(DEVICE_KEYBOARD, KEY_Cp);

		// F1 = Show help popup
		m_EditMappings.button   [EDIT_BUTTON_OPEN_INPUT_HELP][0]   = DeviceInput(DEVICE_KEYBOARD, KEY_F1);

		// Esc = Show Edit Menu
		m_EditMappings.button   [EDIT_BUTTON_OPEN_EDIT_MENU][0]    = DeviceInput(DEVICE_KEYBOARD, KEY_ESC);

		// Escape, Enter = exit play/record
		m_PlayMappings.button   [EDIT_BUTTON_RETURN_TO_EDIT][0]    = DeviceInput(DEVICE_KEYBOARD, KEY_ENTER);
		m_PlayMappings.button   [EDIT_BUTTON_RETURN_TO_EDIT][1]    = DeviceInput(DEVICE_KEYBOARD, KEY_ESC);
		return;
	}

	switch( EDIT_MODE.GetValue() )
	{
	case EditMode_Full:
		/* Don't allow F5/F6 in home mode.  It breaks the "delay creation until first save" logic. */
		m_EditMappings.button[EDIT_BUTTON_OPEN_PREV_STEPS][0] = DeviceInput(DEVICE_KEYBOARD, KEY_F5);
		m_EditMappings.button[EDIT_BUTTON_OPEN_NEXT_STEPS][0] = DeviceInput(DEVICE_KEYBOARD, KEY_F6);
		m_EditMappings.button[EDIT_BUTTON_BPM_DOWN][0] = DeviceInput(DEVICE_KEYBOARD, KEY_F7);
		m_EditMappings.button[EDIT_BUTTON_BPM_UP][0] = DeviceInput(DEVICE_KEYBOARD, KEY_F8);
		m_EditMappings.button[EDIT_BUTTON_STOP_DOWN][0] = DeviceInput(DEVICE_KEYBOARD, KEY_F9);
		m_EditMappings.button[EDIT_BUTTON_STOP_UP][0]  = DeviceInput(DEVICE_KEYBOARD, KEY_F10);
		m_EditMappings.button[EDIT_BUTTON_OFFSET_DOWN][0] = DeviceInput(DEVICE_KEYBOARD, KEY_F11);
		m_EditMappings.button[EDIT_BUTTON_OFFSET_UP][0]  = DeviceInput(DEVICE_KEYBOARD, KEY_F12);

		m_EditMappings.button[EDIT_BUTTON_SAMPLE_START_UP][0] = DeviceInput(DEVICE_KEYBOARD, KEY_RBRACKET);
		m_EditMappings.button[EDIT_BUTTON_SAMPLE_START_DOWN][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LBRACKET);
		m_EditMappings.button[EDIT_BUTTON_SAMPLE_LENGTH_UP][0] = DeviceInput(DEVICE_KEYBOARD, KEY_RBRACKET);
		m_EditMappings.hold[EDIT_BUTTON_SAMPLE_LENGTH_UP][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT);
		m_EditMappings.hold[EDIT_BUTTON_SAMPLE_LENGTH_UP][1] = DeviceInput(DEVICE_KEYBOARD, KEY_RSHIFT);
		m_EditMappings.button[EDIT_BUTTON_SAMPLE_LENGTH_DOWN][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LBRACKET);
		m_EditMappings.hold[EDIT_BUTTON_SAMPLE_LENGTH_DOWN][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT);
		m_EditMappings.hold[EDIT_BUTTON_SAMPLE_LENGTH_DOWN][1] = DeviceInput(DEVICE_KEYBOARD, KEY_RSHIFT);

		m_EditMappings.button[EDIT_BUTTON_PLAY_SAMPLE_MUSIC][0] = DeviceInput(DEVICE_KEYBOARD, KEY_Cm);

		m_EditMappings.button[EDIT_BUTTON_OPEN_BGCHANGE_LAYER1_MENU][0] = DeviceInput(DEVICE_KEYBOARD, KEY_Cb);
		m_EditMappings.button[EDIT_BUTTON_OPEN_BGCHANGE_LAYER2_MENU][0] = DeviceInput(DEVICE_KEYBOARD, KEY_Cb);
		m_EditMappings.hold[EDIT_BUTTON_OPEN_BGCHANGE_LAYER2_MENU][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT);
		m_EditMappings.hold[EDIT_BUTTON_OPEN_BGCHANGE_LAYER2_MENU][1] = DeviceInput(DEVICE_KEYBOARD, KEY_RSHIFT);
		m_EditMappings.button[EDIT_BUTTON_OPEN_COURSE_MENU][0] = DeviceInput(DEVICE_KEYBOARD, KEY_Cc);
		m_EditMappings.button[EDIT_BUTTON_OPEN_COURSE_ATTACK_MENU][0] = DeviceInput(DEVICE_KEYBOARD, KEY_Cv);

		m_EditMappings.button[EDIT_BUTTON_INSERT_SHIFT_PAUSES][0] = DeviceInput(DEVICE_KEYBOARD, KEY_INSERT);
		m_EditMappings.hold[EDIT_BUTTON_INSERT_SHIFT_PAUSES][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LCTRL);
		m_EditMappings.hold[EDIT_BUTTON_INSERT_SHIFT_PAUSES][1] = DeviceInput(DEVICE_KEYBOARD, KEY_RCTRL);
		m_EditMappings.button[EDIT_BUTTON_DELETE_SHIFT_PAUSES][0] = DeviceInput(DEVICE_KEYBOARD, KEY_DEL);
		m_EditMappings.hold[EDIT_BUTTON_DELETE_SHIFT_PAUSES][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LCTRL);
		m_EditMappings.hold[EDIT_BUTTON_DELETE_SHIFT_PAUSES][1] = DeviceInput(DEVICE_KEYBOARD, KEY_RCTRL);
		break;
	}

	m_EditMappings.button[EDIT_BUTTON_COLUMN_0][0] = DeviceInput(DEVICE_KEYBOARD, KEY_C1);
	m_EditMappings.button[EDIT_BUTTON_COLUMN_1][0] = DeviceInput(DEVICE_KEYBOARD, KEY_C2);
	m_EditMappings.button[EDIT_BUTTON_COLUMN_2][0] = DeviceInput(DEVICE_KEYBOARD, KEY_C3);
	m_EditMappings.button[EDIT_BUTTON_COLUMN_3][0] = DeviceInput(DEVICE_KEYBOARD, KEY_C4);
	m_EditMappings.button[EDIT_BUTTON_COLUMN_4][0] = DeviceInput(DEVICE_KEYBOARD, KEY_C5);
	m_EditMappings.button[EDIT_BUTTON_COLUMN_5][0] = DeviceInput(DEVICE_KEYBOARD, KEY_C6);
	m_EditMappings.button[EDIT_BUTTON_COLUMN_6][0] = DeviceInput(DEVICE_KEYBOARD, KEY_C7);
	m_EditMappings.button[EDIT_BUTTON_COLUMN_7][0] = DeviceInput(DEVICE_KEYBOARD, KEY_C8);
	m_EditMappings.button[EDIT_BUTTON_COLUMN_8][0] = DeviceInput(DEVICE_KEYBOARD, KEY_C9);
	m_EditMappings.button[EDIT_BUTTON_COLUMN_9][0] = DeviceInput(DEVICE_KEYBOARD, KEY_C0);

	m_EditMappings.button[EDIT_BUTTON_RIGHT_SIDE][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LALT);
	m_EditMappings.button[EDIT_BUTTON_RIGHT_SIDE][1] = DeviceInput(DEVICE_KEYBOARD, KEY_RALT);
	m_EditMappings.button[EDIT_BUTTON_LAY_MINE_OR_ROLL][0]   = DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT);
	// m_EditMappings.button[EDIT_BUTTON_LAY_TAP_ATTACK][0] = DeviceInput(DEVICE_KEYBOARD, KEY_RSHIFT);

	m_EditMappings.button    [EDIT_BUTTON_SCROLL_SPEED_UP][0] = DeviceInput(DEVICE_KEYBOARD, KEY_UP);
	m_EditMappings.hold[EDIT_BUTTON_SCROLL_SPEED_UP][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LCTRL);
	m_EditMappings.hold[EDIT_BUTTON_SCROLL_SPEED_UP][1] = DeviceInput(DEVICE_KEYBOARD, KEY_RCTRL);

	m_EditMappings.button    [EDIT_BUTTON_SCROLL_SPEED_DOWN][0] = DeviceInput(DEVICE_KEYBOARD, KEY_DOWN);
	m_EditMappings.hold[EDIT_BUTTON_SCROLL_SPEED_DOWN][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LCTRL);
	m_EditMappings.hold[EDIT_BUTTON_SCROLL_SPEED_DOWN][1] = DeviceInput(DEVICE_KEYBOARD, KEY_RCTRL);

	m_EditMappings.button[EDIT_BUTTON_SCROLL_SELECT][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT);
	m_EditMappings.button[EDIT_BUTTON_SCROLL_SELECT][1] = DeviceInput(DEVICE_KEYBOARD, KEY_RSHIFT);

	m_EditMappings.button[EDIT_BUTTON_LAY_SELECT][0] = DeviceInput(DEVICE_KEYBOARD, KEY_SPACE);

	m_EditMappings.button[EDIT_BUTTON_SNAP_NEXT][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LEFT);
	m_EditMappings.button[EDIT_BUTTON_SNAP_PREV][0] = DeviceInput(DEVICE_KEYBOARD, KEY_RIGHT);

	m_EditMappings.button[EDIT_BUTTON_OPEN_EDIT_MENU][0] = DeviceInput(DEVICE_KEYBOARD, KEY_ESC);
	m_EditMappings.button[EDIT_BUTTON_OPEN_AREA_MENU][0] = DeviceInput(DEVICE_KEYBOARD, KEY_ENTER);
	m_EditMappings.button[EDIT_BUTTON_OPEN_INPUT_HELP][0] = DeviceInput(DEVICE_KEYBOARD, KEY_F1);
	
	m_EditMappings.button[EDIT_BUTTON_BAKE_RANDOM_FROM_SONG_GROUP][0] = DeviceInput(DEVICE_KEYBOARD, KEY_Cb);
	m_EditMappings.hold[EDIT_BUTTON_BAKE_RANDOM_FROM_SONG_GROUP][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LALT);
	m_EditMappings.hold[EDIT_BUTTON_BAKE_RANDOM_FROM_SONG_GROUP][1] = DeviceInput(DEVICE_KEYBOARD, KEY_RALT);
	m_EditMappings.button[EDIT_BUTTON_BAKE_RANDOM_FROM_SONG_GROUP_AND_GENRE][0] = DeviceInput(DEVICE_KEYBOARD, KEY_Cb);
	m_EditMappings.hold[EDIT_BUTTON_BAKE_RANDOM_FROM_SONG_GROUP_AND_GENRE][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LCTRL);
	m_EditMappings.hold[EDIT_BUTTON_BAKE_RANDOM_FROM_SONG_GROUP_AND_GENRE][1] = DeviceInput(DEVICE_KEYBOARD, KEY_RCTRL);

	m_EditMappings.button[EDIT_BUTTON_PLAY_FROM_START][0] = DeviceInput(DEVICE_KEYBOARD, KEY_Cp);
	m_EditMappings.hold[EDIT_BUTTON_PLAY_FROM_START][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LCTRL);
	m_EditMappings.hold[EDIT_BUTTON_PLAY_FROM_START][1] = DeviceInput(DEVICE_KEYBOARD, KEY_RCTRL);
	m_EditMappings.button[EDIT_BUTTON_PLAY_FROM_CURSOR][0] = DeviceInput(DEVICE_KEYBOARD, KEY_Cp);
	m_EditMappings.hold[EDIT_BUTTON_PLAY_FROM_CURSOR][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT);
	m_EditMappings.hold[EDIT_BUTTON_PLAY_FROM_CURSOR][1] = DeviceInput(DEVICE_KEYBOARD, KEY_RSHIFT);
	m_EditMappings.button[EDIT_BUTTON_PLAY_SELECTION][0] = DeviceInput(DEVICE_KEYBOARD, KEY_Cp);
	m_EditMappings.button[EDIT_BUTTON_RECORD_SELECTION][0] = DeviceInput(DEVICE_KEYBOARD, KEY_Cr);
	m_EditMappings.hold[EDIT_BUTTON_RECORD_SELECTION][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LCTRL);
	m_EditMappings.hold[EDIT_BUTTON_RECORD_SELECTION][1] = DeviceInput(DEVICE_KEYBOARD, KEY_RCTRL);

	m_EditMappings.button[EDIT_BUTTON_INSERT][0] = DeviceInput(DEVICE_KEYBOARD, KEY_INSERT);
	m_EditMappings.button[EDIT_BUTTON_INSERT][1] = DeviceInput(DEVICE_KEYBOARD, KEY_BACKSLASH);
	m_EditMappings.button[EDIT_BUTTON_DELETE][0] = DeviceInput(DEVICE_KEYBOARD, KEY_DEL);

	m_EditMappings.button[EDIT_BUTTON_ADJUST_FINE][0] = DeviceInput(DEVICE_KEYBOARD, KEY_RALT);
	m_EditMappings.button[EDIT_BUTTON_ADJUST_FINE][1] = DeviceInput(DEVICE_KEYBOARD, KEY_LALT);
	
	m_EditMappings.button[EDIT_BUTTON_UNDO][1] = DeviceInput(DEVICE_KEYBOARD, KEY_Cu);
	
	m_PlayMappings.button[EDIT_BUTTON_RETURN_TO_EDIT][0] = DeviceInput(DEVICE_KEYBOARD, KEY_ESC);
	m_PlayMappings.button[EDIT_BUTTON_TOGGLE_ASSIST_TICK][0] = DeviceInput(DEVICE_KEYBOARD, KEY_F4);
	m_PlayMappings.button[EDIT_BUTTON_TOGGLE_AUTOPLAY][0] = DeviceInput(DEVICE_KEYBOARD, KEY_F8);

	m_RecordMappings.button[EDIT_BUTTON_LAY_MINE_OR_ROLL][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT);
	m_RecordMappings.button[EDIT_BUTTON_LAY_MINE_OR_ROLL][1] = DeviceInput(DEVICE_KEYBOARD, KEY_RSHIFT);
	m_RecordMappings.button[EDIT_BUTTON_REMOVE_NOTE][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LALT);
	m_RecordMappings.button[EDIT_BUTTON_REMOVE_NOTE][1] = DeviceInput(DEVICE_KEYBOARD, KEY_RALT);
	m_RecordMappings.button[EDIT_BUTTON_RETURN_TO_EDIT][0] = DeviceInput(DEVICE_KEYBOARD, KEY_ESC);

	m_RecordPausedMappings.button[EDIT_BUTTON_PLAY_SELECTION][0] = DeviceInput(DEVICE_KEYBOARD, KEY_Cp);
	m_RecordPausedMappings.button[EDIT_BUTTON_RECORD_SELECTION][0] = DeviceInput(DEVICE_KEYBOARD, KEY_Cr);
	m_RecordPausedMappings.hold[EDIT_BUTTON_RECORD_SELECTION][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LCTRL);
	m_RecordPausedMappings.hold[EDIT_BUTTON_RECORD_SELECTION][1] = DeviceInput(DEVICE_KEYBOARD, KEY_RCTRL);
	m_RecordPausedMappings.button[EDIT_BUTTON_RECORD_FROM_CURSOR][0] = DeviceInput(DEVICE_KEYBOARD, KEY_Cr);
	m_RecordPausedMappings.button[EDIT_BUTTON_RETURN_TO_EDIT][0] = DeviceInput(DEVICE_KEYBOARD, KEY_ESC);
	m_RecordPausedMappings.button[EDIT_BUTTON_UNDO][0] = DeviceInput(DEVICE_KEYBOARD, KEY_Cu);
}

#endif

/* Given a DeviceInput that was just depressed, return an active edit function. */
bool ScreenEdit::DeviceToEdit( DeviceInput DeviceI, EditButton &button ) const
{
	ASSERT( DeviceI.IsValid() );

	const MapEditToDI *pCurrentMap = GetCurrentMap();

	/* First, search to see if a key that requires a modifier is pressed. */
	FOREACH_EditButton(e)
	{
		for( int slot = 0; slot < NUM_EDIT_TO_DEVICE_SLOTS; ++slot )
		{
			if( pCurrentMap->button[e][slot] == DeviceI && pCurrentMap->hold[e][0].IsValid() )
			{
				/* The button maps to this function. */
				button = e;

				/* This function has one or more shift modifier attached. */
				for( int holdslot = 0; holdslot < NUM_EDIT_TO_DEVICE_SLOTS; ++holdslot )
				{
					DeviceInput hDI = pCurrentMap->hold[e][holdslot];
					if( hDI.IsValid() && INPUTFILTER->IsBeingPressed(hDI) )
						return true;
				}
			}
		}
	}

	/* No shifted keys matched.  See if any unshifted inputs are bound to this key. */  
	FOREACH_EditButton(e)
	{
		for( int slot = 0; slot < NUM_EDIT_TO_DEVICE_SLOTS; ++slot )
		{
			if( pCurrentMap->button[e][slot] == DeviceI && !pCurrentMap->hold[e][0].IsValid() )
			{
				/* The button maps to this function. */
				button = e;
				return true;
			}
		}
	}

	button = EDIT_BUTTON_INVALID;

	return false;
}

/* If DeviceI was just pressed, return true if button is triggered.  (More than one
 * function may be mapped to a key.) */
bool ScreenEdit::EditPressed( EditButton button, const DeviceInput &DeviceI )
{
	ASSERT( DeviceI.IsValid() );

	const MapEditToDI *pCurrentMap = GetCurrentMap();

	/* First, search to see if a key that requires a modifier is pressed. */
	bool bPrimaryButtonPressed = false;
	for( int slot = 0; slot < NUM_EDIT_TO_DEVICE_SLOTS; ++slot )
	{
		if( pCurrentMap->button[button][slot] == DeviceI )
			bPrimaryButtonPressed = true;
	}

	if( !bPrimaryButtonPressed )
		return false;

	/* The button maps to this function.  Does the function has one or more shift modifiers attached? */
	if( !pCurrentMap->hold[button][0].IsValid() )
		return true;

	for( int holdslot = 0; holdslot < NUM_EDIT_TO_DEVICE_SLOTS; ++holdslot )
	{
		DeviceInput hDI = pCurrentMap->hold[button][holdslot];
		if( INPUTFILTER->IsBeingPressed(hDI) )
			return true;
	}

	/* No shifted keys matched. */  
	return false;
}

bool ScreenEdit::EditToDevice( EditButton button, int iSlotNum, DeviceInput &DeviceI ) const
{
	ASSERT( iSlotNum < NUM_EDIT_TO_DEVICE_SLOTS );
	const MapEditToDI *pCurrentMap = GetCurrentMap();
	if( pCurrentMap->button[button][iSlotNum].IsValid() )
		DeviceI = pCurrentMap->button[button][iSlotNum];
	else if( pCurrentMap->hold[button][iSlotNum].IsValid() )
		DeviceI = pCurrentMap->hold[button][iSlotNum];
	return DeviceI.IsValid();
}

bool ScreenEdit::EditIsBeingPressed( EditButton button ) const
{
	for( int slot = 0; slot < NUM_EDIT_TO_DEVICE_SLOTS; ++slot )
	{
		DeviceInput DeviceI;
		if( EditToDevice( button, slot, DeviceI ) && INPUTFILTER->IsBeingPressed(DeviceI) )
			return true;
	}

	return false;
}

const MapEditToDI *ScreenEdit::GetCurrentMap() const
{
	switch( m_EditState )
	{
	case STATE_EDITING: return &m_EditMappings;
	case STATE_PLAYING: return &m_PlayMappings;
	case STATE_RECORDING: return &m_RecordMappings;
	case STATE_RECORDING_PAUSED: return &m_RecordPausedMappings;
	default: FAIL_M( ssprintf("%i",m_EditState) );
	}
}


static MenuDef g_EditHelp(
	"ScreenMiniMenuEditHelp",
#if defined(XBOX)
	MenuRowDef( -1, "L + Up/Down: Change zoom",				false, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( -1, "R + Up/Down: Drag area marker",			false, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( -1, "L + Select: Play selection",				false, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( -1, "R + Start: Play whole song",				false, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( -1, "R + Select: Record",					false, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( -1, "L + Black: Toggle assist tick",			false, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( -1, "R + White: Insert beat and shift down",		false, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( -1, "R + Black: Delete beat and shift up",			false, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( -1, "R + button: Lay mine",					false, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( -1, "L + button: Add to/remove from right half",		false, EditMode_Practice, true, 0, NULL )
#else
	MenuRowDef( -1, "PgUp/PgDn: jump measure",				false, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( -1, "Home/End: jump to first/last beat",			false, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( -1, "Ctrl + Up/Down: Change zoom",				false, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( -1, "Shift + Up/Down: Drag area marker",			false, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( -1, "P: Play selection",					false, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( -1, "Ctrl + P: Play whole song",				false, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( -1, "Shift + P: Play current beat to end",			false, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( -1, "Ctrl + R: Record",					false, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( -1, "F4: Toggle assist tick",				false, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( -1, "F5/F6: Next/prev steps of same StepsType",		false, EditMode_Full,     true, 0, NULL ),
	MenuRowDef( -1, "F7/F8: Decrease/increase BPM at cur beat",		false, EditMode_Full,     true, 0, NULL ),
	MenuRowDef( -1, "F9/F10: Decrease/increase stop at cur beat",		false, EditMode_Full,     true, 0, NULL ),
	MenuRowDef( -1, "F11/F12: Decrease/increase music offset",		false, EditMode_Full,     true, 0, NULL ),
			/* XXX: This would be better as a single submenu, to let people tweak
			* and play the sample several times (without having to re-enter the
			* menu each time), so it doesn't use a whole bunch of hotkeys. */
	MenuRowDef( -1, "[ and ]: Decrease/increase sample music start",	false, EditMode_Full,     true, 0, NULL ),
	MenuRowDef( -1, "{ and }: Decrease/increase sample music length",	false, EditMode_Full,     true, 0, NULL ),
	MenuRowDef( -1, "M: Play sample music",					false, EditMode_Full,     true, 0, NULL ),
	MenuRowDef( -1, "B: Add/Edit Background Change",			false, EditMode_Full,     true, 0, NULL ),
	MenuRowDef( -1, "Insert: Insert beat and shift down",			false, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( -1, "Ctrl + Insert: Shift BPM changes and stops down one beat",
										false, EditMode_Full,     true, 0, NULL ),
	MenuRowDef( -1, "Delete: Delete beat and shift up",			false, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( -1, "Ctrl + Delete: Shift BPM changes and stops up one beat",
										false, EditMode_Full,     true, 0, NULL ),
	MenuRowDef( -1, "Shift + number: Lay mine",				false, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( -1, "Alt + number: Add to/remove from right half",		false, EditMode_Practice, true, 0, NULL )
#endif
);

static MenuDef g_PracticeHelp(
	"ScreenMiniMenuPracticeHelp",
	MenuRowDef( -1, "Up, Down: Move cursor",		false, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( -1, "PgUp, PgDn: Jump measure",		false, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( -1, "Home, End: Jump to first/last beat",	false, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( -1, "Hold Shift: Select region",		false, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( -1, "Left, Right: Zoom",			false, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( -1, "Enter: Play selection",		false, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( -1, "P: Play from cursor",			false, EditMode_Practice, true, 0, NULL )
);

static MenuDef g_MainMenu(
	"ScreenMiniMenuMainMenu",
	MenuRowDef( ScreenEdit::edit_steps_information,		"Edit Steps Information",	true, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( ScreenEdit::play_whole_song,		"Play Whole Song",		true, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( ScreenEdit::play_current_beat_to_end,	"Play Current Beat to End",	true, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( ScreenEdit::save,				"Save",				true, EditMode_Home, true, 0, NULL ),
	MenuRowDef( ScreenEdit::revert_to_last_save,		"Revert to Last Save",		true, EditMode_Home, true, 0, NULL ),
	MenuRowDef( ScreenEdit::revert_from_disk,		"Revert from Disk",		true, EditMode_Full, true, 0, NULL ),
	MenuRowDef( ScreenEdit::options,			"Editor Options",		true, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( ScreenEdit::edit_song_info,			"Edit Song Info",		true, EditMode_Full, true, 0, NULL ),
	MenuRowDef( ScreenEdit::edit_bpm,			"Edit BPM Change",		true, EditMode_Full, true, 0, NULL ),
	MenuRowDef( ScreenEdit::edit_stop,			"Edit Stop",			true, EditMode_Full, true, 0, NULL ),
	MenuRowDef( ScreenEdit::play_preview_music,		"Play Preview Music",		true, EditMode_Full, true, 0, NULL ),
	MenuRowDef( ScreenEdit::exit,				"Exit Editor",			true, EditMode_Practice, true, 0, NULL )
);

static MenuDef g_AreaMenu(
	"ScreenMiniMenuAreaMenu",
	MenuRowDef( ScreenEdit::cut,			"Cut",					true, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( ScreenEdit::copy,			"Copy",					true, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( ScreenEdit::paste_at_current_beat,	"Paste at current beat",		true, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( ScreenEdit::paste_at_begin_marker,	"Paste at begin marker",		true, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( ScreenEdit::clear,			"Clear area",				true, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( ScreenEdit::quantize,		"Quantize",				true, EditMode_Practice, true, 0, "4th","8th","12th","16th","24th","32nd","48th","64th","192nd"),
	MenuRowDef( ScreenEdit::turn,			"Turn",					true, EditMode_Practice, true, 0, "Left","Right","Mirror","Shuffle","SuperShuffle" ),
	MenuRowDef( ScreenEdit::transform,		"Transform",				true, EditMode_Practice, true, 0, "NoHolds","NoMines","Little","Wide","Big","Quick","Skippy","Mines","Echo","Stomp","Planted","Floored","Twister","NoJumps","NoHands","NoQuads","NoStretch" ),
	MenuRowDef( ScreenEdit::alter,			"Alter",				true, EditMode_Practice, true, 0, "Autogen To Fill Width","Backwards","Swap Sides","Copy Left To Right","Copy Right To Left","Clear Left","Clear Right","Collapse To One","Collapse Left","Shift Left","Shift Right" ),
	MenuRowDef( ScreenEdit::tempo,			"Tempo",				true, EditMode_Full, true, 0, "Compress 2x","Compress 3->2","Compress 4->3","Expand 3->4","Expand 2->3","Expand 2x" ),
	MenuRowDef( ScreenEdit::play,			"Play selection",			true, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( ScreenEdit::record,			"Record in selection",			true, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( ScreenEdit::insert_and_shift,	"Insert beat and shift down",		true, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( ScreenEdit::delete_and_shift,	"Delete beat and shift up",		true, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( ScreenEdit::shift_pauses_forward,	"Shift pauses and BPM changes down",	true, EditMode_Full, true, 0, NULL ),
	MenuRowDef( ScreenEdit::shift_pauses_backward,	"Shift pauses and BPM changes up",	true, EditMode_Full, true, 0, NULL ),
	MenuRowDef( ScreenEdit::convert_to_pause,	"Convert selection to pause",		true, EditMode_Full, true, 0, NULL ),
	MenuRowDef( ScreenEdit::convert_pause_to_beat,	"Convert pause to beats",		true, EditMode_Full, true, 0, NULL ),
	MenuRowDef( ScreenEdit::undo,			"Undo",					true, EditMode_Practice, true, 0, NULL )
);

static MenuDef g_StepsInformation(
	"ScreenMiniMenuStepsInformation",
	MenuRowDef( ScreenEdit::difficulty,	"Difficulty",		true, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( ScreenEdit::meter,		"Meter",		true, EditMode_Practice, false, 0, "1","2","3","4","5","6","7","8","9","10","11","12","13" ),
	MenuRowDef( ScreenEdit::description,	"Description",		true, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( ScreenEdit::predict_meter,	"Predicted Meter",	false, EditMode_Full, true, 0, NULL ),
	MenuRowDef( ScreenEdit::tap_notes,	"Tap Steps",		false, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( ScreenEdit::jumps,		"Jumps",		false, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( ScreenEdit::hands,		"Hands",		false, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( ScreenEdit::quads,		"Quads",		false, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( ScreenEdit::holds,		"Holds",		false, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( ScreenEdit::mines,		"Mines",		false, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( ScreenEdit::stream,		"Stream",		false, EditMode_Full, true, 0, NULL ),
	MenuRowDef( ScreenEdit::voltage,	"Voltage",		false, EditMode_Full, true, 0, NULL ),
	MenuRowDef( ScreenEdit::air,		"Air",			false, EditMode_Full, true, 0, NULL ),
	MenuRowDef( ScreenEdit::freeze,		"Freeze",		false, EditMode_Full, true, 0, NULL ),
	MenuRowDef( ScreenEdit::chaos,		"Chaos",		false, EditMode_Full, true, 0, NULL )
);

static MenuDef g_SongInformation(
	"ScreenMiniMenuSongInformation",
	MenuRowDef( ScreenEdit::main_title,			"Main title",			true, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( ScreenEdit::sub_title,			"Sub title",			true, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( ScreenEdit::artist,				"Artist",			true, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( ScreenEdit::credit,				"Credit",			true, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( ScreenEdit::main_title_transliteration,	"Main title transliteration",	true, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( ScreenEdit::sub_title_transliteration,	"Sub title transliteration",	true, EditMode_Practice, true, 0, NULL ),
	MenuRowDef( ScreenEdit::artist_transliteration,		"Artist transliteration",	true, EditMode_Practice, true, 0, NULL )
);

enum { song_bganimation, song_movie, song_bitmap, global_bganimation, global_movie, global_movie_song_group, global_movie_song_group_and_genre, dynamic_random, baked_random, none };
static bool EnabledIfSet1SongBGAnimation();
static bool EnabledIfSet1SongMovie();
static bool EnabledIfSet1SongBitmap();
static bool EnabledIfSet1GlobalBGAnimation();
static bool EnabledIfSet1GlobalMovie();
static bool EnabledIfSet1GlobalMovieSongGroup();
static bool EnabledIfSet1GlobalMovieSongGroupAndGenre();
static bool EnabledIfSet2SongBGAnimation();
static bool EnabledIfSet2SongMovie();
static bool EnabledIfSet2SongBitmap();
static bool EnabledIfSet2GlobalBGAnimation();
static bool EnabledIfSet2GlobalMovie();
static bool EnabledIfSet2GlobalMovieSongGroup();
static bool EnabledIfSet2GlobalMovieSongGroupAndGenre();
static MenuDef g_BackgroundChange(
	"ScreenMiniMenuBackgroundChange",
	MenuRowDef( ScreenEdit::layer,						"Layer",				false,						EditMode_Full, true, 0, "" ),
	MenuRowDef( ScreenEdit::rate,						"Rate",					true,						EditMode_Full, false, 10, "0%","10%","20%","30%","40%","50%","60%","70%","80%","90%","100%","120%","140%","160%","180%","200%" ),
	MenuRowDef( ScreenEdit::transition,					"Force Transition",			true,						EditMode_Full, true, 0, NULL ),
	MenuRowDef( ScreenEdit::effect,						"Force Effect",				true,						EditMode_Full, true, 0, NULL ),
	MenuRowDef( ScreenEdit::color1,						"Force Color 1",			true,						EditMode_Full, false, 0, "","1,1,1,1","0.5,0.5,0.5,1","1,1,1,0.5","0,0,0,1","1,0,0,1","0,1,0,1","0,0,1,1","1,1,0,1","0,1,1,1","1,0,1,1" ),
	MenuRowDef( ScreenEdit::color2,						"Force Color 2",			true,						EditMode_Full, false, 0, "","1,1,1,1","0.5,0.5,0.5,1","1,1,1,0.5","0,0,0,1","1,0,0,1","0,1,0,1","0,0,1,1","1,1,0,1","0,1,1,1","1,0,1,1" ),
	MenuRowDef( ScreenEdit::file1_type,					"File1 Type",				true,						EditMode_Full, true, 0, "Song BGAnimation", "Song Movie", "Song Bitmap", "Global BGAnimation", "Global Movie", "Global Movie from Song Group", "Global Movie from Song Group and Genre", "Dynamic Random", "Baked Random", "None" ),
	MenuRowDef( ScreenEdit::file1_song_bganimation,				"File1 Song BGAnimation",		EnabledIfSet1SongBGAnimation,			EditMode_Full, true, 0, NULL ),
	MenuRowDef( ScreenEdit::file1_song_movie,				"File1 Song Movie",			EnabledIfSet1SongMovie,				EditMode_Full, true, 0, NULL ),
	MenuRowDef( ScreenEdit::file1_song_still,				"File1 Song Still",			EnabledIfSet1SongBitmap,			EditMode_Full, true, 0, NULL ),
	MenuRowDef( ScreenEdit::file1_global_bganimation,			"File1 Global BGAnimation",		EnabledIfSet1GlobalBGAnimation,			EditMode_Full, true, 0, NULL ),
	MenuRowDef( ScreenEdit::file1_global_movie,				"File1 Global Movie",			EnabledIfSet1GlobalMovie,			EditMode_Full, true, 0, NULL ),
	MenuRowDef( ScreenEdit::file1_global_movie_song_group,			"File1 Global Movie (Group)",		EnabledIfSet1GlobalMovieSongGroup,		EditMode_Full, true, 0, NULL ),
	MenuRowDef( ScreenEdit::file1_global_movie_song_group_and_genre,	"File1 Global Movie (Group + Genre)",	EnabledIfSet1GlobalMovieSongGroupAndGenre,	EditMode_Full, true, 0, NULL ),
	MenuRowDef( ScreenEdit::file2_type,					"File2 Type",				true,						EditMode_Full, true, 0, "Song BGAnimation", "Song Movie", "Song Bitmap", "Global BGAnimation", "Global Movie", "Global Movie from Song Group", "Global Movie from Song Group and Genre", "Dynamic Random", "Baked Random", "None" ),
	MenuRowDef( ScreenEdit::file2_song_bganimation,				"File2 Song BGAnimation",		EnabledIfSet2SongBGAnimation,			EditMode_Full, true, 0, NULL ),
	MenuRowDef( ScreenEdit::file2_song_movie,				"File2 Song Movie",			EnabledIfSet2SongMovie,				EditMode_Full, true, 0, NULL ),
	MenuRowDef( ScreenEdit::file2_song_still,				"File2 Song Still",			EnabledIfSet2SongBitmap,			EditMode_Full, true, 0, NULL ),
	MenuRowDef( ScreenEdit::file2_global_bganimation,			"File2 Global BGAnimation",		EnabledIfSet2GlobalBGAnimation,			EditMode_Full, true, 0, NULL ),
	MenuRowDef( ScreenEdit::file2_global_movie,				"File2 Global Movie",			EnabledIfSet2GlobalMovie,			EditMode_Full, true, 0, NULL ),
	MenuRowDef( ScreenEdit::file2_global_movie_song_group,			"File2 Global Movie (Group)",		EnabledIfSet2GlobalMovieSongGroup,		EditMode_Full, true, 0, NULL ),
	MenuRowDef( ScreenEdit::file2_global_movie_song_group_and_genre,	"File2 Global Movie (Group + Genre)",	EnabledIfSet2GlobalMovieSongGroupAndGenre,	EditMode_Full, true, 0, NULL ),
	MenuRowDef( ScreenEdit::delete_change,					"Remove Change",			true,						EditMode_Full, true, 0, NULL )
);
static bool EnabledIfSet1SongBGAnimation()		{ return ScreenMiniMenu::s_viLastAnswers[ScreenEdit::file1_type] == song_bganimation			&& !g_BackgroundChange.rows[ScreenEdit::file1_song_bganimation].choices.empty(); }
static bool EnabledIfSet1SongMovie()			{ return ScreenMiniMenu::s_viLastAnswers[ScreenEdit::file1_type] == song_movie				&& !g_BackgroundChange.rows[ScreenEdit::file1_song_movie].choices.empty(); }
static bool EnabledIfSet1SongBitmap()			{ return ScreenMiniMenu::s_viLastAnswers[ScreenEdit::file1_type] == song_bitmap				&& !g_BackgroundChange.rows[ScreenEdit::file1_song_still].choices.empty(); }
static bool EnabledIfSet1GlobalBGAnimation()		{ return ScreenMiniMenu::s_viLastAnswers[ScreenEdit::file1_type] == global_bganimation			&& !g_BackgroundChange.rows[ScreenEdit::file1_global_bganimation].choices.empty(); }
static bool EnabledIfSet1GlobalMovie()			{ return ScreenMiniMenu::s_viLastAnswers[ScreenEdit::file1_type] == global_movie			&& !g_BackgroundChange.rows[ScreenEdit::file1_global_movie].choices.empty(); }
static bool EnabledIfSet1GlobalMovieSongGroup()		{ return ScreenMiniMenu::s_viLastAnswers[ScreenEdit::file1_type] == global_movie_song_group		&& !g_BackgroundChange.rows[ScreenEdit::file1_global_movie_song_group].choices.empty(); }
static bool EnabledIfSet1GlobalMovieSongGroupAndGenre() { return ScreenMiniMenu::s_viLastAnswers[ScreenEdit::file1_type] == global_movie_song_group_and_genre	&& !g_BackgroundChange.rows[ScreenEdit::file1_global_movie_song_group_and_genre].choices.empty(); }
static bool EnabledIfSet2SongBGAnimation()		{ return ScreenMiniMenu::s_viLastAnswers[ScreenEdit::file2_type] == song_bganimation			&& !g_BackgroundChange.rows[ScreenEdit::file2_song_bganimation].choices.empty(); }
static bool EnabledIfSet2SongMovie()			{ return ScreenMiniMenu::s_viLastAnswers[ScreenEdit::file2_type] == song_movie				&& !g_BackgroundChange.rows[ScreenEdit::file2_song_movie].choices.empty(); }
static bool EnabledIfSet2SongBitmap()			{ return ScreenMiniMenu::s_viLastAnswers[ScreenEdit::file2_type] == song_bitmap				&& !g_BackgroundChange.rows[ScreenEdit::file2_song_still].choices.empty(); }
static bool EnabledIfSet2GlobalBGAnimation()		{ return ScreenMiniMenu::s_viLastAnswers[ScreenEdit::file2_type] == global_bganimation			&& !g_BackgroundChange.rows[ScreenEdit::file2_global_bganimation].choices.empty(); }
static bool EnabledIfSet2GlobalMovie()			{ return ScreenMiniMenu::s_viLastAnswers[ScreenEdit::file2_type] == global_movie			&& !g_BackgroundChange.rows[ScreenEdit::file2_global_movie].choices.empty(); }
static bool EnabledIfSet2GlobalMovieSongGroup()		{ return ScreenMiniMenu::s_viLastAnswers[ScreenEdit::file2_type] == global_movie_song_group		&& !g_BackgroundChange.rows[ScreenEdit::file2_global_movie_song_group].choices.empty(); }
static bool EnabledIfSet2GlobalMovieSongGroupAndGenre() { return ScreenMiniMenu::s_viLastAnswers[ScreenEdit::file2_type] == global_movie_song_group_and_genre	&& !g_BackgroundChange.rows[ScreenEdit::file2_global_movie_song_group_and_genre].choices.empty(); }

static RString GetOneBakedRandomFile( Song *pSong, bool bTryGenre = true )
{
	vector<RString> vsPathsOut; 
	vector<RString> vsNamesOut;
	BackgroundUtil::GetGlobalRandomMovies(
		pSong,
		"",
		vsPathsOut, 
		vsNamesOut,
		bTryGenre );
	if( !vsNamesOut.empty() )
		return vsNamesOut[rand()%vsNamesOut.size()];
	else
		return RString();
}

static MenuDef g_InsertTapAttack(
	"ScreenMiniMenuInsertTapAttack",
	MenuRowDef( -1, "Duration seconds",	true, EditMode_Practice, false, 3, "5","10","15","20","25","30","35","40","45" ),
	MenuRowDef( -1, "Set modifiers",	true, EditMode_Practice, true, 0, "Press Start" )
);

static MenuDef g_InsertCourseAttack(
	"ScreenMiniMenuInsertCourseAttack",
	MenuRowDef( ScreenEdit::duration,	"Duration seconds",	true, EditMode_Practice, false, 3, "5","10","15","20","25","30","35","40","45" ),
	MenuRowDef( ScreenEdit::set_mods,	"Set modifiers",	true, EditMode_Practice, true, 0, "Press Start" ),
	MenuRowDef( ScreenEdit::remove,		"Remove",		true, EditMode_Practice, true, 0, "Press Start" )
);

static MenuDef g_CourseMode(
	"ScreenMiniMenuCourseDisplay",
	MenuRowDef( -1, "Play mods from course",	true, EditMode_Practice, true, 0, NULL )
);

// HACK: need to remember the track we're inserting on so
// that we can lay the attack note after coming back from
// menus.
static int g_iLastInsertTapAttackTrack = -1;
static float g_fLastInsertAttackDurationSeconds = -1;
static BackgroundLayer g_CurrentBGChangeLayer = BACKGROUND_LAYER_INVALID;

REGISTER_SCREEN_CLASS( ScreenEdit );

void ScreenEdit::Init()
{
	ASSERT( GAMESTATE->m_pCurSong );
	ASSERT( GAMESTATE->m_pCurSteps[0] );

	EDIT_MODE.Load(m_sName,"EditMode");
	ScreenWithMenuElements::Init();

	InitEditMappings();

	// save the originals for reverting later
	CopyToLastSave();

	m_CurrentAction = MAIN_MENU_CHOICE_INVALID;

	// set both players to joined so the credit message doesn't show
	FOREACH_PlayerNumber( p )
		GAMESTATE->m_bSideIsJoined[p] = true;
	SCREENMAN->RefreshCreditsMessages();

	m_pSong = GAMESTATE->m_pCurSong;
	m_pSteps = GAMESTATE->m_pCurSteps[PLAYER_1];
	m_bReturnToRecordMenuAfterPlay = false;
	m_fBeatToReturnTo = 0;


	GAMESTATE->m_bGameplayLeadIn.Set( true );
	GAMESTATE->m_EditMode = EDIT_MODE.GetValue();
	GAMESTATE->m_fSongBeat = 0;
	m_fTrailingBeat = GAMESTATE->m_fSongBeat;

	GAMESTATE->StoreSelectedOptions();

	m_iShiftAnchor = -1;
	m_iStartPlayingAt = -1;
	m_iStopPlayingAt = -1;

	this->AddChild( &m_Background );

	m_SnapDisplay.SetXY( EDIT_X, PLAYER_Y_STANDARD );
	m_SnapDisplay.Load( PLAYER_1 );
	m_SnapDisplay.SetZoom( 0.5f );
	this->AddChild( &m_SnapDisplay );

	if( NOTESKIN->DoesNoteSkinExist("note") )
		m_PlayerStateEdit.m_PlayerOptions.m_sNoteSkin = "note";	// change noteskin before loading all of the edit Actors
	m_PlayerStateEdit.m_PlayerNumber = PLAYER_1;
	m_PlayerStateEdit.m_PlayerOptions.m_sNoteSkin = GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerOptions.m_sNoteSkin;

	m_pSteps->GetNoteData( m_NoteDataEdit );
	m_NoteFieldEdit.SetXY( EDIT_X, EDIT_Y );
	m_NoteFieldEdit.SetZoom( 0.5f );
	m_NoteFieldEdit.Init( &m_PlayerStateEdit, PLAYER_HEIGHT*2 );
	m_NoteFieldEdit.Load( &m_NoteDataEdit, -240, 850 );
	this->AddChild( &m_NoteFieldEdit );

	m_NoteDataRecord.SetNumTracks( m_NoteDataEdit.GetNumTracks() );
	m_NoteFieldRecord.SetXY( RECORD_X, RECORD_Y );
	m_NoteFieldRecord.Init( GAMESTATE->m_pPlayerState[PLAYER_1], PLAYER_HEIGHT );
	m_NoteFieldRecord.Load( &m_NoteDataRecord, -(int)SCREEN_HEIGHT/2, (int)SCREEN_HEIGHT/2 );
	this->AddChild( &m_NoteFieldRecord );

	m_EditState = STATE_INVALID;
	TransitionEditState( STATE_EDITING );
	
	m_bRemoveNoteButtonDown = false;

	m_Clipboard.SetNumTracks( m_NoteDataEdit.GetNumTracks() );

	m_bHasUndo = false;
	m_Undo.SetNumTracks( m_NoteDataEdit.GetNumTracks() );


	m_Player.Init( "Player", GAMESTATE->m_pPlayerState[PLAYER_1], NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL );
	GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerController = PC_HUMAN;
	m_Player.SetXY( PLAYER_X, PLAYER_Y );
	this->AddChild( &m_Player );

	this->AddChild( &m_Foreground );

	m_textInputTips.SetName( "InputTips" );
	m_textInputTips.LoadFromFont( THEME->GetPathF("ScreenEdit","InputTips") );
	m_textInputTips.SetText( INPUT_TIPS_TEXT );
	SET_XY_AND_ON_COMMAND( m_textInputTips );
	this->AddChild( &m_textInputTips );

	m_textInfo.SetName( "Info" );
	m_textInfo.LoadFromFont( THEME->GetPathF("ScreenEdit","Info") );
	SET_XY_AND_ON_COMMAND( m_textInfo );
	this->AddChild( &m_textInfo );

	m_textPlayRecordHelp.SetName( "PlayRecordHelp" );
	m_textPlayRecordHelp.LoadFromFont( THEME->GetPathF("ScreenEdit","PlayRecordHelp") );
	m_textPlayRecordHelp.SetText( PLAY_RECORD_HELP_TEXT );
	SET_XY_AND_ON_COMMAND( m_textPlayRecordHelp );
	this->AddChild( &m_textPlayRecordHelp );

	this->SortByDrawOrder();


	m_soundAddNote.Load(	THEME->GetPathS("ScreenEdit","AddNote"), true );
	m_soundRemoveNote.Load(	THEME->GetPathS("ScreenEdit","RemoveNote"), true );
	m_soundChangeLine.Load( THEME->GetPathS("ScreenEdit","line"), true );
	m_soundChangeSnap.Load( THEME->GetPathS("ScreenEdit","snap"), true );
	m_soundMarker.Load(	THEME->GetPathS("ScreenEdit","marker"), true );
	m_soundSwitch.Load(	THEME->GetPathS("ScreenEdit","switch") );
	m_soundSave.Load(	THEME->GetPathS("ScreenEdit","save") );

	m_soundMusic.Load( m_pSong->GetMusicPath() );

	m_soundAssistTick.Load( THEME->GetPathS("ScreenEdit","assist tick"), true );

	this->HandleScreenMessage( SM_UpdateTextInfo );
	m_bTextInfoNeedsUpdate = true;
}

ScreenEdit::~ScreenEdit()
{
	// UGLY: Don't delete the Song's steps.
	m_songLastSave.DetachSteps();

	LOG->Trace( "ScreenEdit::~ScreenEdit()" );
	m_soundMusic.StopPlaying();
}

// play assist ticks
void ScreenEdit::PlayTicks()
{
	if( !GAMESTATE->m_SongOptions.m_bAssistTick || m_EditState != STATE_PLAYING )
		return;
			
	/* Sound cards have a latency between when a sample is Play()ed and when the sound
	 * will start coming out the speaker.  Compensate for this by boosting fPositionSeconds
	 * ahead.  This is just to make sure that we request the sound early enough for it to
	 * come out on time; the actual precise timing is handled by SetStartTime. */
	float fPositionSeconds = GAMESTATE->m_fMusicSeconds;
	fPositionSeconds += SOUND->GetPlayLatency() + (float)CommonMetrics::TICK_EARLY_SECONDS + 0.250f;
	const float fSongBeat = GAMESTATE->m_pCurSong->GetBeatFromElapsedTime( fPositionSeconds );

	const int iSongRow = max( 0, BeatToNoteRowNotRounded( fSongBeat ) );
	static int iRowLastCrossed = -1;
	if( iSongRow < iRowLastCrossed )
		iRowLastCrossed = iSongRow;

	int iTickRow = -1;
	// for each index we crossed since the last update:
	FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE( m_Player.m_NoteData, r, iRowLastCrossed+1, iSongRow+1 )
		if( m_Player.m_NoteData.IsThereATapOrHoldHeadAtRow( r ) )
			iTickRow = r;

	iRowLastCrossed = iSongRow;

	if( iTickRow != -1 )
	{
		const float fTickBeat = NoteRowToBeat( iTickRow );
		const float fTickSecond = GAMESTATE->m_pCurSong->m_Timing.GetElapsedTimeFromBeat( fTickBeat );
		float fSecondsUntil = fTickSecond - GAMESTATE->m_fMusicSeconds;
		fSecondsUntil /= m_soundMusic.GetPlaybackRate(); /* 2x music rate means the time until the tick is halved */

		RageSoundParams p;
		p.m_StartTime = GAMESTATE->m_LastBeatUpdate + (fSecondsUntil - (float)CommonMetrics::TICK_EARLY_SECONDS);
		m_soundAssistTick.Play( &p );
	}
}

void ScreenEdit::PlayPreviewMusic()
{
	SOUND->PlayMusic( 
		m_pSong->GetMusicPath(), 
		NULL,
		false,
		m_pSong->m_fMusicSampleStartSeconds,
		m_pSong->m_fMusicSampleLengthSeconds,
		1.5f );
}

void ScreenEdit::MakeFilteredMenuDef( const MenuDef* pDef, MenuDef &menu )
{
	menu = *pDef;
	menu.rows.clear();

	vector<MenuRowDef> aRows;
	FOREACH_CONST( MenuRowDef, pDef->rows, r )
	{
		// Don't add rows that aren't applicable to this edit mode.
		if( EDIT_MODE >= r->emShowIn )
			menu.rows.push_back( *r );
	}
}

void ScreenEdit::EditMiniMenu( const MenuDef* pDef, ScreenMessage SM_SendOnOK, ScreenMessage SM_SendOnCancel )
{
	/* Reload options. */
	MenuDef menu("");
	MakeFilteredMenuDef( pDef, menu );
	ScreenMiniMenu::MiniMenu( &menu, SM_SendOnOK, SM_SendOnCancel );
}

void ScreenEdit::Update( float fDeltaTime )
{
	m_PlayerStateEdit.Update( fDeltaTime );

	if( m_soundMusic.IsPlaying() )
	{
		RageTimer tm;
		const float fSeconds = m_soundMusic.GetPositionSeconds( NULL, &tm );
		GAMESTATE->UpdateSongPosition( fSeconds, GAMESTATE->m_pCurSong->m_Timing, tm );
	}

	if( m_EditState == STATE_RECORDING  )	
	{
		for( int t=0; t<GAMESTATE->GetCurrentStyle()->m_iColsPerPlayer; t++ )	// for each track
		{
			StyleInput StyleI( PLAYER_1, t );
			float fSecsHeld = INPUTMAPPER->GetSecsHeld( StyleI );
			fSecsHeld = min( fSecsHeld, m_RemoveNoteButtonLastChanged.Ago() );
			if( fSecsHeld == 0 )
				continue;

			float fStartPlayingAtBeat = NoteRowToBeat(m_iStartPlayingAt);
			if( GAMESTATE->m_fSongBeat <= fStartPlayingAtBeat )
				continue;

			float fStartedHoldingSeconds = m_soundMusic.GetPositionSeconds() - fSecsHeld;
			float fStartBeat = max( fStartPlayingAtBeat, m_pSong->GetBeatFromElapsedTime(fStartedHoldingSeconds) );
			float fEndBeat = max( fStartBeat, GAMESTATE->m_fSongBeat );
			fEndBeat = min( fEndBeat, NoteRowToBeat(m_iStopPlayingAt) );

			// Round start and end to the nearest snap interval
			fStartBeat = Quantize( fStartBeat, NoteTypeToBeat(m_SnapDisplay.GetNoteType()) );
			fEndBeat = Quantize( fEndBeat, NoteTypeToBeat(m_SnapDisplay.GetNoteType()) );

			if( m_bRemoveNoteButtonDown )
			{
				m_NoteDataRecord.ClearRangeForTrack( BeatToNoteRow(fStartBeat), BeatToNoteRow(fEndBeat), t );
			}
			else if( fSecsHeld > RECORD_HOLD_SECONDS )
			{
				// create or extend a hold or roll note
				TapNote tn = TAP_ORIGINAL_HOLD_HEAD;
				if( EditIsBeingPressed(EDIT_BUTTON_LAY_MINE_OR_ROLL) )
					tn = TAP_ORIGINAL_ROLL_HEAD;
				m_NoteDataRecord.AddHoldNote( t, BeatToNoteRow(fStartBeat), BeatToNoteRow(fEndBeat), tn );
			}
		}
	}

	//
	// check for end of playback/record
	//
	if( m_EditState == STATE_RECORDING  ||  m_EditState == STATE_PLAYING )
	{
		/*
		 * If any arrow is being held, continue for up to half a second after
		 * the end marker.  This makes it possible to start a hold note near
		 * the end of the range.  We won't allow placing anything outside of the
		 * range.
		 */
		bool bButtonIsBeingPressed = false;
		for( int t=0; t<GAMESTATE->GetCurrentStyle()->m_iColsPerPlayer; t++ )	// for each track
		{
			StyleInput StyleI( PLAYER_1, t );
			if( INPUTMAPPER->IsButtonDown(StyleI) )
				bButtonIsBeingPressed = true;
		}

		float fLastBeat = NoteRowToBeat(m_iStopPlayingAt);
		if( bButtonIsBeingPressed && m_EditState == STATE_RECORDING )
		{
			float fSeconds = m_pSong->m_Timing.GetElapsedTimeFromBeat( fLastBeat );
			fLastBeat = m_pSong->m_Timing.GetBeatFromElapsedTime( fSeconds + 0.5f );
		}

		if( GAMESTATE->m_fSongBeat > fLastBeat )
			TransitionEditState( STATE_EDITING );
	}


	//LOG->Trace( "ScreenEdit::Update(%f)", fDeltaTime );
	ScreenWithMenuElements::Update( fDeltaTime );


	// Update trailing beat
	float fDelta = GAMESTATE->m_fSongBeat - m_fTrailingBeat;
	if( fabsf(fDelta) < 10 )
		fapproach( m_fTrailingBeat, GAMESTATE->m_fSongBeat,
			fDeltaTime*40 / m_NoteFieldEdit.GetPlayerState()->m_CurrentPlayerOptions.m_fScrollSpeed );
	else
		fapproach( m_fTrailingBeat, GAMESTATE->m_fSongBeat,
			fabsf(fDelta) * fDeltaTime*5 );

	PlayTicks();
}

void ScreenEdit::UpdateTextInfo()
{
	if( m_pSteps == NULL )
		return;

	// Don't update the text during playback or record.  It causes skips.
	if( m_EditState != STATE_EDITING )
		return;

	if( !m_bTextInfoNeedsUpdate )
		return;

	m_bTextInfoNeedsUpdate = false;

	RString sNoteType = NoteTypeToString(m_SnapDisplay.GetNoteType()) + " notes";

	RString sText;
	sText += ssprintf( "Current beat:\n  %.3f\n",		GAMESTATE->m_fSongBeat );
	sText += ssprintf( "Current sec:\n  %.3f\n",		m_pSong->GetElapsedTimeFromBeat(GAMESTATE->m_fSongBeat) );
	switch( EDIT_MODE.GetValue() )
	{
	case EditMode_Practice:
		break;
	case EditMode_Home:
	case EditMode_Full:
		sText += ssprintf( "Snap to:\n  %s\n",				sNoteType.c_str() );
		break;
	default:
		ASSERT(0);
	}
	sText += ssprintf( "Selection beat:\n  %s\n  %s\n",	m_NoteFieldEdit.m_iBeginMarker==-1 ? "----" : ssprintf("%.3f",NoteRowToBeat(m_NoteFieldEdit.m_iBeginMarker)).c_str(), m_NoteFieldEdit.m_iEndMarker==-1 ? "----" : ssprintf("%.3f",NoteRowToBeat(m_NoteFieldEdit.m_iEndMarker)).c_str() );
	switch( EDIT_MODE.GetValue() )
	{
	case EditMode_Practice:
	case EditMode_Home:
		break;
	case EditMode_Full:
		sText += ssprintf( "Difficulty:\n  %s\n",		DifficultyToString( m_pSteps->GetDifficulty() ).c_str() );
		sText += ssprintf( "Description:\n  %s\n",		m_pSteps->GetDescription().c_str() );
		sText += ssprintf( "Main title:\n  %s\n", m_pSong->m_sMainTitle.c_str() );
		sText += ssprintf( "Sub title:\n  %s\n", m_pSong->m_sSubTitle.c_str() );
		break;
	default:
		ASSERT(0);
	}
	sText += ssprintf( "Tap steps:\n  %d\n", m_NoteDataEdit.GetNumTapNotes() );
	sText += ssprintf( "Jumps:\n  %d\n", m_NoteDataEdit.GetNumJumps() );
	sText += ssprintf( "Hands:\n  %d\n", m_NoteDataEdit.GetNumHands() );
	sText += ssprintf( "Holds:\n  %d\n", m_NoteDataEdit.GetNumHoldNotes() );
	sText += ssprintf( "Mines:\n  %d\n", m_NoteDataEdit.GetNumMines() );
	switch( EDIT_MODE.GetValue() )
	{
	case EditMode_Practice:
	case EditMode_Home:
		break;
	case EditMode_Full:
		sText += ssprintf( "Beat 0 Offset:\n  %.3f secs\n",	m_pSong->m_Timing.m_fBeat0OffsetInSeconds );
		sText += ssprintf( "Preview Start:\n  %.3f secs\n",	m_pSong->m_fMusicSampleStartSeconds );
		sText += ssprintf( "Preview Length:\n  %.3f secs\n",m_pSong->m_fMusicSampleLengthSeconds );
		break;
	default:
		ASSERT(0);
	}

	m_textInfo.SetText( sText );
}

void ScreenEdit::DrawPrimitives()
{
	// HACK:  Draw using the trailing beat
	float fSongBeat = GAMESTATE->m_fSongBeat;	// save song beat
	GAMESTATE->m_fSongBeat = m_fTrailingBeat;	// put trailing beat in effect

	ScreenWithMenuElements::DrawPrimitives();

	GAMESTATE->m_fSongBeat = fSongBeat;	// restore real song beat
}

void ScreenEdit::Input( const InputEventPlus &input )
{
//	LOG->Trace( "ScreenEdit::Input()" );

	if( m_In.IsTransitioning() || m_Out.IsTransitioning() )
		return;

	EditButton EditB;
	DeviceToEdit( input.DeviceI, EditB );

	if( EditB == EDIT_BUTTON_REMOVE_NOTE )
	{
		// Ugly: we need to know when the button was pressed or released, so we
		// can clamp operations to that time.  Should InputFilter keep track of
		// last release, too?
		m_bRemoveNoteButtonDown = (input.type != IET_RELEASE);
		m_RemoveNoteButtonLastChanged.Touch();
	}

	switch( m_EditState )
	{
	case STATE_EDITING:
		InputEdit( input, EditB );
		m_bTextInfoNeedsUpdate = true;
		break;
	case STATE_RECORDING:
		InputRecord( input, EditB );
		break;
	case STATE_RECORDING_PAUSED:
		InputRecordPaused( input, EditB );
		break;
	case STATE_PLAYING:
		InputPlay( input, EditB );
		break;
	default:	ASSERT(0);
	}
}

static void ShiftToRightSide( int &iCol, int iNumTracks )
{
	switch( GAMESTATE->GetCurrentStyle()->m_StyleType )
	{
	case ONE_PLAYER_ONE_SIDE:
		break;
	case TWO_PLAYERS_TWO_SIDES:
	case ONE_PLAYER_TWO_SIDES:
		iCol += iNumTracks/2;
		break;
	default:
		ASSERT(0);
	}
}

static LocalizedString SWITCHED_TO				( "ScreenEdit", "Switched to" );
static LocalizedString NO_BACKGROUNDS_AVAILABLE	( "ScreenEdit", "No backgrounds available" );
void ScreenEdit::InputEdit( const InputEventPlus &input, EditButton EditB )
{
	if( input.type == IET_LEVEL_CHANGED )
		return;		// don't care

	if( input.type == IET_RELEASE )
	{
		if( EditPressed( EDIT_BUTTON_SCROLL_SELECT, input.DeviceI ) )
			m_iShiftAnchor = -1;
		return;
	}

	const int iSongBeat = BeatToNoteRow(GAMESTATE->m_fSongBeat);
	switch( EditB )
	{
	case EDIT_BUTTON_COLUMN_0:
	case EDIT_BUTTON_COLUMN_1:
	case EDIT_BUTTON_COLUMN_2:
	case EDIT_BUTTON_COLUMN_3:
	case EDIT_BUTTON_COLUMN_4:
	case EDIT_BUTTON_COLUMN_5:
	case EDIT_BUTTON_COLUMN_6:
	case EDIT_BUTTON_COLUMN_7:
	case EDIT_BUTTON_COLUMN_8:
	case EDIT_BUTTON_COLUMN_9:
		{
			if( input.type != IET_FIRST_PRESS )
				break;	// We only care about first presses

			int iCol = EditB - EDIT_BUTTON_COLUMN_0;


			// Alt + number = input to right half
			if( EditIsBeingPressed(EDIT_BUTTON_RIGHT_SIDE) )
				ShiftToRightSide( iCol, m_NoteDataEdit.GetNumTracks() );


			const float fSongBeat = GAMESTATE->m_fSongBeat;
			const int iSongIndex = BeatToNoteRow( fSongBeat );

			if( iCol >= m_NoteDataEdit.GetNumTracks() )	// this button is not in the range of columns for this Style
				break;

			// check for to see if the user intended to remove a HoldNote
			int iHeadRow;
			if( m_NoteDataEdit.IsHoldNoteAtRow( iCol, iSongIndex, &iHeadRow ) )
			{
				m_soundRemoveNote.Play();
				SaveUndo();
				m_NoteDataEdit.SetTapNote( iCol, iHeadRow, TAP_EMPTY );
				// Don't CheckNumberOfNotesAndUndo.  We don't want to revert any change that removes notes.
			}
			else if( m_NoteDataEdit.GetTapNote(iCol, iSongIndex).type != TapNote::empty )
			{
				m_soundRemoveNote.Play();
				SaveUndo();
				m_NoteDataEdit.SetTapNote( iCol, iSongIndex, TAP_EMPTY );
				// Don't CheckNumberOfNotesAndUndo.  We don't want to revert any change that removes notes.
			}
			else if( EditIsBeingPressed(EDIT_BUTTON_LAY_MINE_OR_ROLL) )
			{
				m_soundAddNote.Play();
				SaveUndo();
				m_NoteDataEdit.SetTapNote(iCol, iSongIndex, TAP_ORIGINAL_MINE );
				CheckNumberOfNotesAndUndo();
			}
			else if( EditIsBeingPressed(EDIT_BUTTON_LAY_TAP_ATTACK) )
			{
				g_iLastInsertTapAttackTrack = iCol;
				EditMiniMenu( &g_InsertTapAttack, SM_BackFromInsertTapAttack );
			}
			else
			{
				m_soundAddNote.Play();
				SaveUndo();
				m_NoteDataEdit.SetTapNote(iCol, iSongIndex, TAP_ORIGINAL_TAP );
				CheckNumberOfNotesAndUndo();
			}
		}
		break;
	case EDIT_BUTTON_SCROLL_SPEED_UP:
	case EDIT_BUTTON_SCROLL_SPEED_DOWN:
		{
			PlayerState *pPlayerState = const_cast<PlayerState *> (m_NoteFieldEdit.GetPlayerState());
			float& fScrollSpeed = pPlayerState->m_PlayerOptions.m_fScrollSpeed;
			float fNewScrollSpeed = fScrollSpeed;

			if( EditB == EDIT_BUTTON_SCROLL_SPEED_DOWN )
			{
				if( fScrollSpeed == 4 )
					fNewScrollSpeed = 2;
				else if( fScrollSpeed == 2 )
					fNewScrollSpeed = 1;
			}
			else if( EditB == EDIT_BUTTON_SCROLL_SPEED_UP )
			{
				if( fScrollSpeed == 2 )
					fNewScrollSpeed = 4;
				else if( fScrollSpeed == 1 )
					fNewScrollSpeed = 2;
			}

			if( fNewScrollSpeed != fScrollSpeed )
			{
				fScrollSpeed = fNewScrollSpeed;
				m_soundMarker.Play();
			}
			break;
		}

		break;
	case EDIT_BUTTON_SCROLL_HOME:
		ScrollTo( 0 );
		break;
	case EDIT_BUTTON_SCROLL_END:
		ScrollTo( m_NoteDataEdit.GetLastBeat() );
		break;
	case EDIT_BUTTON_SCROLL_UP_LINE:
	case EDIT_BUTTON_SCROLL_UP_PAGE:
	case EDIT_BUTTON_SCROLL_DOWN_LINE:
	case EDIT_BUTTON_SCROLL_DOWN_PAGE:
		{
			float fBeatsToMove=0.f;
			switch( EditB )
			{
			case EDIT_BUTTON_SCROLL_UP_LINE:
			case EDIT_BUTTON_SCROLL_DOWN_LINE:
				fBeatsToMove = NoteTypeToBeat( m_SnapDisplay.GetNoteType() );
				if( EditB == EDIT_BUTTON_SCROLL_UP_LINE )	
					fBeatsToMove *= -1;
				break;
			case EDIT_BUTTON_SCROLL_UP_PAGE:
			case EDIT_BUTTON_SCROLL_DOWN_PAGE:
				fBeatsToMove = BEATS_PER_MEASURE;
				if( EditB == EDIT_BUTTON_SCROLL_UP_PAGE )	
					fBeatsToMove *= -1;
				break;
			}

			float fDestinationBeat = GAMESTATE->m_fSongBeat + fBeatsToMove;
			fDestinationBeat = Quantize( fDestinationBeat, NoteTypeToBeat(m_SnapDisplay.GetNoteType()) );

			ScrollTo( fDestinationBeat );
		}
		break;
	case EDIT_BUTTON_SCROLL_NEXT_MEASURE:
		{
			float fDestinationBeat = GAMESTATE->m_fSongBeat + BEATS_PER_MEASURE;
			fDestinationBeat = ftruncf( fDestinationBeat, (float)BEATS_PER_MEASURE );
			ScrollTo( fDestinationBeat );
			break;
		}
	case EDIT_BUTTON_SCROLL_PREV_MEASURE:
		{
			float fDestinationBeat = QuantizeUp( GAMESTATE->m_fSongBeat, (float)BEATS_PER_MEASURE );
			fDestinationBeat -= (float)BEATS_PER_MEASURE;
			ScrollTo( fDestinationBeat );
			break;
		}
	case EDIT_BUTTON_SCROLL_NEXT:
		{
			int iRow = BeatToNoteRow( GAMESTATE->m_fSongBeat );
			NoteDataUtil::GetNextEditorPosition( m_NoteDataEdit, iRow );
			ScrollTo( NoteRowToBeat(iRow) );
		}
		break;
	case EDIT_BUTTON_SCROLL_PREV:
		{
			int iRow = BeatToNoteRow( GAMESTATE->m_fSongBeat );
			NoteDataUtil::GetPrevEditorPosition( m_NoteDataEdit, iRow );
			ScrollTo( NoteRowToBeat(iRow) );
		}
		break;
	case EDIT_BUTTON_SNAP_NEXT:
		if( m_SnapDisplay.PrevSnapMode() )
			OnSnapModeChange();
		break;
	case EDIT_BUTTON_SNAP_PREV:
		if( m_SnapDisplay.NextSnapMode() )
			OnSnapModeChange();
		break;
	case EDIT_BUTTON_LAY_SELECT:
		if( m_NoteFieldEdit.m_iBeginMarker==-1 && m_NoteFieldEdit.m_iEndMarker==-1 )
		{
			// lay begin marker
			m_NoteFieldEdit.m_iBeginMarker = BeatToNoteRow(GAMESTATE->m_fSongBeat);
		}
		else if( m_NoteFieldEdit.m_iEndMarker==-1 )	// only begin marker is laid
		{
			if( iSongBeat == m_NoteFieldEdit.m_iBeginMarker )
			{
				m_NoteFieldEdit.m_iBeginMarker = -1;
			}
			else
			{
				m_NoteFieldEdit.m_iEndMarker = max( m_NoteFieldEdit.m_iBeginMarker, iSongBeat );
				m_NoteFieldEdit.m_iBeginMarker = min( m_NoteFieldEdit.m_iBeginMarker, iSongBeat );
			}
		}
		else	// both markers are laid
		{
			m_NoteFieldEdit.m_iBeginMarker = iSongBeat;
			m_NoteFieldEdit.m_iEndMarker = -1;
		}
		m_soundMarker.Play();
		break;
	case EDIT_BUTTON_OPEN_AREA_MENU:
		{
			// update enabled/disabled in g_AreaMenu
			bool bAreaSelected = m_NoteFieldEdit.m_iBeginMarker!=-1 && m_NoteFieldEdit.m_iEndMarker!=-1;
			g_AreaMenu.rows[cut].bEnabled = bAreaSelected;
			g_AreaMenu.rows[copy].bEnabled = bAreaSelected;
			g_AreaMenu.rows[paste_at_current_beat].bEnabled = !m_Clipboard.IsEmpty();
			g_AreaMenu.rows[paste_at_begin_marker].bEnabled = !m_Clipboard.IsEmpty() != 0 && m_NoteFieldEdit.m_iBeginMarker!=-1;
			g_AreaMenu.rows[clear].bEnabled = bAreaSelected;
			g_AreaMenu.rows[quantize].bEnabled = bAreaSelected;
			g_AreaMenu.rows[turn].bEnabled = bAreaSelected;
			g_AreaMenu.rows[transform].bEnabled = bAreaSelected;
			g_AreaMenu.rows[alter].bEnabled = bAreaSelected;
			g_AreaMenu.rows[tempo].bEnabled = bAreaSelected;
			g_AreaMenu.rows[play].bEnabled = bAreaSelected;
			g_AreaMenu.rows[record].bEnabled = bAreaSelected;
			g_AreaMenu.rows[convert_to_pause].bEnabled = bAreaSelected;
			g_AreaMenu.rows[undo].bEnabled = m_bHasUndo;
			EditMiniMenu( &g_AreaMenu, SM_BackFromAreaMenu );
		}
		break;
	case EDIT_BUTTON_OPEN_EDIT_MENU:
		EditMiniMenu( &g_MainMenu, SM_BackFromMainMenu );
		break;
	case EDIT_BUTTON_OPEN_INPUT_HELP:
		if( EDIT_MODE.GetValue() == EditMode_Practice )
			EditMiniMenu( &g_PracticeHelp );
		else
			EditMiniMenu( &g_EditHelp );
		break;
	case EDIT_BUTTON_TOGGLE_ASSIST_TICK:
		GAMESTATE->m_SongOptions.m_bAssistTick ^= 1;
		break;
	case EDIT_BUTTON_OPEN_NEXT_STEPS:
	case EDIT_BUTTON_OPEN_PREV_STEPS:
		{
			// don't keep undo when changing Steps
			ClearUndo();

			// save current steps
			Steps* pSteps = GAMESTATE->m_pCurSteps[PLAYER_1];
			ASSERT( pSteps );
			pSteps->SetNoteData( m_NoteDataEdit );

			// Get all Steps of this StepsType
			StepsType st = pSteps->m_StepsType;
			vector<Steps*> vSteps;
			GAMESTATE->m_pCurSong->GetSteps( vSteps, st );

			// Sort them by difficulty.
			StepsUtil::SortStepsByTypeAndDifficulty( vSteps );

			// Find out what index the current Steps are
			vector<Steps*>::iterator it = find( vSteps.begin(), vSteps.end(), pSteps );
			ASSERT( it != vSteps.end() );

			switch( EditB )
			{
			case EDIT_BUTTON_OPEN_PREV_STEPS:	
				if( it==vSteps.begin() )
				{
					SCREENMAN->PlayInvalidSound();
					return;
				}
				it--;
				break;
			case EDIT_BUTTON_OPEN_NEXT_STEPS:
				it++;
				if( it==vSteps.end() )
				{
					SCREENMAN->PlayInvalidSound();
					return;
				}
				break;
			default:	ASSERT(0);	return;
			}

			pSteps = *it;
			GAMESTATE->m_pCurSteps[PLAYER_1].Set( pSteps );
			m_pSteps = pSteps;
			pSteps->GetNoteData( m_NoteDataEdit );

			RString s = ssprintf(
				SWITCHED_TO.GetValue() + " %s %s '%s' (%d of %d)",
				GAMEMAN->StepsTypeToString( pSteps->m_StepsType ).c_str(),
				DifficultyToString( pSteps->GetDifficulty() ).c_str(),
				pSteps->GetDescription().c_str(),
				it - vSteps.begin() + 1,
				int(vSteps.size()) );
			SCREENMAN->SystemMessage( s );
			m_soundSwitch.Play();
		}
		break;
	case EDIT_BUTTON_BPM_UP:
	case EDIT_BUTTON_BPM_DOWN:
		{
			float fBPM = m_pSong->GetBPMAtBeat( GAMESTATE->m_fSongBeat );
			float fDeltaBPM;
			switch( EditB )
			{
			case EDIT_BUTTON_BPM_UP:		fDeltaBPM = +0.020f;		break;
			case EDIT_BUTTON_BPM_DOWN:		fDeltaBPM = -0.020f;		break;
			default:	ASSERT(0);						return;
			}
			if( EditIsBeingPressed( EDIT_BUTTON_ADJUST_FINE ) )
			{
				fDeltaBPM /= 20;	// .001 bpm
			}
			else 
			{
				switch( input.type )
				{
				case IET_SLOW_REPEAT:	fDeltaBPM *= 10;	break;
				case IET_FAST_REPEAT:	fDeltaBPM *= 40;	break;
				}
			}
			
			float fNewBPM = fBPM + fDeltaBPM;
			m_pSong->SetBPMAtBeat( GAMESTATE->m_fSongBeat, fNewBPM );
		}
		break;
	case EDIT_BUTTON_STOP_UP:
	case EDIT_BUTTON_STOP_DOWN:
		{
			float fStopDelta;
			switch( EditB )
			{
			case EDIT_BUTTON_STOP_UP:		fStopDelta = +0.020f;		break;
			case EDIT_BUTTON_STOP_DOWN:		fStopDelta = -0.020f;		break;
			default:	ASSERT(0);						return;
			}
			if( EditIsBeingPressed( EDIT_BUTTON_ADJUST_FINE ) )
			{
				fStopDelta /= 20; /* 1ms */
			}
			else 
			{
				switch( input.type )
				{
				case IET_SLOW_REPEAT:	fStopDelta *= 10;	break;
				case IET_FAST_REPEAT:	fStopDelta *= 40;	break;
				}
			}
			unsigned i;
			for( i=0; i<m_pSong->m_Timing.m_StopSegments.size(); i++ )
			{
				if( m_pSong->m_Timing.m_StopSegments[i].m_iStartRow == BeatToNoteRow(GAMESTATE->m_fSongBeat) )
					break;
			}

			if( i == m_pSong->m_Timing.m_StopSegments.size() )	// there is no BPMSegment at the current beat
			{
				// create a new StopSegment
				if( fStopDelta > 0 )
					m_pSong->AddStopSegment( StopSegment(BeatToNoteRow(GAMESTATE->m_fSongBeat), fStopDelta) );
			}
			else	// StopSegment being modified is m_Timing.m_StopSegments[i]
			{
				m_pSong->m_Timing.m_StopSegments[i].m_fStopSeconds += fStopDelta;
				if( m_pSong->m_Timing.m_StopSegments[i].m_fStopSeconds <= 0 )
					m_pSong->m_Timing.m_StopSegments.erase( m_pSong->m_Timing.m_StopSegments.begin()+i,
													  m_pSong->m_Timing.m_StopSegments.begin()+i+1);
			}
		}
		break;
	case EDIT_BUTTON_OFFSET_UP:
	case EDIT_BUTTON_OFFSET_DOWN:
		{
			float fOffsetDelta;
			switch( EditB )
			{
			case EDIT_BUTTON_OFFSET_DOWN:	fOffsetDelta = -0.02f;		break;
			case EDIT_BUTTON_OFFSET_UP:		fOffsetDelta = +0.02f;		break;
			default:	ASSERT(0);						return;
			}
			if( EditIsBeingPressed( EDIT_BUTTON_ADJUST_FINE ) )
			{
				fOffsetDelta /= 20; /* 1ms */
			}
			else 
			{
				switch( input.type )
				{
				case IET_SLOW_REPEAT:	fOffsetDelta *= 10;	break;
				case IET_FAST_REPEAT:	fOffsetDelta *= 40;	break;
				}
			}
			m_pSong->m_Timing.m_fBeat0OffsetInSeconds += fOffsetDelta;
		}
		break;
	case EDIT_BUTTON_SAMPLE_START_UP:
	case EDIT_BUTTON_SAMPLE_START_DOWN:
		{
			float fDelta;
			switch( EditB )
			{
			case EDIT_BUTTON_SAMPLE_START_DOWN:		fDelta = -0.02f;	break;
			case EDIT_BUTTON_SAMPLE_START_UP:			fDelta = +0.02f;	break;
			case EDIT_BUTTON_SAMPLE_LENGTH_DOWN:		fDelta = -0.02f;	break;
			case EDIT_BUTTON_SAMPLE_LENGTH_UP:			fDelta = +0.02f;	break;
			default:	ASSERT(0);						return;
			}
			switch( input.type )
			{
			case IET_SLOW_REPEAT:	fDelta *= 10;	break;
			case IET_FAST_REPEAT:	fDelta *= 40;	break;
			}

			if( EditB == EDIT_BUTTON_SAMPLE_LENGTH_DOWN || EditB == EDIT_BUTTON_SAMPLE_LENGTH_UP )
			{
				m_pSong->m_fMusicSampleLengthSeconds += fDelta;
				m_pSong->m_fMusicSampleLengthSeconds = max(m_pSong->m_fMusicSampleLengthSeconds,0);
			} else {
				m_pSong->m_fMusicSampleStartSeconds += fDelta;
				m_pSong->m_fMusicSampleStartSeconds = max(m_pSong->m_fMusicSampleStartSeconds,0);
			}
		}
		break;
	case EDIT_BUTTON_PLAY_SAMPLE_MUSIC:
		PlayPreviewMusic();
		break;
	case EDIT_BUTTON_OPEN_BGCHANGE_LAYER1_MENU:
	case EDIT_BUTTON_OPEN_BGCHANGE_LAYER2_MENU:
		switch( EditB )
		{
		case EDIT_BUTTON_OPEN_BGCHANGE_LAYER1_MENU: g_CurrentBGChangeLayer = BACKGROUND_LAYER_1; break;
		case EDIT_BUTTON_OPEN_BGCHANGE_LAYER2_MENU: g_CurrentBGChangeLayer = BACKGROUND_LAYER_2; break;
		}

		{
			//
			// Fill in option names
			//
			vector<RString> vThrowAway;

			MenuDef &menu = g_BackgroundChange;

			menu.rows[layer].choices[0] = ssprintf("%d",g_CurrentBGChangeLayer);
			BackgroundUtil::GetBackgroundTransitions(	"", vThrowAway, menu.rows[transition].choices );
			g_BackgroundChange.rows[transition].choices.insert( g_BackgroundChange.rows[transition].choices.begin(), "" );	// add "no transition"
			BackgroundUtil::GetBackgroundEffects(		"", vThrowAway, menu.rows[effect].choices );
			menu.rows[effect].choices.insert( menu.rows[effect].choices.begin(), "" );	// add "default effect"

			BackgroundUtil::GetSongBGAnimations(   m_pSong, "", vThrowAway, menu.rows[file1_song_bganimation].choices );
			BackgroundUtil::GetSongMovies(         m_pSong, "", vThrowAway, menu.rows[file1_song_movie].choices );
			BackgroundUtil::GetSongBitmaps(        m_pSong, "", vThrowAway, menu.rows[file1_song_still].choices );
			BackgroundUtil::GetGlobalBGAnimations( m_pSong, "", vThrowAway, menu.rows[file1_global_bganimation].choices );	// NULL to get all background files
			BackgroundUtil::GetGlobalRandomMovies( m_pSong, "", vThrowAway, menu.rows[file1_global_movie].choices, false, false );	// all backgrounds
			BackgroundUtil::GetGlobalRandomMovies( m_pSong, "", vThrowAway, menu.rows[file1_global_movie_song_group].choices, false, true );	// song group's backgrounds
			BackgroundUtil::GetGlobalRandomMovies( m_pSong, "", vThrowAway, menu.rows[file1_global_movie_song_group_and_genre].choices, true, true );	// song group and genre's backgrounds

			menu.rows[file2_type].choices					= menu.rows[file1_type].choices;
			menu.rows[file2_song_bganimation].choices			= menu.rows[file1_song_bganimation].choices;
			menu.rows[file2_song_movie].choices				= menu.rows[file1_song_movie].choices;
			menu.rows[file2_song_still].choices				= menu.rows[file1_song_still].choices;
			menu.rows[file2_global_bganimation].choices			= menu.rows[file1_global_bganimation].choices;
			menu.rows[file2_global_movie].choices				= menu.rows[file1_global_movie].choices;
			menu.rows[file2_global_movie_song_group].choices		= menu.rows[file1_global_movie_song_group].choices;
			menu.rows[file2_global_movie_song_group_and_genre].choices	= menu.rows[file1_global_movie_song_group_and_genre].choices;
	

			//
			// Fill in line's enabled/disabled
			//
			bool bAlreadyBGChangeHere = false;
			BackgroundChange bgChange; 
			FOREACH( BackgroundChange, m_pSong->GetBackgroundChanges(g_CurrentBGChangeLayer), bgc )
			{
				if( bgc->m_fStartBeat == GAMESTATE->m_fSongBeat )
				{
					bAlreadyBGChangeHere = true;
					bgChange = *bgc;
				}
			}

#define FILL_ENABLED( x )	menu.rows[x].bEnabled	= menu.rows[x].choices.size() > 0;
			FILL_ENABLED( transition );
			FILL_ENABLED( effect );
			FILL_ENABLED( file1_song_bganimation );
			FILL_ENABLED( file1_song_movie );
			FILL_ENABLED( file1_song_still );
			FILL_ENABLED( file1_global_bganimation );
			FILL_ENABLED( file1_global_movie );
			FILL_ENABLED( file1_global_movie_song_group );
			FILL_ENABLED( file1_global_movie_song_group_and_genre );
			FILL_ENABLED( file2_song_bganimation );
			FILL_ENABLED( file2_song_movie );
			FILL_ENABLED( file2_song_still );
			FILL_ENABLED( file2_global_bganimation );
			FILL_ENABLED( file2_global_movie );
			FILL_ENABLED( file2_global_movie_song_group );
			FILL_ENABLED( file2_global_movie_song_group_and_genre );
#undef FILL_ENABLED
			menu.rows[delete_change].bEnabled = bAlreadyBGChangeHere;


			// set default choices
			menu.rows[rate].					SetDefaultChoiceIfPresent( ssprintf("%2.0f%%",bgChange.m_fRate*100) );
			menu.rows[transition].					SetDefaultChoiceIfPresent( bgChange.m_sTransition );
			menu.rows[effect].					SetDefaultChoiceIfPresent( bgChange.m_def.m_sEffect );
			menu.rows[file1_type].iDefaultChoice			= none;
			if( bgChange.m_def.m_sFile1 == RANDOM_BACKGROUND_FILE )								menu.rows[file1_type].iDefaultChoice	= dynamic_random;
			if( menu.rows[file1_song_bganimation].			SetDefaultChoiceIfPresent( bgChange.m_def.m_sFile1 ) )	menu.rows[file1_type].iDefaultChoice	= song_bganimation;
			if( menu.rows[file1_song_movie].			SetDefaultChoiceIfPresent( bgChange.m_def.m_sFile1 ) )	menu.rows[file1_type].iDefaultChoice	= song_movie;
			if( menu.rows[file1_song_still].			SetDefaultChoiceIfPresent( bgChange.m_def.m_sFile1 ) )	menu.rows[file1_type].iDefaultChoice	= song_bitmap;
			if( menu.rows[file1_global_bganimation].		SetDefaultChoiceIfPresent( bgChange.m_def.m_sFile1 ) )	menu.rows[file1_type].iDefaultChoice	= global_bganimation;
			if( menu.rows[file1_global_movie].			SetDefaultChoiceIfPresent( bgChange.m_def.m_sFile1 ) )	menu.rows[file1_type].iDefaultChoice	= global_movie;
			if( menu.rows[file1_global_movie_song_group].		SetDefaultChoiceIfPresent( bgChange.m_def.m_sFile1 ) )	menu.rows[file1_type].iDefaultChoice	= global_movie_song_group;
			if( menu.rows[file1_global_movie_song_group_and_genre].	SetDefaultChoiceIfPresent( bgChange.m_def.m_sFile1 ) )	menu.rows[file1_type].iDefaultChoice	= global_movie_song_group_and_genre;
			menu.rows[file2_type].iDefaultChoice			= none;
			if( bgChange.m_def.m_sFile2 == RANDOM_BACKGROUND_FILE )								menu.rows[file2_type].iDefaultChoice	= dynamic_random;
			if( menu.rows[file2_song_bganimation].			SetDefaultChoiceIfPresent( bgChange.m_def.m_sFile2 ) )	menu.rows[file2_type].iDefaultChoice	= song_bganimation;
			if( menu.rows[file2_song_movie].			SetDefaultChoiceIfPresent( bgChange.m_def.m_sFile2 ) )	menu.rows[file2_type].iDefaultChoice	= song_movie;
			if( menu.rows[file2_song_still].			SetDefaultChoiceIfPresent( bgChange.m_def.m_sFile2 ) )	menu.rows[file2_type].iDefaultChoice	= song_bitmap;
			if( menu.rows[file2_global_bganimation].		SetDefaultChoiceIfPresent( bgChange.m_def.m_sFile2 ) )	menu.rows[file2_type].iDefaultChoice	= global_bganimation;
			if( menu.rows[file2_global_movie].			SetDefaultChoiceIfPresent( bgChange.m_def.m_sFile2 ) )	menu.rows[file2_type].iDefaultChoice	= global_movie;
			if( menu.rows[file2_global_movie_song_group].		SetDefaultChoiceIfPresent( bgChange.m_def.m_sFile2 ) )	menu.rows[file2_type].iDefaultChoice	= global_movie_song_group;
			if( menu.rows[file2_global_movie_song_group_and_genre].	SetDefaultChoiceIfPresent( bgChange.m_def.m_sFile2 ) )	menu.rows[file2_type].iDefaultChoice	= global_movie_song_group_and_genre;
			menu.rows[color1].					SetDefaultChoiceIfPresent( bgChange.m_def.m_sColor1 );
			menu.rows[color2].					SetDefaultChoiceIfPresent( bgChange.m_def.m_sColor2 );

			EditMiniMenu( &g_BackgroundChange, SM_BackFromBGChange );
		}
		break;

	case EDIT_BUTTON_OPEN_COURSE_MENU:
		{
			g_CourseMode.rows[0].choices.clear();
			g_CourseMode.rows[0].choices.push_back( "OFF" );
			g_CourseMode.rows[0].iDefaultChoice = 0;

			vector<Course*> courses;
			SONGMAN->GetAllCourses( courses, false );
			for( unsigned i = 0; i < courses.size(); ++i )
			{
				const Course *crs = courses[i];

				bool bUsesThisSong = false;
				for( unsigned e = 0; e < crs->m_vEntries.size(); ++e )
				{
					if( crs->m_vEntries[e].pSong != m_pSong )
						continue;
					bUsesThisSong = true;
				}

				if( bUsesThisSong )
				{
					g_CourseMode.rows[0].choices.push_back( crs->GetDisplayFullTitle() );
					if( crs == GAMESTATE->m_pCurCourse )
						g_CourseMode.rows[0].iDefaultChoice = g_CourseMode.rows[0].choices.size()-1;
				}
			}

			EditMiniMenu( &g_CourseMode, SM_BackFromCourseModeMenu );
		}
		break;
	case EDIT_BUTTON_OPEN_COURSE_ATTACK_MENU:
		{
			Course *pCourse = GAMESTATE->m_pCurCourse;
			CourseEntry &ce = pCourse->m_vEntries[GAMESTATE->m_iEditCourseEntryIndex];
			bool bAnyAttackAtThisBeat = false;
			FOREACH_CONST( Attack, ce.attacks, a )
			{
				if( a->fStartSecond == m_pSong->GetElapsedTimeFromBeat(GAMESTATE->m_fSongBeat) )
				{
					bAnyAttackAtThisBeat = true;
					break;
				}
			}
			
			g_InsertCourseAttack.rows[remove].bEnabled = bAnyAttackAtThisBeat;

			EditMiniMenu( &g_InsertCourseAttack, SM_BackFromInsertCourseAttack );
		}
		break;
	case EDIT_BUTTON_BAKE_RANDOM_FROM_SONG_GROUP:
	case EDIT_BUTTON_BAKE_RANDOM_FROM_SONG_GROUP_AND_GENRE:
		{
			bool bTryGenre = EditB == EDIT_BUTTON_BAKE_RANDOM_FROM_SONG_GROUP_AND_GENRE;
			RString sName = GetOneBakedRandomFile(m_pSong, bTryGenre);
			if( sName.empty() )
			{
				SCREENMAN->PlayInvalidSound();
				SCREENMAN->SystemMessage( NO_BACKGROUNDS_AVAILABLE );
			}
			else
			{
				bool bAlreadyBGChangeHere = false;
				BackgroundLayer iLayer = BACKGROUND_LAYER_1;
				BackgroundChange bgChange;
				bgChange.m_fStartBeat = GAMESTATE->m_fSongBeat;
				FOREACH( BackgroundChange, m_pSong->GetBackgroundChanges(iLayer), bgc )
				{
					if( bgc->m_fStartBeat == GAMESTATE->m_fSongBeat )
					{
						bAlreadyBGChangeHere = true;
						bgChange = *bgc;
						m_pSong->GetBackgroundChanges(iLayer).erase( bgc );
						break;
					}
				}
				bgChange.m_def.m_sFile1 = sName;
				m_pSong->AddBackgroundChange( iLayer, bgChange );
				m_soundMarker.Play();
			}
		}
		break;

	case EDIT_BUTTON_PLAY_FROM_START:
		HandleMainMenuChoice( play_whole_song );
		break;

	case EDIT_BUTTON_PLAY_FROM_CURSOR:
		HandleMainMenuChoice( play_current_beat_to_end );
		break;

	case EDIT_BUTTON_PLAY_SELECTION:
		if( m_NoteFieldEdit.m_iBeginMarker!=-1 && m_NoteFieldEdit.m_iEndMarker!=-1 )
			HandleAreaMenuChoice( play );
		else
			HandleMainMenuChoice( play_current_beat_to_end );
		break;
	case EDIT_BUTTON_RECORD_SELECTION:
		if( m_NoteFieldEdit.m_iBeginMarker!=-1 && m_NoteFieldEdit.m_iEndMarker!=-1 )
			HandleAreaMenuChoice( record );
		else
		{
			if( g_iDefaultRecordLength.Get() == -1 )
			{
				m_iStartPlayingAt = BeatToNoteRow(GAMESTATE->m_fSongBeat);
				m_iStopPlayingAt = m_NoteDataEdit.GetLastRow() + 1;
			}
			else
			{
				m_iStartPlayingAt = BeatToNoteRow( ftruncf(GAMESTATE->m_fSongBeat, g_iDefaultRecordLength.Get()) );
				m_iStopPlayingAt = m_iStartPlayingAt + BeatToNoteRow( g_iDefaultRecordLength.Get() );
			}

			if( GAMESTATE->m_pCurSteps[0]->IsAnEdit() )
				m_iStopPlayingAt = min( m_iStopPlayingAt, BeatToNoteRow(GetMaximumBeatForNewNote()) );

			if( m_iStartPlayingAt >= m_iStopPlayingAt )
			{
				SCREENMAN->PlayInvalidSound();
				return;
			}

			TransitionEditState( STATE_RECORDING );
		}
		break;
	case EDIT_BUTTON_RECORD_FROM_CURSOR:
		m_iStartPlayingAt = BeatToNoteRow(GAMESTATE->m_fSongBeat);
		m_iStopPlayingAt = m_NoteDataEdit.GetLastRow();
		m_iStopPlayingAt += BeatToNoteRow(4);		// give a one measure lead out
		TransitionEditState( STATE_RECORDING );
		break;

	case EDIT_BUTTON_INSERT:
		HandleAreaMenuChoice( insert_and_shift );
		SCREENMAN->PlayInvalidSound();
		break;

	case EDIT_BUTTON_INSERT_SHIFT_PAUSES:
		HandleAreaMenuChoice( shift_pauses_forward );
		SCREENMAN->PlayInvalidSound();
		break;

	case EDIT_BUTTON_DELETE:
		HandleAreaMenuChoice( delete_and_shift );
		SCREENMAN->PlayInvalidSound();
		break;

	case EDIT_BUTTON_DELETE_SHIFT_PAUSES:
		HandleAreaMenuChoice( shift_pauses_backward );
		SCREENMAN->PlayInvalidSound();
		break;

	case EDIT_BUTTON_UNDO:
		Undo();
		break;
	}
}

void ScreenEdit::InputRecord( const InputEventPlus &input, EditButton EditB )
{
	if( EditB == EDIT_BUTTON_RETURN_TO_EDIT )
	{
		TransitionEditState( STATE_EDITING );
		return;
	}	

	if( input.StyleI.player != PLAYER_1 )
		return;		// ignore

	const int iCol = input.StyleI.col;

	switch( input.type )
	{
	case IET_FIRST_PRESS:
		{
			if( EditIsBeingPressed(EDIT_BUTTON_REMOVE_NOTE) )
			{
				// Remove notes in Update.
				break;
			}

			// Add a tap
			float fBeat = GAMESTATE->m_fSongBeat;
			fBeat = Quantize( fBeat, NoteTypeToBeat(m_SnapDisplay.GetNoteType()) );
			
			const int iRow = BeatToNoteRow( fBeat );
			if( iRow < 0 )
				break;

			// Don't add outside of the range.
			if( iRow < m_iStartPlayingAt || iRow >= m_iStopPlayingAt )
				return;

			// Remove hold if any so that we don't have taps inside of a hold.
			int iHeadRow;
			if( m_NoteDataRecord.IsHoldNoteAtRow( iCol, iRow, &iHeadRow ) )
				m_NoteDataRecord.SetTapNote( iCol, iHeadRow, TAP_EMPTY );

			TapNote tn = TAP_ORIGINAL_TAP;
			if( EditIsBeingPressed(EDIT_BUTTON_LAY_MINE_OR_ROLL) )
				tn = TAP_ORIGINAL_MINE;

			m_NoteDataRecord.SetTapNote( iCol, iRow, tn );
			m_NoteFieldRecord.Step( iCol, TNS_W1 );
		}
		break;
	case IET_SLOW_REPEAT:
	case IET_FAST_REPEAT:
	case IET_RELEASE:
		// don't add or extend holds here; we do it in Update()
		break;
	}
}

void ScreenEdit::InputRecordPaused( const InputEventPlus &input, EditButton EditB )
{
	if( input.type != IET_FIRST_PRESS )
		return;		// don't care

	switch( EditB )
	{
	case EDIT_BUTTON_UNDO:
		/* We've already actually committed changes to m_NoteDataEdit, so all we have
		 * to do to undo is Undo() as usual, and copy the note data back in. */
		Undo();
		m_NoteDataRecord.CopyAll( m_NoteDataEdit );
		break;

	case EDIT_BUTTON_PLAY_SELECTION:
		TransitionEditState( STATE_PLAYING );
		break;

	case EDIT_BUTTON_RECORD_SELECTION:
		TransitionEditState( STATE_RECORDING );
		break;

	case EDIT_BUTTON_RECORD_FROM_CURSOR:
		if( GAMESTATE->m_pCurSteps[0]->IsAnEdit() )
		{
			float fMaximumBeat = GetMaximumBeatForNewNote();
			if( NoteRowToBeat(m_iStopPlayingAt) >= fMaximumBeat )
			{
				// We're already at the end.
				SCREENMAN->PlayInvalidSound();
				return;
			}
		}

		/* Pick up where we left off. */
		{
			int iSize = m_iStopPlayingAt - m_iStartPlayingAt;
			m_iStartPlayingAt = m_iStopPlayingAt;
			m_iStopPlayingAt += iSize;

			if( GAMESTATE->m_pCurSteps[0]->IsAnEdit() )
				m_iStopPlayingAt = min( m_iStopPlayingAt, BeatToNoteRow(GetMaximumBeatForNewNote()) );
		}

		TransitionEditState( STATE_RECORDING );
		break;

	case EDIT_BUTTON_RETURN_TO_EDIT:
		TransitionEditState( STATE_EDITING );
		break;
	}
}

void ScreenEdit::InputPlay( const InputEventPlus &input, EditButton EditB )
{
	if( input.type != IET_FIRST_PRESS )
		return;

	switch( EditB )
	{
	case EDIT_BUTTON_RETURN_TO_EDIT:
		/* When exiting play mode manually, leave the cursor where it is. */
		m_fBeatToReturnTo = GAMESTATE->m_fSongBeat;
		TransitionEditState( STATE_EDITING );
		break;
	case EDIT_BUTTON_TOGGLE_ASSIST_TICK:
		GAMESTATE->m_SongOptions.m_bAssistTick ^= 1;
		break;
	case EDIT_BUTTON_TOGGLE_AUTOPLAY:
		{
			PREFSMAN->m_AutoPlay.Set( PREFSMAN->m_AutoPlay!=PC_HUMAN ? PC_HUMAN:PC_AUTOPLAY );
			FOREACH_HumanPlayer( p )
				GAMESTATE->m_pPlayerState[p]->m_PlayerController = PREFSMAN->m_AutoPlay;
		}
		break;
	case EDIT_BUTTON_OFFSET_UP:
	case EDIT_BUTTON_OFFSET_DOWN:
		{
			float fOffsetDelta;
			switch( EditB )
			{
			case EDIT_BUTTON_OFFSET_DOWN:	fOffsetDelta = -0.020f;		break;
			case EDIT_BUTTON_OFFSET_UP:	fOffsetDelta = +0.020f;		break;
			default:	ASSERT(0);						return;
			}

			if( EditIsBeingPressed( EDIT_BUTTON_ADJUST_FINE ) )
			{
				fOffsetDelta /= 20; /* 1ms */
			}
			else 
			{
				switch( input.type )
				{
				case IET_SLOW_REPEAT:	fOffsetDelta *= 10;	break;
				case IET_FAST_REPEAT:	fOffsetDelta *= 40;	break;
				}
			}

			m_pSong->m_Timing.m_fBeat0OffsetInSeconds += fOffsetDelta;
		}
		break;
	}

	switch( input.StyleI.player )
	{
	case PLAYER_1:	
		if( PREFSMAN->m_AutoPlay == PC_HUMAN )
			m_Player.Step( input.StyleI.col, input.DeviceI.ts ); 
		break;
	}

}

void ScreenEdit::TransitionEditState( EditState em )
{
	EditState old = m_EditState;
	
	// If we're going from recording to paused, come back when we're done.
	if( old == STATE_RECORDING_PAUSED && em == STATE_PLAYING )
		m_bReturnToRecordMenuAfterPlay = true;

	//
	// If switching out of record, open the menu.
	//
	{
		bool bGoToRecordMenu = (old == STATE_RECORDING);
		if( m_bReturnToRecordMenuAfterPlay && old == STATE_PLAYING )
		{
			bGoToRecordMenu = true;
			m_bReturnToRecordMenuAfterPlay = false;
		}
		
		if( bGoToRecordMenu )
			em = STATE_RECORDING_PAUSED;
	}

	/* If we're playing music, sample music or assist ticks when changing modes, stop. */
	SOUND->StopMusic();
	m_soundMusic.StopPlaying();
	m_soundAssistTick.StopPlaying();
	GAMESTATE->m_bGameplayLeadIn.Set( true );

	switch( old )
	{
	case STATE_EDITING:
		// If exiting EDIT mode, save the cursor position.
		m_fBeatToReturnTo = GAMESTATE->m_fSongBeat;
		break;

	case STATE_PLAYING:
		if( GAMESTATE->IsSyncDataChanged() )
			ScreenSaveSync::PromptSaveSync();
		break;

	case STATE_RECORDING:
		SaveUndo();

		// delete old TapNotes in the range
		m_NoteDataEdit.ClearRange( m_iStartPlayingAt, m_iStopPlayingAt );
		m_NoteDataEdit.CopyRange( m_NoteDataRecord, m_iStartPlayingAt, m_iStopPlayingAt, m_iStartPlayingAt );
		m_NoteDataRecord.ClearAll();

		CheckNumberOfNotesAndUndo();
		break;
	}

	switch( em )
	{
	case STATE_EDITING:
		/* Important: people will stop playing, change the BG and start again; make sure we reload */
		m_Background.Unload();
		m_Foreground.Unload();

		/* Restore the cursor position. */
		GAMESTATE->m_fSongBeat = m_fBeatToReturnTo;

		/* Make sure we're snapped. */
		GAMESTATE->m_fSongBeat = Quantize( GAMESTATE->m_fSongBeat, NoteTypeToBeat(m_SnapDisplay.GetNoteType()) );

		/* Playing and recording have lead-ins, which may start before beat 0;
		 * make sure we don't stay there if we escaped out early. */
		GAMESTATE->m_fSongBeat = max( GAMESTATE->m_fSongBeat, 0 );

		break;

	case STATE_PLAYING:
	case STATE_RECORDING:
	{
		GAMESTATE->ResetOriginalSyncData();

		/* Stop displaying course attacks, if any, and return to the player's defaults. */
		GAMESTATE->m_pPlayerState[PLAYER_1]->RemoveActiveAttacks();
		GAMESTATE->m_pPlayerState[PLAYER_1]->RebuildPlayerOptionsFromActiveAttacks();

		if( em == STATE_RECORDING )
			GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerOptions.m_fScrolls[PlayerOptions::SCROLL_CENTERED] = 1.0f;

		/* Snap to current options. */
		GAMESTATE->m_pPlayerState[PLAYER_1]->m_CurrentPlayerOptions = GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerOptions;

		/* Give a 1 second lead-in.  If we're loading Player, this must be done first. */
		float fSeconds = m_pSong->m_Timing.GetElapsedTimeFromBeat( NoteRowToBeat(m_iStartPlayingAt) ) - 1;
		GAMESTATE->UpdateSongPosition( fSeconds, m_pSong->m_Timing );

		GAMESTATE->m_bGameplayLeadIn.Set( false );

		/* Reset the note skin, in case preferences have changed. */
		// XXX
		// GAMESTATE->ResetNoteSkins();
		//GAMESTATE->res

		break;
	}
	case STATE_RECORDING_PAUSED:
		break;
	default:
		ASSERT(0);
	}

	switch( em )
	{
	case STATE_PLAYING:
	{
		/* If we're in course display mode, set that up. */
		SetupCourseAttacks();

		m_Player.Load( m_NoteDataEdit );
		GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerController = PREFSMAN->m_AutoPlay;

		if( g_bEditorShowBGChangesPlay )
		{
			/* FirstBeat affects backgrounds, so commit changes to memory (not to disk)
			 * and recalc it. */
			Steps* pSteps = GAMESTATE->m_pCurSteps[PLAYER_1];
			ASSERT( pSteps );
			pSteps->SetNoteData( m_NoteDataEdit );
			m_pSong->ReCalculateRadarValuesAndLastBeat();

			m_Background.Unload();
			m_Background.LoadFromSong( m_pSong );

			m_Foreground.Unload();
			m_Foreground.LoadFromSong( m_pSong );
		}

		break;
	}

	case STATE_RECORDING:
	case STATE_RECORDING_PAUSED:
	{
		// initialize m_NoteFieldRecord
		m_NoteDataRecord.CopyAll( m_NoteDataEdit );

		// highlight the section being recorded
		m_NoteFieldRecord.m_iBeginMarker = m_iStartPlayingAt;
		m_NoteFieldRecord.m_iEndMarker = m_iStopPlayingAt;

		break;
	}
	}

	//
	// Show/hide depending on em
	//
	m_sprOverlay->PlayCommand( EditStateToString(em) );
	m_sprUnderlay->PlayCommand( EditStateToString(em) );

	m_Background.SetHidden( !g_bEditorShowBGChangesPlay || em == STATE_EDITING );
	m_autoHeader->SetHidden( em != STATE_EDITING );
	m_textInputTips.SetHidden( em != STATE_EDITING );
	m_textInfo.SetHidden( em != STATE_EDITING );
	// Play the OnCommands again so that these will be re-hidden if the OnCommand hides them.
	if( em == STATE_EDITING )
	{
		m_textInputTips.PlayCommand( "On" );
		m_textInfo.PlayCommand( "On" );
	}
	m_textPlayRecordHelp.SetHidden( em == STATE_EDITING );
	m_SnapDisplay.SetHidden( em != STATE_EDITING );
	m_NoteFieldEdit.SetHidden( em != STATE_EDITING );
	m_NoteFieldRecord.SetHidden( em != STATE_RECORDING && em != STATE_RECORDING_PAUSED );
	m_Player.SetHidden( em != STATE_PLAYING );
	m_Foreground.SetHidden( !g_bEditorShowBGChangesPlay || em == STATE_EDITING );

	switch( em )
	{
	case STATE_PLAYING:
	case STATE_RECORDING:
		const float fStartSeconds = m_pSong->GetElapsedTimeFromBeat(GAMESTATE->m_fSongBeat);
		LOG->Trace( "Starting playback at %f", fStartSeconds );

		RageSoundParams p;
		p.SetPlaybackRate( GAMESTATE->m_SongOptions.m_fMusicRate );
		p.m_StartSecond = fStartSeconds;
		p.m_bAccurateSync = true;
		p.StopMode = RageSoundParams::M_CONTINUE;
		m_soundMusic.Play( &p );
		break;
	}

	m_EditState = em;
}

void ScreenEdit::ScrollTo( float fDestinationBeat )
{
	CLAMP( fDestinationBeat, 0, GetMaximumBeatForMoving() );

	// Don't play the sound and do the hold note logic below if our position didn't change.
	const float fOriginalBeat = GAMESTATE->m_fSongBeat;
	if( fOriginalBeat == fDestinationBeat )
		return;

	GAMESTATE->m_fSongBeat = fDestinationBeat;

	// check to see if they're holding a button
	for( int n=0; n<NUM_EDIT_BUTTON_COLUMNS; n++ )
	{
		int iCol = n;

		// Ctrl + number = input to right half
		if( EditIsBeingPressed(EDIT_BUTTON_RIGHT_SIDE) )
			ShiftToRightSide( iCol, m_NoteDataEdit.GetNumTracks() );

		if( iCol >= m_NoteDataEdit.GetNumTracks() )
			continue;	// skip

		EditButton b = EditButton(EDIT_BUTTON_COLUMN_0+n);
		if( !EditIsBeingPressed(b) )
			continue;

		// create a new hold note
		int iStartRow = BeatToNoteRow( min(fOriginalBeat, fDestinationBeat) );
		int iEndRow = BeatToNoteRow( max(fOriginalBeat, fDestinationBeat) );

		// Don't SaveUndo.  We want to undo the whole hold, not just the last segment
		// that the user made.  Dragging the hold bigger can only absorb and remove
		// other taps, so dragging won't cause us to exceed the note limit.
		if( EditIsBeingPressed(EDIT_BUTTON_LAY_MINE_OR_ROLL) )
			m_NoteDataEdit.AddHoldNote( iCol, iStartRow, iEndRow, TAP_ORIGINAL_ROLL_HEAD );
		else
			m_NoteDataEdit.AddHoldNote( iCol, iStartRow, iEndRow, TAP_ORIGINAL_HOLD_HEAD );
	}

	if( EditIsBeingPressed(EDIT_BUTTON_SCROLL_SELECT) )
	{
		/* Shift is being held. 
			*
			* If this is the first time we've moved since shift was depressed,
			* the old position (before this move) becomes the start pos: */
		int iDestinationRow = BeatToNoteRow( fDestinationBeat );
		if( m_iShiftAnchor == -1 )
			m_iShiftAnchor = BeatToNoteRow(fOriginalBeat);
		
		if( iDestinationRow == m_iShiftAnchor )
		{
			/* We're back at the anchor, so we have nothing selected. */
			m_NoteFieldEdit.m_iBeginMarker = m_NoteFieldEdit.m_iEndMarker = -1;
		}
		else
		{
			m_NoteFieldEdit.m_iBeginMarker = m_iShiftAnchor;
			m_NoteFieldEdit.m_iEndMarker = iDestinationRow;
			if( m_NoteFieldEdit.m_iBeginMarker > m_NoteFieldEdit.m_iEndMarker )
				swap( m_NoteFieldEdit.m_iBeginMarker, m_NoteFieldEdit.m_iEndMarker );
		}
	}

	m_soundChangeLine.Play();
}

static LocalizedString SAVE_SUCCESSFUL				( "ScreenEdit", "Save successful." );

void ScreenEdit::HandleScreenMessage( const ScreenMessage SM )
{
	if( SM != SM_UpdateTextInfo )
		m_bTextInfoNeedsUpdate = true;

	if( SM == SM_UpdateTextInfo )
	{
		UpdateTextInfo();
		this->PostScreenMessage( SM_UpdateTextInfo, 0.1f );
	}
	else if( SM == SM_GoToNextScreen )
	{
		GAMESTATE->m_EditMode = EditMode_INVALID;
	}
	else if( SM == SM_BackFromMainMenu )
	{
		HandleMainMenuChoice( (MainMenuChoice)ScreenMiniMenu::s_iLastRowCode, ScreenMiniMenu::s_viLastAnswers );
	}
	else if( SM == SM_BackFromAreaMenu )
	{
		HandleAreaMenuChoice( (AreaMenuChoice)ScreenMiniMenu::s_iLastRowCode, ScreenMiniMenu::s_viLastAnswers );
	}
	else if( SM == SM_BackFromStepsInformation )
	{
		HandleStepsInformationChoice( (StepsInformationChoice)ScreenMiniMenu::s_iLastRowCode, ScreenMiniMenu::s_viLastAnswers );
	}
	else if( SM == SM_BackFromSongInformation )
	{
		HandleSongInformationChoice( (SongInformationChoice)ScreenMiniMenu::s_iLastRowCode, ScreenMiniMenu::s_viLastAnswers );
	}
	else if( SM == SM_BackFromBPMChange )
	{
		float fBPM = strtof( ScreenTextEntry::s_sLastAnswer, NULL );
		if( fBPM > 0 )
			m_pSong->SetBPMAtBeat( GAMESTATE->m_fSongBeat, fBPM );
	}
	else if( SM == SM_BackFromStopChange )
	{
		float fStop = strtof( ScreenTextEntry::s_sLastAnswer, NULL );
		if( fStop >= 0 )
			m_pSong->m_Timing.SetStopAtBeat( GAMESTATE->m_fSongBeat, fStop );
	}
	else if( SM == SM_BackFromBGChange )
	{
		HandleBGChangeChoice( (BGChangeChoice)ScreenMiniMenu::s_iLastRowCode, ScreenMiniMenu::s_viLastAnswers );
	}
	else if( SM == SM_BackFromCourseModeMenu )
	{
		const int num = ScreenMiniMenu::s_viLastAnswers[0];
		GAMESTATE->m_pCurCourse.Set( NULL );
		if( num != 0 )
		{
			const RString name = g_CourseMode.rows[0].choices[num];
			Course *pCourse = SONGMAN->FindCourse( name );
			GAMESTATE->m_pCurCourse.Set( pCourse );
			ASSERT( GAMESTATE->m_pCurCourse );
		}
	}
	else if( SM == SM_BackFromOptions )
	{
		GAMESTATE->StoreSelectedOptions();	// so that the options stick when we reset Course attacks

		// stop any music that screen may have been playing
		SOUND->StopMusic();
	}
	else if( SM == SM_BackFromInsertTapAttack )
	{
		int iDurationChoice = ScreenMiniMenu::s_viLastAnswers[0];
		g_fLastInsertAttackDurationSeconds = strtof( g_InsertTapAttack.rows[0].choices[iDurationChoice], NULL );

		SCREENMAN->AddNewScreenToTop( "ScreenPlayerOptions", SM_BackFromInsertTapAttackPlayerOptions );
	}
	else if( SM == SM_BackFromInsertTapAttackPlayerOptions )
	{
		PlayerOptions poChosen = GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerOptions;
		RString sMods = poChosen.GetString();
		const int row = BeatToNoteRow( GAMESTATE->m_fSongBeat );
		
		TapNote tn(
			TapNote::attack, 
			TapNote::SubType_invalid,
			TapNote::original, 
			sMods,
			g_fLastInsertAttackDurationSeconds, 
			false,
			0 );
		
		SaveUndo();
		m_NoteDataEdit.SetTapNote( g_iLastInsertTapAttackTrack, row, tn );
		CheckNumberOfNotesAndUndo();
	}
	else if( SM == SM_BackFromInsertCourseAttack )
	{
		int iDurationChoice = ScreenMiniMenu::s_viLastAnswers[0];
		g_fLastInsertAttackDurationSeconds = strtof( g_InsertCourseAttack.rows[0].choices[iDurationChoice], NULL );

		if( ScreenMiniMenu::s_iLastRowCode == ScreenEdit::remove )
		{
			Course *pCourse = GAMESTATE->m_pCurCourse;
			CourseEntry &ce = pCourse->m_vEntries[GAMESTATE->m_iEditCourseEntryIndex];
			FOREACH( Attack, ce.attacks, a )
			{
				if( a->fStartSecond == m_pSong->GetElapsedTimeFromBeat(GAMESTATE->m_fSongBeat) )
				{
					ce.attacks.erase( a );
					break;
				}
			}
		}
		else
		{
			SCREENMAN->AddNewScreenToTop( "ScreenPlayerOptions", SM_BackFromInsertCourseAttackPlayerOptions );
		}
	}
	else if( SM == SM_BackFromInsertCourseAttackPlayerOptions )
	{
		PlayerOptions poChosen = GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerOptions;
		RString sMods = poChosen.GetString();

		Course *pCourse = GAMESTATE->m_pCurCourse;
		CourseEntry &ce = pCourse->m_vEntries[GAMESTATE->m_iEditCourseEntryIndex];
		Attack a(
			ATTACK_LEVEL_1,
			m_pSong->GetElapsedTimeFromBeat(GAMESTATE->m_fSongBeat),
			g_fLastInsertAttackDurationSeconds,
			sMods,
			false,
			false );
		ce.attacks.push_back( a );
	}
	else if( SM == SM_DoRevertToLastSave )
	{
		if( ScreenPrompt::s_LastAnswer == ANSWER_YES )
		{
			SaveUndo();
			CopyFromLastSave();
			m_pSteps->GetNoteData( m_NoteDataEdit );
		}
	}
	else if( SM == SM_DoRevertFromDisk )
	{
		if( ScreenPrompt::s_LastAnswer == ANSWER_YES )
		{
			SaveUndo();
			RevertFromDisk();
			m_pSteps->GetNoteData( m_NoteDataEdit );
		}
	}
	else if( SM == SM_DoSaveAndExit ) // just asked "save before exiting? yes, no, cancel"
	{
		switch( ScreenPrompt::s_LastAnswer )
		{
		case ANSWER_YES:
			// This will send SM_SaveSuccessful or SM_SaveFailed.
			HandleMainMenuChoice( ScreenEdit::save_on_exit );
			return;
		case ANSWER_NO:
			/* Don't save; just exit. */
			SCREENMAN->SendMessageToTopScreen( SM_DoExit );
			return;
		case ANSWER_CANCEL:
			break; // do nothing
		}
	}
	else if( SM == SM_SaveSuccessful )
	{
		LOG->Trace( "Save successful." );
		m_pSteps->SetSavedToDisk( true );
		CopyToLastSave();

		if( m_CurrentAction == save_on_exit )
			ScreenPrompt::Prompt( SM_DoExit, SAVE_SUCCESSFUL );
		else
			SCREENMAN->SystemMessage( SAVE_SUCCESSFUL );
	}
	else if( SM == SM_SaveFailed ) // save failed; stay in the editor
	{
		/* We committed the steps to SongManager.  Revert to the last save, and
		 * recommit the reversion to SongManager. */
		LOG->Trace( "Save failed.  Changes uncommitted from memory." );
		CopyFromLastSave();
		m_pSteps->SetNoteData( m_NoteDataEdit );
	}
	else if( SM == SM_DoExit )
	{
		// IMPORTANT: CopyFromLastSave before deleting the Steps below
		CopyFromLastSave();

		// If these steps have never been saved, then we should delete them.
		// If the user created them in the edit menu and never bothered
		// to save them, then they aren't wanted.
		Steps* pSteps = GAMESTATE->m_pCurSteps[PLAYER_1];
		if( !pSteps->GetSavedToDisk() )
		{
			Song* pSong = GAMESTATE->m_pCurSong;
			pSong->DeleteSteps( pSteps );
			m_pSteps = NULL;
			GAMESTATE->m_pCurSteps[PLAYER_1].Set( NULL );
		}

		m_Out.StartTransitioning( SM_GoToNextScreen );
	}
	else if( SM == SM_GainFocus )
	{
		/* We do this ourself. */
		SOUND->HandleSongTimer( false );

		/* When another screen comes up, RageSounds takes over the sound timer.  When we come
		 * back, put the timer back to where it was. */
		GAMESTATE->m_fSongBeat = m_fTrailingBeat;
	}
	else if( SM == SM_LoseFocus )
	{
		/* Snap the trailing beat, in case we lose focus while tweening. */
		m_fTrailingBeat = GAMESTATE->m_fSongBeat;
	}

	ScreenWithMenuElements::HandleScreenMessage( SM );
}

void ScreenEdit::OnSnapModeChange()
{
	m_soundChangeSnap.Play();
			
	NoteType nt = m_SnapDisplay.GetNoteType();
	int iStepIndex = BeatToNoteRow( GAMESTATE->m_fSongBeat );
	int iElementsPerNoteType = BeatToNoteRow( NoteTypeToBeat(nt) );
	int iStepIndexHangover = iStepIndex % iElementsPerNoteType;
	GAMESTATE->m_fSongBeat -= NoteRowToBeat( iStepIndexHangover );
}



// Helper function for below

// Begin helper functions for InputEdit


static void ChangeDescription( const RString &sNew )
{
	Steps* pSteps = GAMESTATE->m_pCurSteps[PLAYER_1];

	/* Don't erase edit descriptions. */
	if( sNew.empty() && pSteps->GetDifficulty() == DIFFICULTY_EDIT )
		return;

	pSteps->SetDescription(sNew);
}

static bool ValidateDescription( const RString &sAnswer, RString &sErrorOut )
{
	if( sAnswer.empty() )
		return true;

	/* Don't allow duplicate edit names within the same StepsType; edit names uniquely
	 * identify the edit. */
	Steps *pSteps = GAMESTATE->m_pCurSteps[PLAYER_1];
	Song *pSong = GAMESTATE->m_pCurSong;
	if( pSteps->GetDifficulty() != DIFFICULTY_EDIT )
		return true;

	/* If unchanged: */
	if( pSteps->GetDescription() == sAnswer )
		return true;

	vector<Steps*> apSteps;
	pSong->GetSteps( apSteps, pSteps->m_StepsType, DIFFICULTY_EDIT, -1, -1, sAnswer );

	if( apSteps.size() == 0 )
		return true;

	/* The name is already used, */
	if( apSteps.size() > 1 )
		LOG->Warn( "Multiple edits for song \"%s\" named \"%s\"", pSong->GetSongDir().c_str(), sAnswer.c_str() );

	sErrorOut = "The supplied name supplied conflicts with another edit.\n\nPlease use a different name.";
	return false;
}

static void ChangeMainTitle( const RString &sNew )
{
	Song* pSong = GAMESTATE->m_pCurSong;
	pSong->m_sMainTitle = sNew;
}

static void ChangeSubTitle( const RString &sNew )
{
	Song* pSong = GAMESTATE->m_pCurSong;
	pSong->m_sSubTitle = sNew;
}

static void ChangeArtist( const RString &sNew )
{
	Song* pSong = GAMESTATE->m_pCurSong;
	pSong->m_sArtist = sNew;
}

static void ChangeCredit( const RString &sNew )
{
	Song* pSong = GAMESTATE->m_pCurSong;
	pSong->m_sCredit = sNew;
}

static void ChangeMainTitleTranslit( const RString &sNew )
{
	Song* pSong = GAMESTATE->m_pCurSong;
	pSong->m_sMainTitleTranslit = sNew;
}

static void ChangeSubTitleTranslit( const RString &sNew )
{
	Song* pSong = GAMESTATE->m_pCurSong;
	pSong->m_sSubTitleTranslit = sNew;
}

static void ChangeArtistTranslit( const RString &sNew )
{
	Song* pSong = GAMESTATE->m_pCurSong;
	pSong->m_sArtistTranslit = sNew;
}

// End helper functions

static LocalizedString REVERT_LAST_SAVE				( "ScreenEdit", "Do you want to revert to your last save?" );
static LocalizedString DESTROY_ALL_UNSAVED_CHANGES	( "ScreenEdit", "This will destroy all unsaved changes." );
static LocalizedString REVERT_FROM_DISK				( "ScreenEdit", "Do you want to revert from disk?" );
static LocalizedString SAVE_CHANGES_BEFORE_EXITING	( "ScreenEdit", "Do you want to save changes before exiting?" );
static LocalizedString ENTER_BPM_VALUE				( "ScreenEdit", "Enter a new BPM value." );
static LocalizedString ENTER_STOP_VALUE				( "ScreenEdit", "Enter a new Stop value." );
void ScreenEdit::HandleMainMenuChoice( MainMenuChoice c, const vector<int> &iAnswers )
{
	switch( c )
	{
		case edit_steps_information:
			{
				/* XXX: If the difficulty is changed from EDIT, and pSteps->WasLoadedFromProfile()
				 * is true, we should warn that the steps will no longer be saved to the profile. */
				Steps* pSteps = GAMESTATE->m_pCurSteps[PLAYER_1];
				float fMusicSeconds = m_soundMusic.GetLengthSeconds();

				g_StepsInformation.rows[difficulty].choices.clear();
				FOREACH_Difficulty( dc )
					g_StepsInformation.rows[difficulty].choices.push_back( "|" + DifficultyToLocalizedString(pSteps->GetDifficulty()) );
				g_StepsInformation.rows[difficulty].iDefaultChoice = pSteps->GetDifficulty();
				g_StepsInformation.rows[difficulty].bEnabled = (EDIT_MODE.GetValue() >= EditMode_Full);
				g_StepsInformation.rows[meter].iDefaultChoice = clamp( pSteps->GetMeter()-1, 0, MAX_METER+1 );
				g_StepsInformation.rows[meter].bEnabled = (EDIT_MODE.GetValue() >= EditMode_Home);
				g_StepsInformation.rows[predict_meter].SetOneUnthemedChoice( ssprintf("%.2f",pSteps->PredictMeter()) );
				g_StepsInformation.rows[description].bEnabled = (EDIT_MODE.GetValue() >= EditMode_Full);
				g_StepsInformation.rows[description].SetOneUnthemedChoice( pSteps->GetDescription() );
				g_StepsInformation.rows[tap_notes].SetOneUnthemedChoice( ssprintf("%d", m_NoteDataEdit.GetNumTapNotes()) );
				g_StepsInformation.rows[jumps].SetOneUnthemedChoice( ssprintf("%d", m_NoteDataEdit.GetNumJumps()) );
				g_StepsInformation.rows[hands].SetOneUnthemedChoice( ssprintf("%d", m_NoteDataEdit.GetNumHands()) );
				g_StepsInformation.rows[quads].SetOneUnthemedChoice( ssprintf("%d", m_NoteDataEdit.GetNumQuads()) );
				g_StepsInformation.rows[holds].SetOneUnthemedChoice( ssprintf("%d", m_NoteDataEdit.GetNumHoldNotes()) );
				g_StepsInformation.rows[mines].SetOneUnthemedChoice( ssprintf("%d", m_NoteDataEdit.GetNumMines()) );
				g_StepsInformation.rows[stream].SetOneUnthemedChoice( ssprintf("%.2f", NoteDataUtil::GetStreamRadarValue(m_NoteDataEdit,fMusicSeconds)) );
				g_StepsInformation.rows[voltage].SetOneUnthemedChoice( ssprintf("%.2f", NoteDataUtil::GetVoltageRadarValue(m_NoteDataEdit,fMusicSeconds)) );
				g_StepsInformation.rows[air].SetOneUnthemedChoice( ssprintf("%.2f", NoteDataUtil::GetAirRadarValue(m_NoteDataEdit,fMusicSeconds)) );
				g_StepsInformation.rows[freeze].SetOneUnthemedChoice( ssprintf("%.2f", NoteDataUtil::GetFreezeRadarValue(m_NoteDataEdit,fMusicSeconds)) );
				g_StepsInformation.rows[chaos].SetOneUnthemedChoice( ssprintf("%.2f", NoteDataUtil::GetChaosRadarValue(m_NoteDataEdit,fMusicSeconds)) );
				EditMiniMenu( &g_StepsInformation, SM_BackFromStepsInformation, SM_None );
			}
			break;
		case play_whole_song:
			{
				m_iStartPlayingAt = 0;
				m_iStopPlayingAt = m_NoteDataEdit.GetLastRow();
				m_iStopPlayingAt += BeatToNoteRow(4);		// give a one measure lead out

				TransitionEditState( STATE_PLAYING );
			}
			break;
		case play_current_beat_to_end:
			{
				m_iStartPlayingAt = BeatToNoteRow(GAMESTATE->m_fSongBeat);
				m_iStopPlayingAt = m_NoteDataEdit.GetLastRow();
				m_iStopPlayingAt += BeatToNoteRow(4);		// give a one measure lead out

				TransitionEditState( STATE_PLAYING );
			}
			break;
		case save:
		case save_on_exit:
			{
				m_CurrentAction = c;

				// copy edit into current Steps
				Song* pSong = GAMESTATE->m_pCurSong;
				Steps* pSteps = GAMESTATE->m_pCurSteps[PLAYER_1];
				ASSERT( pSteps );

				pSteps->SetNoteData( m_NoteDataEdit );
				pSteps->CalculateRadarValues( pSong->m_fMusicLengthSeconds );

				switch( EDIT_MODE.GetValue() )
				{
				case EditMode_Home:
					{
						ASSERT( pSteps->IsAnEdit() );

						// XXX show error
						NotesWriterSM w;
						if( !w.WriteEditFileToMachine(pSong, pSteps) )
							break;

						// HACK: clear undo, so "exit" below knows we don't need to save.
						// This only works because important non-steps data can't be changed in
						// home mode (BPMs, stops).
						ClearUndo();

						SCREENMAN->ZeroNextUpdate();

						HandleScreenMessage( SM_SaveSuccessful );

						/* FIXME
						RString s;
						switch( c )
						{
						case save:			s = "ScreenMemcardSaveEditsAfterSave";	break;
						case save_on_exit:	s = "ScreenMemcardSaveEditsAfterExit";	break;
						default:		ASSERT(0);
						}
						SCREENMAN->AddNewScreenToTop( s );
						*/
					}
					break;
				case EditMode_Full: 
					{
						pSong->Save();
						SCREENMAN->ZeroNextUpdate();

						HandleScreenMessage( SM_SaveSuccessful );
					}
					break;
				case EditMode_Practice:
					break;
				default:
					ASSERT(0);
				}

				m_soundSave.Play();
			}
			break;
		case revert_to_last_save:
			ScreenPrompt::Prompt( SM_DoRevertToLastSave, REVERT_LAST_SAVE.GetValue() + "\n\n" + DESTROY_ALL_UNSAVED_CHANGES.GetValue(), PROMPT_YES_NO, ANSWER_NO );
			break;
		case revert_from_disk:
			ScreenPrompt::Prompt( SM_DoRevertFromDisk, REVERT_FROM_DISK.GetValue() + "\n\n" + DESTROY_ALL_UNSAVED_CHANGES.GetValue(), PROMPT_YES_NO, ANSWER_NO );
			break;
		case options:
			SCREENMAN->AddNewScreenToTop( "ScreenEditOptions", SM_BackFromOptions );
			break;
		case edit_song_info:
			{
				const Song* pSong = GAMESTATE->m_pCurSong;
				g_SongInformation.rows[main_title].SetOneUnthemedChoice( pSong->m_sMainTitle );
				g_SongInformation.rows[sub_title].SetOneUnthemedChoice( pSong->m_sSubTitle );
				g_SongInformation.rows[artist].SetOneUnthemedChoice( pSong->m_sArtist );
				g_SongInformation.rows[credit].SetOneUnthemedChoice( pSong->m_sCredit );
				g_SongInformation.rows[main_title_transliteration].SetOneUnthemedChoice( pSong->m_sMainTitleTranslit );
				g_SongInformation.rows[sub_title_transliteration].SetOneUnthemedChoice( pSong->m_sSubTitleTranslit );
				g_SongInformation.rows[artist_transliteration].SetOneUnthemedChoice( pSong->m_sArtistTranslit );

				EditMiniMenu( &g_SongInformation, SM_BackFromSongInformation );
			}
			break;
		case edit_bpm:
			ScreenTextEntry::TextEntry( 
				SM_BackFromBPMChange, 
				ENTER_BPM_VALUE, 
				ssprintf( "%.4f", m_pSong->GetBPMAtBeat(GAMESTATE->m_fSongBeat) ),
				10
				);
			break;
		case edit_stop:
			ScreenTextEntry::TextEntry( 
				SM_BackFromStopChange, 
				ENTER_STOP_VALUE, 
				ssprintf( "%.4f", m_pSong->m_Timing.GetStopAtRow( BeatToNoteRow(GAMESTATE->m_fSongBeat) ) ),
				10
				);
			break;
		case play_preview_music:
			PlayPreviewMusic();
			break;
		case exit:
			switch( EDIT_MODE.GetValue() )
			{
			case EditMode_Full:
			case EditMode_Home:
				if( m_bHasUndo )
					ScreenPrompt::Prompt( SM_DoSaveAndExit, SAVE_CHANGES_BEFORE_EXITING, PROMPT_YES_NO_CANCEL, ANSWER_CANCEL );
				else
					SCREENMAN->SendMessageToTopScreen( SM_DoExit );
				break;
			case EditMode_Practice:
				SCREENMAN->SendMessageToTopScreen( SM_DoExit );
				break;
			default:
				ASSERT(0);
			}
			break;
		default:
			ASSERT(0);
	};
}

void ScreenEdit::HandleAreaMenuChoice( AreaMenuChoice c, const vector<int> &iAnswers, bool bAllowUndo )
{
	bool bSaveUndo = false;
	switch( c )
	{
		case cut:
		case copy:
		case play:
		case record:
		case undo:			
			bSaveUndo = false;
			break;
		case paste_at_current_beat:
		case paste_at_begin_marker:
		case clear:
		case quantize:
		case turn:
		case transform:
		case alter:
		case tempo:
		case insert_and_shift:
		case delete_and_shift:
		case shift_pauses_forward:
		case shift_pauses_backward:
		case convert_to_pause:
		case convert_pause_to_beat:
			bSaveUndo = true;
			break;
		default:
			ASSERT(0);
	}

	// We call HandleAreaMenuChoice recursively.  Only the outermost HandleAreaMenuChoice
	// should allow Undo so that the inner calls don't also save Undo and mess up the outermost
	if( !bAllowUndo )
		bSaveUndo = false;

	if( bSaveUndo )
		SaveUndo();

	switch( c )
	{
		case cut:
			{
				HandleAreaMenuChoice( copy );
				HandleAreaMenuChoice( clear );
			}
			break;
		case copy:
			{
				ASSERT( m_NoteFieldEdit.m_iBeginMarker!=-1 && m_NoteFieldEdit.m_iEndMarker!=-1 );
				m_Clipboard.ClearAll();
				m_Clipboard.CopyRange( m_NoteDataEdit, m_NoteFieldEdit.m_iBeginMarker, m_NoteFieldEdit.m_iEndMarker );
			}
			break;
		case paste_at_current_beat:
		case paste_at_begin_marker:
			{
				int iDestFirstRow = -1;
				switch( c )
				{
					case paste_at_current_beat:
						iDestFirstRow = BeatToNoteRow( GAMESTATE->m_fSongBeat );
						break;
					case paste_at_begin_marker:
						ASSERT( m_NoteFieldEdit.m_iBeginMarker!=-1 );
						iDestFirstRow = m_NoteFieldEdit.m_iBeginMarker;
						break;
					default:
						ASSERT(0);
				}

				int iRowsToCopy = m_Clipboard.GetLastRow()+1;
				m_NoteDataEdit.CopyRange( m_Clipboard, 0, iRowsToCopy, iDestFirstRow );
			}
			break;
		case clear:
			{
				ASSERT( m_NoteFieldEdit.m_iBeginMarker!=-1 && m_NoteFieldEdit.m_iEndMarker!=-1 );
				m_NoteDataEdit.ClearRange( m_NoteFieldEdit.m_iBeginMarker, m_NoteFieldEdit.m_iEndMarker );
			}
			break;
		case quantize:
			{
				NoteType nt = (NoteType)iAnswers[c];
				NoteDataUtil::SnapToNearestNoteType( m_NoteDataEdit, nt, nt, m_NoteFieldEdit.m_iBeginMarker, m_NoteFieldEdit.m_iEndMarker );
			}
			break;
		case turn:
			{
				const NoteData OldClipboard( m_Clipboard );
				HandleAreaMenuChoice( cut );
				
				StepsType st = GAMESTATE->GetCurrentStyle()->m_StepsType;
				TurnType tt = (TurnType)iAnswers[c];
				switch( tt )
				{
				case left:			NoteDataUtil::Turn( m_Clipboard, st, NoteDataUtil::left );			break;
				case right:			NoteDataUtil::Turn( m_Clipboard, st, NoteDataUtil::right );			break;
				case mirror:		NoteDataUtil::Turn( m_Clipboard, st, NoteDataUtil::mirror );		break;
				case shuffle:		NoteDataUtil::Turn( m_Clipboard, st, NoteDataUtil::shuffle );		break;
				case super_shuffle:	NoteDataUtil::Turn( m_Clipboard, st, NoteDataUtil::super_shuffle );	break;
				default:		ASSERT(0);
				}

				HandleAreaMenuChoice( paste_at_begin_marker );
				m_Clipboard = OldClipboard;
			}
			break;
		case transform:
			{
				int iBeginRow = m_NoteFieldEdit.m_iBeginMarker;
				int iEndRow = m_NoteFieldEdit.m_iEndMarker;
				TransformType tt = (TransformType)iAnswers[c];
				StepsType st = GAMESTATE->GetCurrentStyle()->m_StepsType;

				switch( tt )
				{
				case noholds:	NoteDataUtil::RemoveHoldNotes( m_NoteDataEdit, iBeginRow, iEndRow );	break;
				case nomines:	NoteDataUtil::RemoveMines( m_NoteDataEdit, iBeginRow, iBeginRow );	break;
				case little:	NoteDataUtil::Little( m_NoteDataEdit, iBeginRow, iEndRow );	break;
				case wide:		NoteDataUtil::Wide( m_NoteDataEdit, iBeginRow, iEndRow );	break;
				case big:		NoteDataUtil::Big( m_NoteDataEdit, iBeginRow, iEndRow );		break;
				case quick:		NoteDataUtil::Quick( m_NoteDataEdit, iBeginRow, iEndRow );	break;
				case skippy:	NoteDataUtil::Skippy( m_NoteDataEdit, iBeginRow, iEndRow );	break;
				case add_mines:	NoteDataUtil::AddMines( m_NoteDataEdit, iBeginRow, iEndRow );	break;
				case echo:		NoteDataUtil::Echo( m_NoteDataEdit, iBeginRow, iEndRow );	break;
				case stomp:		NoteDataUtil::Stomp( m_NoteDataEdit, st, iBeginRow, iEndRow );	break;
				case planted:	NoteDataUtil::Planted( m_NoteDataEdit, iBeginRow, iEndRow );	break;
				case floored:	NoteDataUtil::Floored( m_NoteDataEdit, iBeginRow, iEndRow );	break;
				case twister:	NoteDataUtil::Twister( m_NoteDataEdit, iBeginRow, iEndRow );	break;
				case nojumps:	NoteDataUtil::RemoveJumps( m_NoteDataEdit, iBeginRow, iEndRow );	break;
				case nohands:	NoteDataUtil::RemoveHands( m_NoteDataEdit, iBeginRow, iEndRow );	break;
				case noquads:	NoteDataUtil::RemoveQuads( m_NoteDataEdit, iBeginRow, iEndRow );	break;
				case nostretch:	NoteDataUtil::RemoveStretch( m_NoteDataEdit, st, iBeginRow, iBeginRow );	break;
				default:		ASSERT(0);
				}

				// bake in the additions
				NoteDataUtil::ConvertAdditionsToRegular( m_NoteDataEdit );
			}
			break;
		case alter:
			{
				const NoteData OldClipboard( m_Clipboard );
				HandleAreaMenuChoice( cut );
				
				AlterType at = (AlterType)iAnswers[c];
				switch( at )
				{
				case autogen_to_fill_width:	
					{
						NoteData temp( m_Clipboard );
						int iNumUsedTracks = NoteDataUtil::GetNumUsedTracks( temp );
						if( iNumUsedTracks == 0 )
							break;
						temp.SetNumTracks( iNumUsedTracks );
						NoteDataUtil::LoadTransformedSlidingWindow( temp, m_Clipboard, m_Clipboard.GetNumTracks() );
						NoteDataUtil::RemoveStretch( m_Clipboard, GAMESTATE->m_pCurSteps[0]->m_StepsType );
					}
					break;
				case backwards:				NoteDataUtil::Backwards( m_Clipboard );			break;
				case swap_sides:			NoteDataUtil::SwapSides( m_Clipboard );			break;
				case copy_left_to_right:	NoteDataUtil::CopyLeftToRight( m_Clipboard );	break;
				case copy_right_to_left:	NoteDataUtil::CopyRightToLeft( m_Clipboard );	break;
				case clear_left:			NoteDataUtil::ClearLeft( m_Clipboard );			break;
				case clear_right:			NoteDataUtil::ClearRight( m_Clipboard );		break;
				case collapse_to_one:		NoteDataUtil::CollapseToOne( m_Clipboard );		break;
				case collapse_left:			NoteDataUtil::CollapseLeft( m_Clipboard );		break;
				case shift_left:			NoteDataUtil::ShiftLeft( m_Clipboard );			break;
				case shift_right:			NoteDataUtil::ShiftRight( m_Clipboard );		break;
				default:		ASSERT(0);
				}

				HandleAreaMenuChoice( paste_at_begin_marker );
				m_Clipboard = OldClipboard;
			}
			break;
		case tempo:
			{
				// This affects all steps.
				const NoteData OldClipboard( m_Clipboard );
				HandleAreaMenuChoice( cut );
				
				AlterType at = (AlterType)iAnswers[c];
				float fScale = -1;
				switch( at )
				{
				case compress_2x:	fScale = 0.5f;		break;
				case compress_3_2:	fScale = 2.0f/3;	break;
				case compress_4_3:	fScale = 0.75f;		break;
				case expand_4_3:	fScale = 4.0f/3;	break;
				case expand_3_2:	fScale = 1.5f;		break;
				case expand_2x:		fScale = 2;			break;
				default:		ASSERT(0);
				}

				switch( at )
				{
				case compress_2x:	NoteDataUtil::Scale( m_Clipboard, fScale );	break;
				case compress_3_2:	NoteDataUtil::Scale( m_Clipboard, fScale );	break;
				case compress_4_3:	NoteDataUtil::Scale( m_Clipboard, fScale );	break;
				case expand_4_3:	NoteDataUtil::Scale( m_Clipboard, fScale );	break;
				case expand_3_2:	NoteDataUtil::Scale( m_Clipboard, fScale );	break;
				case expand_2x:		NoteDataUtil::Scale( m_Clipboard, fScale );	break;
				default:		ASSERT(0);
				}

				int iOldClipboardRow = m_NoteFieldEdit.m_iEndMarker - m_NoteFieldEdit.m_iBeginMarker;
				int iNewClipboardRow = lrintf( iOldClipboardRow * fScale );
				int iDeltaRows = iNewClipboardRow - iOldClipboardRow;
				int iNewClipboardEndRow = m_NoteFieldEdit.m_iBeginMarker + iNewClipboardRow;
				if( iDeltaRows > 0 )
					NoteDataUtil::InsertRows( m_NoteDataEdit, m_NoteFieldEdit.m_iBeginMarker, iDeltaRows );
				else
					NoteDataUtil::DeleteRows( m_NoteDataEdit, m_NoteFieldEdit.m_iBeginMarker, -iDeltaRows );

				m_pSong->m_Timing.ScaleRegion( fScale, m_NoteFieldEdit.m_iBeginMarker, m_NoteFieldEdit.m_iEndMarker );

				HandleAreaMenuChoice( paste_at_begin_marker );

				const vector<Steps*> sIter = m_pSong->GetAllSteps();
				RString sTempStyle, sTempDiff;
				for( unsigned i = 0; i < sIter.size(); i++ )
				{
					if( sIter[i]->IsAutogen() )
						continue;

					/* XXX: Edits are distinguished by description.  Compare vs m_pSteps. */
					if( (sIter[i]->m_StepsType == GAMESTATE->m_pCurSteps[PLAYER_1]->m_StepsType) &&
						(sIter[i]->GetDifficulty() == GAMESTATE->m_pCurSteps[PLAYER_1]->GetDifficulty()) )
						continue;

					NoteData ndTemp;
					sIter[i]->GetNoteData( ndTemp );
					NoteDataUtil::ScaleRegion( ndTemp, fScale, m_NoteFieldEdit.m_iBeginMarker, m_NoteFieldEdit.m_iEndMarker );
					sIter[i]->SetNoteData( ndTemp );
				}

				m_NoteFieldEdit.m_iEndMarker = iNewClipboardEndRow;

				float fOldBPM = m_pSong->GetBPMAtBeat( NoteRowToBeat(m_NoteFieldEdit.m_iBeginMarker) );
				float fNewBPM = fOldBPM * fScale;
				m_pSong->m_Timing.SetBPMAtRow( m_NoteFieldEdit.m_iBeginMarker, fNewBPM );
				m_pSong->m_Timing.SetBPMAtRow( iNewClipboardEndRow, fOldBPM );
			}
			break;
		case play:
			ASSERT( m_NoteFieldEdit.m_iBeginMarker!=-1 && m_NoteFieldEdit.m_iEndMarker!=-1 );
			m_iStartPlayingAt = m_NoteFieldEdit.m_iBeginMarker;
			m_iStopPlayingAt = m_NoteFieldEdit.m_iEndMarker;
			TransitionEditState( STATE_PLAYING );
			break;
		case record:
			ASSERT( m_NoteFieldEdit.m_iBeginMarker!=-1 && m_NoteFieldEdit.m_iEndMarker!=-1 );
			m_iStartPlayingAt = m_NoteFieldEdit.m_iBeginMarker;
			m_iStopPlayingAt = m_NoteFieldEdit.m_iEndMarker;
			TransitionEditState( STATE_RECORDING );
			break;
		case insert_and_shift:
			NoteDataUtil::InsertRows( m_NoteDataEdit, BeatToNoteRow(GAMESTATE->m_fSongBeat), BeatToNoteRow(1) );
			break;
		case delete_and_shift:
			NoteDataUtil::DeleteRows( m_NoteDataEdit, BeatToNoteRow(GAMESTATE->m_fSongBeat), BeatToNoteRow(1) );
			break;
		case shift_pauses_forward:
			m_pSong->m_Timing.InsertRows( BeatToNoteRow(GAMESTATE->m_fSongBeat), BeatToNoteRow(1) );
			break;
		case shift_pauses_backward:
			m_pSong->m_Timing.DeleteRows( BeatToNoteRow(GAMESTATE->m_fSongBeat), BeatToNoteRow(1) );
			break;
		case convert_to_pause:
			{
				ASSERT( m_NoteFieldEdit.m_iBeginMarker!=-1 && m_NoteFieldEdit.m_iEndMarker!=-1 );
				float fMarkerStart = m_pSong->m_Timing.GetElapsedTimeFromBeat( NoteRowToBeat(m_NoteFieldEdit.m_iBeginMarker) );
				float fMarkerEnd = m_pSong->m_Timing.GetElapsedTimeFromBeat( NoteRowToBeat(m_NoteFieldEdit.m_iEndMarker) );

				// The length of the stop segment we're going to create.  This includes time spent in any
				// stops in the selection, which will be deleted and subsumed into the new stop.
				float fStopLength = fMarkerEnd - fMarkerStart;

				// be sure not to clobber the row at the start - a row at the end
				// can be dropped safely, though
				NoteDataUtil::DeleteRows( m_NoteDataEdit, 
										 m_NoteFieldEdit.m_iBeginMarker + 1,
										 m_NoteFieldEdit.m_iEndMarker-m_NoteFieldEdit.m_iBeginMarker
									   );
				m_pSong->m_Timing.DeleteRows( m_NoteFieldEdit.m_iBeginMarker,
										     m_NoteFieldEdit.m_iEndMarker-m_NoteFieldEdit.m_iBeginMarker );
				m_pSong->m_Timing.SetStopAtRow( m_NoteFieldEdit.m_iBeginMarker, fStopLength );
				m_NoteFieldEdit.m_iBeginMarker = -1;
				m_NoteFieldEdit.m_iEndMarker = -1;
				break;
			}
		case convert_pause_to_beat:
			{
				float fStopSeconds = m_pSong->m_Timing.GetStopAtRow( BeatToNoteRow(GAMESTATE->m_fSongBeat) );
				m_pSong->m_Timing.SetStopAtBeat( GAMESTATE->m_fSongBeat, 0 );

				float fStopBeats = fStopSeconds * m_pSong->GetBPMAtBeat(GAMESTATE->m_fSongBeat) / 60;

				// don't move the step from where it is, just move everything later
				NoteDataUtil::InsertRows( m_NoteDataEdit, BeatToNoteRow(GAMESTATE->m_fSongBeat) + 1, BeatToNoteRow(fStopBeats) );
				m_pSong->m_Timing.InsertRows( BeatToNoteRow(GAMESTATE->m_fSongBeat) + 1, BeatToNoteRow(fStopSeconds) );
			}
			break;
		case undo:
			Undo();
			break;
		default:
			ASSERT(0);
	};

	if( bSaveUndo )
		CheckNumberOfNotesAndUndo();
}

static LocalizedString ENTER_NEW_DESCRIPTION( "ScreenEdit", "Enter a new description." );
void ScreenEdit::HandleStepsInformationChoice( StepsInformationChoice c, const vector<int> &iAnswers )
{
	Steps* pSteps = GAMESTATE->m_pCurSteps[PLAYER_1];
	Difficulty dc = (Difficulty)iAnswers[difficulty];
	pSteps->SetDifficulty( dc );
	int iMeter = iAnswers[meter]+1;
	pSteps->SetMeter( iMeter );
	
	switch( c )
	{
	case description:
		ScreenTextEntry::TextEntry( 
			SM_None, 
			ENTER_NEW_DESCRIPTION, 
			m_pSteps->GetDescription(), 
			(dc == DIFFICULTY_EDIT) ? MAX_EDIT_STEPS_DESCRIPTION_LENGTH : 255,
			ValidateDescription,
			ChangeDescription, 
			NULL 
			);
		break;
	}
}

static LocalizedString ENTER_MAIN_TITLE				("ScreenEdit","Enter a new main title.");
static LocalizedString ENTER_SUB_TITLE				("ScreenEdit","Enter a new sub title.");
static LocalizedString ENTER_ARTIST					("ScreenEdit","Enter a new artist.");
static LocalizedString ENTER_CREDIT					("ScreenEdit","Enter a new credit.");
static LocalizedString ENTER_MAIN_TITLE_TRANSLIT	("ScreenEdit","Enter a new main title transliteration.");
static LocalizedString ENTER_SUB_TITLE_TRANSLIT		("ScreenEdit","Enter a new sub title transliteration.");
static LocalizedString ENTER_ARTIST_TRANSLIT		("ScreenEdit","Enter a new artist transliteration.");
void ScreenEdit::HandleSongInformationChoice( SongInformationChoice c, const vector<int> &iAnswers )
{
	Song* pSong = GAMESTATE->m_pCurSong;

	switch( c )
	{
	case main_title:
		ScreenTextEntry::TextEntry( SM_None, ENTER_MAIN_TITLE, pSong->m_sMainTitle, 100, NULL, ChangeMainTitle, NULL );
		break;
	case sub_title:
		ScreenTextEntry::TextEntry( SM_None, ENTER_SUB_TITLE, pSong->m_sSubTitle, 100, NULL, ChangeSubTitle, NULL );
		break;
	case artist:
		ScreenTextEntry::TextEntry( SM_None, ENTER_ARTIST, pSong->m_sArtist, 100, NULL, ChangeArtist, NULL );
		break;
	case credit:
		ScreenTextEntry::TextEntry( SM_None, ENTER_CREDIT, pSong->m_sCredit, 100, NULL, ChangeCredit, NULL );
		break;
	case main_title_transliteration:
		ScreenTextEntry::TextEntry( SM_None, ENTER_MAIN_TITLE_TRANSLIT, pSong->m_sMainTitleTranslit, 100, NULL, ChangeMainTitleTranslit, NULL );
		break;
	case sub_title_transliteration:
		ScreenTextEntry::TextEntry( SM_None, ENTER_SUB_TITLE_TRANSLIT, pSong->m_sSubTitleTranslit, 100, NULL, ChangeSubTitleTranslit, NULL );
		break;
	case artist_transliteration:
		ScreenTextEntry::TextEntry( SM_None, ENTER_ARTIST_TRANSLIT, pSong->m_sArtistTranslit, 100, NULL, ChangeArtistTranslit, NULL );
		break;
	default:
		ASSERT(0);
	};
}

void ScreenEdit::HandleBGChangeChoice( BGChangeChoice c, const vector<int> &iAnswers )
{
	BackgroundChange newChange;

	FOREACH( BackgroundChange, m_pSong->GetBackgroundChanges(g_CurrentBGChangeLayer), iter )
	{
		if( iter->m_fStartBeat == GAMESTATE->m_fSongBeat )
		{
			newChange = *iter;
			// delete the old change.  We'll add a new one below.
			m_pSong->GetBackgroundChanges(g_CurrentBGChangeLayer).erase( iter );
			break;
		}
	}

	newChange.m_fStartBeat = GAMESTATE->m_fSongBeat;
	newChange.m_fRate = strtof( g_BackgroundChange.rows[rate].choices[iAnswers[rate]], NULL )/100.f;
	newChange.m_sTransition = g_BackgroundChange.rows[transition].choices[iAnswers[transition]];
	newChange.m_def.m_sEffect = g_BackgroundChange.rows[effect].choices[iAnswers[effect]];
	newChange.m_def.m_sColor1 = g_BackgroundChange.rows[color1].choices[iAnswers[color1]];
	newChange.m_def.m_sColor2 = g_BackgroundChange.rows[color2].choices[iAnswers[color2]];
	switch( iAnswers[file1_type] )
	{
	default:	ASSERT(0);
	case none:				newChange.m_def.m_sFile1 = "";								break;
	case dynamic_random:	newChange.m_def.m_sFile1 = RANDOM_BACKGROUND_FILE;			break;
	case baked_random:		newChange.m_def.m_sFile1 = GetOneBakedRandomFile( m_pSong );	break;
	case song_bganimation:
	case song_movie:
	case song_bitmap:
	case global_bganimation:
	case global_movie:
	case global_movie_song_group:
	case global_movie_song_group_and_genre:
		{
			BGChangeChoice row1 = (BGChangeChoice)(file1_song_bganimation + iAnswers[file1_type]);
			newChange.m_def.m_sFile1 = g_BackgroundChange.rows[row1].choices.empty() ? "" : g_BackgroundChange.rows[row1].choices[iAnswers[row1]];
		}
		break;
	}
	switch( iAnswers[file2_type] )
	{
	default:	ASSERT(0);
	case none:				newChange.m_def.m_sFile2 = "";									break;
	case dynamic_random:	newChange.m_def.m_sFile2 = RANDOM_BACKGROUND_FILE;				break;
	case baked_random:		newChange.m_def.m_sFile2 = GetOneBakedRandomFile( m_pSong );	break;
	case song_bganimation:
	case song_movie:
	case song_bitmap:
	case global_bganimation:
	case global_movie:
	case global_movie_song_group:
	case global_movie_song_group_and_genre:
		{
			BGChangeChoice row2 = (BGChangeChoice)(file2_song_bganimation + iAnswers[file2_type]);
			newChange.m_def.m_sFile2 = g_BackgroundChange.rows[row2].choices.empty() ? "" : g_BackgroundChange.rows[row2].choices[iAnswers[row2]];
		}
		break;
	}


	if( c == delete_change || newChange.m_def.m_sFile1.empty() )
	{
		// don't add
	}
	else
	{
		m_pSong->AddBackgroundChange( g_CurrentBGChangeLayer, newChange );
	}
	g_CurrentBGChangeLayer = BACKGROUND_LAYER_INVALID;
}

void ScreenEdit::SetupCourseAttacks()
{
	/* This is the first beat that can be changed without it being visible.  Until
	 * we draw for the first time, any beat can be changed. */
	GAMESTATE->m_pPlayerState[PLAYER_1]->m_fLastDrawnBeat = -100;

	// Put course options into effect.
	GAMESTATE->m_pPlayerState[PLAYER_1]->m_ModsToApply.clear();
	GAMESTATE->m_pPlayerState[PLAYER_1]->RemoveActiveAttacks();


	if( GAMESTATE->m_pCurCourse )
	{
		GAMESTATE->m_pCurCourse->RevertFromDisk();	// Remove this and have a separate reload key?

		AttackArray Attacks;
		for( unsigned e = 0; e < GAMESTATE->m_pCurCourse->m_vEntries.size(); ++e )
		{
			if( GAMESTATE->m_pCurCourse->m_vEntries[e].pSong != m_pSong )
				continue;

			Attacks = GAMESTATE->m_pCurCourse->m_vEntries[e].attacks;
			break;
		}

		for( unsigned i=0; i<Attacks.size(); ++i )
			GAMESTATE->m_pPlayerState[PLAYER_1]->LaunchAttack( Attacks[i] );
	}
	GAMESTATE->m_pPlayerState[PLAYER_1]->RebuildPlayerOptionsFromActiveAttacks();
}

void ScreenEdit::CopyToLastSave()
{
	m_songLastSave = *GAMESTATE->m_pCurSong;
	if( GAMESTATE->m_pCurSteps[PLAYER_1] )
		m_stepsLastSave = *GAMESTATE->m_pCurSteps[PLAYER_1];
}

void ScreenEdit::CopyFromLastSave()
{
	*GAMESTATE->m_pCurSong = m_songLastSave;
	if( GAMESTATE->m_pCurSteps[PLAYER_1] )
		*GAMESTATE->m_pCurSteps[PLAYER_1] = m_stepsLastSave;
}

void ScreenEdit::RevertFromDisk()
{
	StepsID id;
	if( GAMESTATE->m_pCurSteps[PLAYER_1] )
		id.FromSteps( GAMESTATE->m_pCurSteps[PLAYER_1] );

	RString sSongDir = GAMESTATE->m_pCurSong->GetSongDir();
	GAMESTATE->m_pCurSong->LoadFromSongDir( sSongDir );

	if( id.IsValid() )
		GAMESTATE->m_pCurSteps[PLAYER_1].Set( id.ToSteps( GAMESTATE->m_pCurSong, false ) );

	m_songLastSave = *GAMESTATE->m_pCurSong;
	if( GAMESTATE->m_pCurSteps[PLAYER_1] )
		m_stepsLastSave = *GAMESTATE->m_pCurSteps[PLAYER_1];
}

void ScreenEdit::SaveUndo()
{
	m_bHasUndo = true;
	m_Undo.CopyAll( m_NoteDataEdit );
}

static LocalizedString UNDO			("ScreenEdit", "Undo");
static LocalizedString CANT_UNDO	("ScreenEdit", "Can't undo - no undo data.");
void ScreenEdit::Undo()
{
	if( m_bHasUndo )
	{
		// swap
		NoteData temp( m_NoteDataEdit );
		m_NoteDataEdit.CopyAll( m_Undo );
		m_Undo.CopyAll( temp );

		SCREENMAN->SystemMessage( UNDO );
	}
	else
	{
		SCREENMAN->SystemMessage( CANT_UNDO );
		SCREENMAN->PlayInvalidSound();
	}
}

void ScreenEdit::ClearUndo()
{
	m_bHasUndo = false;
	m_Undo.ClearAll();
}

static LocalizedString CREATES_MORE_THAN_NOTES	( "ScreenEdit", "This change creates more than %d notes in a measure." );
static LocalizedString MORE_THAN_NOTES			( "ScreenEdit", "More than %d notes per measure is not allowed.  This change has been reverted." );
static LocalizedString CREATES_NOTES_PAST_END	( "ScreenEdit", "This change creates notes past the end of the music and is not allowed." );
static LocalizedString CHANGE_REVERTED			( "ScreenEdit", "The change has been reverted." );
void ScreenEdit::CheckNumberOfNotesAndUndo()
{
	for( int row=0; row<=m_NoteDataEdit.GetLastRow(); row+=ROWS_PER_MEASURE )
	{
		int iNumNotesThisMeasure = 0;
		FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE( m_NoteDataEdit, r, row, row+ROWS_PER_MEASURE )
			iNumNotesThisMeasure += m_NoteDataEdit.GetNumTapNonEmptyTracks( r );
		if( iNumNotesThisMeasure > MAX_NOTES_PER_MEASURE )
		{
			Undo();
			m_bHasUndo = false;
			RString sError = ssprintf( CREATES_MORE_THAN_NOTES.GetValue() + "\n\n" + MORE_THAN_NOTES.GetValue(), MAX_NOTES_PER_MEASURE, MAX_NOTES_PER_MEASURE );
			ScreenPrompt::Prompt( SM_None, sError );
			return;
		}
	}

	if( GAMESTATE->m_pCurSteps[0]->IsAnEdit() )
	{
		// Check that the action didn't push notes any farther past the last measure.
		// This blocks Insert Beat from pushing past the end, but allows Delete Beat
		// to pull back the notes that are already past the end.
		float fNewLastBeat = m_NoteDataEdit.GetLastBeat();
		bool bLastBeatIncreased = fNewLastBeat > m_Undo.GetLastBeat();
		bool bPassedTheEnd = fNewLastBeat > GetMaximumBeatForNewNote();
		if( bLastBeatIncreased && bPassedTheEnd )
		{
			Undo();
			m_bHasUndo = false;
			RString sError = CREATES_NOTES_PAST_END.GetValue() + "\n\n" + CHANGE_REVERTED.GetValue();
			ScreenPrompt::Prompt( SM_None, sError );
			return;
		}
	}
}

float ScreenEdit::GetMaximumBeatForNewNote() const
{
	switch( EDIT_MODE.GetValue() )
	{
	case EditMode_Practice:
	case EditMode_Home:
		{
			float fEndBeat = GAMESTATE->m_pCurSong->m_fLastBeat;

			// Round up to the next measure end.  Some songs end on weird beats 
			// mid-measure, and it's odd to have movement capped to these weird
			// beats.
			fEndBeat += BEATS_PER_MEASURE;
			fEndBeat = ftruncf( fEndBeat, (float)BEATS_PER_MEASURE );

			return fEndBeat;
		}
	case EditMode_Full:
		return FLT_MAX;
	default:
		ASSERT(0);	return 0;
	}
}

float ScreenEdit::GetMaximumBeatForMoving() const
{
	float fEndBeat = GetMaximumBeatForNewNote();

	// Jump to GetLastBeat even if it's past m_pCurSong->m_fLastBeat
	// so that users can delete garbage steps past then end that they have 
	// have inserted in a text editor.  Once they delete all steps on 
	// GetLastBeat() and move off of that beat, they won't be able to return.
	fEndBeat = max( fEndBeat, m_NoteDataEdit.GetLastBeat() );

	return fEndBeat;
}

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
