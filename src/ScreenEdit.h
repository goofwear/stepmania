	/*
-----------------------------------------------------------------------------
 Class: ScreenEdit

 Desc: Edit, record, playback, or synchronize notes.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "Screen.h"
#include "Sprite.h"
#include "Transition.h"
#include "BitmapText.h"
#include "Player.h"
#include "RageSound.h"
#include "BGAnimation.h"
#include "SnapDisplay.h"
#include "GrayArrowRow.h"


const int NUM_ACTION_MENU_ITEMS = 23;
const int NUM_NAMING_MENU_ITEMS = 6;


class ScreenEdit : public Screen
{
public:
	ScreenEdit( CString sName );
	virtual ~ScreenEdit();
	virtual void Update( float fDeltaTime );
	virtual void DrawPrimitives();
	virtual void Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI );
	void InputEdit( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI );
	void InputRecord( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI );
	void InputPlay( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI );
	virtual void HandleScreenMessage( const ScreenMessage SM );

protected:
	void TransitionFromRecordToEdit();
	void TransitionToEdit();
	bool PlayTicks() const;
	void PlayPreviewMusic();
	void UpdateTextInfo();

	void OnSnapModeChange();
	void MenuItemGainFocus( BitmapText* menuitem );
	void MenuItemLoseFocus( BitmapText* menuitem );


	enum EditMode { MODE_EDITING, MODE_RECORDING, MODE_PLAYING };
	EditMode m_EditMode;

	Song*			m_pSong;
	Steps*			m_pNotes;

	BGAnimation		m_BGAnimation;

	NoteField		m_NoteFieldEdit;
	SnapDisplay		m_SnapDisplay;
	GrayArrowRow	m_GrayArrowRowEdit;

	Sprite			m_sprHelp;
	BitmapText		m_textHelp;
	Sprite			m_sprInfo;
	BitmapText		m_textInfo;		// status information that changes

	// keep track of where we are and what we're doing
	float				m_fTrailingBeat;	// this approaches GAMESTATE->m_fSongBeat, which is the actual beat
	/* The location we were at when shift was pressed, or
	 * -1 when shift isn't pressed: */
	float shiftAnchor;

	NoteData			m_Clipboard;

	RageSound			m_soundChangeLine;
	RageSound			m_soundChangeSnap;
	RageSound			m_soundMarker;

	Transition		m_In;
	Transition		m_Out;


// for MODE_RECORD

	NoteField		m_NoteFieldRecord;
	GrayArrowRow	m_GrayArrowRowRecord;

// for MODE_PLAY

	Player			m_Player;

// for MODE_RECORD and MODE_PLAY

	Quad			m_rectRecordBack;
	RageSound		m_soundMusic;

	RageSound		m_soundAssistTick;


public:
	enum MainMenuChoice {
		edit_notes_statistics,
		play_whole_song,
		play_current_beat_to_end,
		save,
		player_options,
		song_options,
		edit_song_info,
		edit_bg_change,
		play_preview_music,
		exit,
		NUM_MAIN_MENU_CHOICES
	};
	void HandleMainMenuChoice( MainMenuChoice c, int* iAnswers );

	enum AreaMenuChoice {
		cut,
		copy,
		paste_at_current_beat,
		paste_at_begin_marker,
		clear,
		quantize,
		turn,
		transform,
		alter,
		tempo,
		play,
		record,
		insert_and_shift,
		delete_and_shift,
		convert_beat_to_pause,
		convert_pause_to_beat,
		NUM_AREA_MENU_CHOICES
	};
	void HandleAreaMenuChoice( AreaMenuChoice c, int* iAnswers );
	enum TurnType { left, right, mirror, shuffle, super_shuffle, NUM_TURN_TYPES };
	enum TransformType { noholds, nomines, little, wide, big, quick, skippy, mines, echo, planted, stomp, twister, NUM_TRANSFORM_TYPES };
	enum AlterType { backwards, swap_sides, copy_left_to_right, copy_right_to_left, clear_left, clear_right, collapse_to_one, shift_left, shift_right, NUM_ALTER_TYPES };
	// MD 11/02/03 - added additional tempo adjusts which make "sense"
	enum TempoType { compress_2x, compress_3_2, compress_4_3, expand_4_3, expand_3_2, expand_2x, NUM_TEMPO_TYPES };

	enum EditNotesStatisticsChoice {
		difficulty,
		meter,
		description,
		tap_notes,
		hold_notes,
		stream,
		voltage,
		air,
		freeze,
		chaos,
		NUM_EDIT_NOTES_STATISTICS_CHOICES
	};
	void HandleEditNotesStatisticsChoice( EditNotesStatisticsChoice c, int* iAnswers );

	enum EditSongInfoChoice {
		main_title,
		sub_title,
		artist,
		credit,
		main_title_transliteration,
		sub_title_transliteration,
		artist_transliteration,
		NUM_EDIT_SONG_INFO_CHOICES
	};
	void HandleEditSongInfoChoice( EditSongInfoChoice c, int* iAnswers );

	enum BGChangeChoice {
		rate,
		fade_last,
		rewind_movie,
		loop,
		add_random,
		add_song_bganimation,
		add_song_movie,
		add_song_still,
		add_global_random_movie,
		add_global_bganimation,
		add_global_visualization,
		delete_change,
		NUM_BGCHANGE_CHOICES
	};
	void HandleBGChangeChoice( BGChangeChoice c, int* iAnswers );

};



