#ifndef DIFFICULTY_H
#define DIFFICULTY_H

#include "EnumHelper.h"

//
// Player number stuff
//
enum Difficulty
{
	DIFFICULTY_BEGINNER,
	DIFFICULTY_EASY,
	DIFFICULTY_MEDIUM,
	DIFFICULTY_HARD,
	DIFFICULTY_CHALLENGE,
	DIFFICULTY_EDIT,
	NUM_Difficulty,
	Difficulty_Invalid
};
#define FOREACH_Difficulty( dc ) FOREACH_ENUM( Difficulty, dc )
const RString& DifficultyToString( Difficulty dc );
const RString& DifficultyToLocalizedString( Difficulty dc );
Difficulty StringToDifficulty( const RString& sDC );
LuaDeclareType( Difficulty );

enum CourseDifficulty
{
	COURSE_DIFFICULTY_BEGINNER,
	COURSE_DIFFICULTY_EASY,
	COURSE_DIFFICULTY_REGULAR,
	COURSE_DIFFICULTY_DIFFICULT,
	COURSE_DIFFICULTY_CHALLENGE,
	COURSE_DIFFICULTY_EDIT,
	NUM_CourseDifficulty,
	CourseDifficulty_Invalid
};
#define FOREACH_CourseDifficulty( cd ) FOREACH_ENUM( CourseDifficulty, cd )
#define FOREACH_ShownCourseDifficulty( cd ) for( CourseDifficulty cd=GetNextShownCourseDifficulty((CourseDifficulty)-1); cd!=CourseDifficulty_Invalid; cd=GetNextShownCourseDifficulty(cd) )

const RString& CourseDifficultyToString( CourseDifficulty dc );
const RString& CourseDifficultyToLocalizedString( CourseDifficulty dc );
CourseDifficulty StringToCourseDifficulty( const RString& sDC );
LuaDeclareType( CourseDifficulty );
CourseDifficulty GetNextShownCourseDifficulty( CourseDifficulty pn );

#endif

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
