#include "global.h"
#include "ScreenEdit.h"
#include "PrefsManager.h"
#include "SongManager.h"
#include "ScreenManager.h"
#include "GameConstantsAndTypes.h"
#include "PrefsManager.h"
#include "GameManager.h"
#include "GameConstantsAndTypes.h"
#include "RageLog.h"
#include "GameSoundManager.h"
#include "GameState.h"
#include "InputMapper.h"
#include "RageLog.h"
#include "ThemeManager.h"
#include "ScreenMiniMenu.h"
#include "NoteSkinManager.h"
#include "Steps.h"
#include <utility>
#include "NoteFieldPositioning.h"
#include "NoteDataUtil.h"
#include "SongUtil.h"
#include "StepsUtil.h"
#include "Foreach.h"
#include "ScreenDimensions.h"
#include "ThemeMetric.h"
#include "PlayerState.h"
#include "ScreenTextEntry.h"
#include "Style.h"
#include "ActorUtil.h"
#include "ScreenPrompt.h"
#include "CommonMetrics.h"
#include "ScreenPlayerOptions.h"	// for SM_BackFromPlayerOptions
#include "ScreenSongOptions.h"	// for SM_BackFromSongOptions

//
// Defines specific to ScreenEdit
//
const float RECORD_HOLD_SECONDS = 0.3f;

#define EDIT_X			(SCREEN_CENTER_X)

#define PLAYER_X			(SCREEN_CENTER_X)
#define PLAYER_Y			(SCREEN_CENTER_Y)
#define PLAYER_HEIGHT		(360)
#define PLAYER_Y_STANDARD	(PLAYER_Y-PLAYER_HEIGHT/2)

#define PLAY_RECORD_HELP_TEXT	THEME->GetMetric(m_sName,"PlayRecordHelpText")

const AutoScreenMessage SM_BackFromMainMenu;
const AutoScreenMessage SM_BackFromAreaMenu;
const AutoScreenMessage SM_BackFromStepsInformation;
const AutoScreenMessage SM_BackFromEditOptions;
const AutoScreenMessage SM_BackFromSongInformation;
const AutoScreenMessage SM_BackFromBGChange;
const AutoScreenMessage SM_BackFromInsertAttack;
const AutoScreenMessage SM_BackFromInsertAttackModifiers;
const AutoScreenMessage SM_BackFromPrefs;
const AutoScreenMessage SM_BackFromCourseModeMenu;
const AutoScreenMessage SM_DoRevertToLastSave;
const AutoScreenMessage SM_DoUpdateTextInfo;
const AutoScreenMessage SM_BackFromBPMChange;
const AutoScreenMessage SM_BackFromStopChange;
const AutoScreenMessage SM_DoSaveAndExit;
const AutoScreenMessage SM_DoExit;

const CString INPUT_TIPS_TEXT = 
#if !defined(XBOX)
	"Up/Down:\n     change beat\n"
	"Left/Right:\n     change snap\n"
	"Number keys:\n     add/remove\n     tap note\n"
	"Create hold note:\n     Hold a number\n     while moving\n     Up or Down\n"
	"Space bar:\n     Set area\n     marker\n"
	"Enter:\n     Area Menu\n"
	"Escape:\n     Main Menu\n"
	"F1:\n     Show help\n";
#else
	"Up/Down:\n     change beat\n"
	"Left/Right:\n     change snap\n"
	"A/B/X/Y:\n     add/remove\n     tap note\n"
	"Create hold note:\n     Hold a button\n     while moving\n     Up or Down\n"
	"White:\n     Set area\n     marker\n"
	"Start:\n     Area Menu\n"
	"Select:\n     Main Menu\n"
	"Black:\n     Show\n     shortcuts\n";
#endif

#if defined(XBOX)
void ScreenEdit::InitEditMappings()
{
	/* XXX: fill this in */
}
#else
void ScreenEdit::InitEditMappings()
{
	g_EditMappings.Clear();

	g_EditMappings.button[EDIT_BUTTON_COLUMN_0][0] = DeviceInput(DEVICE_KEYBOARD, KEY_C1);
	g_EditMappings.button[EDIT_BUTTON_COLUMN_1][0] = DeviceInput(DEVICE_KEYBOARD, KEY_C2);
	g_EditMappings.button[EDIT_BUTTON_COLUMN_2][0] = DeviceInput(DEVICE_KEYBOARD, KEY_C3);
	g_EditMappings.button[EDIT_BUTTON_COLUMN_3][0] = DeviceInput(DEVICE_KEYBOARD, KEY_C4);
	g_EditMappings.button[EDIT_BUTTON_COLUMN_4][0] = DeviceInput(DEVICE_KEYBOARD, KEY_C5);
	g_EditMappings.button[EDIT_BUTTON_COLUMN_5][0] = DeviceInput(DEVICE_KEYBOARD, KEY_C6);
	g_EditMappings.button[EDIT_BUTTON_COLUMN_6][0] = DeviceInput(DEVICE_KEYBOARD, KEY_C7);
	g_EditMappings.button[EDIT_BUTTON_COLUMN_7][0] = DeviceInput(DEVICE_KEYBOARD, KEY_C8);
	g_EditMappings.button[EDIT_BUTTON_COLUMN_8][0] = DeviceInput(DEVICE_KEYBOARD, KEY_C9);
	g_EditMappings.button[EDIT_BUTTON_COLUMN_9][0] = DeviceInput(DEVICE_KEYBOARD, KEY_C0);

	g_EditMappings.button[EDIT_BUTTON_RIGHT_SIDE][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LALT);
	g_EditMappings.button[EDIT_BUTTON_RIGHT_SIDE][1] = DeviceInput(DEVICE_KEYBOARD, KEY_RALT);
	g_EditMappings.button[EDIT_BUTTON_LAY_MINE][0]   = DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT);
	g_EditMappings.button[EDIT_BUTTON_LAY_ATTACK][0] = DeviceInput(DEVICE_KEYBOARD, KEY_RSHIFT);


	g_EditMappings.button[EDIT_BUTTON_SCROLL_UP_LINE][0] = DeviceInput(DEVICE_KEYBOARD, KEY_UP);
	g_EditMappings.button[EDIT_BUTTON_SCROLL_UP_PAGE][0] = DeviceInput(DEVICE_KEYBOARD, KEY_PGUP);
	g_EditMappings.button[EDIT_BUTTON_SCROLL_DOWN_LINE][0] = DeviceInput(DEVICE_KEYBOARD, KEY_DOWN);
	g_EditMappings.button[EDIT_BUTTON_SCROLL_DOWN_PAGE][0] = DeviceInput(DEVICE_KEYBOARD, KEY_PGDN);
	g_EditMappings.button[EDIT_BUTTON_SCROLL_HOME][0] = DeviceInput(DEVICE_KEYBOARD, KEY_HOME);
	g_EditMappings.button[EDIT_BUTTON_SCROLL_END][0] = DeviceInput(DEVICE_KEYBOARD, KEY_END);

	g_EditMappings.button    [EDIT_BUTTON_SCROLL_SPEED_UP][0] = DeviceInput(DEVICE_KEYBOARD, KEY_UP);
	g_EditMappings.hold[EDIT_BUTTON_SCROLL_SPEED_UP][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LCTRL);
	g_EditMappings.hold[EDIT_BUTTON_SCROLL_SPEED_UP][1] = DeviceInput(DEVICE_KEYBOARD, KEY_RCTRL);

	g_EditMappings.button    [EDIT_BUTTON_SCROLL_SPEED_DOWN][0] = DeviceInput(DEVICE_KEYBOARD, KEY_DOWN);
	g_EditMappings.hold[EDIT_BUTTON_SCROLL_SPEED_DOWN][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LCTRL);
	g_EditMappings.hold[EDIT_BUTTON_SCROLL_SPEED_DOWN][1] = DeviceInput(DEVICE_KEYBOARD, KEY_RCTRL);

	g_EditMappings.button[EDIT_BUTTON_SCROLL_SELECT][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT);
	g_EditMappings.button[EDIT_BUTTON_SCROLL_SELECT][1] = DeviceInput(DEVICE_KEYBOARD, KEY_RSHIFT);

	g_EditMappings.button[EDIT_BUTTON_LAY_SELECT][0] = DeviceInput(DEVICE_KEYBOARD, KEY_SPACE);

	g_EditMappings.button[EDIT_BUTTON_SNAP_NEXT][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LEFT);
	g_EditMappings.button[EDIT_BUTTON_SNAP_PREV][0] = DeviceInput(DEVICE_KEYBOARD, KEY_RIGHT);

	g_EditMappings.button[EDIT_BUTTON_OPEN_EDIT_MENU][0] = DeviceInput(DEVICE_KEYBOARD, KEY_ESC);
	g_EditMappings.button[EDIT_BUTTON_OPEN_AREA_MENU][0] = DeviceInput(DEVICE_KEYBOARD, KEY_ENTER);
	g_EditMappings.button[EDIT_BUTTON_OPEN_AREA_MENU][1] = DeviceInput(DEVICE_KEYBOARD, KEY_KP_ENTER);
	g_EditMappings.button[EDIT_BUTTON_OPEN_BGA_MENU][0] = DeviceInput(DEVICE_KEYBOARD, KEY_Cb);
	g_EditMappings.button[EDIT_BUTTON_OPEN_COURSE_MENU][0] = DeviceInput(DEVICE_KEYBOARD, KEY_Cc);
	g_EditMappings.button[EDIT_BUTTON_OPEN_INPUT_HELP][0] = DeviceInput(DEVICE_KEYBOARD, KEY_F1);

	g_EditMappings.button[EDIT_BUTTON_PLAY_FROM_START][0] = DeviceInput(DEVICE_KEYBOARD, KEY_Cp);
	g_EditMappings.hold[EDIT_BUTTON_PLAY_FROM_START][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LCTRL);
	g_EditMappings.hold[EDIT_BUTTON_PLAY_FROM_START][1] = DeviceInput(DEVICE_KEYBOARD, KEY_RCTRL);
	g_EditMappings.button[EDIT_BUTTON_PLAY_FROM_CURSOR][0] = DeviceInput(DEVICE_KEYBOARD, KEY_Cp);
	g_EditMappings.hold[EDIT_BUTTON_PLAY_FROM_CURSOR][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT);
	g_EditMappings.hold[EDIT_BUTTON_PLAY_FROM_CURSOR][1] = DeviceInput(DEVICE_KEYBOARD, KEY_RSHIFT);
	g_EditMappings.button[EDIT_BUTTON_PLAY_SELECTION][0] = DeviceInput(DEVICE_KEYBOARD, KEY_Cp);
	g_EditMappings.button[EDIT_BUTTON_RECORD][0] = DeviceInput(DEVICE_KEYBOARD, KEY_Cr);
	g_EditMappings.hold[EDIT_BUTTON_RECORD][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LCTRL);
	g_EditMappings.hold[EDIT_BUTTON_RECORD][1] = DeviceInput(DEVICE_KEYBOARD, KEY_RCTRL);

	g_EditMappings.button[EDIT_BUTTON_INSERT][0] = DeviceInput(DEVICE_KEYBOARD, KEY_INSERT);
	g_EditMappings.button[EDIT_BUTTON_DELETE][0] = DeviceInput(DEVICE_KEYBOARD, KEY_DEL);

	g_EditMappings.button[EDIT_BUTTON_INSERT_SHIFT_PAUSES][0] = DeviceInput(DEVICE_KEYBOARD, KEY_INSERT);
	g_EditMappings.hold[EDIT_BUTTON_INSERT_SHIFT_PAUSES][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LCTRL);
	g_EditMappings.hold[EDIT_BUTTON_INSERT_SHIFT_PAUSES][1] = DeviceInput(DEVICE_KEYBOARD, KEY_RCTRL);
	g_EditMappings.button[EDIT_BUTTON_DELETE_SHIFT_PAUSES][0] = DeviceInput(DEVICE_KEYBOARD, KEY_DEL);
	g_EditMappings.hold[EDIT_BUTTON_DELETE_SHIFT_PAUSES][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LCTRL);
	g_EditMappings.hold[EDIT_BUTTON_DELETE_SHIFT_PAUSES][1] = DeviceInput(DEVICE_KEYBOARD, KEY_RCTRL);

	g_EditMappings.button[EDIT_BUTTON_TOGGLE_ASSIST_TICK][0] = DeviceInput(DEVICE_KEYBOARD, KEY_F4);
	g_EditMappings.button[EDIT_BUTTON_PLAY_SAMPLE_MUSIC][0] = DeviceInput(DEVICE_KEYBOARD, KEY_Cm);
	
	g_EditMappings.button[EDIT_BUTTON_OPEN_NEXT_STEPS][0] = DeviceInput(DEVICE_KEYBOARD, KEY_F6);
	g_EditMappings.button[EDIT_BUTTON_OPEN_PREV_STEPS][0] = DeviceInput(DEVICE_KEYBOARD, KEY_F5);

	g_EditMappings.button[EDIT_BUTTON_BPM_UP][0] = DeviceInput(DEVICE_KEYBOARD, KEY_F8);
	g_EditMappings.button[EDIT_BUTTON_BPM_DOWN][0] = DeviceInput(DEVICE_KEYBOARD, KEY_F7);
	g_EditMappings.button[EDIT_BUTTON_STOP_UP][0]  = DeviceInput(DEVICE_KEYBOARD, KEY_F10);
	g_EditMappings.button[EDIT_BUTTON_STOP_DOWN][0] = DeviceInput(DEVICE_KEYBOARD, KEY_F9);
	g_EditMappings.button[EDIT_BUTTON_OFFSET_UP][0]  = DeviceInput(DEVICE_KEYBOARD, KEY_F12);
	g_EditMappings.button[EDIT_BUTTON_OFFSET_DOWN][0] = DeviceInput(DEVICE_KEYBOARD, KEY_F11);
	g_EditMappings.button[EDIT_BUTTON_SAMPLE_START_UP][0] = DeviceInput(DEVICE_KEYBOARD, KEY_RBRACKET);
	g_EditMappings.button[EDIT_BUTTON_SAMPLE_START_DOWN][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LBRACKET);
	g_EditMappings.button[EDIT_BUTTON_SAMPLE_LENGTH_UP][0] = DeviceInput(DEVICE_KEYBOARD, KEY_RBRACKET);
	g_EditMappings.hold[EDIT_BUTTON_SAMPLE_LENGTH_UP][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT);
	g_EditMappings.hold[EDIT_BUTTON_SAMPLE_LENGTH_UP][1] = DeviceInput(DEVICE_KEYBOARD, KEY_RSHIFT);
	g_EditMappings.button[EDIT_BUTTON_SAMPLE_LENGTH_DOWN][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LBRACKET);
	g_EditMappings.hold[EDIT_BUTTON_SAMPLE_LENGTH_DOWN][0] = DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT);
	g_EditMappings.hold[EDIT_BUTTON_SAMPLE_LENGTH_DOWN][1] = DeviceInput(DEVICE_KEYBOARD, KEY_RSHIFT);

	g_EditMappings.button[EDIT_BUTTON_ADJUST_FINE][0] = DeviceInput(DEVICE_KEYBOARD, KEY_RALT);
	g_EditMappings.button[EDIT_BUTTON_ADJUST_FINE][1] = DeviceInput(DEVICE_KEYBOARD, KEY_LALT);
	
	g_PlayMappings.button[EDIT_BUTTON_RETURN_TO_EDIT][0] = DeviceInput(DEVICE_KEYBOARD, KEY_ESC);
	g_PlayMappings.button[EDIT_BUTTON_TOGGLE_ASSIST_TICK][0] = DeviceInput(DEVICE_KEYBOARD, KEY_F4);
	g_PlayMappings.button[EDIT_BUTTON_TOGGLE_AUTOPLAY][0] = DeviceInput(DEVICE_KEYBOARD, KEY_F8);
	g_PlayMappings.button[EDIT_BUTTON_OFFSET_UP][0] = DeviceInput(DEVICE_KEYBOARD, KEY_F12);
	g_PlayMappings.button[EDIT_BUTTON_OFFSET_DOWN][0] = DeviceInput(DEVICE_KEYBOARD, KEY_F11);

	g_RecordMappings.button[EDIT_BUTTON_RETURN_TO_EDIT][0] = DeviceInput(DEVICE_KEYBOARD, KEY_ESC);
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
					if( INPUTFILTER->IsBeingPressed(hDI) )
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
	switch( m_EditMode )
	{
	case MODE_EDITING: return &g_EditMappings;
	case MODE_PLAYING: return &g_PlayMappings;
	case MODE_RECORDING: return &g_RecordMappings;
	default: FAIL_M( ssprintf("%i",m_EditMode) );
	}
}

static Menu g_EditHelp(
	"ScreenMiniMenuEditHelp",
#if !defined(XBOX)
	MenuRow( -1, "PgUp/PgDn: jump measure",							false, true, 0, NULL ),
	MenuRow( -1, "Home/End: jump to first/last beat",				false, true, 0, NULL ),
	MenuRow( -1, "Ctrl + Up/Down: Change zoom",						false, true, 0, NULL ),
	MenuRow( -1, "Shift + Up/Down: Drag area marker",				false, true, 0, NULL ),
	MenuRow( -1, "P: Play selection",								false, true, 0, NULL ),
	MenuRow( -1, "Ctrl + P: Play whole song",						false, true, 0, NULL ),
	MenuRow( -1, "Shift + P: Play current beat to end",				false, true, 0, NULL ),
	MenuRow( -1, "Ctrl + R: Record",								false, true, 0, NULL ),
	MenuRow( -1, "F4: Toggle assist tick",							false, true, 0, NULL ),
	MenuRow( -1, "F5/F6: Next/prev steps of same StepsType",		false, true, 0, NULL ),
	MenuRow( -1, "F7/F8: Decrease/increase BPM at cur beat",		false, true, 0, NULL ),
	MenuRow( -1, "F9/F10: Decrease/increase stop at cur beat",		false, true, 0, NULL ),
	MenuRow( -1, "F11/F12: Decrease/increase music offset",			false, true, 0, NULL ),
			/* XXX: This would be better as a single submenu, to let people tweak
			* and play the sample several times (without having to re-enter the
			* menu each time), so it doesn't use a whole bunch of hotkeys. */
	MenuRow( -1, "[ and ]: Decrease/increase sample music start",	false, true, 0, NULL ),
	MenuRow( -1, "{ and }: Decrease/increase sample music length",	false, true, 0, NULL ),
	MenuRow( -1, "M: Play sample music",							false, true, 0, NULL ),
	MenuRow( -1, "B: Add/Edit Background Change",					false, true, 0, NULL ),
	MenuRow( -1, "Insert: Insert beat and shift down",				false, true, 0, NULL ),
	MenuRow( -1, "Ctrl + Insert: Shift BPM changes and stops down one beat",
																	false, true, 0, NULL ),
	MenuRow( -1, "Delete: Delete beat and shift up",				false, true, 0, NULL ),
	MenuRow( -1, "Ctrl + Delete: Shift BPM changes and stops up one beat",
																	false, true, 0, NULL ),
	MenuRow( -1, "Shift + number: Lay mine",						false, true, 0, NULL ),
	MenuRow( -1, "Alt + number: Add to/remove from right half",		false, true, 0, NULL )
#else
	MenuRow( -1, "L + Up/Down: Change zoom",						false, true, 0, NULL ),
	MenuRow( -1, "R + Up/Down: Drag area marker",					false, true, 0, NULL ),
	MenuRow( -1, "L + Select: Play selection",						false, true, 0, NULL ),
	MenuRow( -1, "R + Start: Play whole song",						false, true, 0, NULL ),
	MenuRow( -1, "R + Select: Record",								false, true, 0, NULL ),
	MenuRow( -1, "L + Black: Toggle assist tick",					false, true, 0, NULL ),
	MenuRow( -1, "R + White: Insert beat and shift down",			false, true, 0, NULL ),
	MenuRow( -1, "R + Black: Delete beat and shift up",				false, true, 0, NULL ),
	MenuRow( -1, "R + button: Lay mine",							false, true, 0, NULL ),
	MenuRow( -1, "L + button: Add to/remove from right half",		false, true, 0, NULL )
#endif
);

static Menu g_MainMenu(
	"ScreenMiniMenuMainMenu",
	MenuRow( ScreenEdit::edit_steps_information,	"Edit Steps Information",		true, true, 0, NULL ),
	MenuRow( ScreenEdit::play_whole_song,			"Play Whole Song",				true, true, 0, NULL ),
	MenuRow( ScreenEdit::play_current_beat_to_end,	"Play Current Beat to End",		true, true, 0, NULL ),
	MenuRow( ScreenEdit::save,						"Save",							true, true, 0, NULL ),
	MenuRow( ScreenEdit::revert_to_last_save,		"Revert to Last Save",			true, true, 0, NULL ),
	MenuRow( ScreenEdit::player_options,			"Player Options",				true, true, 0, NULL ),
	MenuRow( ScreenEdit::song_options,				"Song Options",					true, false, 0, NULL ),
	MenuRow( ScreenEdit::edit_song_info,			"Edit Song Info",				true, false, 0, NULL ),
	MenuRow( ScreenEdit::edit_bpm,					"Edit BPM Change",				true, false, 0, NULL ),
	MenuRow( ScreenEdit::edit_stop,					"Edit Stop",					true, false, 0, NULL ),
	MenuRow( ScreenEdit::edit_bg_change,			"Add/Edit Background Change",	true, false, 0, NULL ),
	MenuRow( ScreenEdit::play_preview_music,		"Play Preview Music",			true, false, 0, NULL ),
	MenuRow( ScreenEdit::preferences,				"Preferences",					true, false, 0, NULL ),
	MenuRow( ScreenEdit::exit,						"Exit",							true, true, 0, NULL )
);

static Menu g_AreaMenu(
	"ScreenMiniMenuAreaMenu",
	MenuRow( ScreenEdit::cut,					"Cut",								true, true, 0, NULL ),
	MenuRow( ScreenEdit::copy,					"Copy",								true, true, 0, NULL ),
	MenuRow( ScreenEdit::paste_at_current_beat,	"Paste at current beat",			true, true, 0, NULL ),
	MenuRow( ScreenEdit::paste_at_begin_marker,	"Paste at begin marker",			true, true, 0, NULL ),
	MenuRow( ScreenEdit::clear,					"Clear area",						true, true, 0, NULL ),
	MenuRow( ScreenEdit::quantize,				"Quantize",							true, true, 0, "4th","8th","12th","16th","24th","32nd","48th","64th","192nd"),
	MenuRow( ScreenEdit::turn,					"Turn",								true, true, 0, "Left","Right","Mirror","Shuffle","SuperShuffle" ),
	MenuRow( ScreenEdit::transform,				"Transform",						true, true, 0, "NoHolds","NoMines","Little","Wide","Big","Quick","Skippy","Mines","Echo","Stomp","Planted","Floored","Twister","NoJumps","NoHands","NoQuads" ),
	MenuRow( ScreenEdit::alter,					"Alter",							true, true, 0, "Backwards","Swap Sides","Copy Left To Right","Copy Right To Left","Clear Left","Clear Right","Collapse To One","Collapse Left","Shift Left","Shift Right" ),
	MenuRow( ScreenEdit::tempo,					"Tempo",							true, true, 0, "Compress 2x","Compress 3->2","Compress 4->3","Expand 3->4","Expand 2->3","Expand 2x" ),
	MenuRow( ScreenEdit::play,					"Play selection",					true, true, 0, NULL ),
	MenuRow( ScreenEdit::record,				"Record in selection",				true, true, 0, NULL ),
	MenuRow( ScreenEdit::insert_and_shift,		"Insert beat and shift down",		true, true, 0, NULL ),
	MenuRow( ScreenEdit::delete_and_shift,		"Delete beat and shift up",			true, true, 0, NULL ),
	MenuRow( ScreenEdit::shift_pauses_forward,	"Shift pauses and BPM changes down",true, false, 0, NULL ),
	MenuRow( ScreenEdit::shift_pauses_backward,	"Shift pauses and BPM changes up",	true, false, 0, NULL ),
	MenuRow( ScreenEdit::convert_beat_to_pause,	"Convert beats to pause",			true, false, 0, NULL ),
	MenuRow( ScreenEdit::convert_pause_to_beat,	"Convert pause to beats",			true, false, 0, NULL ),
	MenuRow( ScreenEdit::undo,					"Undo",								true, true, 0, NULL )
);

static Menu g_StepsInformation(
	"ScreenMiniMenuStepsInformation",
	MenuRow( ScreenEdit::difficulty,	"Difficulty",		true, true,  0, "BEGINNER","EASY","MEDIUM","HARD","CHALLENGE","EDIT" ),
	MenuRow( ScreenEdit::meter,			"Meter",			true, true,  0, "1","2","3","4","5","6","7","8","9","10","11","12","13" ),
	MenuRow( ScreenEdit::description,	"Description",		true, true,  0, NULL ),
	MenuRow( ScreenEdit::predict_meter,	"Predicted Meter",	false, true, 0, NULL ),
	MenuRow( ScreenEdit::tap_notes,		"Tap Steps",		false, true, 0, NULL ),
	MenuRow( ScreenEdit::jumps,			"Jumps",			false, true, 0, NULL ),
	MenuRow( ScreenEdit::hands,			"Hands",			false, true, 0, NULL ),
	MenuRow( ScreenEdit::quads,			"Quads",			false, true, 0, NULL ),
	MenuRow( ScreenEdit::holds,			"Holds",			false, true, 0, NULL ),
	MenuRow( ScreenEdit::mines,			"Mines",			false, true, 0, NULL ),
	MenuRow( ScreenEdit::stream,		"Stream",			false, true, 0, NULL ),
	MenuRow( ScreenEdit::voltage,		"Voltage",			false, true, 0, NULL ),
	MenuRow( ScreenEdit::air,			"Air",				false, true, 0, NULL ),
	MenuRow( ScreenEdit::freeze,		"Freeze",			false, true, 0, NULL ),
	MenuRow( ScreenEdit::chaos,			"Chaos",			false, true, 0, NULL )
);

static Menu g_SongInformation(
	"ScreenMiniMenuSongInfomation",
	MenuRow( ScreenEdit::main_title,					"Main title",					true, true, 0, NULL ),
	MenuRow( ScreenEdit::sub_title,						"Sub title",					true, true, 0, NULL ),
	MenuRow( ScreenEdit::artist,						"Artist",						true, true, 0, NULL ),
	MenuRow( ScreenEdit::credit,						"Credit",						true, true, 0, NULL ),
	MenuRow( ScreenEdit::main_title_transliteration,	"Main title transliteration",	true, true, 0, NULL ),
	MenuRow( ScreenEdit::sub_title_transliteration,		"Sub title transliteration",	true, true, 0, NULL ),
	MenuRow( ScreenEdit::artist_transliteration,		"Artist transliteration",		true, true, 0, NULL )
);

static Menu g_BackgroundChange(
	"ScreenMiniMenuBackgroundChange",
	MenuRow( ScreenEdit::rate,						"Rate (applies to new adds)",			true, true, 10, "0%","10%","20%","30%","40%","50%","60%","70%","80%","90%","100%","120%","140%","160%","180%","200%" ),
	MenuRow( ScreenEdit::fade_last,					"Fade Last (applies to new adds)",		true, true, 0, "NO","YES" ),
	MenuRow( ScreenEdit::rewind_movie,				"Rewind Movie (applies to new adds)",	true, true, 0, "NO","YES" ),
	MenuRow( ScreenEdit::loop,						"Loop (applies to new adds)",			true, true, 1, "NO","YES" ),
	MenuRow( ScreenEdit::add_random,				"Add Change to random",					true, true, 0, NULL ),
	MenuRow( ScreenEdit::add_song_bganimation,		"Add Change to song BGAnimation",		true, true, 0, NULL ),
	MenuRow( ScreenEdit::add_song_movie,			"Add Change to song Movie",				true, true, 0, NULL ),
	MenuRow( ScreenEdit::add_song_still,			"Add Change to song Still",				true, true, 0, NULL ),
	MenuRow( ScreenEdit::add_global_random_movie,	"Add Change to global Random Movie",	true, true, 0, NULL ),
	MenuRow( ScreenEdit::add_global_bganimation,	"Add Change to global BGAnimation",		true, true, 0, NULL ),
	MenuRow( ScreenEdit::add_global_visualization, "Add Change to global Visualization",	true, true, 0, NULL ),
	MenuRow( ScreenEdit::delete_change,				"Remove Change",						true, true, 0, NULL )
);

static Menu g_Prefs(
	"ScreenMiniMenuPreferences",
	MenuRow( ScreenEdit::pref_show_bgs_play,		"Show BGChanges during Play/Record",	true, true, 0, "NO","YES" )
);

static Menu g_InsertAttack(
	"ScreenMiniMenuInsertAttack",
	MenuRow( -1, "Duration seconds",			true, true, 3, "5","10","15","20","25","30","35","40","45" ),
	MenuRow( -1, "Set modifiers",				true, true, 0, "PRESS START" )
);

static Menu g_CourseMode(
	"ScreenMiniMenuCourseDisplay",
	MenuRow( -1, "Play mods from course",		true, true, 0, NULL )
);

// HACK: need to remember the track we're inserting on so
// that we can lay the attack note after coming back from
// menus.
int g_iLastInsertAttackTrack = -1;
float g_fLastInsertAttackDurationSeconds = -1;

REGISTER_SCREEN_CLASS( ScreenEdit );
ScreenEdit::ScreenEdit( CString sName ) : ScreenWithMenuElements( sName )
{
	LOG->Trace( "ScreenEdit::ScreenEdit()" );

	PREV_SCREEN.Load( sName, "PrevScreen" );
	
	ASSERT( GAMESTATE->m_pCurSong );
	ASSERT( GAMESTATE->m_pCurSteps[0] );
}

void ScreenEdit::Init()
{
	ScreenWithMenuElements::Init();

	InitEditMappings();

	/* We do this ourself. */
	SOUND->HandleSongTimer( false );


	// save the originals for reverting later
	CopyToLastSave();

	m_CurrentAction = MAIN_MENU_CHOICE_INVALID;

	// set both players to joined so the credit message doesn't show
	FOREACH_PlayerNumber( p )
		GAMESTATE->m_bSideIsJoined[p] = true;
	SCREENMAN->RefreshCreditsMessages();

	m_pSong = GAMESTATE->m_pCurSong;
	m_pSteps = GAMESTATE->m_pCurSteps[PLAYER_1];
	m_pAttacksFromCourse = NULL;


	GAMESTATE->m_bPastHereWeGo = false;
	GAMESTATE->m_bEditing = true;
	GAMESTATE->m_fSongBeat = 0;
	m_fTrailingBeat = GAMESTATE->m_fSongBeat;

	/* Not all games have a noteskin named "note" ... */
	if( NOTESKIN->DoesNoteSkinExist("note") )
		GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerOptions.m_sNoteSkin = "note";	// change noteskin before loading all of the edit Actors
	// Reset player options, but don't reset the current NoteSkin
	CString sNoteSkin = GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerOptions.m_sNoteSkin;
	GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerOptions.Init();	// don't allow weird options in editor.  It doesn't handle reverse well.
	GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerOptions.m_sNoteSkin = sNoteSkin;

	GAMESTATE->ResetNoteSkins();
	GAMESTATE->StoreSelectedOptions();

	g_iShiftAnchor = -1;

	TransitionEditMode( MODE_EDITING );
	
	this->AddChild( &m_Background );

	m_SnapDisplay.SetXY( EDIT_X, PLAYER_Y_STANDARD );
	m_SnapDisplay.Load( PLAYER_1 );
	m_SnapDisplay.SetZoom( 0.5f );
	this->AddChild( &m_SnapDisplay );

	NoteData noteData;
	m_pSteps->GetNoteData( noteData );
	m_NoteDataEdit.CopyAll( noteData );

	m_NoteFieldEdit.SetXY( EDIT_X, PLAYER_Y );
	m_NoteFieldEdit.SetZoom( 0.5f );
	m_NoteFieldEdit.Init( GAMESTATE->m_pPlayerState[PLAYER_1], PLAYER_HEIGHT*2 );
	m_NoteFieldEdit.Load( &m_NoteDataEdit, -240, 800 );
	this->AddChild( &m_NoteFieldEdit );

	m_NoteDataRecord.CopyAll( noteData );
	m_NoteFieldRecord.SetXY( EDIT_X, PLAYER_Y );
	m_NoteFieldRecord.Init( GAMESTATE->m_pPlayerState[PLAYER_1], 350 );
	m_NoteFieldRecord.Load( &m_NoteDataRecord, -150, 350 );
	this->AddChild( &m_NoteFieldRecord );

	m_Clipboard.SetNumTracks( m_NoteDataEdit.GetNumTracks() );

	m_Undo.SetNumTracks( m_NoteDataEdit.GetNumTracks() );


	/* XXX: Do we actually have to send real note data here, and to m_NoteFieldRecord? 
	 * (We load again on play/record.) */
	m_Player.Init( "Player", GAMESTATE->m_pPlayerState[PLAYER_1], NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL );
	m_Player.Load( noteData );
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
	m_soundMarker.Load(		THEME->GetPathS("ScreenEdit","marker"), true );
	m_soundSwitch.Load(		THEME->GetPathS("ScreenEdit","switch") );
	m_soundSave.Load(		THEME->GetPathS("ScreenEdit","save") );


	m_soundMusic.Load( m_pSong->GetMusicPath() );

	m_soundAssistTick.Load( THEME->GetPathS("ScreenEdit","assist tick"), true );

	this->HandleScreenMessage( SM_DoUpdateTextInfo );
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
	if( !GAMESTATE->m_SongOptions.m_bAssistTick || m_EditMode != MODE_PLAYING )
		return;
			
	/* Sound cards have a latency between when a sample is Play()ed and when the sound
	 * will start coming out the speaker.  Compensate for this by boosting fPositionSeconds
	 * ahead.  This is just to make sure that we request the sound early enough for it to
	 * come out on time; the actual precise timing is handled by SetStartTime. */
	float fPositionSeconds = GAMESTATE->m_fMusicSeconds;
	fPositionSeconds += SOUND->GetPlayLatency() + (float)TICK_EARLY_SECONDS + 0.250f;
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
		p.StartTime = GAMESTATE->m_LastBeatUpdate + (fSecondsUntil - (float)TICK_EARLY_SECONDS);
		m_soundAssistTick.Play( &p );
	}
}

void ScreenEdit::PlayPreviewMusic()
{
	SOUND->PlayMusic("");
	SOUND->PlayMusic( m_pSong->GetMusicPath(), false,
		m_pSong->m_fMusicSampleStartSeconds,
		m_pSong->m_fMusicSampleLengthSeconds,
		1.5f );
}

void ScreenEdit::Update( float fDeltaTime )
{
	if( m_soundMusic.IsPlaying() )
	{
		RageTimer tm;
		const float fSeconds = m_soundMusic.GetPositionSeconds( NULL, &tm );
		GAMESTATE->UpdateSongPosition( fSeconds, GAMESTATE->m_pCurSong->m_Timing, tm );
	}

	if( m_EditMode == MODE_RECORDING  )	
	{
		// add or extend holds

		for( int t=0; t<GAMESTATE->GetCurrentStyle()->m_iColsPerPlayer; t++ )	// for each track
		{
			StyleInput StyleI( PLAYER_1, t );
			float fSecsHeld = INPUTMAPPER->GetSecsHeld( StyleI );

			if( fSecsHeld > RECORD_HOLD_SECONDS && GAMESTATE->m_fSongBeat > 0 )
			{
				// add or extend hold
				const float fHoldStartSeconds = m_soundMusic.GetPositionSeconds() - fSecsHeld;

				float fStartBeat = max( 0, m_pSong->GetBeatFromElapsedTime( fHoldStartSeconds ) );
				float fEndBeat = max( fStartBeat, GAMESTATE->m_fSongBeat );

				// Round hold start and end to the nearest snap interval
				fStartBeat = Quantize( fStartBeat, NoteTypeToBeat(m_SnapDisplay.GetNoteType()) );
				fEndBeat = Quantize( fEndBeat, NoteTypeToBeat(m_SnapDisplay.GetNoteType()) );

				// create a new hold note
				m_NoteDataRecord.AddHoldNote( t, BeatToNoteRow(fStartBeat), BeatToNoteRow(fEndBeat), TAP_ORIGINAL_HOLD_HEAD );
			}
		}
	}

	if( m_EditMode == MODE_RECORDING  ||  m_EditMode == MODE_PLAYING )
	{
		// check for end of playback/record

		if( GAMESTATE->m_fSongBeat > NoteRowToBeat(m_NoteFieldEdit.m_iEndMarker) + 4 )		// give a one measure lead out
		{
			TransitionEditMode( MODE_EDITING );
			GAMESTATE->m_fSongBeat = NoteRowToBeat( m_NoteFieldEdit.m_iEndMarker );
		}
	}


	//LOG->Trace( "ScreenEdit::Update(%f)", fDeltaTime );
	ScreenWithMenuElements::Update( fDeltaTime );


	// Update trailing beat
	float fDelta = GAMESTATE->m_fSongBeat - m_fTrailingBeat;
	if( fabsf(fDelta) < 10 )
		fapproach( m_fTrailingBeat, GAMESTATE->m_fSongBeat,
			fDeltaTime*40 / GAMESTATE->m_pPlayerState[PLAYER_1]->m_CurrentPlayerOptions.m_fScrollSpeed );
	else
		fapproach( m_fTrailingBeat, GAMESTATE->m_fSongBeat,
			fabsf(fDelta) * fDeltaTime*5 );

	PlayTicks();
}

void ScreenEdit::UpdateTextInfo()
{
	if( m_pSteps == NULL )
		return;

	int iNumTaps = m_NoteDataEdit.GetNumTapNotes();
	int iNumJumps = m_NoteDataEdit.GetNumJumps();
	int iNumHands = m_NoteDataEdit.GetNumHands();
	int iNumHolds = m_NoteDataEdit.GetNumHoldNotes();
	int iNumMines = m_NoteDataEdit.GetNumMines();

	CString sNoteType = NoteTypeToString(m_SnapDisplay.GetNoteType()) + " notes";

	CString sText;
	sText += ssprintf( "Current beat:\n  %.3f\n",		GAMESTATE->m_fSongBeat );
	sText += ssprintf( "Current sec:\n  %.3f\n",		m_pSong->GetElapsedTimeFromBeat(GAMESTATE->m_fSongBeat) );
	sText += ssprintf( "Snap to:\n  %s\n",				sNoteType.c_str() );
	sText += ssprintf( "Selection beat:\n  %s\n  %s\n",	m_NoteFieldEdit.m_iBeginMarker==-1 ? "----" : ssprintf("%.3f",NoteRowToBeat(m_NoteFieldEdit.m_iBeginMarker)).c_str(), m_NoteFieldEdit.m_iEndMarker==-1 ? "----" : ssprintf("%.3f",NoteRowToBeat(m_NoteFieldEdit.m_iEndMarker)).c_str() );
	if( !HOME_EDIT_MODE )
		sText += ssprintf( "Difficulty:\n  %s\n",		DifficultyToString( m_pSteps->GetDifficulty() ).c_str() );
	if( !HOME_EDIT_MODE )
	{
		sText += ssprintf( "Description:\n  %s\n",		GAMESTATE->m_pCurSteps[PLAYER_1] ? GAMESTATE->m_pCurSteps[PLAYER_1]->GetDescription().c_str() : "no description" );
		sText += ssprintf( "Main title:\n  %s\n", m_pSong->m_sMainTitle.c_str() );
		sText += ssprintf( "Sub title:\n  %s\n", m_pSong->m_sSubTitle.c_str() );
	}
	sText += ssprintf( "Tap steps:\n  %d\n", iNumTaps );
	sText += ssprintf( "Jumps:\n  %d\n", iNumJumps );
	sText += ssprintf( "Hands:\n  %d\n", iNumHands );
	sText += ssprintf( "Holds:\n  %d\n", iNumHolds );
	sText += ssprintf( "Mines:\n  %d\n", iNumMines );
	if( !HOME_EDIT_MODE )
		sText += ssprintf( "Beat 0 Offset:\n  %.3f secs\n",	m_pSong->m_Timing.m_fBeat0OffsetInSeconds );
	if( !HOME_EDIT_MODE )
		sText += ssprintf( "Preview Start:\n  %.2f secs\n",	m_pSong->m_fMusicSampleStartSeconds );
	if( !HOME_EDIT_MODE )
		sText += ssprintf( "Preview Length:\n  %.2f secs\n",m_pSong->m_fMusicSampleLengthSeconds );

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

void ScreenEdit::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
//	LOG->Trace( "ScreenEdit::Input()" );

	if( m_In.IsTransitioning() || m_Out.IsTransitioning() )
		return;

	const DeviceInput& di = DeviceI;

	EditButton EditB;
	DeviceToEdit( di, EditB );
	switch( m_EditMode )
	{
	case MODE_EDITING:		InputEdit( di, type, GameI, MenuI, StyleI, EditB );	break;
	case MODE_RECORDING:	InputRecord( di, type, GameI, MenuI, StyleI, EditB );	break;
	case MODE_PLAYING:		InputPlay( di, type, GameI, MenuI, StyleI, EditB );	break;
	default:	ASSERT(0);
	}

	/* Make sure the displayed time is up-to-date after possibly changing something,
	 * so it doesn't feel lagged. */
	UpdateTextInfo();
}


void ScreenEdit::InputEdit( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI, EditButton EditB )
{
	if( type == IET_LEVEL_CHANGED )
		return;		// don't care

	if( type == IET_RELEASE )
	{
		if( EditPressed( EDIT_BUTTON_SCROLL_SELECT, DeviceI ) )
			g_iShiftAnchor = -1;
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
			if( type != IET_FIRST_PRESS )
				break;	// We only care about first presses

			int iCol = EditB - EDIT_BUTTON_COLUMN_0;


			// Alt + number = input to right half
			if( EditIsBeingPressed(EDIT_BUTTON_RIGHT_SIDE) )
				iCol += m_NoteDataEdit.GetNumTracks()/2;


			const float fSongBeat = GAMESTATE->m_fSongBeat;
			const int iSongIndex = BeatToNoteRow( fSongBeat );

			if( iCol >= m_NoteDataEdit.GetNumTracks() )	// this button is not in the range of columns for this Style
				break;

			// check for to see if the user intended to remove a HoldNote
			int iHeadRow;
			if( m_NoteDataEdit.IsHoldNoteAtBeat( iCol, iSongIndex, &iHeadRow ) )
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
			else if( EditIsBeingPressed(EDIT_BUTTON_LAY_MINE) )
			{
				m_soundAddNote.Play();
				SaveUndo();
				m_NoteDataEdit.SetTapNote(iCol, iSongIndex, TAP_ORIGINAL_MINE );
				CheckNumberOfNotesAndUndo();
			}
			else if( EditIsBeingPressed(EDIT_BUTTON_LAY_ATTACK) )
			{
				g_iLastInsertAttackTrack = iCol;
				SCREENMAN->MiniMenu( &g_InsertAttack, SM_BackFromInsertAttack );
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
			float& fScrollSpeed = GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerOptions.m_fScrollSpeed;
			float fNewScrollSpeed = fScrollSpeed;

			if( DeviceI.button == KEY_UP )
			{
				if( fScrollSpeed == 4 )
					fNewScrollSpeed = 2;
				else if( fScrollSpeed == 2 )
					fNewScrollSpeed = 1;
			}
			else if( DeviceI.button == KEY_DOWN )
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
				GAMESTATE->StoreSelectedOptions();
			}
			break;
		}

		break;
	case EDIT_BUTTON_SCROLL_UP_LINE:
	case EDIT_BUTTON_SCROLL_UP_PAGE:
	case EDIT_BUTTON_SCROLL_DOWN_LINE:
	case EDIT_BUTTON_SCROLL_DOWN_PAGE:
	case EDIT_BUTTON_SCROLL_HOME:
	case EDIT_BUTTON_SCROLL_END:
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
			case EDIT_BUTTON_SCROLL_HOME:
				fBeatsToMove = -GAMESTATE->m_fSongBeat;
				break;
			case EDIT_BUTTON_SCROLL_END:
				fBeatsToMove = m_NoteDataEdit.GetLastBeat() - GAMESTATE->m_fSongBeat;
				break;
			}

			const float fStartBeat = GAMESTATE->m_fSongBeat;
			float fEndBeat = max( GAMESTATE->m_fSongBeat + fBeatsToMove, 0 );
			fEndBeat = Quantize( fEndBeat , NoteTypeToBeat(m_SnapDisplay.GetNoteType()) );
			GAMESTATE->m_fSongBeat = fEndBeat;

			// check to see if they're holding a button
			for( int n=0; n<=9; n++ )	// for each number key
			{
				int iCol = n;

				// Ctrl + number = input to right half
				if( EditIsBeingPressed(EDIT_BUTTON_RIGHT_SIDE) )
					iCol += m_NoteDataEdit.GetNumTracks()/2;

				if( iCol >= m_NoteDataEdit.GetNumTracks() )
					continue;	// skip

				EditButton b = EditButton(EDIT_BUTTON_COLUMN_0+n);
				if( !EditIsBeingPressed(b) )
					continue;

				// create a new hold note
				int iStartRow = BeatToNoteRow( min(fStartBeat, fEndBeat) );
				int iEndRow = BeatToNoteRow( max(fStartBeat, fEndBeat) );

				iStartRow = max( iStartRow, 0 );
				iEndRow = max( iEndRow, 0 );

				// Don't SaveUndo.  We want to undo the whole hold, not just the last segment
				// that the user made.  Dragging the hold bigger can only absorb and remove
				// other taps, so dragging won't cause us to exceed the note limit.
				m_NoteDataEdit.AddHoldNote( iCol, iStartRow, iEndRow, TAP_ORIGINAL_HOLD_HEAD );
			}

			if( EditIsBeingPressed(EDIT_BUTTON_SCROLL_SELECT) )
			{
				/* Shift is being held. 
				 *
				 * If this is the first time we've moved since shift was depressed,
				 * the old position (before this move) becomes the start pos: */
				int iEndBeat = BeatToNoteRow( fEndBeat );
				if( g_iShiftAnchor == -1 )
					g_iShiftAnchor = BeatToNoteRow(fStartBeat);
				
				if( iEndBeat == g_iShiftAnchor )
				{
					/* We're back at the anchor, so we have nothing selected. */
					m_NoteFieldEdit.m_iBeginMarker = m_NoteFieldEdit.m_iEndMarker = -1;
				}
				else
				{
					m_NoteFieldEdit.m_iBeginMarker = g_iShiftAnchor;
					m_NoteFieldEdit.m_iEndMarker = iEndBeat;
					if( m_NoteFieldEdit.m_iBeginMarker > m_NoteFieldEdit.m_iEndMarker )
						swap( m_NoteFieldEdit.m_iBeginMarker, m_NoteFieldEdit.m_iEndMarker );
				}
			}

			m_soundChangeLine.Play();
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
			g_AreaMenu.rows[paste_at_current_beat].bEnabled = m_Clipboard.GetLastBeat() != 0;
			g_AreaMenu.rows[paste_at_begin_marker].bEnabled = m_Clipboard.GetLastBeat() != 0 && m_NoteFieldEdit.m_iBeginMarker!=-1;
			g_AreaMenu.rows[clear].bEnabled = bAreaSelected;
			g_AreaMenu.rows[quantize].bEnabled = bAreaSelected;
			g_AreaMenu.rows[turn].bEnabled = bAreaSelected;
			g_AreaMenu.rows[transform].bEnabled = bAreaSelected;
			g_AreaMenu.rows[alter].bEnabled = bAreaSelected;
			g_AreaMenu.rows[tempo].bEnabled = bAreaSelected;
			g_AreaMenu.rows[play].bEnabled = bAreaSelected;
			g_AreaMenu.rows[record].bEnabled = bAreaSelected;
			g_AreaMenu.rows[convert_beat_to_pause].bEnabled = bAreaSelected;
			g_AreaMenu.rows[undo].bEnabled = m_Undo.GetLastBeat() != 0;
			SCREENMAN->MiniMenu( &g_AreaMenu, SM_BackFromAreaMenu );
		}
		break;
	case EDIT_BUTTON_OPEN_EDIT_MENU:
		SCREENMAN->MiniMenu( &g_MainMenu, SM_BackFromMainMenu );
		break;
	case EDIT_BUTTON_OPEN_INPUT_HELP:
		SCREENMAN->MiniMenu( &g_EditHelp, SM_None );
		break;
	case EDIT_BUTTON_TOGGLE_ASSIST_TICK:
		GAMESTATE->m_SongOptions.m_bAssistTick ^= 1;
		break;
	case EDIT_BUTTON_OPEN_NEXT_STEPS:
	case EDIT_BUTTON_OPEN_PREV_STEPS:
		{
			// don't keep undo when changing Steps
			m_Undo.ClearAll();

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
			SCREENMAN->SystemMessage( ssprintf(
				"Switched to %s %s '%s'",
				GAMEMAN->StepsTypeToString( pSteps->m_StepsType ).c_str(),
				DifficultyToString( pSteps->GetDifficulty() ).c_str(),
				pSteps->GetDescription().c_str() ) );
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
				fDeltaBPM /= 2;
			else switch( type )
			{
			case IET_SLOW_REPEAT:	fDeltaBPM *= 10;	break;
			case IET_FAST_REPEAT:	fDeltaBPM *= 40;	break;
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
				fStopDelta /= 20; /* 1ms */
			else switch( type )
			{
			case IET_SLOW_REPEAT:	fStopDelta *= 10;	break;
			case IET_FAST_REPEAT:	fStopDelta *= 40;	break;
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
				fOffsetDelta /= 20; /* 1ms */
			else switch( type )
			{
			case IET_SLOW_REPEAT:	fOffsetDelta *= 10;	break;
			case IET_FAST_REPEAT:	fOffsetDelta *= 40;	break;
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
			switch( type )
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
	case EDIT_BUTTON_OPEN_BGA_MENU:
		HandleMainMenuChoice( edit_bg_change );
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
			for( unsigned e = 0; e < crs->m_entries.size(); ++e )
			{
				if( crs->m_entries[e].type != COURSE_ENTRY_FIXED )
					continue;
				if( crs->m_entries[e].pSong != m_pSong )
					continue;
				bUsesThisSong = true;
			}

			if( bUsesThisSong )
			{
				g_CourseMode.rows[0].choices.push_back( crs->GetFullDisplayTitle() );
				if( crs == m_pAttacksFromCourse )
					g_CourseMode.rows[0].iDefaultChoice = g_CourseMode.rows[0].choices.size()-1;
			}
		}

		SCREENMAN->MiniMenu( &g_CourseMode, SM_BackFromCourseModeMenu );
		break;
	}
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
	case EDIT_BUTTON_RECORD:
		if( m_NoteFieldEdit.m_iBeginMarker!=-1 && m_NoteFieldEdit.m_iEndMarker!=-1 )
			HandleAreaMenuChoice( record );
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
	}
}

void ScreenEdit::InputRecord( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI, EditButton EditB )
{
	if( EditB == EDIT_BUTTON_RETURN_TO_EDIT )
	{
		TransitionEditMode( MODE_EDITING );
		return;
	}	

	if( StyleI.player != PLAYER_1 )
		return;		// ignore

	const int iCol = StyleI.col;

	switch( type )
	{
	case IET_FIRST_PRESS:
		{
			// Add a tap

			float fBeat = GAMESTATE->m_fSongBeat;
			fBeat = Quantize( fBeat, NoteTypeToBeat(m_SnapDisplay.GetNoteType()) );
			
			const int iRow = BeatToNoteRow( fBeat );
			if( iRow < 0 )
				break;

			m_NoteDataRecord.SetTapNote(iCol, iRow, TAP_ORIGINAL_TAP);
			m_NoteFieldRecord.Step( iCol, TNS_MARVELOUS );
		}
		break;
	case IET_SLOW_REPEAT:
	case IET_FAST_REPEAT:
	case IET_RELEASE:
		// don't add or extend holds here
		break;
	}
}

void ScreenEdit::InputPlay( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI, EditButton EditB )
{
	if( type != IET_FIRST_PRESS )
		return;

	switch( EditB )
	{
	case EDIT_BUTTON_RETURN_TO_EDIT:
		TransitionEditMode( MODE_EDITING );
		break;
	case EDIT_BUTTON_TOGGLE_ASSIST_TICK:
		GAMESTATE->m_SongOptions.m_bAssistTick ^= 1;
		break;
	case EDIT_BUTTON_TOGGLE_AUTOPLAY:
		{
			PREFSMAN->m_bAutoPlay = !PREFSMAN->m_bAutoPlay;
			FOREACH_HumanPlayer( p )
				GAMESTATE->m_pPlayerState[p]->m_PlayerController = PREFSMAN->m_bAutoPlay?PC_AUTOPLAY:PC_HUMAN;
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
				fOffsetDelta /= 20; /* 1ms */
			else switch( type )
			{
			case IET_SLOW_REPEAT:	fOffsetDelta *= 10;	break;
			case IET_FAST_REPEAT:	fOffsetDelta *= 40;	break;
			}

			m_pSong->m_Timing.m_fBeat0OffsetInSeconds += fOffsetDelta;
		}
		break;
	}

	switch( StyleI.player )
	{
	case PLAYER_1:	
		if( !PREFSMAN->m_bAutoPlay )
			m_Player.Step( StyleI.col, DeviceI.ts ); 
		break;
	}

}

void ScreenEdit::TransitionEditMode( EditMode em )
{
	EditMode old = m_EditMode;
	
	switch( em )
	{
	case MODE_EDITING:
		m_sprOverlay->PlayCommand( "Edit" );

		/* Important: people will stop playing, change the BG and start again; make sure we reload */
		m_Background.Unload();
		m_Foreground.Unload();

		GAMESTATE->m_bPastHereWeGo = false;
		m_soundMusic.StopPlaying();
		m_soundAssistTick.StopPlaying(); /* Stop any queued assist ticks. */

		/* Make sure we're snapped. */
		GAMESTATE->m_fSongBeat = Quantize( GAMESTATE->m_fSongBeat, NoteTypeToBeat(m_SnapDisplay.GetNoteType()) );

		/* Playing and recording have lead-ins, which may start before beat 0;
			* make sure we don't stay there if we escaped out early. */
		GAMESTATE->m_fSongBeat = max( GAMESTATE->m_fSongBeat, 0 );

		/* Stop displaying course attacks, if any. */
		GAMESTATE->RemoveAllActiveAttacks();
		GAMESTATE->RebuildPlayerOptionsFromActiveAttacks( PLAYER_1 );
		GAMESTATE->m_pPlayerState[PLAYER_1]->m_CurrentPlayerOptions = GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerOptions;

		if( old == MODE_RECORDING )
		{
			SaveUndo();

			// delete old TapNotes in the range
			m_NoteDataEdit.ClearRange( m_NoteFieldEdit.m_iBeginMarker, m_NoteFieldEdit.m_iEndMarker );
			m_NoteDataEdit.CopyRange( m_NoteDataRecord, m_NoteFieldEdit.m_iBeginMarker, m_NoteFieldEdit.m_iEndMarker, m_NoteFieldEdit.m_iBeginMarker );
			m_NoteDataRecord.ClearAll();

			CheckNumberOfNotesAndUndo();
		}
		break;
	case MODE_RECORDING:
		break;
	case MODE_PLAYING:
		break;
	default:
		ASSERT(0);
	}


	//
	// Show/hide depending on em
	//
	m_Background.SetHidden( !PREFSMAN->m_bEditorShowBGChangesPlay || em == MODE_EDITING );
	m_sprUnderlay->SetHidden( em != MODE_EDITING );
	m_sprOverlay->SetHidden( em != MODE_EDITING );
	m_textInputTips.SetHidden( em != MODE_EDITING );
	m_textInfo.SetHidden( em != MODE_EDITING );
	// Play the OnCommands again so that these will be re-hidden if the OnCommand hides them.
	if( em == MODE_EDITING )
	{
		m_textInputTips.PlayCommand( "On" );
		m_textInfo.PlayCommand( "On" );
	}
	m_textPlayRecordHelp.SetHidden( em == MODE_EDITING );
	m_SnapDisplay.SetHidden( em != MODE_EDITING );
	m_NoteFieldEdit.SetHidden( em != MODE_EDITING );
	m_NoteFieldRecord.SetHidden( em != MODE_RECORDING );
	m_Player.SetHidden( em != MODE_PLAYING );
	m_Foreground.SetHidden( !PREFSMAN->m_bEditorShowBGChangesPlay || em == MODE_EDITING );


	m_EditMode = em;
}

void ScreenEdit::HandleScreenMessage( const ScreenMessage SM )
{
	if( SM == SM_GoToNextScreen )
	{
		SCREENMAN->SetNewScreen( PREV_SCREEN );
		GAMESTATE->m_bEditing = false;
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
	else if( SM == SM_BackFromPrefs )
	{
		PREFSMAN->m_bEditorShowBGChangesPlay = !!ScreenMiniMenu::s_viLastAnswers[pref_show_bgs_play];
		PREFSMAN->SaveGlobalPrefsToDisk();
	}
	else if( SM == SM_BackFromCourseModeMenu )
	{
		const int num = ScreenMiniMenu::s_viLastAnswers[0];
		m_pAttacksFromCourse = NULL;
		if( num != 0 )
		{
			const CString name = g_CourseMode.rows[0].choices[num];
			m_pAttacksFromCourse = SONGMAN->FindCourse( name );
			ASSERT( m_pAttacksFromCourse );
		}
	}
	else if( 
		SM == SM_BackFromPlayerOptions ||
		SM == SM_BackFromSongOptions )
	{
		// coming back from PlayerOptions or SongOptions
		GAMESTATE->StoreSelectedOptions();

		// stop any music that screen may have been playing
		SOUND->StopMusic();
	}
	else if( SM == SM_BackFromInsertAttack )
	{
		int iDurationChoice = ScreenMiniMenu::s_viLastAnswers[0];
		g_fLastInsertAttackDurationSeconds = strtof( g_InsertAttack.rows[0].choices[iDurationChoice], NULL );
		GAMESTATE->StoreSelectedOptions();	// save so that we don't lose the options chosen for edit and playback

		// XXX: Fix me
		//SCREENMAN->AddNewScreenToTop( "ScreenPlayerOptions", SM_BackFromInsertAttackModifiers );
	}
	else if( SM == SM_BackFromInsertAttackModifiers )
	{
		PlayerOptions poChosen = GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerOptions;
		CString sMods = poChosen.GetString();
		const int row = BeatToNoteRow( GAMESTATE->m_fSongBeat );
		
		TapNote tn(
			TapNote::attack, TapNote::original, 
			sMods,
			g_fLastInsertAttackDurationSeconds, 
			false,
			0 );
		
		SaveUndo();
		m_NoteDataEdit.SetTapNote( g_iLastInsertAttackTrack, row, tn );
		CheckNumberOfNotesAndUndo();

		GAMESTATE->RestoreSelectedOptions();	// restore the edit and playback options
	}
	else if( SM == SM_DoRevertToLastSave )
	{
		if( ScreenPrompt::s_LastAnswer == ANSWER_YES )
		{
			CopyFromLastSave();
			m_pSteps->GetNoteData( m_NoteDataEdit );
			m_Undo.ClearAll();
		}
	}
	else if( SM == SM_DoUpdateTextInfo )
	{
		this->PostScreenMessage( SM_DoUpdateTextInfo, 0.5f );
		UpdateTextInfo();
	}
	else if( SM == SM_DoSaveAndExit ) // just asked "save before exiting? yes, no, cancel"
	{
		switch( ScreenPrompt::s_LastAnswer )
		{
		case ANSWER_YES:
			// This will send SM_Success or SM_Failure.
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
	else if( SM == SM_Success )
	{
		LOG->Trace( "Save successful." );
		m_pSteps->SetSavedToDisk( true );
		CopyToLastSave();

		if( m_CurrentAction == save_on_exit )
			HandleScreenMessage( SM_DoExit );
	}
	else if( SM == SM_Failure ) // save failed; stay in the editor
	{
		/* We committed the steps to SongManager.  Revert to the last save, and
		 * recommit the reversion to SongManager. */
		LOG->Trace( "Save failed.  Changes uncommitted from memory." );
		CopyFromLastSave();
		m_pSteps->SetNoteData( m_NoteDataEdit );
	}
	else if( SM == SM_DoExit )
	{
		// IMPORTANT: CopyFromLastSave before deleteing the Steps below
		CopyFromLastSave();

		// If these steps have never been saved, then we should delete them.
		// If the user created them in the edit menu and never bothered
		// to save them, then they aren't wanted.
		Steps* pSteps = GAMESTATE->m_pCurSteps[PLAYER_1];
		if( !pSteps->GetSavedToDisk() )
		{
			Song* pSong = GAMESTATE->m_pCurSong;
			pSong->RemoveSteps( pSteps );
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


void ChangeDescription( CString sNew )
{
	Steps* pSteps = GAMESTATE->m_pCurSteps[PLAYER_1];
	pSteps->SetDescription(sNew);
}

void ChangeMainTitle( CString sNew )
{
	Song* pSong = GAMESTATE->m_pCurSong;
	pSong->m_sMainTitle = sNew;
}

void ChangeSubTitle( CString sNew )
{
	Song* pSong = GAMESTATE->m_pCurSong;
	pSong->m_sSubTitle = sNew;
}

void ChangeArtist( CString sNew )
{
	Song* pSong = GAMESTATE->m_pCurSong;
	pSong->m_sArtist = sNew;
}

void ChangeCredit( CString sNew )
{
	Song* pSong = GAMESTATE->m_pCurSong;
	pSong->m_sCredit = sNew;
}

void ChangeMainTitleTranslit( CString sNew )
{
	Song* pSong = GAMESTATE->m_pCurSong;
	pSong->m_sMainTitleTranslit = sNew;
}

void ChangeSubTitleTranslit( CString sNew )
{
	Song* pSong = GAMESTATE->m_pCurSong;
	pSong->m_sSubTitleTranslit = sNew;
}

void ChangeArtistTranslit( CString sNew )
{
	Song* pSong = GAMESTATE->m_pCurSong;
	pSong->m_sArtistTranslit = sNew;
}

// End helper functions

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

				g_StepsInformation.rows[difficulty].iDefaultChoice = pSteps->GetDifficulty();
				g_StepsInformation.rows[difficulty].bEnabled = !HOME_EDIT_MODE;
				g_StepsInformation.rows[meter].iDefaultChoice = clamp( pSteps->GetMeter()-1, 0, MAX_METER+1 );
				g_StepsInformation.rows[predict_meter].choices.resize(1);g_StepsInformation.rows[predict_meter].choices[0] = ssprintf("%.2f",pSteps->PredictMeter());
				g_StepsInformation.rows[description].choices.resize(1);	g_StepsInformation.rows[description].choices[0] = pSteps->GetDescription();	g_StepsInformation.rows[description].bEnabled = !HOME_EDIT_MODE;
				g_StepsInformation.rows[tap_notes].choices.resize(1);	g_StepsInformation.rows[tap_notes].choices[0] = ssprintf("%d", m_NoteDataEdit.GetNumTapNotes());
				g_StepsInformation.rows[jumps].choices.resize(1);		g_StepsInformation.rows[jumps].choices[0] = ssprintf("%d", m_NoteDataEdit.GetNumJumps());
				g_StepsInformation.rows[hands].choices.resize(1);		g_StepsInformation.rows[hands].choices[0] = ssprintf("%d", m_NoteDataEdit.GetNumHands());
				g_StepsInformation.rows[quads].choices.resize(1);		g_StepsInformation.rows[quads].choices[0] = ssprintf("%d", m_NoteDataEdit.GetNumQuads());
				g_StepsInformation.rows[holds].choices.resize(1);		g_StepsInformation.rows[holds].choices[0] = ssprintf("%d", m_NoteDataEdit.GetNumHoldNotes());
				g_StepsInformation.rows[mines].choices.resize(1);		g_StepsInformation.rows[mines].choices[0] = ssprintf("%d", m_NoteDataEdit.GetNumMines());
				g_StepsInformation.rows[stream].choices.resize(1);		g_StepsInformation.rows[stream].choices[0] = ssprintf("%.2f", NoteDataUtil::GetStreamRadarValue(m_NoteDataEdit,fMusicSeconds));
				g_StepsInformation.rows[voltage].choices.resize(1);		g_StepsInformation.rows[voltage].choices[0] = ssprintf("%.2f", NoteDataUtil::GetVoltageRadarValue(m_NoteDataEdit,fMusicSeconds));
				g_StepsInformation.rows[air].choices.resize(1);			g_StepsInformation.rows[air].choices[0] = ssprintf("%.2f", NoteDataUtil::GetAirRadarValue(m_NoteDataEdit,fMusicSeconds));
				g_StepsInformation.rows[freeze].choices.resize(1);		g_StepsInformation.rows[freeze].choices[0] = ssprintf("%.2f", NoteDataUtil::GetFreezeRadarValue(m_NoteDataEdit,fMusicSeconds));
				g_StepsInformation.rows[chaos].choices.resize(1);		g_StepsInformation.rows[chaos].choices[0] = ssprintf("%.2f", NoteDataUtil::GetChaosRadarValue(m_NoteDataEdit,fMusicSeconds));
				SCREENMAN->MiniMenu( &g_StepsInformation, SM_BackFromStepsInformation );
			}
			break;
		case play_whole_song:
			{
				m_NoteFieldEdit.m_iBeginMarker = 0;
				m_NoteFieldEdit.m_iEndMarker = m_NoteDataEdit.GetLastRow();
				HandleAreaMenuChoice( play );
			}
			break;
		case play_current_beat_to_end:
			{
				m_NoteFieldEdit.m_iBeginMarker = BeatToNoteRow(GAMESTATE->m_fSongBeat);
				m_NoteFieldEdit.m_iEndMarker = m_NoteDataEdit.GetLastRow();
				HandleAreaMenuChoice( play );
			}
			break;
		case save:
		case save_on_exit:
			{
				m_CurrentAction = c;

				// copy edit into current Steps
				Steps* pSteps = GAMESTATE->m_pCurSteps[PLAYER_1];
				ASSERT( pSteps );

				pSteps->SetNoteData( m_NoteDataEdit );

				if( HOME_EDIT_MODE )
				{
					CString s;
					switch( c )
					{
					case save:			s = "ScreenMemcardSaveEditsAfterSave";	break;
					case save_on_exit:	s = "ScreenMemcardSaveEditsAfterExit";	break;
					default:		ASSERT(0);
					}
					SCREENMAN->AddNewScreenToTop( s );
				}
				else
				{
					pSteps->SetSavedToDisk( true );
					CopyToLastSave();

					GAMESTATE->m_pCurSong->Save();
					SCREENMAN->ZeroNextUpdate();

					// we shouldn't say we're saving a DWI if we're on any game besides
					// dance, it just looks tacky and people may be wondering where the
					// DWI file is :-)
					if ((int)pSteps->m_StepsType <= (int)STEPS_TYPE_DANCE_SOLO) 
						SCREENMAN->SystemMessage( "Saved as SM and DWI." );
					else
						SCREENMAN->SystemMessage( "Saved as SM." );

					HandleScreenMessage( SM_Success );
				}

				m_soundSave.Play();
			}
			break;
		case revert_to_last_save:
			SCREENMAN->Prompt( SM_DoRevertToLastSave,
				"Do you want to revert to your last save?\n\nThis will destroy all unsaved changes.",
				PROMPT_YES_NO, ANSWER_NO );
			break;
		case player_options:
			SCREENMAN->AddNewScreenToTop( "ScreenPlayerOptions" );
			break;
		case song_options:
			SCREENMAN->AddNewScreenToTop( "ScreenSongOptions" );
			break;
		case edit_song_info:
			{
				Song* pSong = GAMESTATE->m_pCurSong;
				g_SongInformation.rows[main_title].choices.resize(1);					g_SongInformation.rows[main_title].choices[0] = pSong->m_sMainTitle;
				g_SongInformation.rows[sub_title].choices.resize(1);					g_SongInformation.rows[sub_title].choices[0] = pSong->m_sSubTitle;
				g_SongInformation.rows[artist].choices.resize(1);						g_SongInformation.rows[artist].choices[0] = pSong->m_sArtist;
				g_SongInformation.rows[credit].choices.resize(1);						g_SongInformation.rows[credit].choices[0] = pSong->m_sCredit;
				g_SongInformation.rows[main_title_transliteration].choices.resize(1);	g_SongInformation.rows[main_title_transliteration].choices[0] = pSong->m_sMainTitleTranslit;
				g_SongInformation.rows[sub_title_transliteration].choices.resize(1);	g_SongInformation.rows[sub_title_transliteration].choices[0] = pSong->m_sSubTitleTranslit;
				g_SongInformation.rows[artist_transliteration].choices.resize(1);		g_SongInformation.rows[artist_transliteration].choices[0] = pSong->m_sArtistTranslit;

				SCREENMAN->MiniMenu( &g_SongInformation, SM_BackFromSongInformation );
			}
			break;
		case edit_bpm:
			SCREENMAN->TextEntry( 
				SM_BackFromBPMChange, 
				"Enter new BPM value.", 
				ssprintf( "%.4f", m_pSong->GetBPMAtBeat(GAMESTATE->m_fSongBeat) ),
				10
				);
			break;
		case edit_stop:
			{
				unsigned i;
				for( i=0; i<m_pSong->m_Timing.m_StopSegments.size(); i++ )
				{
					// XXX
					if( m_pSong->m_Timing.m_StopSegments[i].m_iStartRow == BeatToNoteRow(GAMESTATE->m_fSongBeat) )
						break;
				}
				if ( i == m_pSong->m_Timing.m_StopSegments.size() )
				{
					SCREENMAN->TextEntry( 
						SM_BackFromStopChange, 
						"Enter new Stop value.", 
						"0.00", 
						10
						);
				}
				else
				{
					SCREENMAN->TextEntry( 
						SM_BackFromStopChange, 
						"Enter new Stop value.", 
						ssprintf( "%.4f", m_pSong->m_Timing.m_StopSegments[i].m_fStopSeconds ),
						10
						);
				}
				break;
			}
		case edit_bg_change:
			{
				//
				// Fill in option names
				//

				// m_pSong->GetSongDir() has trailing slash
				g_BackgroundChange.rows[add_song_bganimation].choices.clear();
				GetDirListing( m_pSong->GetSongDir()+"*", g_BackgroundChange.rows[add_song_bganimation].choices, true );

				g_BackgroundChange.rows[add_song_movie].choices.clear();
				GetDirListing( m_pSong->GetSongDir()+"*.avi", g_BackgroundChange.rows[add_song_movie].choices, false );
				GetDirListing( m_pSong->GetSongDir()+"*.mpg", g_BackgroundChange.rows[add_song_movie].choices, false );
				GetDirListing( m_pSong->GetSongDir()+"*.mpeg", g_BackgroundChange.rows[add_song_movie].choices, false );

				g_BackgroundChange.rows[add_song_still].choices.clear();
				GetDirListing( m_pSong->GetSongDir()+"*.png", g_BackgroundChange.rows[add_song_still].choices, false );
				GetDirListing( m_pSong->GetSongDir()+"*.jpg", g_BackgroundChange.rows[add_song_still].choices, false );
				GetDirListing( m_pSong->GetSongDir()+"*.gif", g_BackgroundChange.rows[add_song_still].choices, false );
				GetDirListing( m_pSong->GetSongDir()+"*.bmp", g_BackgroundChange.rows[add_song_still].choices, false );

				g_BackgroundChange.rows[add_global_random_movie].choices.clear();
				GetDirListing( RANDOMMOVIES_DIR+"*.avi", g_BackgroundChange.rows[add_global_random_movie].choices, false );
				GetDirListing( RANDOMMOVIES_DIR+"*.mpg", g_BackgroundChange.rows[add_global_random_movie].choices, false );
				GetDirListing( RANDOMMOVIES_DIR+"*.mpeg", g_BackgroundChange.rows[add_global_random_movie].choices, false );
				
				vector<CString> vSubDirs;
				GetDirListing( RANDOMMOVIES_DIR+"*", vSubDirs, true );
				FOREACH_CONST( CString, vSubDirs, s )
				{
					const CString &sSubDir = *s;
					vector<CString> vsMovies;

					GetDirListing( RANDOMMOVIES_DIR+sSubDir+"/*.avi", vsMovies, false );
					GetDirListing( RANDOMMOVIES_DIR+sSubDir+"/*.mpg", vsMovies, false );
					GetDirListing( RANDOMMOVIES_DIR+sSubDir+"/*.mpeg", vsMovies, false );

					FOREACH_CONST( CString, vsMovies, m )
						g_BackgroundChange.rows[add_global_random_movie].choices.push_back( sSubDir +"/"+ *m );
				}

				g_BackgroundChange.rows[add_global_bganimation].choices.clear();
				GetDirListing( BG_ANIMS_DIR+"*", g_BackgroundChange.rows[add_global_bganimation].choices, true );

				g_BackgroundChange.rows[add_global_visualization].choices.clear();
				GetDirListing( VISUALIZATIONS_DIR+"*.avi", g_BackgroundChange.rows[add_global_visualization].choices, false );
				GetDirListing( VISUALIZATIONS_DIR+"*.mpg", g_BackgroundChange.rows[add_global_visualization].choices, false );
				GetDirListing( VISUALIZATIONS_DIR+"*.mpeg", g_BackgroundChange.rows[add_global_visualization].choices, false );


				//
				// Fill in line enabled/disabled
				//
				bool bAlreadyBGChangeHere = false;
				BackgroundChange bgChange; 
				FOREACH( BackgroundChange, m_pSong->m_BackgroundChanges, bgc )
				{
					if( bgc->m_fStartBeat == GAMESTATE->m_fSongBeat )
					{
						bAlreadyBGChangeHere = true;
						bgChange = *bgc;
					}
				}

				g_BackgroundChange.rows[add_random].bEnabled = true;
				g_BackgroundChange.rows[add_song_bganimation].bEnabled = g_BackgroundChange.rows[add_song_bganimation].choices.size() > 0;
				g_BackgroundChange.rows[add_song_movie].bEnabled = g_BackgroundChange.rows[add_song_movie].choices.size() > 0;
				g_BackgroundChange.rows[add_song_still].bEnabled = g_BackgroundChange.rows[add_song_still].choices.size() > 0;
				g_BackgroundChange.rows[add_global_random_movie].bEnabled = g_BackgroundChange.rows[add_global_random_movie].choices.size() > 0;
				g_BackgroundChange.rows[add_global_bganimation].bEnabled = g_BackgroundChange.rows[add_global_bganimation].choices.size() > 0;
				g_BackgroundChange.rows[add_global_visualization].bEnabled = g_BackgroundChange.rows[add_global_visualization].choices.size() > 0;
				g_BackgroundChange.rows[delete_change].bEnabled = bAlreadyBGChangeHere;
					

				// set default choices
				g_BackgroundChange.rows[rate].						SetDefaultChoiceIfPresent( ssprintf("%2.0f%%",bgChange.m_fRate*100) );
				g_BackgroundChange.rows[fade_last].iDefaultChoice	= bgChange.m_bFadeLast ? 1 : 0;
				g_BackgroundChange.rows[rewind_movie].iDefaultChoice = bgChange.m_bRewindMovie ? 1 : 0;
				g_BackgroundChange.rows[loop].iDefaultChoice			= bgChange.m_bLoop ? 1 : 0;
				g_BackgroundChange.rows[add_song_bganimation].		SetDefaultChoiceIfPresent( bgChange.m_sBGName );
				g_BackgroundChange.rows[add_song_movie].			SetDefaultChoiceIfPresent( bgChange.m_sBGName );
				g_BackgroundChange.rows[add_song_still].			SetDefaultChoiceIfPresent( bgChange.m_sBGName );
				g_BackgroundChange.rows[add_global_random_movie].	SetDefaultChoiceIfPresent( bgChange.m_sBGName );
				g_BackgroundChange.rows[add_global_bganimation].	SetDefaultChoiceIfPresent( bgChange.m_sBGName );
				g_BackgroundChange.rows[add_global_visualization].	SetDefaultChoiceIfPresent( bgChange.m_sBGName );


				SCREENMAN->MiniMenu( &g_BackgroundChange, SM_BackFromBGChange );
			}
			break;
		case preferences:
			g_Prefs.rows[pref_show_bgs_play].iDefaultChoice = PREFSMAN->m_bEditorShowBGChangesPlay;

			SCREENMAN->MiniMenu( &g_Prefs, SM_BackFromPrefs );
			break;

		case play_preview_music:
			PlayPreviewMusic();
			break;
		case exit:
			SCREENMAN->Prompt(
				SM_DoSaveAndExit,
				"Do you want to save changes before exiting?",
				PROMPT_YES_NO_CANCEL, ANSWER_NO );
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
		case convert_beat_to_pause:
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
			{
				int iDestFirstRow = BeatToNoteRow( GAMESTATE->m_fSongBeat );
				m_NoteDataEdit.CopyRange( m_Clipboard, 0, m_Clipboard.GetLastRow()+1, iDestFirstRow );
			}
			break;
		case paste_at_begin_marker:
			{
				ASSERT( m_NoteFieldEdit.m_iBeginMarker!=-1 );
				m_NoteDataEdit.CopyRange( m_Clipboard, 0, m_Clipboard.GetLastRow()+1, m_NoteFieldEdit.m_iBeginMarker );
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

				int iOldClipboardBeats = m_NoteFieldEdit.m_iEndMarker - m_NoteFieldEdit.m_iBeginMarker;
				int iNewClipboardBeats = lrintf( iOldClipboardBeats * fScale );
				int iDeltaBeats = iNewClipboardBeats - iOldClipboardBeats;
				int iNewClipboardEndBeat = m_NoteFieldEdit.m_iBeginMarker + iNewClipboardBeats;
				NoteDataUtil::ShiftRows( m_NoteDataEdit, m_NoteFieldEdit.m_iBeginMarker, iDeltaBeats );
				m_pSong->m_Timing.ScaleRegion( fScale, m_NoteFieldEdit.m_iBeginMarker, m_NoteFieldEdit.m_iEndMarker );

				HandleAreaMenuChoice( paste_at_begin_marker );

				const vector<Steps*> sIter = m_pSong->GetAllSteps();
				CString sTempStyle, sTempDiff;
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

				m_NoteFieldEdit.m_iEndMarker = iNewClipboardEndBeat;

				float fOldBPM = m_pSong->GetBPMAtBeat( NoteRowToBeat(m_NoteFieldEdit.m_iBeginMarker) );
				float fNewBPM = fOldBPM * fScale;
				m_pSong->SetBPMAtBeat( NoteRowToBeat(m_NoteFieldEdit.m_iBeginMarker), fNewBPM );
				m_pSong->SetBPMAtBeat( NoteRowToBeat(iNewClipboardEndBeat), fOldBPM );
			}
			break;
		case play:
			{
				ASSERT( m_NoteFieldEdit.m_iBeginMarker!=-1 && m_NoteFieldEdit.m_iEndMarker!=-1 );

				SOUND->PlayMusic("");

				TransitionEditMode( MODE_PLAYING );
				m_sprOverlay->PlayCommand( "Play" );

				GAMESTATE->m_bPastHereWeGo = true;

				/* Reset the note skin, in case preferences have changed. */
				GAMESTATE->ResetNoteSkins();

				/* Give a 1 measure lead-in.  Set this before loading Player, so it knows
				 * where we're starting. */
				float fSeconds = m_pSong->m_Timing.GetElapsedTimeFromBeat( NoteRowToBeat(m_NoteFieldEdit.m_iBeginMarker) - 4 );
				GAMESTATE->UpdateSongPosition( fSeconds, m_pSong->m_Timing );

				/* If we're in course display mode, set that up. */
				SetupCourseAttacks();

				m_Player.Load( m_NoteDataEdit );
				GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerController = PREFSMAN->m_bAutoPlay?PC_AUTOPLAY:PC_HUMAN;

				const float fStartSeconds = m_pSong->GetElapsedTimeFromBeat(GAMESTATE->m_fSongBeat) ;
				LOG->Trace( "Starting playback at %f", fStartSeconds );
			
				if( PREFSMAN->m_bEditorShowBGChangesPlay )
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

				RageSoundParams p;
				p.SetPlaybackRate( GAMESTATE->m_SongOptions.m_fMusicRate );
				p.m_StartSecond = fStartSeconds;
				p.AccurateSync = true;
				p.StopMode = RageSoundParams::M_CONTINUE;
				m_soundMusic.Play( &p );
			}
			break;
		case record:
			{
				ASSERT( m_NoteFieldEdit.m_iBeginMarker!=-1 && m_NoteFieldEdit.m_iEndMarker!=-1 );

				SOUND->PlayMusic("");

				TransitionEditMode( MODE_RECORDING );
				m_sprOverlay->PlayCommand( "Record" );
				GAMESTATE->m_bPastHereWeGo = true;

				/* Reset the note skin, in case preferences have changed. */
				GAMESTATE->ResetNoteSkins();

				// initialize m_NoteFieldRecord
				m_NoteDataRecord.SetNumTracks( m_NoteDataEdit.GetNumTracks() );
				m_NoteDataRecord.CopyAll( m_NoteDataEdit );
				m_NoteFieldRecord.Load( &m_NoteDataRecord, -150, 350 );

				GAMESTATE->m_fSongBeat = NoteRowToBeat(m_NoteFieldEdit.m_iBeginMarker - ROWS_PER_MEASURE );	// give a 1 measure lead-in
				float fStartSeconds = m_pSong->GetElapsedTimeFromBeat(GAMESTATE->m_fSongBeat);
				LOG->Trace( "Starting playback at %f", fStartSeconds );

				RageSoundParams p;
				p.SetPlaybackRate( GAMESTATE->m_SongOptions.m_fMusicRate );
				p.m_StartSecond = fStartSeconds;
				p.AccurateSync = true;
				p.StopMode = RageSoundParams::M_CONTINUE;
				m_soundMusic.Play( &p );
			}
			break;
		case insert_and_shift:
			NoteDataUtil::ShiftRows( m_NoteDataEdit, BeatToNoteRow(GAMESTATE->m_fSongBeat), BeatToNoteRow(1) );
			break;
		case delete_and_shift:
			NoteDataUtil::ShiftRows( m_NoteDataEdit, BeatToNoteRow(GAMESTATE->m_fSongBeat), BeatToNoteRow(-1) );
			break;
		case shift_pauses_forward:
			m_pSong->m_Timing.ShiftRows( BeatToNoteRow(GAMESTATE->m_fSongBeat), BeatToNoteRow(+1) );
			break;
		case shift_pauses_backward:
			m_pSong->m_Timing.ShiftRows( BeatToNoteRow(GAMESTATE->m_fSongBeat), BeatToNoteRow(-1) );
			break;
		// MD 11/02/03 - Converting selected region to a pause of the same length.
		case convert_beat_to_pause:
			{
				ASSERT( m_NoteFieldEdit.m_iBeginMarker!=-1 && m_NoteFieldEdit.m_iEndMarker!=-1 );
				float fMarkerStart = m_pSong->m_Timing.GetElapsedTimeFromBeat( NoteRowToBeat(m_NoteFieldEdit.m_iBeginMarker) );
				float fMarkerEnd = m_pSong->m_Timing.GetElapsedTimeFromBeat( NoteRowToBeat(m_NoteFieldEdit.m_iEndMarker) );
				float fStopLength = fMarkerEnd - fMarkerStart;
				// be sure not to clobber the row at the start - a row at the end
				// can be dropped safely, though
				NoteDataUtil::ShiftRows( m_NoteDataEdit, 
										 m_NoteFieldEdit.m_iBeginMarker + 1,
										 -m_NoteFieldEdit.m_iEndMarker+m_NoteFieldEdit.m_iBeginMarker
									   );
				m_pSong->m_Timing.ShiftRows( m_NoteFieldEdit.m_iBeginMarker + 1,
										     -m_NoteFieldEdit.m_iEndMarker+m_NoteFieldEdit.m_iBeginMarker
										   );
				unsigned i;
				for( i=0; i<m_pSong->m_Timing.m_StopSegments.size(); i++ )
				{
					float fStart = m_pSong->m_Timing.GetElapsedTimeFromBeat(NoteRowToBeat(m_pSong->m_Timing.m_StopSegments[i].m_iStartRow));
					float fEnd = fStart + m_pSong->m_Timing.m_StopSegments[i].m_fStopSeconds;
					if( fStart > fMarkerEnd || fEnd < fMarkerStart )
						continue;
					else {
						if( fStart > fMarkerStart )
							m_pSong->m_Timing.m_StopSegments[i].m_iStartRow = m_NoteFieldEdit.m_iBeginMarker;
						m_pSong->m_Timing.m_StopSegments[i].m_fStopSeconds = fStopLength;
						break;
					}
				}

				if( i == m_pSong->m_Timing.m_StopSegments.size() )	// there is no BPMSegment at the current beat
					m_pSong->AddStopSegment( StopSegment(m_NoteFieldEdit.m_iBeginMarker, fStopLength) );
				m_NoteFieldEdit.m_iEndMarker = -1;
				break;
			}
		// MD 11/02/03 - Converting a pause at the current beat into beats.
		//    I know this will break holds that cross the pause.  Anyone who
		//    wants to rewrite this to fix that behavior is welcome to - I'm
		//    not sure how exactly to do it without making this a lot longer
		//    than it is.
		// NOTE: Fixed this so that holds aren't broken by it.  Working in 2s and
		// 3s makes this work better, too. :-)  It sorta makes you wonder WHY we
		// don't bring it into 2s and 3s when we bring up the editor.
		case convert_pause_to_beat:
			{
				float fBPMatPause = m_pSong->GetBPMAtBeat( GAMESTATE->m_fSongBeat );
				unsigned i;
				for( i=0; i<m_pSong->m_Timing.m_StopSegments.size(); i++ )
				{
					if( m_pSong->m_Timing.m_StopSegments[i].m_iStartRow == BeatToNoteRow(GAMESTATE->m_fSongBeat) )
						break;
				}

				if( i == m_pSong->m_Timing.m_StopSegments.size() )	// there is no BPMSegment at the current beat
					break;
				else	// StopSegment being modified is m_Timing.m_StopSegments[i]
				{
					float fStopLength = m_pSong->m_Timing.m_StopSegments[i].m_fStopSeconds;
					m_pSong->m_Timing.m_StopSegments.erase( m_pSong->m_Timing.m_StopSegments.begin()+i,
												   m_pSong->m_Timing.m_StopSegments.begin()+i+1);
					fStopLength *= fBPMatPause;
					fStopLength /= 60;
					// don't move the step from where it is, just move everything later
					NoteDataUtil::ShiftRows( m_NoteDataEdit, BeatToNoteRow(GAMESTATE->m_fSongBeat) + 1, BeatToNoteRow(fStopLength) );
					m_pSong->m_Timing.ShiftRows( BeatToNoteRow(GAMESTATE->m_fSongBeat) + 1, BeatToNoteRow(fStopLength) );
				}
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
		SCREENMAN->TextEntry( 
			SM_None, 
			"Enter a description.", 
			m_pSteps->GetDescription(), 
			MAX_DESCRIPTION_LENGTH,
			NULL,
			ChangeDescription, 
			NULL 
			);
		break;
	}
}

void ScreenEdit::HandleSongInformationChoice( SongInformationChoice c, const vector<int> &iAnswers )
{
	Song* pSong = GAMESTATE->m_pCurSong;

	switch( c )
	{
	case main_title:
		SCREENMAN->TextEntry( SM_None, "Edit main title.\nPress Enter to confirm,\nEscape to cancel.", pSong->m_sMainTitle, 100, NULL, ChangeMainTitle, NULL );
		break;
	case sub_title:
		SCREENMAN->TextEntry( SM_None, "Edit sub title.\nPress Enter to confirm,\nEscape to cancel.", pSong->m_sSubTitle, 100, NULL, ChangeSubTitle, NULL );
		break;
	case artist:
		SCREENMAN->TextEntry( SM_None, "Edit artist.\nPress Enter to confirm,\nEscape to cancel.", pSong->m_sArtist, 100, NULL, ChangeArtist, NULL );
		break;
	case credit:
		SCREENMAN->TextEntry( SM_None, "Edit credit.\nPress Enter to confirm,\nEscape to cancel.", pSong->m_sCredit, 100, NULL, ChangeCredit, NULL );
		break;
	case main_title_transliteration:
		SCREENMAN->TextEntry( SM_None, "Edit main title transliteration.\nPress Enter to confirm,\nEscape to cancel.", pSong->m_sMainTitleTranslit, 100, NULL, ChangeMainTitleTranslit, NULL );
		break;
	case sub_title_transliteration:
		SCREENMAN->TextEntry( SM_None, "Edit sub title transliteration.\nPress Enter to confirm,\nEscape to cancel.", pSong->m_sSubTitleTranslit, 100, NULL, ChangeSubTitleTranslit, NULL );
		break;
	case artist_transliteration:
		SCREENMAN->TextEntry( SM_None, "Edit artist transliteration.\nPress Enter to confirm,\nEscape to cancel.", pSong->m_sArtistTranslit, 100, NULL, ChangeArtistTranslit, NULL );
		break;
	default:
		ASSERT(0);
	};
}

void ScreenEdit::HandleBGChangeChoice( BGChangeChoice c, const vector<int> &iAnswers )
{
	BackgroundChange newChange;

	FOREACH( BackgroundChange, m_pSong->m_BackgroundChanges, iter )
	{
		if( iter->m_fStartBeat == GAMESTATE->m_fSongBeat )
		{
			newChange = *iter;
			// delete the old change.  We'll add a new one below.
			m_pSong->m_BackgroundChanges.erase( iter );
			break;
		}
	}

	newChange.m_fStartBeat = GAMESTATE->m_fSongBeat;

	switch( c )
	{
	case add_random:
		newChange.m_sBGName = "-random-";
		break;
	case add_song_bganimation:
	case add_song_movie:
	case add_song_still:
	case add_global_random_movie:
	case add_global_bganimation:
	case add_global_visualization:
		newChange.m_sBGName = g_BackgroundChange.rows[c].choices[iAnswers[c]];
		break;
	case delete_change:
		newChange.m_sBGName = "";
		break;
	default:
		break;
	};

	newChange.m_fRate = strtof( g_BackgroundChange.rows[rate].choices[iAnswers[rate]], NULL )/100.f;
	newChange.m_bFadeLast = !!iAnswers[fade_last];
	newChange.m_bRewindMovie = !!iAnswers[rewind_movie];
	newChange.m_bLoop = !!iAnswers[loop];

	if( newChange.m_sBGName != "" )
		m_pSong->AddBackgroundChange( newChange );
}

void ScreenEdit::SetupCourseAttacks()
{
	/* This is the first beat that can be changed without it being visible.  Until
	 * we draw for the first time, any beat can be changed. */
	GAMESTATE->m_pPlayerState[PLAYER_1]->m_fLastDrawnBeat = -100;

	// Put course options into effect.
	GAMESTATE->m_pPlayerState[PLAYER_1]->m_ModsToApply.clear();
	GAMESTATE->RemoveActiveAttacksForPlayer( PLAYER_1 );


	if( m_pAttacksFromCourse )
	{
		m_pAttacksFromCourse->LoadFromCRSFile( m_pAttacksFromCourse->m_sPath );

		AttackArray Attacks;
		for( unsigned e = 0; e < m_pAttacksFromCourse->m_entries.size(); ++e )
		{
			if( m_pAttacksFromCourse->m_entries[e].type != COURSE_ENTRY_FIXED )
				continue;
			if( m_pAttacksFromCourse->m_entries[e].pSong != m_pSong )
				continue;

			Attacks = m_pAttacksFromCourse->m_entries[e].attacks;
			break;
		}

		for( unsigned i=0; i<Attacks.size(); ++i )
			GAMESTATE->LaunchAttack( PLAYER_1, Attacks[i] );
	}
	GAMESTATE->RebuildPlayerOptionsFromActiveAttacks( PLAYER_1 );
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

void ScreenEdit::SaveUndo()
{
	m_Undo.CopyAll( m_NoteDataEdit );
}

void ScreenEdit::Undo()
{
	m_NoteDataEdit.CopyAll( m_Undo );
	m_Undo.ClearAll();
}

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
			CString sError = ssprintf(
				"This change creates more than %d notes in a measure.\n\nMore than %d notes per measure is not allowed.  This change has been reverted.", 
				MAX_NOTES_PER_MEASURE, MAX_NOTES_PER_MEASURE );
			SCREENMAN->Prompt( SM_None, sError );
			return;
		}
	}
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
