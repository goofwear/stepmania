#ifndef CRYPTOPP_IDA_H
#define CRYPTOPP_IDA_H

#include "mqueue.h"
#include "filters.h"
#include "channels.h"
#include <map>
#include <vector>

NAMESPACE_BEGIN(CryptoPP)

/// base class for secret sharing and information dispersal
class RawIDA : public AutoSignaling<Unflushable<Multichannel<Filter> > >
{
public:
	RawIDA(BufferedTransformation *attachment=NULL)
		: AutoSignaling<Unflushable<Multichannel<Filter> > >(attachment) {}

	unsigned int GetThreshold() const {return m_threshold;}
	void AddOutputChannel(word32 channelId);
	void ChannelData(word32 channelId, const byte *inString, unsigned int length, bool messageEnd);
	unsigned int InputBuffered(word32 channelId) const;

	void ChannelInitialize(const std::string &channel, const NameValuePairs &parameters=g_nullNameValuePairs, int propagation=-1);
	unsigned int ChannelPut2(const std::string &channel, const byte *begin, unsigned int length, int messageEnd, bool blocking)
	{
		if (!blocking)
			throw BlockingInputOnly("RawIDA");
		ChannelData(StringToWord<word32>(channel), begin, length, messageEnd != 0);
		return 0;
	}

protected:
	virtual void FlushOutputQueues();
	virtual void OutputMessageEnds();

	unsigned int InsertInputChannel(word32 channelId);
	unsigned int LookupInputChannel(word32 channelId) const;
	void ComputeV(unsigned int);
	void PrepareInterpolation();
	void ProcessInputQueues();

	std::map<word32, unsigned int> m_inputChannelMap;
	std::map<word32, unsigned int>::iterator m_lastMapPosition;
	std::vector<MessageQueue> m_inputQueues;
	std::vector<word32> m_inputChannelIds, m_outputChannelIds, m_outputToInput;
	std::vector<std::string> m_outputChannelIdStrings;
	std::vector<ByteQueue> m_outputQueues;
	int m_threshold;
	unsigned int m_channelsReady, m_channelsFinished;
	std::vector<SecBlock<word32> > m_v;
	SecBlock<word32> m_u, m_w, m_y;
};

/// a variant of Shamir's Secret Sharing Algorithm
class SecretSharing : public CustomSignalPropagation<Filter>
{
public:
	SecretSharing(RandomNumberGenerator &rng, int threshold, int nShares, BufferedTransformation *attachment=NULL, bool addPadding=true)
		: CustomSignalPropagation<Filter>(attachment), m_rng(rng), m_ida(new OutputProxy(*this, true))
		{Initialize(MakeParameters("RecoveryThreshold", threshold)("NumberOfShares", nShares)("AddPadding", addPadding), 0);}

	void Initialize(const NameValuePairs &parameters=g_nullNameValuePairs, int propagation=-1);
	unsigned int Put2(const byte *begin, unsigned int length, int messageEnd, bool blocking);
	bool Flush(bool hardFlush, int propagation=-1, bool blocking=true) {return m_ida.Flush(hardFlush, propagation, blocking);}

protected:
	RandomNumberGenerator &m_rng;
	RawIDA m_ida;
	bool m_pad;
};

/// a variant of Shamir's Secret Sharing Algorithm
class SecretRecovery : public RawIDA
{
public:
	SecretRecovery(int threshold, BufferedTransformation *attachment=NULL, bool removePadding=true)
		: RawIDA(attachment)
		{Initialize(MakeParameters("RecoveryThreshold", threshold)("RemovePadding", removePadding), 0);}

	void Initialize(const NameValuePairs &parameters=g_nullNameValuePairs, int propagation=-1);

protected:
	void FlushOutputQueues();
	void OutputMessageEnds();

	bool m_pad;
};

/// a variant of Rabin's Information Dispersal Algorithm
class InformationDispersal : public CustomSignalPropagation<Filter>
{
public:
	InformationDispersal(int threshold, int nShares, BufferedTransformation *attachment=NULL, bool addPadding=true)
		: CustomSignalPropagation<Filter>(attachment), m_ida(new OutputProxy(*this, true))
		{Initialize(MakeParameters("RecoveryThreshold", threshold)("NumberOfShares", nShares)("AddPadding", addPadding), 0);}

	void Initialize(const NameValuePairs &parameters=g_nullNameValuePairs, int propagation=-1);
	unsigned int Put2(const byte *begin, unsigned int length, int messageEnd, bool blocking);
	bool Flush(bool hardFlush, int propagation=-1, bool blocking=true) {return m_ida.Flush(hardFlush, propagation, blocking);}

protected:
	RawIDA m_ida;
	bool m_pad;
	unsigned int m_nextChannel;
};

/// a variant of Rabin's Information Dispersal Algorithm
class InformationRecovery : public RawIDA
{
public:
	InformationRecovery(int threshold, BufferedTransformation *attachment=NULL, bool removePadding=true)
		: RawIDA(attachment)
		{Initialize(MakeParameters("RecoveryThreshold", threshold)("RemovePadding", removePadding), 0);}

	void Initialize(const NameValuePairs &parameters=g_nullNameValuePairs, int propagation=-1);

protected:
	void FlushOutputQueues();
	void OutputMessageEnds();

	bool m_pad;
	ByteQueue m_queue;
};

class PaddingRemover : public Unflushable<Filter>
{
public:
	PaddingRemover(BufferedTransformation *attachment=NULL)
		: Unflushable<Filter>(attachment), m_possiblePadding(false) {}

	void IsolatedInitialize(const NameValuePairs &parameters) {m_possiblePadding = false;}
	unsigned int Put2(const byte *begin, unsigned int length, int messageEnd, bool blocking);

	// GetPossiblePadding() == false at the end of a message indicates incorrect padding
	bool GetPossiblePadding() const {return m_possiblePadding;}

private:
	bool m_possiblePadding;
	unsigned long m_zeroCount;
};

NAMESPACE_END

#endif
