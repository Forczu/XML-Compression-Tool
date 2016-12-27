#pragma once
#include "AbstractReadByteStrategy.h"

class ReadTwoBytesStrategy : public AbstractReadByteStrategy
{
	int read(std::string const & str, int & index) override
	{
		short a = short(
			(unsigned char)(str[index + 0]) << 8 |
			(unsigned char)(str[index + 1]) << 0);
		return a;
	}

	std::vector<unsigned char> writeToBytes(int paramInt) override
	{
		std::vector<unsigned char> bytes = shortToBytes(paramInt);
		return bytes;
	}

	int getSize() override
	{
		return 2;
	}
};