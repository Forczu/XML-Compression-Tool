#include "ReadTwoBytesStrategy.h"

int ReadTwoBytesStrategy::read(std::string const & str, int & index)
{
	short a = short(
		(unsigned char)(str[index + 0]) << 8 |
		(unsigned char)(str[index + 1]) << 0);
	return a;
}

std::vector<char> ReadTwoBytesStrategy::writeToBytes(int paramInt)
{
	std::vector<char> bytes = shortToBytes(paramInt);
	return bytes;
}

int ReadTwoBytesStrategy::getSize()
{
	return 2;
}