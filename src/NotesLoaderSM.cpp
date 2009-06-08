#include "global.h"
#include "NotesLoaderSM.h"
#include "BackgroundUtil.h"
#include "GameManager.h"
#include "MsdFile.h"
#include "NoteTypes.h"
#include "RageFileManager.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "Song.h"
#include "SongManager.h"
#include "Steps.h"

const int MAX_EDIT_STEPS_SIZE_BYTES		= 30*1024;	// 30KB

// TODO: Timing data with Steps.  Include a fallback for files that don't include this info.

static void LoadFromSMTokens( 
			     RString sStepsType, 
			     RString sDescription,
			     RString sDifficulty,
			     RString sMeter,
			     RString sRadarValues,
			     RString sNoteData,
			     RString sBPMData,
			     RString sStopsData,
			     RString sTimeSignaturesData,
			     TimingData td,
			     Steps &out
			     )
{
	out.SetSavedToDisk( true );	// we're loading from disk, so this is by definintion already saved

	Trim( sStepsType );
	Trim( sDescription );
	Trim( sDifficulty );
	Trim( sNoteData );
	Trim( sBPMData );
	Trim( sStopsData );
	Trim( sTimeSignaturesData );

	//	LOG->Trace( "Steps::LoadFromSMTokens()" );

	out.m_StepsType = GameManager::StringToStepsType( sStepsType );
	out.SetDescription( sDescription );
	out.SetDifficulty( DwiCompatibleStringToDifficulty(sDifficulty) );

	// HACK:  We used to store SMANIAC as Difficulty_Hard with special description.
	// Now, it has its own Difficulty_Challenge
	if( sDescription.CompareNoCase("smaniac") == 0 ) 
		out.SetDifficulty( Difficulty_Challenge );
	// HACK:  We used to store CHALLENGE as Difficulty_Hard with special description.
	// Now, it has its own Difficulty_Challenge
	if( sDescription.CompareNoCase("challenge") == 0 ) 
		out.SetDifficulty( Difficulty_Challenge );

	out.SetMeter( atoi(sMeter) );
	vector<RString> saValues;
	split( sRadarValues, ",", saValues, true );
	if( saValues.size() == NUM_RadarCategory * NUM_PLAYERS )
	{
		RadarValues v[NUM_PLAYERS];
		FOREACH_PlayerNumber( pn )
			FOREACH_ENUM( RadarCategory, rc )
			v[pn][rc] = StringToFloat( saValues[pn*NUM_RadarCategory + rc] );
		out.SetCachedRadarValues( v );
	}

	// TODO: add proper timingdata
	// bpms
	if( sBPMData.CompareNoCase("") == 0 )
	{
		// import BPM data from Song
		LOG->Trace( "imported timing data / bpm at beat 8: %f", td.GetBPMAtBeat(8) );
		out.m_Timing.m_BPMSegments = td.m_BPMSegments;
	}
	else
	{
		// BPM data provided in Steps
		LOG->Trace( "Steps::LoadFromSMTokens(): attempting to load bpm segments from steps..." );
		vector<RString> arrayBPMChangeExpressions;
		split( sBPMData, ",", arrayBPMChangeExpressions );
		
		for( unsigned b=0; b<arrayBPMChangeExpressions.size(); b++ )
		{
			vector<RString> arrayBPMChangeValues;
			split( arrayBPMChangeExpressions[b], "=", arrayBPMChangeValues );
			// XXX: Hard to tell which file caused this.
			if( arrayBPMChangeValues.size() != 2 )
			{
				LOG->UserLog( "[LoadFromSMTokens] Song file", "(UNKNOWN)", "has an invalid #%s value \"%s\" (must have exactly one '='), ignored.",
					     "BPM:", arrayBPMChangeExpressions[b].c_str() );
				continue;
			}
			
			const float fBeat = StringToFloat( arrayBPMChangeValues[0] );
			const float fNewBPM = StringToFloat( arrayBPMChangeValues[1] );
			
			if( fNewBPM > 0.0f )
				out.AddBPMSegment( BPMSegment(BeatToNoteRow(fBeat), fNewBPM) );
			else
				LOG->UserLog( "[LoadFromSMTokens] Song file", "(UNKNOWN)", "has an invalid BPM change at beat %f, BPM %f.", fBeat, fNewBPM );
		}
	}
	
	// stops
	if( sStopsData.CompareNoCase("") == 0 )
	{
		// import Stops data from Song
		if( td.HasStops() )
			LOG->Trace( "imported timing data / song timing data has stops" );
		else
			LOG->Trace( "imported timing data / song timing data has no stops" );
		out.m_Timing.m_StopSegments = td.m_StopSegments;
	}
	else
	{
		// Stops data provided in Steps
		LOG->Trace( "Steps::LoadFromSMTokens(): attempting to load stop segments from steps..." );
		vector<RString> arrayFreezeExpressions;
		split( sStopsData, ",", arrayFreezeExpressions );
		
		for( unsigned f=0; f<arrayFreezeExpressions.size(); f++ )
		{
			vector<RString> arrayFreezeValues;
			split( arrayFreezeExpressions[f], "=", arrayFreezeValues );
			if( arrayFreezeValues.size() != 2 )
			{
				// XXX: Hard to tell which file caused this.
				LOG->UserLog( "[LoadFromSMTokens] Song file", "(UNKNOWN)", "has an invalid #%s value \"%s\" (must have exactly one '='), ignored.",
					     "Stops:", arrayFreezeExpressions[f].c_str() );
				continue;
			}
			
			const float fFreezeBeat = StringToFloat( arrayFreezeValues[0] );
			const float fFreezeSeconds = StringToFloat( arrayFreezeValues[1] );
			
			StopSegment new_seg( BeatToNoteRow(fFreezeBeat), fFreezeSeconds );
			
			//	LOG->Trace( "Adding a freeze segment: beat: %f, seconds = %f", new_seg.m_fStartBeat, new_seg.m_fStopSeconds );
			
			out.AddStopSegment( new_seg );
		}
	}
	
	// time signatures
	if( sTimeSignaturesData.CompareNoCase("") == 0 )
	{
		// import Time Signatures data from Song
		if( td.HasTimeSignatures() )
			LOG->Trace( "imported timing data / song timing data has time signatures" );
		else
			LOG->Trace( "imported timing data / song timing data has no time signatures" );
		out.m_Timing.m_vTimeSignatureSegments = td.m_vTimeSignatureSegments;
	}
	else
	{
		// Time Signatures data provided in Steps
		LOG->Trace( "Steps::LoadFromSMTokens(): attempting to load time signature segments from steps..." );
		vector<RString> arrayTimeSignatureExpressions;
		split( sTimeSignaturesData, ",", arrayTimeSignatureExpressions );
		
		for( unsigned f=0; f<arrayTimeSignatureExpressions.size(); f++ )
		{
			vector<RString> arrayTimeSignatureValues;
			split( arrayTimeSignatureExpressions[f], "=", arrayTimeSignatureValues );
			if( arrayTimeSignatureValues.size() != 3 )
			{
				// XXX: Hard to tell which file caused this.
				LOG->UserLog( "[LoadFromSMTokens] Song file", "(UNKNOWN)", "has an invalid #%s value \"%s\" (must have exactly two '='s), ignored.",
					     "Time Signatures:", arrayTimeSignatureExpressions[f].c_str() );
				continue;
			}
			
			const float fTimeSignatureBeat = StringToFloat( arrayTimeSignatureValues[0] );
			int fTimeSignatureNume =  atoi( arrayTimeSignatureValues[1] ) ;
			int fTimeSignatureDeno =  atoi( arrayTimeSignatureValues[2] ) ;
			
			TimeSignatureSegment new_seg;
			new_seg.m_iStartRow = BeatToNoteRow( fTimeSignatureBeat );
			new_seg.m_iNumerator = fTimeSignatureNume;
			new_seg.m_iDenominator = fTimeSignatureDeno;
			
			//	LOG->Trace( "Adding a time signature segment: beat: %f, numerator = %f, denominator = %f", fTimeSignatureBeat, fTimeSignatureNume, fTimeSignatureDeno );
			
			out.AddTimeSignatureSegment( new_seg );
		}
	}
	
	out.SetSMNoteData( sNoteData );

	out.TidyUpData();
}

void SMLoader::GetApplicableFiles( const RString &sPath, vector<RString> &out )
{
	GetDirListing( sPath + RString("*.sm"), out );
}

bool SMLoader::LoadTimingFromFile( const RString &fn, TimingData &out )
{
	MsdFile msd;
	if( !msd.ReadFile( fn, true ) )  // unescape
	{
		LOG->UserLog( "Song file", fn, "couldn't be loaded: %s", msd.GetError().c_str() );
		return false;
	}

	out.m_sFile = fn;
	LoadTimingFromSMFile( msd, out );
	return true;
}

// TODO: Make this work from Steps
void SMLoader::LoadTimingFromSMFile( const MsdFile &msd, TimingData &out )
{
	out.m_fBeat0OffsetInSeconds = 0;
	out.m_BPMSegments.clear();
	out.m_StopSegments.clear();

	for( unsigned i=0; i<msd.GetNumValues(); i++ )
	{
		const MsdFile::value_t &sParams = msd.GetValue(i);
		RString sValueName = sParams[0];
		sValueName.MakeUpper();

		if( sValueName=="OFFSET" )
		{
			out.m_fBeat0OffsetInSeconds = StringToFloat( sParams[1] );
		}
		else if( sValueName=="STOPS" || sValueName=="FREEZES" )
		{
			vector<RString> arrayFreezeExpressions;
			split( sParams[1], ",", arrayFreezeExpressions );

			for( unsigned f=0; f<arrayFreezeExpressions.size(); f++ )
			{
				vector<RString> arrayFreezeValues;
				split( arrayFreezeExpressions[f], "=", arrayFreezeValues );
				if( arrayFreezeValues.size() != 2 )
				{
					// XXX: Hard to tell which file caused this.
					LOG->UserLog( "Song file", "(UNKNOWN)", "has an invalid #%s value \"%s\" (must have exactly one '='), ignored.",
						sValueName.c_str(), arrayFreezeExpressions[f].c_str() );
					continue;
				}

				const float fFreezeBeat = StringToFloat( arrayFreezeValues[0] );
				const float fFreezeSeconds = StringToFloat( arrayFreezeValues[1] );

				StopSegment new_seg( BeatToNoteRow(fFreezeBeat), fFreezeSeconds );

				//				LOG->Trace( "Adding a freeze segment: beat: %f, seconds = %f", new_seg.m_fStartBeat, new_seg.m_fStopSeconds );

				out.AddStopSegment( new_seg );
			}
		}

		else if( sValueName=="BPMS" )
		{
			vector<RString> arrayBPMChangeExpressions;
			split( sParams[1], ",", arrayBPMChangeExpressions );

			for( unsigned b=0; b<arrayBPMChangeExpressions.size(); b++ )
			{
				vector<RString> arrayBPMChangeValues;
				split( arrayBPMChangeExpressions[b], "=", arrayBPMChangeValues );
				// XXX: Hard to tell which file caused this.
				if( arrayBPMChangeValues.size() != 2 )
				{
					LOG->UserLog( "Song file", "(UNKNOWN)", "has an invalid #%s value \"%s\" (must have exactly one '='), ignored.",
						sValueName.c_str(), arrayBPMChangeExpressions[b].c_str() );
					continue;
				}

				const float fBeat = StringToFloat( arrayBPMChangeValues[0] );
				const float fNewBPM = StringToFloat( arrayBPMChangeValues[1] );

				if( fNewBPM > 0.0f )
					out.AddBPMSegment( BPMSegment(BeatToNoteRow(fBeat), fNewBPM) );
				else
					LOG->UserLog( "Song file", "(UNKNOWN)", "has an invalid BPM change at beat %f, BPM %f.", fBeat, fNewBPM );
			}
		}

		else if( sValueName=="TIMESIGNATURES" )
		{
			vector<RString> vs1;
			split( sParams[1], ",", vs1 );

			FOREACH_CONST( RString, vs1, s1 )
			{
				vector<RString> vs2;
				split( *s1, "=", vs2 );

				if( vs2.size() < 3 )
				{
					LOG->UserLog( "Song file", "(UNKNOWN)", "has an invalid time signature change with %i values.", (int)vs2.size() );
					continue;
				}

				const float fBeat = StringToFloat( vs2[0] );

				TimeSignatureSegment seg;
				seg.m_iStartRow = BeatToNoteRow(fBeat);
				seg.m_iNumerator = atoi( vs2[1] ); 
				seg.m_iDenominator = atoi( vs2[2] ); 

				if( fBeat < 0 )
				{
					LOG->UserLog( "Song file", "(UNKNOWN)", "has an invalid time signature change with beat %f.", fBeat );
					continue;
				}

				if( seg.m_iNumerator < 1 )
				{
					LOG->UserLog( "Song file", "(UNKNOWN)", "has an invalid time signature change with beat %f, iNumerator %i.", fBeat, seg.m_iNumerator );
					continue;
				}

				if( seg.m_iDenominator < 1 )
				{
					LOG->UserLog( "Song file", "(UNKNOWN)", "has an invalid time signature change with beat %f, iDenominator %i.", fBeat, seg.m_iDenominator );
					continue;
				}

				out.AddTimeSignatureSegment( seg );
			}
		}

	}
}

bool LoadFromBGChangesString( BackgroundChange &change, const RString &sBGChangeExpression )
{
	vector<RString> aBGChangeValues;
	split( sBGChangeExpression, "=", aBGChangeValues, false );

	aBGChangeValues.resize( min((int)aBGChangeValues.size(),11) );

	switch( aBGChangeValues.size() )
	{
	case 11:
		change.m_def.m_sColor2 = aBGChangeValues[10];
		// .sm files made before we started escaping will still have '^' instead of ','
		change.m_def.m_sColor2.Replace( '^', ',' );
		// fall through
	case 10:
		change.m_def.m_sColor1 = aBGChangeValues[9];
		// .sm files made before we started escaping will still have '^' instead of ','
		change.m_def.m_sColor1.Replace( '^', ',' );
		// fall through
	case 9:
		change.m_sTransition = aBGChangeValues[8];
		// fall through
	case 8:
		change.m_def.m_sFile2 = aBGChangeValues[7];
		// fall through
	case 7:
		change.m_def.m_sEffect = aBGChangeValues[6];
		// fall through
	case 6:
		// param 7 overrides this.
		// Backward compatibility:
		if( change.m_def.m_sEffect.empty() )
		{
			bool bLoop = atoi( aBGChangeValues[5] ) != 0;
			if( !bLoop )
				change.m_def.m_sEffect = SBE_StretchNoLoop;
		}
		// fall through
	case 5:
		// param 7 overrides this.
		// Backward compatibility:
		if( change.m_def.m_sEffect.empty() )
		{
			bool bRewindMovie = atoi( aBGChangeValues[4] ) != 0;
			if( bRewindMovie )
				change.m_def.m_sEffect = SBE_StretchRewind;
		}
		// fall through
	case 4:
		// param 9 overrides this.
		// Backward compatibility:
		if( change.m_sTransition.empty() )
			change.m_sTransition = (atoi( aBGChangeValues[3] ) != 0) ? "CrossFade" : "";
		// fall through
	case 3:
		change.m_fRate = StringToFloat( aBGChangeValues[2] );
		// fall through
	case 2:
		change.m_def.m_sFile1 = aBGChangeValues[1];
		// fall through
	case 1:
		change.m_fStartBeat = StringToFloat( aBGChangeValues[0] );
		// fall through
	}

	return aBGChangeValues.size() >= 2;
}

bool SMLoader::LoadFromSMFile( const RString &sPath, Song &out, bool bFromCache )
{
	LOG->Trace( "Song::LoadFromSMFile(%s)", sPath.c_str() );

	MsdFile msd;
	if( !msd.ReadFile( sPath, true ) )  // unescape
	{
		LOG->UserLog( "Song file", sPath, "couldn't be opened: %s", msd.GetError().c_str() );
		return false;
	}

	out.m_Timing.m_sFile = sPath;
	LoadTimingFromSMFile( msd, out.m_Timing );

	for( unsigned i=0; i<msd.GetNumValues(); i++ )
	{
		int iNumParams = msd.GetNumParams(i);
		const MsdFile::value_t &sParams = msd.GetValue(i);
		RString sValueName = sParams[0];
		sValueName.MakeUpper();

		// handle the data
		/* Don't use GetMainAndSubTitlesFromFullTitle; that's only for heuristically
		* splitting other formats that *don't* natively support #SUBTITLE. */
		if( sValueName=="TITLE" )
			out.m_sMainTitle = sParams[1];

		else if( sValueName=="SUBTITLE" )
			out.m_sSubTitle = sParams[1];

		else if( sValueName=="ARTIST" )
			out.m_sArtist = sParams[1];

		else if( sValueName=="TITLETRANSLIT" )
			out.m_sMainTitleTranslit = sParams[1];

		else if( sValueName=="SUBTITLETRANSLIT" )
			out.m_sSubTitleTranslit = sParams[1];

		else if( sValueName=="ARTISTTRANSLIT" )
			out.m_sArtistTranslit = sParams[1];

		else if( sValueName=="GENRE" )
			out.m_sGenre = sParams[1];

		else if( sValueName=="CREDIT" )
			out.m_sCredit = sParams[1];

		else if( sValueName=="BANNER" )
			out.m_sBannerFile = sParams[1];

		else if( sValueName=="BACKGROUND" )
			out.m_sBackgroundFile = sParams[1];

		/* Save "#LYRICS" for later, so we can add an internal lyrics tag. */
		else if( sValueName=="LYRICSPATH" )
			out.m_sLyricsFile = sParams[1];

		else if( sValueName=="CDTITLE" )
			out.m_sCDTitleFile = sParams[1];

		else if( sValueName=="MUSIC" )
			out.m_sMusicFile = sParams[1];

		else if( sValueName=="INSTRUMENTTRACK" )
		{
			vector<RString> vs1;
			split( sParams[1], ",", vs1 );
			FOREACH_CONST( RString, vs1, s )
			{
				vector<RString> vs2;
				split( *s, "=", vs2 );
				if( vs2.size() >= 2 )
				{
					InstrumentTrack it = StringToInstrumentTrack( vs2[0] );
					if( it != InstrumentTrack_Invalid )
						out.m_sInstrumentTrackFile[it] = vs2[1];
				}
			}
		}

		else if( sValueName=="MUSICLENGTH" )
		{
			if( !bFromCache )
				continue;
			out.m_fMusicLengthSeconds = StringToFloat( sParams[1] );
		}

		else if( sValueName=="LASTBEATHINT" )
			out.m_fSpecifiedLastBeat = StringToFloat( sParams[1] );

		else if( sValueName=="MUSICBYTES" )
			; /* ignore */

		/* We calculate these.  Some SMs in circulation have bogus values for
		* these, so make sure we always calculate it ourself. */
		else if( sValueName=="FIRSTBEAT" )
		{
			if( bFromCache )
				out.m_fFirstBeat = StringToFloat( sParams[1] );
		}
		else if( sValueName=="LASTBEAT" )
		{
			if( bFromCache )
				out.m_fLastBeat = StringToFloat( sParams[1] );
		}
		else if( sValueName=="SONGFILENAME" )
		{
			if( bFromCache )
				out.m_sSongFileName = sParams[1];
		}
		else if( sValueName=="HASMUSIC" )
		{
			if( bFromCache )
				out.m_bHasMusic = atoi( sParams[1] ) != 0;
		}
		else if( sValueName=="HASBANNER" )
		{
			if( bFromCache )
				out.m_bHasBanner = atoi( sParams[1] ) != 0;
		}

		else if( sValueName=="SAMPLESTART" )
			out.m_fMusicSampleStartSeconds = HHMMSSToSeconds( sParams[1] );

		else if( sValueName=="SAMPLELENGTH" )
			out.m_fMusicSampleLengthSeconds = HHMMSSToSeconds( sParams[1] );

		else if( sValueName=="DISPLAYBPM" )
		{
			// #DISPLAYBPM:[xxx][xxx:xxx]|[*]; 
			if( sParams[1] == "*" )
				out.m_DisplayBPMType = Song::DISPLAY_RANDOM;
			else 
			{
				out.m_DisplayBPMType = Song::DISPLAY_SPECIFIED;
				out.m_fSpecifiedBPMMin = StringToFloat( sParams[1] );
				if( sParams[2].empty() )
					out.m_fSpecifiedBPMMax = out.m_fSpecifiedBPMMin;
				else
					out.m_fSpecifiedBPMMax = StringToFloat( sParams[2] );
			}
		}

		else if( sValueName=="SELECTABLE" )
		{
			if(!stricmp(sParams[1],"YES"))
				out.m_SelectionDisplay = out.SHOW_ALWAYS;
			else if(!stricmp(sParams[1],"NO"))
				out.m_SelectionDisplay = out.SHOW_NEVER;
			/* The following two cases are just fixes to make sure simfiles that used 3.9+ features
			 * are not excluded here */
			else if(!stricmp(sParams[1],"ES") || !stricmp(sParams[1],"OMES"))
				out.m_SelectionDisplay = out.SHOW_ALWAYS;
			else if( atoi(sParams[1]) > 0 )
				out.m_SelectionDisplay = out.SHOW_ALWAYS;
			else
				LOG->UserLog( "Song file", sPath, "has an unknown #SELECTABLE value, \"%s\"; ignored.", sParams[1].c_str() );
		}

		else if( sValueName.Left(strlen("BGCHANGES"))=="BGCHANGES" || sValueName=="ANIMATIONS" )
		{
			BackgroundLayer iLayer = BACKGROUND_LAYER_1;
			if( sscanf(sValueName, "BGCHANGES%d", &*ConvertValue<int>(&iLayer)) == 1 )
				enum_add(iLayer, -1);	// #BGCHANGES2 = BACKGROUND_LAYER_2

			bool bValid = iLayer>=0 && iLayer<NUM_BackgroundLayer;
			if( !bValid )
			{
				LOG->UserLog( "Song file", sPath, "has a #BGCHANGES tag \"%s\" that is out of range.", sValueName.c_str() );
			}
			else
			{
				vector<RString> aBGChangeExpressions;
				split( sParams[1], ",", aBGChangeExpressions );

				for( unsigned b=0; b<aBGChangeExpressions.size(); b++ )
				{
					BackgroundChange change;
					if( LoadFromBGChangesString( change, aBGChangeExpressions[b] ) )
						out.AddBackgroundChange( iLayer, change );
				}
			}
		}

		else if( sValueName=="FGCHANGES" )
		{
			vector<RString> aFGChangeExpressions;
			split( sParams[1], ",", aFGChangeExpressions );

			for( unsigned b=0; b<aFGChangeExpressions.size(); b++ )
			{
				BackgroundChange change;
				if( LoadFromBGChangesString( change, aFGChangeExpressions[b] ) )
					out.AddForegroundChange( change );
			}
		}

		else if( sValueName=="KEYSOUNDS" )
		{
			split( sParams[1], ",", out.m_vsKeysoundFile );
		}

		// Attacks loaded from file
		else if( sValueName=="ATTACKS" )
		{
			// Build the RString vector here so we can write it to file again later
			for( unsigned s=1; s < sParams.params.size(); ++s )
				out.m_sAttackString.push_back( sParams[s] );

			Attack attack;
			float end = -9999;

			for( unsigned j=1; j < sParams.params.size(); ++j )
			{
				vector<RString> sBits;
				split( sParams[j], "=", sBits, false );

				// Need an identifer and a value for this to work
				if( sBits.size() < 2 )
					continue;

				TrimLeft( sBits[0] );
				TrimRight( sBits[0] );

				if( !sBits[0].CompareNoCase("TIME") )
					attack.fStartSecond = strtof( sBits[1], NULL );
				else if( !sBits[0].CompareNoCase("LEN") )
					attack.fSecsRemaining = strtof( sBits[1], NULL );
				else if( !sBits[0].CompareNoCase("END") )
					end = strtof( sBits[1], NULL );
				else if( !sBits[0].CompareNoCase("MODS") )
				{
					attack.sModifiers = sBits[1];

					if( end != -9999 )
					{
						attack.fSecsRemaining = end - attack.fStartSecond;
						end = -9999;
					}

					if( attack.fSecsRemaining < 0.0f )
						attack.fSecsRemaining = 0.0f;

					out.m_Attacks.push_back( attack );
				}
			}
		}

		else if( sValueName=="NOTES" || sValueName=="NOTES2" )
		{
			if( iNumParams < 7 )
			{
				LOG->UserLog( "Song file", sPath, "has %d fields in a #NOTES tag, but should have at least 7.", iNumParams );
				continue;
			}

			Steps* pNewNotes = new Steps;
			
			// Check the format used: old format, or BPM in steps format.
			if( iNumParams == 10 )
			{
				LoadFromSMTokens(
						 sParams[1],	// StepsType
						 sParams[2],	// Description
						 sParams[3],	// Difficulty
						 sParams[4],	// Meter
						 sParams[5],	// RadarValues
						 sParams[6],	// NoteData
						 sParams[7],	// BPMs
						 sParams[8],	// Stops
						 sParams[9],	// Time Signatures
						 out.m_Timing,	// Song timing data, just in case.
						 *pNewNotes );	 
			}
			else
			{
				LoadFromSMTokens( 
						 sParams[1],	// StepsType
						 sParams[2],	// Description
						 sParams[3],	// Difficulty
						 sParams[4],	// Meter
						 sParams[5],	// RadarValues
						 sParams[6],	// NoteData
						 "",		// BPMs
						 "",		// Stops
						 "",		// Time Signatures
						 out.m_Timing,	// Song timing data, mandatory.
						 *pNewNotes );
			}
			out.AddSteps( pNewNotes );
		}
		else if( sValueName=="OFFSET" || sValueName=="BPMS" || sValueName=="STOPS" || sValueName=="FREEZES" || sValueName=="TIMESIGNATURES" )
			;
		else
			LOG->UserLog( "Song file", sPath, "has an unexpected value named \"%s\".", sValueName.c_str() );
	}

	return true;
}


bool SMLoader::LoadFromDir( const RString &sPath, Song &out )
{
	vector<RString> aFileNames;
	GetApplicableFiles( sPath, aFileNames );

	if( aFileNames.size() > 1 )
	{
		LOG->UserLog( "Song", sPath, "has more than one SM file. There should be only one!" );
		return false;
	}

	/* We should have exactly one; if we had none, we shouldn't have been
	* called to begin with. */
	ASSERT( aFileNames.size() == 1 );

	return LoadFromSMFile( sPath + aFileNames[0], out );
}

bool SMLoader::LoadEditFromFile( RString sEditFilePath, ProfileSlot slot, bool bAddStepsToSong )
{
	LOG->Trace( "SMLoader::LoadEditFromFile(%s)", sEditFilePath.c_str() );

	int iBytes = FILEMAN->GetFileSizeInBytes( sEditFilePath );
	if( iBytes > MAX_EDIT_STEPS_SIZE_BYTES )
	{
		LOG->UserLog( "Edit file", sEditFilePath, "is unreasonably large. It won't be loaded." );
		return false;
	}

	MsdFile msd;
	if( !msd.ReadFile( sEditFilePath, true ) )  // unescape
	{
		LOG->UserLog( "Edit file", sEditFilePath, "couldn't be opened: %s", msd.GetError().c_str() );
		return false;
	}

	return LoadEditFromMsd( msd, sEditFilePath, slot, bAddStepsToSong );
}

bool SMLoader::LoadEditFromBuffer( const RString &sBuffer, const RString &sEditFilePath, ProfileSlot slot )
{
	MsdFile msd;
	msd.ReadFromString( sBuffer, true );  // unescape
	return LoadEditFromMsd( msd, sEditFilePath, slot, true );
}

bool SMLoader::LoadEditFromMsd( const MsdFile &msd, const RString &sEditFilePath, ProfileSlot slot, bool bAddStepsToSong )
{
	Song* pSong = NULL;

	for( unsigned i=0; i<msd.GetNumValues(); i++ )
	{
		int iNumParams = msd.GetNumParams(i);
		const MsdFile::value_t &sParams = msd.GetValue(i);
		RString sValueName = sParams[0];
		sValueName.MakeUpper();

		// handle the data
		if( sValueName=="SONG" )
		{
			if( pSong )
			{
				LOG->UserLog( "Edit file", sEditFilePath, "has more than one #SONG tag." );
				return false;
			}

			RString sSongFullTitle = sParams[1];
			sSongFullTitle.Replace( '\\', '/' );

			pSong = SONGMAN->FindSong( sSongFullTitle );
			if( pSong == NULL )
			{
				LOG->UserLog( "Edit file", sEditFilePath, "requires a song \"%s\" that isn't present.", sSongFullTitle.c_str() );
				return false;
			}

			if( pSong->GetNumStepsLoadedFromProfile(slot) >= MAX_EDITS_PER_SONG_PER_PROFILE )
			{
				LOG->UserLog( "Song file", sSongFullTitle, "already has the maximum number of edits allowed for ProfileSlotP%d.", slot+1 );
				return false;
			}
		}

		else if( sValueName=="NOTES" )
		{
			if( pSong == NULL )
			{
				LOG->UserLog( "Edit file", sEditFilePath, "doesn't have a #SONG tag preceeding the first #NOTES tag." );
				return false;
			}

			if( iNumParams < 7 )
			{
				LOG->UserLog( "Edit file", sEditFilePath, "has %d fields in a #NOTES tag, but should have at least 7.", iNumParams );
				continue;
			}

			if( !bAddStepsToSong )
				return true;

			Steps* pNewNotes = new Steps;
			
			// Check the format used: old format, or BPM in steps format.
			if( iNumParams == 10 )
			{
				LoadFromSMTokens(
						 sParams[1],		// StepsType
						 sParams[2],		// Description
						 sParams[3],		// Difficulty
						 sParams[4],		// Meter
						 sParams[5],		// RadarValues
						 sParams[6],		// NoteData
						 sParams[7],		// BPMs
						 sParams[8],		// Stops
						 sParams[9],		// Time Signatures
						 pSong->m_Timing,	// Song timing data, just in case.
						 *pNewNotes );	 
			}
			else
			{
				LoadFromSMTokens( 
						 sParams[1],		// StepsType
						 sParams[2],		// Description
						 sParams[3],		// Difficulty
						 sParams[4],		// Meter
						 sParams[5],		// RadarValues
						 sParams[6],		// NoteData
						 "",			// BPMs
						 "",			// Stops
						 "",			// Time Signatures
						 pSong->m_Timing,	// Song timing data, mandatory.
						 *pNewNotes );
			}

			pNewNotes->SetLoadedFromProfile( slot );
			pNewNotes->SetDifficulty( Difficulty_Edit );
			pNewNotes->SetFilename( sEditFilePath );

			if( pSong->IsEditAlreadyLoaded(pNewNotes) )
			{
				LOG->UserLog( "Edit file", sEditFilePath, "is a duplicate of another edit that was already loaded." );
				SAFE_DELETE( pNewNotes );
				return false;
			}

			pSong->AddSteps( pNewNotes );
			return true;	// Only allow one Steps per edit file!
		}
		else
		{
			LOG->UserLog( "Edit file", sEditFilePath, "has an unexpected value \"%s\".", sValueName.c_str() );
		}
	}

	return true;

}

void SMLoader::TidyUpData( Song &song, bool bFromCache )
{
	/*
	* Hack: if the song has any changes at all (so it won't use a random BGA)
	* and doesn't end with "-nosongbg-", add a song background BGC.  Remove
	* "-nosongbg-" if it exists.
	*
	* This way, songs that were created earlier, when we added the song BG
	* at the end by default, will still behave as expected; all new songs will
	* have to add an explicit song BG tag if they want it.  This is really a
	* formatting hack only; nothing outside of SMLoader ever sees "-nosongbg-".
	*/
	vector<BackgroundChange> &bg = song.GetBackgroundChanges(BACKGROUND_LAYER_1);
	if( !bg.empty() )
	{
		/* BGChanges have been sorted.  On the odd chance that a BGChange exists
		* with a very high beat, search the whole list. */
		bool bHasNoSongBgTag = false;

		for( unsigned i = 0; !bHasNoSongBgTag && i < bg.size(); ++i )
		{
			if( !bg[i].m_def.m_sFile1.CompareNoCase(NO_SONG_BG_FILE) )
			{
				bg.erase( bg.begin()+i );
				bHasNoSongBgTag = true;
			}
		}

		/* If there's no -nosongbg- tag, add the song BG. */
		if( !bHasNoSongBgTag ) do
		{
			/* If we're loading cache, -nosongbg- should always be in there.  We must
			* not call IsAFile(song.GetBackgroundPath()) when loading cache. */
			if( bFromCache )
				break;

			/* If BGChanges already exist after the last beat, don't add the background
			* in the middle. */
			if( !bg.empty() && bg.back().m_fStartBeat-0.0001f >= song.m_fLastBeat )
				break;

			/* If the last BGA is already the song BGA, don't add a duplicate. */
			if( !bg.empty() && !bg.back().m_def.m_sFile1.CompareNoCase(song.m_sBackgroundFile) )
				break;

			if( !IsAFile( song.GetBackgroundPath() ) )
				break;

			bg.push_back( BackgroundChange(song.m_fLastBeat,song.m_sBackgroundFile) );
		} while(0);
	}
}

/*
* (c) 2001-2004 Chris Danford, Glenn Maynard
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
