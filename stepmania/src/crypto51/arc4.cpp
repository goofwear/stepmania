// arc4.cpp - written and placed in the public domain by Wei Dai

// The ARC4 algorithm was first revealed in an anonymous email to the
// cypherpunks mailing list. This file originally contained some
// code copied from this email. The code has since been rewritten in order
// to clarify the copyright status of this file. It should now be
// completely in the public domain.

#include "pch.h"
#include "arc4.h"

NAMESPACE_BEGIN(CryptoPP)

void ARC4_TestInstantiations()
{
	ARC4 x;
}

ARC4_Base::~ARC4_Base()
{
	m_x = m_y = 0;
}

void ARC4_Base::UncheckedSetKey(const NameValuePairs &params, const byte *key, unsigned int keyLen)
{
	AssertValidKeyLength(keyLen);

	m_x = 1;
	m_y = 0;

	unsigned int i;
	for (i=0; i<256; i++)
		m_state[i] = i;

	unsigned int keyIndex = 0, stateIndex = 0;
	for (i=0; i<256; i++)
	{
		unsigned int a = m_state[i];
		stateIndex += key[keyIndex] + a;
		stateIndex &= 0xff;
		m_state[i] = m_state[stateIndex];
		m_state[stateIndex] = a;
		if (++keyIndex >= keyLen)
			keyIndex = 0;
	}

	int discardBytes = params.GetIntValueWithDefault("DiscardBytes", GetDefaultDiscardBytes());
	DiscardBytes(discardBytes);
}

template <class T>
static inline unsigned int MakeByte(T &x, T &y, byte *s)
{
	unsigned int a = s[x];
	y = (y+a) & 0xff;
	unsigned int b = s[y];
	s[x] = b;
	s[y] = a;
	x = (x+1) & 0xff;
	return s[(a+b) & 0xff];
}

byte ARC4_Base::GenerateByte()
{
	return MakeByte(m_x, m_y, m_state);
}

void ARC4_Base::ProcessData(byte *outString, const byte *inString, unsigned int length)
{
	if (length == 0)
		return;

	byte *const s = m_state;
	unsigned int x = m_x;
	unsigned int y = m_y;

	if (inString == outString)
	{
		do
		{
			*outString++ ^= MakeByte(x, y, s);
		} while (--length);
	}
	else
	{
		do
		{
			*outString++ = *inString++ ^ MakeByte(x, y, s);
		}
		while(--length);
	}

	m_x = x;
	m_y = y;
}

void ARC4_Base::DiscardBytes(unsigned int length)
{
	if (length == 0)
		return;

	byte *const s = m_state;
	unsigned int x = m_x;
	unsigned int y = m_y;

	do
	{
		MakeByte(x, y, s);
	}
	while(--length);

	m_x = x;
	m_y = y;
}

NAMESPACE_END
