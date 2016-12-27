#pragma once
#include "AbstractReadByteStrategy.h"

class ReadOneBytesStrategy : public AbstractReadByteStrategy
{
	int read(std::string const & str, int & index) override
	{
		char a = str[index];
		return a;
	}

	std::vector<char> writeToBytes(int paramInt) override
	{
		std::vector<char> bytes;
		auto a = intToByte(paramInt);
		bytes.push_back(a);
		return bytes;
	}

	int getSize() override
	{
		return 1;
	}
};