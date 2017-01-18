#pragma once
#include "AbstractReadByteStrategy.h"
#include "ReadByteStrategyFactory.h"

namespace
{
	class ReadFourBytesStrategy : public AbstractReadByteStrategy
	{
		int read(std::string const & str, int & index) override;

		std::vector<char> writeToBytes(int paramInt) override;

		int getSize() override;
	};

	AbstractReadByteStrategy * getInstance() { return new ReadFourBytesStrategy; }
	const ReadStrategy name = ReadStrategy::Int;
	const bool registered = ReadByteStrategyFactory::Instance().registerStrategy(name, getInstance);
}