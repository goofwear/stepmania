#ifndef NOTEFIELD_POSITIONING_H
#define NOTEFIELD_POSITIONING_H

#include "PlayerNumber.h"
#include "StyleDef.h"
#include "PlayerNumber.h"
#include "Style.h"
#include "Actor.h"

#include <set>

class IniFile;

struct NoteFieldMode
{
	NoteFieldMode();
	bool MatchesCurrentGame() const;
	void Load(IniFile &ini, CString id, int pn = -1);

	void BeginDrawTrack(int tn);
	void EndDrawTrack(int tn);

	/* Visual name. */
	CString m_Name;

	/* Unique ID from the INI. */
	CString m_Id;

	/* Styles that this is valid for; empty means all. */
	set<Style> Styles;

	Actor m_Center;
	Actor m_CenterTrack[MAX_NOTE_TRACKS];

	CString GrayButtonNames[MAX_NOTE_TRACKS];
	CString NoteButtonNames[MAX_NOTE_TRACKS];
	CString GhostButtonNames[MAX_NOTE_TRACKS];

	/* 0 = no perspective */
	float m_fFov, m_fNear, m_fFar;
	Actor m_Position;
	Actor m_PositionTrack[MAX_NOTE_TRACKS];

	float m_fFirstPixelToDrawScale, m_fLastPixelToDrawScale;
	CString m_Backdrop;

	CString m_JudgmentCmd, m_ComboCmd, m_AttackDisplayCmd;
	CString m_HoldJudgmentCmd[MAX_NOTE_TRACKS];
};

class NoteFieldPositioning
{
public:
	NoteFieldPositioning(CString fn);
	void Load(PlayerNumber pn);

	void GetNamesForCurrentGame(vector<CString> &IDs);
	bool IsValidModeForCurrentStyle(CString mode) const;
	bool IsValidModeForAnyStyle(CString mode) const;

private:
	CString m_Filename;
	vector<NoteFieldMode> Modes;
	int GetID(const CString &name) const;
};

extern NoteFieldMode g_NoteFieldMode[NUM_PLAYERS];

#endif
