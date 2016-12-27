#pragma once
#include "AbstractReadByteStrategy.h"

class ReadFourBytesStrategy : public AbstractReadByteStrategy
{
	int read(std::string const & str, int & index) override
	{
		int a = int(
			(unsigned char)(str[index + 0]) << 24 |
			(unsigned char)(str[index + 1]) << 16 |
			(unsigned char)(str[index + 2]) << 8  |
			(unsigned char)(str[index + 3]) << 0);
		return a;
	}

	std::vector<char> writeToBytes(int paramInt) override
	{
		std::vector<char> bytes = intToBytes(paramInt);
		return bytes;
	}

	int getSize() override
	{
		return 4;
	}
};