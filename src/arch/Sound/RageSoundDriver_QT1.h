#ifndef RAGE_SOUND_QT1
#define RAGE_SOUND_QT1
/*
 *  RageSoundDriver_QT1.h
 *  stepmania
 *
 *  Use only a single Sound Manager channel to output sound.
 *
 *  Created by Steve Checkoway on Mon Jun 23 2003.
 *  Copyright (c) 2003 Steve Checkoway. All rights reserved.
 *
 */

namespace QT
{
#include <QuickTime/QuickTime.h>
}
#include "RageSound.h"
#include "RageSoundDriver_Generic_Software.h"

class RageSound_QT1: public RageSound_Generic_Software
{
private:
/*    struct sound
    {
        RageSoundBase *snd;
        bool stopping;
        int flush_pos;
        sound() { snd=NULL; stopping=false; flush_pos=0; }
    };*/

//    vector<sound *> sounds;
    QT::ComponentInstance clock;
    QT::SndChannelPtr channel;
    int last_pos;
    float latency;

protected:
//    virtual void StartMixing(RageSoundBase *snd);
//    virtual void StopMixing(RageSoundBase *snd);
    int64_t GetPosition(const RageSoundBase *snd) const;
//    void Update (float delta);
    float GetPlayLatency() const;

public:
    RageSound_QT1();
    virtual ~RageSound_QT1();
    static void GetData(QT::SndChannel *chan, QT::SndCommand *cmd_passed);
};

#endif /* RAGE_SOUND_QT1 */

/*
 * (c) 2003-2004 Steve Checkoway
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
