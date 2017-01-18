#include "ReadFourBytesStrategy.h"

int ReadFourBytesStrategy::read(std::string const & str, int & index)
{
	int a = int(
		(unsigned char)(str[index + 0]) << 24 |
		(unsigned char)(str[index + 1]) << 16 |
		(unsigned char)(str[index + 2]) << 8 |
		(unsigned char)(str[index + 3]) << 0);
	return a;
}

std::vector<char> ReadFourBytesStrategy::writeToBytes(int paramInt)
{
	std::vector<char> bytes = intToBytes(paramInt);
	return bytes;
}

int ReadFourBytesStrategy::getSize()
{
	return 4;
}