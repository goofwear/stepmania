#ifndef NOTES_LOADER_DWI_H
#define NOTES_LOADER_DWI_H

#include "song.h"
#include "Steps.h"
#include "GameInput.h"
#include "NotesLoader.h"

#include "song.h"
#include "Steps.h"

/* Return NA if no files in the directory can be loaded by
 * this loader, OK on success, ERROR if an applicable file was found
 * but there was a fatal error loading.  (ERROR not used yet--we
 * always throw.)
 */
class DWILoader: public NotesLoader {
	void DWIcharToNote( char c, GameController i, DanceNote &note1Out, DanceNote &note2Out );
	void DWIcharToNoteCol( char c, GameController i, int &col1Out, int &col2Out );

	bool LoadFromDWITokens( 
		CString sMode, CString sDescription, CString sNumFeet, CString sStepData1, 
		CString sStepData2,
		Steps &out );

	bool LoadFromDWIFile( CString sPath, Song &out );

	static float ParseBrokenDWITimestamp(const CString &arg1, const CString &arg2, const CString &arg3);
	static bool Is192( const CString &str, int pos );
	CString m_sLoadingFile;

public:
	void GetApplicableFiles( CString sPath, CStringArray &out );
	bool Loadable( CString sPath );
	bool LoadFromDir( CString sPath, Song &out );
};

#endif
