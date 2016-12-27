#pragma once
#include <string>

class AbstractReadByteStrategy
{
public:
	virtual int read(std::string const & str, int & index) = 0;

	virtual std::vector<unsigned char> writeToBytes(int paramInt) = 0;

	virtual int getSize() = 0;

	unsigned char intToByte(int paramInt)
	{
		unsigned char a = paramInt;
		return a;
	}

	std::vector<unsigned char> intToBytes(int paramInt)
	{
		std::vector<unsigned char> arrayOfBytes(4);
		for (int i = 0; i < 4; ++i)
			arrayOfBytes[3 - i] = (paramInt >> (i * 8)) & 0xFF;
		return arrayOfBytes;
	}

	std::vector<unsigned char> shortToBytes(short paramInt)
	{
		std::vector<unsigned char> arrayOfBytes(2);
		for (int i = 0; i < 2; ++i)
			arrayOfBytes[i] = (paramInt >> (i * 8)) & 0xFF;
		return arrayOfBytes;
	}

	int bytesToInt(std::string const & str, int index)
	{
		const int size = 4;
		unsigned char a[size];
		for (int i = 0; i < size; ++i)
			a[i] = str[index + i];
		int result = int(a[0] << 24 | a[1] << 16 | a[2] << 8 | a[3]);
		return result;
	}

	int bytesToFloat(std::string const & str, int index)
	{
		const int size = 4;
		unsigned char a[size];
		for (int i = 0; i < size; ++i)
			a[i] = str[index + i];
		float f;
		memcpy(&f, a, size);
		return f;
	}

	int bytesToShort(std::string const & str, int index)
	{
		const int size = 2;
		unsigned char a[size];
		for (int i = 0; i < size; ++i)
			a[i] = str[index + i];
		short result = *(short*)a;
		return result;
	}
};