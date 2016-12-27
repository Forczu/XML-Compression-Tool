#pragma once
#include "AbstractReadByteStrategy.h"

class ReadOneBytesStrategy : public AbstractReadByteStrategy
{
	int read(std::string const & str, int & index) override
	{
		unsigned char a = (unsigned char)(str[index]);
		return a;
	}

	std::vector<unsigned char> writeToBytes(int paramInt) override
	{
		std::vector<unsigned char> bytes;
		auto a = intToByte(paramInt);
		bytes.push_back(a);
		return bytes;
	}

	int getSize() override
	{
		return 1;
	}
};