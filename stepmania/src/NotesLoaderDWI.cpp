#include "global.h"

#include "NotesLoaderDWI.h"
#include "NotesLoader.h"
#include "RageLog.h"
#include "RageException.h"
#include "MsdFile.h"
#include "RageUtil.h"
#include "RageUtil_CharConversions.h"
#include "NoteData.h"

#include <map>
using namespace std;

static std::map<int,int> g_mapDanceNoteToNoteDataColumn;

void DWILoader::DWIcharToNote( char c, GameController i, DanceNote &note1Out, DanceNote &note2Out )
{
	switch( c )
		{
		case '0':	note1Out = DANCE_NOTE_NONE;			note2Out = DANCE_NOTE_NONE;			break;
		case '1':	note1Out = DANCE_NOTE_PAD1_DOWN;	note2Out = DANCE_NOTE_PAD1_LEFT;	break;
		case '2':	note1Out = DANCE_NOTE_PAD1_DOWN;	note2Out = DANCE_NOTE_NONE;			break;
		case '3':	note1Out = DANCE_NOTE_PAD1_DOWN;	note2Out = DANCE_NOTE_PAD1_RIGHT;	break;
		case '4':	note1Out = DANCE_NOTE_PAD1_LEFT;	note2Out = DANCE_NOTE_NONE;			break;
		case '5':	note1Out = DANCE_NOTE_NONE;			note2Out = DANCE_NOTE_NONE;			break;
		case '6':	note1Out = DANCE_NOTE_PAD1_RIGHT;	note2Out = DANCE_NOTE_NONE;			break;
		case '7':	note1Out = DANCE_NOTE_PAD1_UP;		note2Out = DANCE_NOTE_PAD1_LEFT;	break;
		case '8':	note1Out = DANCE_NOTE_PAD1_UP;		note2Out = DANCE_NOTE_NONE;			break;
		case '9':	note1Out = DANCE_NOTE_PAD1_UP;		note2Out = DANCE_NOTE_PAD1_RIGHT;	break;
		case 'A':	note1Out = DANCE_NOTE_PAD1_UP;		note2Out = DANCE_NOTE_PAD1_DOWN;	break;
		case 'B':	note1Out = DANCE_NOTE_PAD1_LEFT;	note2Out = DANCE_NOTE_PAD1_RIGHT;	break;
		case 'C':	note1Out = DANCE_NOTE_PAD1_UPLEFT;	note2Out = DANCE_NOTE_NONE;			break;
		case 'D':	note1Out = DANCE_NOTE_PAD1_UPRIGHT;	note2Out = DANCE_NOTE_NONE;			break;
		case 'E':	note1Out = DANCE_NOTE_PAD1_LEFT;	note2Out = DANCE_NOTE_PAD1_UPLEFT;	break;
		case 'F':	note1Out = DANCE_NOTE_PAD1_UPLEFT;	note2Out = DANCE_NOTE_PAD1_DOWN;	break;
		case 'G':	note1Out = DANCE_NOTE_PAD1_UPLEFT;	note2Out = DANCE_NOTE_PAD1_UP;		break;
		case 'H':	note1Out = DANCE_NOTE_PAD1_UPLEFT;	note2Out = DANCE_NOTE_PAD1_RIGHT;	break;
		case 'I':	note1Out = DANCE_NOTE_PAD1_LEFT;	note2Out = DANCE_NOTE_PAD1_UPRIGHT;	break;
		case 'J':	note1Out = DANCE_NOTE_PAD1_DOWN;	note2Out = DANCE_NOTE_PAD1_UPRIGHT;	break;
		case 'K':	note1Out = DANCE_NOTE_PAD1_UP;		note2Out = DANCE_NOTE_PAD1_UPRIGHT;	break;
		case 'L':	note1Out = DANCE_NOTE_PAD1_UPRIGHT;	note2Out = DANCE_NOTE_PAD1_RIGHT;	break;
		case 'M':	note1Out = DANCE_NOTE_PAD1_UPLEFT;	note2Out = DANCE_NOTE_PAD1_UPRIGHT;	break;
		default:	
			LOG->Warn( "Encountered invalid DWI note character '%c' in '%s'", c, m_sLoadingFile.c_str() );
			note1Out = DANCE_NOTE_NONE;			note2Out = DANCE_NOTE_NONE;			break;
	}

	switch( i )
	{
	case GAME_CONTROLLER_1:
		break;
	case GAME_CONTROLLER_2:
		if( note1Out != DANCE_NOTE_NONE )
			note1Out += 6;
		if( note2Out != DANCE_NOTE_NONE )
			note2Out += 6;
		break;
	default:
		ASSERT( false );
	}
}

void DWILoader::DWIcharToNoteCol( char c, GameController i, int &col1Out, int &col2Out )
{
	DanceNote note1, note2;
	DWIcharToNote( c, i, note1, note2 );

	if( note1 != DANCE_NOTE_NONE )
		col1Out = g_mapDanceNoteToNoteDataColumn[note1];
	else
		col1Out = -1;

	if( note2 != DANCE_NOTE_NONE )
		col2Out = g_mapDanceNoteToNoteDataColumn[note2];
	else
		col2Out = -1;
}

/* Ack.  DWI used to use <...> to indicate 1/192nd notes; at some
 * point, <...> was changed to indicate jumps, and `' was used for
 * 1/192nds.  So, we have to do a check to figure out what it really
 * means.  If it contains 0s, it's most likely 192nds; otherwise,
 * it's most likely a jump.  Search for a 0 before the next >: */
bool DWILoader::Is192( CString &sStepData, int pos )
{
	while( pos < (int) sStepData.size() )
	{
		if( sStepData[pos] == '>' )
			return false;
		if( sStepData[pos] == '0' )
			return true;
		++pos;
	}
	
	return false;
}

bool DWILoader::LoadFromDWITokens( 
	CString sMode, 
	CString sDescription,
	CString sNumFeet,
	CString sStepData1, 
	CString sStepData2,
	Steps &out)
{
	LOG->Trace( "Steps::LoadFromDWITokens()" );

	out.m_NotesType = STEPS_TYPE_INVALID;

	sStepData1.Replace( "\n", "" );
	sStepData1.Replace( "\r", "" );
	sStepData1.Replace( " ", "" );
	sStepData2.Replace( "\n", "" );
	sStepData2.Replace( "\r", "" );
	sStepData2.Replace( " ", "" );

	if(		 sMode == "SINGLE" )	out.m_NotesType = STEPS_TYPE_DANCE_SINGLE;
	else if( sMode == "DOUBLE" )	out.m_NotesType = STEPS_TYPE_DANCE_DOUBLE;
	else if( sMode == "COUPLE" )	out.m_NotesType = STEPS_TYPE_DANCE_COUPLE;
	else if( sMode == "SOLO" )		out.m_NotesType = STEPS_TYPE_DANCE_SOLO;
	else	
	{
		ASSERT(0);	// Unrecognized DWI notes format
		out.m_NotesType = STEPS_TYPE_DANCE_SINGLE;
	}


	g_mapDanceNoteToNoteDataColumn.clear();
	switch( out.m_NotesType )
	{
	case STEPS_TYPE_DANCE_SINGLE:
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_LEFT] = 0;
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_DOWN] = 1;
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_UP] = 2;
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_RIGHT] = 3;
		break;
	case STEPS_TYPE_DANCE_DOUBLE:
	case STEPS_TYPE_DANCE_COUPLE:
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_LEFT] = 0;
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_DOWN] = 1;
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_UP] = 2;
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_RIGHT] = 3;
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD2_LEFT] = 4;
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD2_DOWN] = 5;
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD2_UP] = 6;
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD2_RIGHT] = 7;
		break;
	case STEPS_TYPE_DANCE_SOLO:
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_LEFT] = 0;
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_UPLEFT] = 1;
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_DOWN] = 2;
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_UP] = 3;
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_UPRIGHT] = 4;
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_RIGHT] = 5;
		break;
	default:
		ASSERT(0);
	}

	int iNumFeet = atoi(sNumFeet);
	out.SetDescription(sDescription);
	out.SetMeter(iNumFeet);
	if( sDescription.CompareNoCase("basic")==0 )		out.SetDifficulty( DIFFICULTY_EASY );
	else if( sDescription.CompareNoCase("trick")==0 )	out.SetDifficulty( DIFFICULTY_MEDIUM );
	else if( sDescription.CompareNoCase("maniac")==0 )	out.SetDifficulty( DIFFICULTY_HARD );
	else if( sDescription.CompareNoCase("smaniac")==0 )	out.SetDifficulty( DIFFICULTY_CHALLENGE );

	NoteData* pNoteData = new NoteData;
	ASSERT( pNoteData );
	pNoteData->SetNumTracks( g_mapDanceNoteToNoteDataColumn.size() );

	for( int pad=0; pad<2; pad++ )		// foreach pad
	{
		CString sStepData;
		switch( pad )
		{
		case 0:
			sStepData = sStepData1;
			break;
		case 1:
			if( sStepData2 == "" )	// no data
				continue;	// skip
			sStepData = sStepData2;
			break;
		default:
			ASSERT( false );
		}

		sStepData.Replace("\t", "");
		sStepData.Replace(" ", "");

		double fCurrentBeat = 0;
		double fCurrentIncrementer = 1.0/8 * BEATS_PER_MEASURE;

		for( int i=0; i<sStepData.GetLength(); )
		{
			char c = sStepData[i++];
			switch( c )
			{
			// begins a series
			case '(':
				fCurrentIncrementer = 1.0/16 * BEATS_PER_MEASURE;
				break;
			case '[':
				fCurrentIncrementer = 1.0/24 * BEATS_PER_MEASURE;
				break;
			case '{':
				fCurrentIncrementer = 1.0/64 * BEATS_PER_MEASURE;
				break;
			case '`':
				fCurrentIncrementer = 1.0/192 * BEATS_PER_MEASURE;
				break;

			// ends a series
			case ')':
			case ']':
			case '}':
			case '\'':
			case '>':
				fCurrentIncrementer = 1.0/8 * BEATS_PER_MEASURE;
				break;

			default:	// this is a note character
			{
				if( c == '!' )
				{
					LOG->Warn( "Unexpected character in '%s': '!'", m_sLoadingFile.c_str() );
					continue;
				}

				bool jump = false;
				if( c == '<' )
				{
					/* Arr.  Is this a jump or a 1/192 marker? */
					if( Is192( sStepData, i ) )
					{
						fCurrentIncrementer = 1.0/192 * BEATS_PER_MEASURE;
						break;
					}
					
					/* It's a jump.  We need to keep reading notes until we hit a >. */
					jump = true;
					i++;
				}
				
				const int iIndex = BeatToNoteRow( (float)fCurrentBeat );
				i--;
				do {
					c = sStepData[i++];

					if( jump && c == '>' )
						break;

					int iCol1, iCol2;
					DWIcharToNoteCol( c, (GameController)pad, iCol1, iCol2 );

					if( iCol1 != -1 )
						pNoteData->SetTapNote(iCol1, iIndex, TAP_TAP);
					if( iCol2 != -1 )
						pNoteData->SetTapNote(iCol2, iIndex, TAP_TAP);

					if( sStepData[i] == '!' )
					{
						i++;
						const char holdChar = sStepData[i++];
						
						DWIcharToNoteCol( holdChar, (GameController)pad, iCol1, iCol2 );

						if( iCol1 != -1 )
							pNoteData->SetTapNote(iCol1, iIndex, TAP_HOLD_HEAD);
						if( iCol2 != -1 )
							pNoteData->SetTapNote(iCol2, iIndex, TAP_HOLD_HEAD);

					}
				}
				while( jump );
				fCurrentBeat += fCurrentIncrementer;
			}
			break;
		}
		}
	}

	// this will expand the HoldNote begin markers we wrote into actual HoldNotes
	pNoteData->Convert2sAnd3sToHoldNotes();

	ASSERT( pNoteData->GetNumTracks() > 0 );

	out.SetNoteData(pNoteData);

	delete pNoteData;
	out.TidyUpData();

	return true;
}

/* STUPID ALERT!
 * This value can be in either "HH:MM:SS.sssss", "MM:SS.sssss", "SSS.sssss"
 * or milliseconds.
 * What's even more dumb is that the value can contain a ':'.  Colon is supposed to be a parameter separator!
 */
float DWILoader::ParseBrokenDWITimestamp(const CString &arg1, const CString &arg2, const CString &arg3)
{
	if(arg1.empty()) return 0;

	/* 1+ args */
	if(arg2.empty())
	{
		/* If the value contains a period, treat it as seconds; otherwise ms. */
		if(arg1.find_first_of(".") != arg1.npos)
			return (float)atof(arg1.c_str());
		else
			return float(atof(arg1.c_str())) / 1000.f;
	}

	/* 2+ args */
	if(arg3.empty())
		return TimeToSeconds( arg1+":"+arg2 );

	/* 3+ args */
	return TimeToSeconds( arg1+":"+arg2+":"+arg3 );
}

bool DWILoader::LoadFromDWIFile( CString sPath, Song &out )
{
	LOG->Trace( "Song::LoadFromDWIFile(%s)", sPath.c_str() );
	m_sLoadingFile = sPath;

	MsdFile msd;
	bool bResult = msd.ReadFile( sPath );
	if( !bResult )
		RageException::Throw( "Error opening file '%s' for reading.", sPath.c_str() );

	for( unsigned i=0; i<msd.GetNumValues(); i++ )
	{
		int iNumParams = msd.GetNumParams(i);
		const MsdFile::value_t &sParams = msd.GetValue(i);
		CString sValueName = sParams[0];

		if(iNumParams < 1)
		{
			LOG->Warn("Got \"%s\" tag with no parameters in '%s'", sValueName.c_str(), m_sLoadingFile.c_str() );
			continue;
		}

		// handle the data
		if( 0==stricmp(sValueName,"FILE") )
			out.m_sMusicFile = sParams[1];

		else if( 0==stricmp(sValueName,"TITLE") )
		{
			GetMainAndSubTitlesFromFullTitle( sParams[1], out.m_sMainTitle, out.m_sSubTitle );

			/* As far as I know, there's no spec on the encoding of this text. (I didn't
			 * look very hard, though.)  I've seen at least one file in ISO-8859-1. */
			ConvertString( out.m_sMainTitle, "utf-8,english" );
			ConvertString( out.m_sSubTitle, "utf-8,english" );
		}

		else if( 0==stricmp(sValueName,"ARTIST") )
		{
			out.m_sArtist = sParams[1];
			ConvertString( out.m_sArtist, "utf-8,english" );
		}

		else if( 0==stricmp(sValueName,"CDTITLE") )
			out.m_sCDTitleFile = sParams[1];

		else if( 0==stricmp(sValueName,"BPM") )
			out.AddBPMSegment( BPMSegment(0, (float)atof(sParams[1])) );		

		else if( 0==stricmp(sValueName,"DISPLAYBPM") )
		{
			// #DISPLAYBPM:[xxx..xxx]|[xxx]|[*]; 
		    int iMin, iMax;
			/* We can't parse this as a float with sscanf, since '.' is a valid
			 * character in a float.  (We could do it with a regex, but it's not
			 * worth bothering with since we don't display fractional BPM anyway.) */
		    if( sscanf( sParams[1], "%i..%i", &iMin, &iMax ) == 2 )
			{
				out.m_DisplayBPMType = Song::DISPLAY_SPECIFIED;
				out.m_fSpecifiedBPMMin = (float) iMin;
				out.m_fSpecifiedBPMMax = (float) iMax;
			}
			else if( sscanf( sParams[1], "%i", &iMin ) == 1 )
			{
				out.m_DisplayBPMType = Song::DISPLAY_SPECIFIED;
				out.m_fSpecifiedBPMMin = out.m_fSpecifiedBPMMax = (float) iMin;
			}
			else
			{
				out.m_DisplayBPMType = Song::DISPLAY_RANDOM;
			}
		}

		else if( 0==stricmp(sValueName,"GAP") )
			// the units of GAP is 1/1000 second
			out.m_fBeat0OffsetInSeconds = -atoi( sParams[1] ) / 1000.0f;

		else if( 0==stricmp(sValueName,"SAMPLESTART") )
			out.m_fMusicSampleStartSeconds = ParseBrokenDWITimestamp(sParams[1], sParams[2], sParams[3]);

		else if( 0==stricmp(sValueName,"SAMPLELENGTH") )
			out.m_fMusicSampleLengthSeconds = ParseBrokenDWITimestamp(sParams[1], sParams[2], sParams[3]);

		else if( 0==stricmp(sValueName,"FREEZE") )
		{
			CStringArray arrayFreezeExpressions;
			split( sParams[1], ",", arrayFreezeExpressions );

			for( unsigned f=0; f<arrayFreezeExpressions.size(); f++ )
			{
				CStringArray arrayFreezeValues;
				split( arrayFreezeExpressions[f], "=", arrayFreezeValues );
				float fIndex = (float)atof( arrayFreezeValues[0] ) * ROWS_PER_BEAT / 4.0f;
				float fFreezeBeat = NoteRowToBeat( fIndex );
				float fFreezeSeconds = (float)atof( arrayFreezeValues[1] ) / 1000.0f;
				
				out.AddStopSegment( StopSegment(fFreezeBeat, fFreezeSeconds) );
				LOG->Trace( "Adding a freeze segment: beat: %f, seconds = %f", fFreezeBeat, fFreezeSeconds );
			}
		}

		else if( 0==stricmp(sValueName,"CHANGEBPM")  || 0==stricmp(sValueName,"BPMCHANGE") )
		{
			CStringArray arrayBPMChangeExpressions;
			split( sParams[1], ",", arrayBPMChangeExpressions );

			for( unsigned b=0; b<arrayBPMChangeExpressions.size(); b++ )
			{
				CStringArray arrayBPMChangeValues;
				split( arrayBPMChangeExpressions[b], "=", arrayBPMChangeValues );
				if(arrayBPMChangeValues.size() != 2) {
					LOG->Warn( "Invalid CHANGEBPM in '%s': '%s'", m_sLoadingFile.c_str(), arrayBPMChangeExpressions[b].c_str() );
					continue;
				}
				float fIndex = (float)atof( arrayBPMChangeValues[0] ) * ROWS_PER_BEAT / 4.0f;
				float fBeat = NoteRowToBeat( fIndex );
				float fNewBPM = (float)atof( arrayBPMChangeValues[1] );
				
				out.AddBPMSegment( BPMSegment(fBeat, fNewBPM) );
			}
		}

		else if( 0==stricmp(sValueName,"SINGLE")  || 
			     0==stricmp(sValueName,"DOUBLE")  ||
				 0==stricmp(sValueName,"COUPLE")  || 
				 0==stricmp(sValueName,"SOLO"))
		{
			Steps* pNewNotes = new Steps;
			LoadFromDWITokens( 
				sParams[0], 
				sParams[1], 
				sParams[2], 
				sParams[3], 
				(iNumParams==5) ? sParams[4] : "",
				*pNewNotes
				);
			if(pNewNotes->m_NotesType != STEPS_TYPE_INVALID)
				out.m_apNotes.push_back( pNewNotes );
			else
				delete pNewNotes;
		}
		else if( 0==stricmp(sValueName,"DISPLAYTITLE") ||
			0==stricmp(sValueName,"DISPLAYARTIST") )
		{
			/* We don't want to support these tags.  However, we don't want
			 * to pick up images used here as song images (eg. banners). */
			CString param = sParams[1];
			/* "{foo} ... {foo2}" */
			unsigned pos = 0;
			while( pos < CString::npos )
			{

				unsigned startpos = param.find('{', pos);
				if( startpos == CString::npos )
					break;
				unsigned endpos = param.find('}', startpos);
				if( endpos == CString::npos )
					break;

				CString sub = param.substr( startpos+1, endpos-startpos-1 );

				pos = endpos + 1;

				BlacklistedImages.insert( sub.c_str() );
			}
		}
		else
			// do nothing.  We don't care about this value name
			;
	}

	return true;
}

void DWILoader::GetApplicableFiles( CString sPath, CStringArray &out )
{
	GetDirListing( sPath + CString("*.dwi"), out );
}

bool DWILoader::LoadFromDir( CString sPath, Song &out )
{
	CStringArray aFileNames;
	GetApplicableFiles( sPath, aFileNames );

	if( aFileNames.size() > 1 )
		RageException::Throw( "There is more than one DWI file in '%s'.  There should be only one!", sPath.c_str() );

	/* We should have exactly one; if we had none, we shouldn't have been
	 * called to begin with. */
	ASSERT( aFileNames.size() == 1 );

	return LoadFromDWIFile( sPath + aFileNames[0], out );
}

