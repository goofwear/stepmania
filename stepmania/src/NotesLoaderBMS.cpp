#include "global.h"

#include "NotesLoaderBMS.h"
#include "NotesLoader.h"
#include "NoteData.h"
#include "GameConstantsAndTypes.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "GameManager.h"
#include "RageException.h"
#include "RageFile.h"

// BMS encoding:     tap-hold
// 4&8panel:   Player1     Player2
//    Left		11-51		21-61
//    Down		13-53		23-63
//    Up		15-55		25-65
//    Right		16-56		26-66
//	6panel:	   Player1
//    Left		11-51
//    Left+Up	12-52
//    Down		13-53
//    Up		14-54
//    Up+Right	15-55
//    Right		16-56
//
//	Notice that 15 and 25 have double meanings!  What were they thinking???
//	While reading in, use the 6 panel mapping.  After reading in, detect if only 4 notes
//	are used.  If so, shift the Up+Right column back to the Up column
//

// MD 10/26/03 - Hey, folks, BMSes are used for things BESIDES DDR steps, 
//    and so we're borking up BMSes that are for pnm/bm/etc.
//
// pnm-nine:   11-15,21-24
// pnm-five:   13-15,21-22
// bm-single:  11-16
// bm-double:  11-16,21-26
// bm-single7: 11-16,18-19
// bm-double7: 11-16,18-19,21-26,28-29
// 
// So the magics for these are:
// pnm-nine: nothing >5, with 12, 14, 22 and/or 24
// pnm-five: nothing >5, with 14 and/or 22
// bm-*: 18/19 - 7-key modes have 16/17/26/27
//

int iTracks[MAX_NOTE_TRACKS];

void BMSLoader::PushTrackNumForMagic( int iTrackNum ) {
	int ix = (iTrackNum < 20) ? (iTrackNum - 11) : (iTrackNum - 12);
	iTracks[ix]++;
}
StepsType BMSLoader::CheckTracksMagic( void ) {
	int iTrackCount = 0;
	for (int ix = 0; ix<MAX_NOTE_TRACKS; ix++) {
		if(iTracks[ix] != 0) iTrackCount++;
	}
	// Panel counts:
	// 4 - DDR
	// 5 - PNM 5-key
	// 6 - DDR Solo, BM 5-key
	// 8 - DDR Double. BM 7-key
	// 9 - PNM 9-key, BM 7-key
	// 12 - BM Double 5-key
	// 16 - BM Double 7-key
	switch(iTrackCount) {
	case 4:
		return STEPS_TYPE_DANCE_SINGLE;
	case 5:
		return STEPS_TYPE_PNM_FIVE;
	case 6:
		return STEPS_TYPE_DANCE_SOLO;
	case 8:
		// Could also be couple or 7-key.
		// TODO: Insert the magic check between IIDX and DDR Double - 12, 14, 18, or 19.
		//       Until BM 7-key is supported in stepfiles, we'll assume it's DDR.
		return STEPS_TYPE_DANCE_DOUBLE;
	case 9:
		return STEPS_TYPE_PNM_NINE;
	case 12:
		// TODO: Uncomment these once BM Double 5/7-key is supported.
		// return STEPS_TYPE_BM_DOUBLE;
	case 16:
		// return STEPS_TYPE_BM_DOUBLE7;
	default:
		return STEPS_TYPE_INVALID;
	}
}

void BMSLoader::mapBMSTrackToDanceNote( int iBMSTrack, int &iDanceColOut, char &cNoteCharOut )
{
	if( iBMSTrack > 40 )
	{
		cNoteCharOut = '2';
		iBMSTrack -= 40;
	}
	else
	{
		cNoteCharOut = '1';
	}

	switch( iBMSTrack )
	{
/* MD 10/26/03 - Let's try this with the new numbers.
	case 11:	iDanceColOut = DANCE_NOTE_PAD1_LEFT;	break;
	case 12:	iDanceColOut = DANCE_NOTE_PAD1_UPLEFT;	break;
	case 13:	iDanceColOut = DANCE_NOTE_PAD1_DOWN;	break;
	case 14:	iDanceColOut = DANCE_NOTE_PAD1_UP;		break;
	case 15:	iDanceColOut = DANCE_NOTE_PAD1_UPRIGHT;	break;
	case 16:	iDanceColOut = DANCE_NOTE_PAD1_RIGHT;	break;
	case 21:	iDanceColOut = DANCE_NOTE_PAD2_LEFT;	break;
	case 22:	iDanceColOut = DANCE_NOTE_PAD2_UPLEFT;	break;
	case 23:	iDanceColOut = DANCE_NOTE_PAD2_DOWN;	break;
	case 24:	iDanceColOut = DANCE_NOTE_PAD2_UP;		break;
	case 25:	iDanceColOut = DANCE_NOTE_PAD2_UPRIGHT;	break;
	case 26:	iDanceColOut = DANCE_NOTE_PAD2_RIGHT;	break;
*/
	case 11:	iDanceColOut = BMS_P1_KEY1;				break;
	case 12:	iDanceColOut = BMS_P1_KEY2;				break;
	case 13:	iDanceColOut = BMS_P1_KEY3;				break;
	case 14:	iDanceColOut = BMS_P1_KEY4;				break;
	case 15:	iDanceColOut = BMS_P1_KEY5;				break;
	case 16:	iDanceColOut = BMS_P1_TURN;				break;
	case 18:	iDanceColOut = BMS_P1_KEY6;				break;
	case 19:	iDanceColOut = BMS_P1_KEY7;				break;
	case 21:	iDanceColOut = BMS_P2_KEY1;				break;
	case 22:	iDanceColOut = BMS_P2_KEY2;				break;
	case 23:	iDanceColOut = BMS_P2_KEY3;				break;
	case 24:	iDanceColOut = BMS_P2_KEY4;				break;
	case 25:	iDanceColOut = BMS_P2_KEY5;				break;
	case 26:	iDanceColOut = BMS_P2_TURN;				break;
	case 28:	iDanceColOut = BMS_P2_KEY6;				break;
	case 29:	iDanceColOut = BMS_P2_KEY7;				break;
	default:	iDanceColOut = -1;						break;
	}
}

bool BMSLoader::LoadFromBMSFile( const CString &sPath, Steps &out )
{
	LOG->Trace( "Steps::LoadFromBMSFile( '%s' )", sPath.c_str() );

	out.m_StepsType = STEPS_TYPE_INVALID;

	NoteData* pNoteData = new NoteData;
	pNoteData->SetNumTracks( MAX_NOTE_TRACKS );

	ifstream file(sPath);
	if( file.bad() )
		RageException::Throw( "Failed to open %s for reading.", sPath.c_str() );

	CString line;
	while( getline(file, line) )	// foreach line
	{
		StripCrnl(line);
		CString value_name;		// fill these in
		CString value_data;	

		// BMS value names can be separated by a space or a colon.
		int iIndexOfFirstColon = line.Find( ":" );
		int iIndexOfFirstSpace = line.Find( " " );

		if( iIndexOfFirstColon == -1 )
			iIndexOfFirstColon = 10000;
		if( iIndexOfFirstSpace == -1 )
			iIndexOfFirstSpace = 10000;
		
		int iIndexOfSeparator = min( iIndexOfFirstSpace, iIndexOfFirstColon );

		if( iIndexOfSeparator != 10000 )
		{
			value_name = line.substr( 0, iIndexOfSeparator );
			value_data = line;	// the rest
			value_data.erase(0,iIndexOfSeparator+1);
		}
		else	// no separator
		{
			value_name = line;
		}

		value_name.MakeLower();

		if( -1 != value_name.Find("#player") ) 
		{
			switch( atoi(value_data) )
			{
			case 1:		// 4 or 6 single
				out.m_StepsType = STEPS_TYPE_DANCE_SINGLE;
				// if the mode should be solo, then we'll update m_DanceStyle below when we read in step data
				break;
			case 2:		// couple/battle
				out.m_StepsType = STEPS_TYPE_DANCE_COUPLE;
				break;
			case 3:		// double
				// Fix it if we find that.
				out.m_StepsType = STEPS_TYPE_DANCE_DOUBLE;
				break;
			default:
				LOG->Warn( "Invalid value '%d' for '#player'", atoi(value_data) );
				delete pNoteData;
				return false;
			}
		}
		if( -1 != value_name.Find("#title") )
		{
			value_data.MakeLower();
			
			// extract the Steps description (looks like 'Music <BASIC>')
			int iPosOpenBracket = value_data.Find( "<" );
			if( iPosOpenBracket == -1 )
				iPosOpenBracket = value_data.Find( "(" );
			int iPosCloseBracket = value_data.Find( ">" );
			if( iPosCloseBracket == -1 )
				iPosCloseBracket = value_data.Find( ")" );

			if( iPosOpenBracket != -1  &&  iPosCloseBracket != -1 )
				value_data = value_data.substr( iPosOpenBracket+1, iPosCloseBracket-iPosOpenBracket-1 );
			LOG->Trace( "Steps description found to be '%s'", value_data.c_str() );

			out.SetDescription(value_data);

			// if there's a 6 in the description, it's probably part of "6panel" or "6-panel"
			if( value_data.Find("6") != -1 )
				out.m_StepsType = STEPS_TYPE_DANCE_SOLO;
		}
		if( -1 != value_name.Find("#playlevel") ) 
		{
			out.SetMeter(atoi(value_data));
		}
		else if( value_name.size() >= 6 && value_name[0] == '#'
			 && IsAnInt( value_name.substr(1,3) )
			 && IsAnInt( value_name.substr(4,2) ) )	// this is step or offset data.  Looks like "#00705"
		{
			int iMeasureNo	= atoi( value_name.substr(1,3).c_str() );
			int iTrackNum	= atoi( value_name.substr(4,2).c_str() );

			// MD 10/26/03 - fix for Pop N' and such, including "if there are six panels, then we have Solo" - check here,
			//    then put the correct step type later
			BMS_PushTrackNumForMagic(iTrackNum);
			// end MD 10/26/03

			CString &sNoteData = value_data;
			vector<bool> arrayNotes;

			for( int i=0; i+1<sNoteData.GetLength(); i+=2 )
			{
				bool bThisIsANote = sNoteData.substr(i,2) != "00";
				arrayNotes.push_back( bThisIsANote );
			}

			const unsigned iNumNotesInThisMeasure = arrayNotes.size();
			//LOG->Trace( "%s:%s: iMeasureNo = %d, iNoteNum = %d, iNumNotesInThisMeasure = %d", 
			//	valuename.c_str(), sNoteData.c_str(), iMeasureNo, iNoteNum, iNumNotesInThisMeasure );


			for( unsigned j=0; j<iNumNotesInThisMeasure; j++ )
			{
				if( arrayNotes[j] )
				{
					float fPercentThroughMeasure = (float)j/(float)iNumNotesInThisMeasure;

					const int iNoteIndex = (int) ( (iMeasureNo + fPercentThroughMeasure)
									 * BEATS_PER_MEASURE * ROWS_PER_BEAT );
					int iColumnNumber;
					char cNoteChar;
					
					mapBMSTrackToDanceNote( iTrackNum, iColumnNumber, cNoteChar );

					if( iColumnNumber != -1 )
						pNoteData->SetTapNote(iColumnNumber, iNoteIndex, cNoteChar);
				}
			}
		}
	}
	
	// MD 10/26/03 - dance-couple is the only one we should retain unchanged.
	if( out.m_StepsType != STEPS_TYPE_DANCE_COUPLE)
	{
		out.m_StepsType = CheckTracksMagic();
	}

	/* We fix this later, since we're not moving columns around right now.
	if( out.m_StepsType == STEPS_TYPE_DANCE_SINGLE  || 
		out.m_StepsType == STEPS_TYPE_DANCE_DOUBLE  || 
		out.m_StepsType == STEPS_TYPE_DANCE_COUPLE)	// if there are 4 panels, then the Up+Right track really contains the notes for Up
	{
		pNoteData->MoveTapNoteTrack(DANCE_NOTE_PAD1_UP, DANCE_NOTE_PAD1_UPRIGHT);
	}
	*/

	// we're done reading in all of the BMS values
	if( out.m_StepsType == STEPS_TYPE_INVALID )
	{
		LOG->Warn("Couldn't determine note types of file '%s'", sPath.c_str() );
		delete pNoteData;
		return false;
	}

	int iNumNewTracks = GameManager::NotesTypeToNumTracks( out.m_StepsType );
	int iTransformNewToOld[MAX_NOTE_TRACKS];

	int i;
	for( i = 0; i < MAX_NOTE_TRACKS; ++i)
		iTransformNewToOld[i] = -1;

	switch( out.m_StepsType )
	{
	// MD 10/26/03 fix PNM &c.
	case STEPS_TYPE_DANCE_SINGLE:
		iTransformNewToOld[0] = BMS_P1_KEY1;
		iTransformNewToOld[1] = BMS_P1_KEY3;
		iTransformNewToOld[2] = BMS_P1_KEY5;
		iTransformNewToOld[3] = BMS_P1_TURN;
		break;
	case STEPS_TYPE_DANCE_DOUBLE:
	case STEPS_TYPE_DANCE_COUPLE:
		iTransformNewToOld[0] = BMS_P1_KEY1;
		iTransformNewToOld[1] = BMS_P1_KEY3;
		iTransformNewToOld[2] = BMS_P1_KEY5;
		iTransformNewToOld[3] = BMS_P1_TURN;
		iTransformNewToOld[4] = BMS_P2_KEY1;
		iTransformNewToOld[5] = BMS_P2_KEY3;
		iTransformNewToOld[6] = BMS_P2_KEY5;
		iTransformNewToOld[7] = BMS_P2_TURN;
		break;
	case STEPS_TYPE_DANCE_SOLO:
	case STEPS_TYPE_BM_SINGLE:
		// Hey! Why the hell are these exactly the same? :-)
		iTransformNewToOld[0] = BMS_P1_KEY1;
		iTransformNewToOld[1] = BMS_P1_KEY2;
		iTransformNewToOld[2] = BMS_P1_KEY3;
		iTransformNewToOld[3] = BMS_P1_KEY4;
		iTransformNewToOld[4] = BMS_P1_KEY5;
		iTransformNewToOld[5] = BMS_P1_TURN;
		break;
	case STEPS_TYPE_PNM_FIVE:
		iTransformNewToOld[0] = BMS_P1_KEY3;
		iTransformNewToOld[1] = BMS_P1_KEY4;
		iTransformNewToOld[2] = BMS_P1_KEY5;
		iTransformNewToOld[3] = BMS_P2_KEY1;
		iTransformNewToOld[4] = BMS_P2_KEY2;
		break;		
	case STEPS_TYPE_PNM_NINE:
		iTransformNewToOld[0] = BMS_P1_KEY1;
		iTransformNewToOld[1] = BMS_P1_KEY2;
		iTransformNewToOld[2] = BMS_P1_KEY3;
		iTransformNewToOld[3] = BMS_P1_KEY4;
		iTransformNewToOld[4] = BMS_P1_KEY5;
		iTransformNewToOld[5] = BMS_P2_KEY1;
		iTransformNewToOld[6] = BMS_P2_KEY2;
		iTransformNewToOld[7] = BMS_P2_KEY3;
		iTransformNewToOld[8] = BMS_P2_KEY4;
		break;
	/* MD 10/26/03 - uncomment this section when we get around to BM support outside 5-key
	case STEPS_TYPE_BM_DOUBLE:
		iTransformNewToOld[0] = BMS_P1_KEY1;
		iTransformNewToOld[1] = BMS_P1_KEY2;
		iTransformNewToOld[2] = BMS_P1_KEY3;
		iTransformNewToOld[3] = BMS_P1_KEY4;
		iTransformNewToOld[4] = BMS_P1_KEY5;
		iTransformNewToOld[5] = BMS_P1_TURN;
		iTransformNewToOld[6] = BMS_P2_KEY1;
		iTransformNewToOld[7] = BMS_P2_KEY2;
		iTransformNewToOld[8] = BMS_P2_KEY3;
		iTransformNewToOld[9] = BMS_P2_KEY4;
		iTransformNewToOld[10] = BMS_P2_KEY5;
		iTransformNewToOld[11] = BMS_P2_TURN;
		break;
	case STEPS_TYPE_BM_SINGLE7:
		iTransformNewToOld[0] = BMS_P1_KEY1;
		iTransformNewToOld[1] = BMS_P1_KEY2;
		iTransformNewToOld[2] = BMS_P1_KEY3;
		iTransformNewToOld[3] = BMS_P1_KEY4;
		iTransformNewToOld[4] = BMS_P1_KEY5;
		iTransformNewToOld[5] = BMS_P1_KEY6;
		iTransformNewToOld[6] = BMS_P1_KEY7;
		iTransformNewToOld[7] = BMS_P1_TURN;
	case STEPS_TYPE_BM_DOUBLE7:
		iTransformNewToOld[0] = BMS_P1_KEY1;
		iTransformNewToOld[1] = BMS_P1_KEY2;
		iTransformNewToOld[2] = BMS_P1_KEY3;
		iTransformNewToOld[3] = BMS_P1_KEY4;
		iTransformNewToOld[4] = BMS_P1_KEY5;
		iTransformNewToOld[5] = BMS_P1_KEY6;
		iTransformNewToOld[6] = BMS_P1_KEY7;
		iTransformNewToOld[7] = BMS_P1_TURN;
		iTransformNewToOld[8] = BMS_P2_KEY1;
		iTransformNewToOld[9] = BMS_P2_KEY2;
		iTransformNewToOld[10] = BMS_P2_KEY3;
		iTransformNewToOld[11] = BMS_P2_KEY4;
		iTransformNewToOld[12] = BMS_P2_KEY5;
		iTransformNewToOld[13] = BMS_P2_KEY6;
		iTransformNewToOld[14] = BMS_P2_KEY7;
		iTransformNewToOld[15] = BMS_P2_TURN;
	*/
	default:
		ASSERT(0);
	}

	NoteData* pNoteData2 = new NoteData;
	pNoteData2->SetNumTracks( iNumNewTracks );
	pNoteData2->LoadTransformed( pNoteData, iNumNewTracks, iTransformNewToOld );

	out.SetNoteData(pNoteData2);

	delete pNoteData;
	delete pNoteData2;

	out.TidyUpData();

	return true;
}

void BMSLoader::GetApplicableFiles( CString sPath, CStringArray &out )
{
	GetDirListing( sPath + CString("*.bms"), out );
}

bool BMSLoader::LoadFromDir( CString sDir, Song &out )
{
	LOG->Trace( "Song::LoadFromBMSDir(%s)", sDir.c_str() );

	CStringArray arrayBMSFileNames;
	GetApplicableFiles( sDir, arrayBMSFileNames );

	/* We should have at least one; if we had none, we shouldn't have been
	 * called to begin with. */
	ASSERT( arrayBMSFileNames.size() );

	// load the Steps from the rest of the BMS files
	unsigned i;
	for( i=0; i<arrayBMSFileNames.size(); i++ ) 
	{
		Steps* pNewNotes = new Steps;

		const bool ok = LoadFromBMSFile( out.GetSongDir() + arrayBMSFileNames[i], *pNewNotes );
		if( ok )
			out.m_apNotes.push_back( pNewNotes );
		else
			delete pNewNotes;
	}

	CString sPath = out.GetSongDir() + arrayBMSFileNames[0];

	ifstream file(sPath);
	if( file.bad() )
		RageException::Throw( "Failed to open %s for reading.", sPath.c_str() );

	CString line;
	while( getline(file, line) )	// foreach line
	{
		StripCrnl(line);

		CString value_name;		// fill these in
		CString value_data;	

		// BMS value names can be separated by a space or a colon.
		int iIndexOfFirstColon = line.Find( ":" );
		int iIndexOfFirstSpace = line.Find( " " );

		if( iIndexOfFirstColon == -1 )
			iIndexOfFirstColon = 10000;
		if( iIndexOfFirstSpace == -1 )
			iIndexOfFirstSpace = 10000;
		
		int iIndexOfSeparator = min( iIndexOfFirstSpace, iIndexOfFirstColon );

		if( iIndexOfSeparator != 10000 )
		{
			value_name = line.substr( 0, iIndexOfSeparator );
			value_data = line;	// the rest
			value_data.erase(0,iIndexOfSeparator+1);
		}
		else	// no separator
		{
			value_name = line;
		}


		value_name.MakeLower();


		// handle the data
		if( value_name == "#title" ) 
		{
			// strip Steps type out of description leaving only song title - looks like 'B4U <BASIC>'
			unsigned iIndex = value_data.find_last_of('<');
			if( iIndex == value_data.npos )
				iIndex = value_data.find_last_of('(');
			if( iIndex != value_data.npos )
			{
				value_data = value_data.Left( iIndex );
				GetMainAndSubTitlesFromFullTitle( value_data, out.m_sMainTitle, out.m_sSubTitle );
			}
			else
				out.m_sMainTitle = value_data;
		}
		else if( value_name == "#artist" ) 
		{
			out.m_sArtist = value_data;
		}
		else if( value_name == "#bpm" ) 
		{
			BPMSegment newSeg( 0, (float)atof(value_data) );
			out.AddBPMSegment( newSeg );
		
			LOG->Trace( "Inserting new BPM change at beat %f, BPM %f", newSeg.m_fStartBeat, newSeg.m_fBPM );
		}
		else if( value_name == "#backbmp" ) 
		{
			out.m_sBackgroundFile = value_data;
		}
		else if( value_name == "#wav" ) 
		{
			out.m_sMusicFile = value_data;
		}
		else if( value_name.size() >= 6 && value_name[0] == '#'
			 && IsAnInt( value_name.substr(1,3) )
			 && IsAnInt( value_name.substr(4,2) ) )	// this is step or offset data.  Looks like "#00705"
		{
			int iMeasureNo	= atoi( value_name.substr(1,3).c_str() );
			int iBMSTrackNo	= atoi( value_name.substr(4,2).c_str() );

			CString sNoteData = value_data;
			vector<int> arrayNotes;

			for( int i=0; i+1<sNoteData.GetLength(); i+=2 )
			{
				CString sNote = sNoteData.substr(i,2);
				int iNote;
				sscanf( sNote, "%x", &iNote );	// data is in hexadecimal
				arrayNotes.push_back( iNote );
			}

			const unsigned iNumNotesInThisMeasure = arrayNotes.size();
			//LOG->Trace( "%s:%s: iMeasureNo = %d, iBMSTrackNo = %d, iNumNotesInThisMeasure = %d", 
			//	valuename.c_str(), sNoteData.c_str(), iMeasureNo, iBMSTrackNo, iNumNotesInThisMeasure );
			for( unsigned j=0; j<iNumNotesInThisMeasure; j++ )
			{
				if( arrayNotes[j] == 0 )
					continue;

				float fPercentThroughMeasure = (float)j/(float)iNumNotesInThisMeasure;

				// index is in quarter beats starting at beat 0
				int iStepIndex = (int) ( (iMeasureNo + fPercentThroughMeasure)
								 * BEATS_PER_MEASURE * ROWS_PER_BEAT );

				switch( iBMSTrackNo )
				{
				case 1:	{ // background music track
					float fBeatOffset = fBeatOffset = NoteRowToBeat( (float)iStepIndex );
					if( fBeatOffset > 10 )	// some BPMs's play the music again at the end.  Why?  Who knows...
						break;
					float fBPS;
					fBPS = out.m_BPMSegments[0].m_fBPM/60.0f;
					out.m_fBeat0OffsetInSeconds = fBeatOffset / fBPS;
					//LOG->Trace( "Found offset to be index %d, beat %f", iStepIndex, NoteRowToBeat(iStepIndex) );
					break;
				}
				case 3:	{ // bpm change
					BPMSegment newSeg( NoteRowToBeat(iStepIndex), (float)arrayNotes[j] );
					out.AddBPMSegment( newSeg );
					LOG->Trace( "Inserting new BPM change at beat %f, BPM %f", newSeg.m_fStartBeat, newSeg.m_fBPM );
					break;
				}

				// Let me just take a moment to express how frustrated I am with the new, 
				// poorly-designed changes to the BMS format.
				//
				//
				// AAAAAAAAAAAAAAAAAAAAAAAAAAAAAaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaahhhhhhhhhhhhhhhhhhhhhhhh!!!!!!!!!!!!!!!
				//
				// Thank you.

				// MD 10/26/03 - And allow me to add to that my opinion of the BMS format in general.
				//
				// ()*&@^*&^@$(*&^!@(*&^($*&^!#(*&@!&*#*#*@#*@#(*@(%@(*!
				//
				// end MD 10/26/03

				case 8:	{ // indirect bpm
					// This is a very inefficient way to parse, but it doesn't matter much
					// because this is only parsed on the first run after the song is installed.
					CString sTagToLookFor = ssprintf( "#BPM%02x", arrayNotes[j] );
					float fBPM = -1;


					// open the song file again and and look for this tag's value
					ifstream file(sPath);
					if( file.bad() )
						RageException::Throw( "Failed to open %s for reading.", sPath.c_str() );

					CString line;
					while( getline(file, line) )	// foreach line
					{
						StripCrnl(line);
						CString value_name;		// fill these in
						CString value_data;	

						// BMS value names can be separated by a space or a colon.
						int iIndexOfFirstColon = line.Find( ":" );
						int iIndexOfFirstSpace = line.Find( " " );

						if( iIndexOfFirstColon == -1 )
							iIndexOfFirstColon = 10000;
						if( iIndexOfFirstSpace == -1 )
							iIndexOfFirstSpace = 10000;
						
						int iIndexOfSeparator = min( iIndexOfFirstSpace, iIndexOfFirstColon );

						if( iIndexOfSeparator != 10000 )
						{
							value_name = line.substr( 0, iIndexOfSeparator );
							value_data = line;	// the rest
							value_data.erase(0,iIndexOfSeparator+1);
						}
						else	// no separator
						{
							value_name = line;
						}

						if( 0==stricmp(value_name, sTagToLookFor) )
						{
							fBPM = (float)atof( value_data );
							break;
						}
					}

					if( fBPM == -1 )	// we didn't find the line we were looking for
					{
						LOG->Trace( "WARNING:  Couldn't find tag '%s' in '%s'.", sTagToLookFor.c_str(), sPath.c_str() );
					}
					else
					{
						BPMSegment newSeg( NoteRowToBeat(iStepIndex), fBPM );
						out.AddBPMSegment( newSeg );
						LOG->Trace( "Inserting new BPM change at beat %f, BPM %f", newSeg.m_fStartBeat, newSeg.m_fBPM );
					}

					break;
				}
				case 9:	{ // stop
					// This is a very inefficient way to parse, but it doesn't 
					// matter much because this is only parsed on the first run after the song is installed.
					CString sTagToLookFor = ssprintf( "#STOP%02x", arrayNotes[j] );
					float fFreezeStartBeat = NoteRowToBeat(iStepIndex);
					float fFreezeSecs = -1;


					// open the song file again and and look for this tag's value
					ifstream file(sPath);
					if( file.bad() )
						RageException::Throw( "Failed to open %s for reading.", sPath.c_str() );

					CString line;
					while( getline(file, line) )	// foreach line
					{
						StripCrnl(line);
						CString value_name;		// fill these in
						CString value_data;	

						// BMS value names can be separated by a space or a colon.
						int iIndexOfFirstColon = line.Find( ":" );
						int iIndexOfFirstSpace = line.Find( " " );

						if( iIndexOfFirstColon == -1 )
							iIndexOfFirstColon = 10000;
						if( iIndexOfFirstSpace == -1 )
							iIndexOfFirstSpace = 10000;
						
						int iIndexOfSeparator = min( iIndexOfFirstSpace, iIndexOfFirstColon );

						if( iIndexOfSeparator != 10000 )
						{
							value_name = line.substr( 0, iIndexOfSeparator );
							value_data = line;	// the rest
							value_data.erase(0,iIndexOfSeparator+1);
						}
						else	// no separator
						{
							value_name = line;
						}

						if( 0==stricmp(value_name, sTagToLookFor) )
						{
							// find the BPM at the time of this freeze
							float fBPM = -1;
							for( unsigned i=0; i<out.m_BPMSegments.size()-1; i++ )
							{
								if( out.m_BPMSegments[i].m_fStartBeat <= fFreezeStartBeat && 
									out.m_BPMSegments[i+1].m_fStartBeat > fFreezeStartBeat )
								{
									fBPM = out.m_BPMSegments[i].m_fBPM;
									break;
								}
							}
							// the BPM segment of this beat is the last BPM segment
							if( fBPM == -1 )
								fBPM = out.m_BPMSegments[out.m_BPMSegments.size()-1].m_fBPM;

							fFreezeSecs = (float)atof(value_data)/(fBPM*0.81f);	// I have no idea what units these are in, so I experimented until finding this factor.
							break;
						}
					}

					if( fFreezeSecs == -1 )	// we didn't find the line we were looking for
					{
						LOG->Trace( "WARNING:  Couldn't find tag '%s' in '%s'.", sTagToLookFor.c_str(), sPath.c_str() );
					}
					else
					{
						StopSegment newSeg( fFreezeStartBeat, fFreezeSecs );
						out.AddStopSegment( newSeg );
						LOG->Trace( "Inserting new Freeze at beat %f, secs %f", newSeg.m_fStartBeat, newSeg.m_fStopSeconds );
					}

					break;
				}
				}
			}
		}
	}

	for( i=0; i<out.m_BPMSegments.size(); i++ )
		LOG->Trace( "There is a BPM change at beat %f, BPM %f, index %d", 
					out.m_BPMSegments[i].m_fStartBeat, out.m_BPMSegments[i].m_fBPM, i );

	return true;
}

