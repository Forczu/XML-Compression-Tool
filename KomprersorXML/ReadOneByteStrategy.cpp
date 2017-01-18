#include "ReadOneByteStrategy.h"

int ReadOneByteStrategy::read(std::string const & str, int & index)
{
	char a = str[index];
	return a;
}

std::vector<char> ReadOneByteStrategy::writeToBytes(int paramInt)
{
	std::vector<char> bytes;
	auto a = intToByte(paramInt);
	bytes.push_back(a);
	return bytes;
}

int ReadOneByteStrategy::getSize()
{
	return 1;
}