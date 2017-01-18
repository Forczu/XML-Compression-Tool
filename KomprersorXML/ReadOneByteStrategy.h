#pragma once
#include "ReadByteStrategyFactory.h"
#include "AbstractReadByteStrategy.h"

namespace
{
	class ReadOneByteStrategy : public AbstractReadByteStrategy
	{
		int read(std::string const & str, int & index) override;

		std::vector<char> writeToBytes(int paramInt) override;

		int getSize() override;
	};

	AbstractReadByteStrategy * getInstance() { return new ReadOneByteStrategy; }
	const ReadStrategy name = ReadStrategy::Char;
	const bool registered = ReadByteStrategyFactory::Instance().registerStrategy(name, getInstance);
}