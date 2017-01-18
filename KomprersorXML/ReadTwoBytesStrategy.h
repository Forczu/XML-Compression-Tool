#pragma once
#include "AbstractReadByteStrategy.h"
#include "ReadByteStrategyFactory.h"

namespace
{
	/// <summary>
	/// Strategia definujaca odczytywanie dwoch bajtow jako znacznik
	/// </summary>
	/// <seealso cref="AbstractReadByteStrategy" />
	class ReadTwoBytesStrategy : public AbstractReadByteStrategy
	{
		int read(std::string const & str, int & index) override;

		std::vector<char> writeToBytes(int paramInt) override;

		int getSize() override;
	};

	AbstractReadByteStrategy * getInstance() { return new ReadTwoBytesStrategy; }
	const ReadStrategy name = ReadStrategy::Short;
	const bool registered = ReadByteStrategyFactory::Instance().registerStrategy(name, getInstance);
}