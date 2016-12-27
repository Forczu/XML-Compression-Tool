#pragma once
#include "port.h"
#include "qsmodel.h"
#include "rangecod.h"
#include <io.h>
#include <fcntl.h>
#include <string>
#include <vector>
#include <unordered_map>

class LzssCoder
{
protected:
	// wartosci graniczne
	static const long MAX_OFFSET_BITS = 15;
	static const long MAX_OFFSET = 1 << MAX_OFFSET_BITS;
	static const long MAX_LENGTH_BITS = 8;
	static const long MAX_LENGTH = 1 << MAX_LENGTH_BITS;
	static const int  MIN_LENGTH = 3;

	// zmienne zwiazane z biblioteka range coder
	rangecoder rc;
	qsmodel flagModel, letterModel, offsetModel;
	static const int LENGTH_MODEL_SIZE = MAX_OFFSET_BITS + 1;
	qsmodel lengthModel[LENGTH_MODEL_SIZE];
	static const int FLAG_ALPHABET_SIZE = 3;
	static const int LETTER_ALPHABET_SIZE = 257;
	static const int OFFSET_ALPHABET_SIZE = 252;
	static const int LENGTH_ALPHABET_SIZE = MAX_LENGTH;
	static const int LG_TOTF = 12;
	static const int RESCALE = 2000;
	static const int COMPRESS = 1;
	static const int DECOMPRESS = 0;
	static const int MAX_LITTLE_OFFSET = 250;
	
	/// <summary>
	/// Bufor na wczytane dane
	/// </summary>
	char * _buffer;
	int bufSize, bufPos;

	typedef std::vector<int> PositionVector;
	typedef std::unordered_map<long, PositionVector> MyMap;
	MyMap myMap;

public:
	void encode(std::string const & source, std::string const & target, int letterAlphabetSize = LETTER_ALPHABET_SIZE)
	{
		toBuffer(source);
		encode(target, letterAlphabetSize);
		delete[] _buffer;
	}

	void encode(std::vector<int> const & source, std::string const & target, int letterAlphabetSize = LETTER_ALPHABET_SIZE)
	{
		toBuffer(source);
		encode(target, letterAlphabetSize);
		delete[] _buffer;
	}

	void encode(std::vector<std::string> const & source, std::string const & target, int letterAlphabetSize = LETTER_ALPHABET_SIZE)
	{
		toBuffer(source);
		encode(target, letterAlphabetSize);
		delete[] _buffer;
	}

	void encode(std::vector<unsigned char> const & source, std::string const & target, int letterAlphabetSize = LETTER_ALPHABET_SIZE)
	{
		toBuffer(source);
		encode(target, letterAlphabetSize);
	}

	void toBuffer(std::vector<unsigned char> const & source)
	{
		bufSize = source.size();
		_buffer = (char*)source.data();
	}

	std::string decode(std::string const & source, int & pos, int letterAlphabetSize = LETTER_ALPHABET_SIZE)
	{
		auto in = freopen(source.c_str(), "rb", stdin);
		fseek(in, pos, SEEK_SET);
		std::string output;
		int currentPosition = 0, ch, sysfreq, ltfreq;
		initializeModels(DECOMPRESS, letterAlphabetSize);
		// rozpoczecie dekompresji
		start_decoding(&rc);
		int symbol;
		while (true)
		{
			ltfreq = decode_culshift(&rc, LG_TOTF);
			// odczytanie flagi
			symbol = qsgetsym(&flagModel, ltfreq);
			if (symbol == 2)
				// koniec pliku
				break;
			loadSymbol(flagModel, symbol, sysfreq, ltfreq);
			// litera
			if (symbol == 1)
			{
				ltfreq = decode_culshift(&rc, LG_TOTF);
				symbol = qsgetsym(&letterModel, ltfreq);
				if (symbol == EOF)
					break;
				loadSymbol(letterModel, symbol, sysfreq, ltfreq);
				output += symbol;
			}
			else if (symbol == 0)
			{
				ltfreq = decode_culshift(&rc, LG_TOTF);
				// offset
				unsigned char offset = qsgetsym(&offsetModel, ltfreq);
				loadSymbol(offsetModel, offset, sysfreq, ltfreq);
				unsigned short newOffset = offset;
				if (newOffset > MAX_LITTLE_OFFSET)
				{
					newOffset += decodeNBits(&rc, MAX_OFFSET_BITS);
				}
				ltfreq = decode_culshift(&rc, LG_TOTF);
				int log = ceilLog2(newOffset);
				// dlugosc
				unsigned char length = qsgetsym(&lengthModel[log], ltfreq);
				loadSymbol(lengthModel[log], length, sysfreq, ltfreq);
				int curPosition = output.length();
				int position = curPosition - newOffset;
				std::string seqToCopy = output.substr(position, length);
				output += seqToCopy;
			}
		}
		done_decoding(&rc);
		deleteModels();
		pos = ftell(in);
		fclose(in);
		return output;
	}

protected:

	void toBuffer(std::string const & source)
	{
		bufSize = source.size();
		_buffer = new char[bufSize + 1];
		for (int i = 0; i < bufSize; ++i)
			_buffer[i] = source[i];
		_buffer[bufSize] = '\0';
	}

	void toBuffer(std::vector<int> const & source)
	{
		bufSize = 0;
		std::vector<std::string> words;
		for (std::vector<int>::const_iterator numberIt = source.cbegin(); numberIt != source.cend(); ++numberIt)
		{
			std::string number = std::to_string(*numberIt);
			words.push_back(number);
			bufSize += number.size();
			bufSize += 1;
		}
		_buffer = new char[bufSize + 1];
		int index = 0;
		for (std::vector<std::string>::const_iterator wordIt = words.cbegin(); wordIt != words.cend(); ++wordIt)
		{
			for (std::string::const_iterator letterIt = wordIt->cbegin(); letterIt != wordIt->cend(); ++letterIt, ++index)
			{
				_buffer[index] = *letterIt;
			}
			_buffer[index++] = ' ';
		}
		_buffer[bufSize] = '\0';
	}

	void toBuffer(std::vector<std::string> const & source)
	{
		bufSize = 0;
		for (std::vector<std::string>::const_iterator it = source.cbegin(); it != source.cend(); ++it)
		{
			bufSize += it->size();
			bufSize += 1;
		}
		_buffer = new char[bufSize + 1];
		int index = 0;
		for (std::vector<std::string>::const_iterator wordIt = source.cbegin(); wordIt != source.cend(); ++wordIt)
		{
			for (std::string::const_iterator letterIt = wordIt->cbegin(); letterIt != wordIt->cend(); ++letterIt, ++index)
			{
				_buffer[index] = *letterIt;
			}
			_buffer[index++] = ' ';
		}
		_buffer[bufSize] = '\0';
	}

	void encode(std::string const & target, int letterAlphabetSize)
	{
		myMap.clear();
		bufPos = 0;
		auto out = freopen(target.c_str(), "ab", stdout);
		initializeModels(COMPRESS, letterAlphabetSize);
		start_encoding(&rc, 33, 0);

		while (bufPos < bufSize)
		{
			int maxLength, maxOffset;
			long hash = getHash(bufPos);
			auto &positions = myMap[hash];
			if (!positions.empty())
			{
				bool seqFound = getLongestSequenceLength(positions, maxOffset, maxLength);
				if (seqFound)
				{
					std::string substr;
					for (int i = bufPos - maxOffset; i < bufPos - maxOffset + maxLength + 1; ++i)
					{
						substr += _buffer[i];
					}
					writeTripple(maxOffset, maxLength);
				}
				else
				{
					writePair(_buffer[bufPos]);
				}
			}
			else
			{
				writePair(_buffer[bufPos]);
			}
		}
		int ch, syfreq, ltfreq;
		qsgetfreq(&flagModel, 2, &syfreq, &ltfreq);
		encode_shift(&rc, syfreq, ltfreq, LG_TOTF);
		done_encoding(&rc);
		deleteModels();
		fclose(out);
	}

	void initializeModels(int mode, int letterAlphabetSize)
	{
		initqsmodel(&flagModel, FLAG_ALPHABET_SIZE, LG_TOTF, RESCALE, NULL, mode);
		initqsmodel(&letterModel, letterAlphabetSize, LG_TOTF, RESCALE, NULL, mode);
		initqsmodel(&offsetModel, OFFSET_ALPHABET_SIZE, LG_TOTF, RESCALE, NULL, mode);
		for (int i = 0; i < LENGTH_MODEL_SIZE; i++)
		{
			initqsmodel(&lengthModel[i], LENGTH_ALPHABET_SIZE, LG_TOTF, RESCALE, NULL, mode);
		}
	}

	long getHash(int x)
	{
		return (_buffer[(x)] << 16) | (_buffer[(x)+1] << 8) | (_buffer[(x)+2]);
	}

	bool getLongestSequenceLength(PositionVector & positions, int & maxOffset, int & maxLength)
	{
		maxOffset = 0, maxLength = 0;
		PositionVector::iterator it = positions.begin();
		while (it != positions.end())
		{
			int position = *it;
			int offset = bufPos - position;
			if (offset >= MAX_OFFSET)
			{
				it = positions.erase(it);
				continue;
			}
			int currentLength = 0;
			int currentPosition = bufPos + currentLength;
			int myPosition = position + currentLength;
			while (_buffer[currentPosition] == _buffer[myPosition]
				&& currentLength < MAX_LENGTH - 1
				&& currentLength < offset - 1)
			{
				++currentLength;
				++currentPosition;
				++myPosition;
			}
			if (currentLength > maxLength && currentLength >= MIN_LENGTH)
			{
				maxLength = currentLength;
				maxOffset = bufPos - myPosition + currentLength;
			}
			++it;
		}
		return maxOffset > 0;
	}

	void writeTripple(unsigned int offset, unsigned int length)
	{
		int sysfreq, ltfreq;
		int zero = 0;
		// zapis zera
		saveSymbol(flagModel, zero, sysfreq, ltfreq);
		if (offset <= MAX_LITTLE_OFFSET)
		{
			// zwykly zapis offsetu, taki sam jak na lab 01
			saveSymbol(offsetModel, offset, sysfreq, ltfreq);
		}
		else
		{
			// zapis duzego offsetu
			int longOffsetSymbol = 251;
			unsigned int newOffset = offset - longOffsetSymbol;
			qsgetfreq(&offsetModel, longOffsetSymbol, &sysfreq, &ltfreq);
			encode_shift(&rc, sysfreq, ltfreq, LG_TOTF);
			encodeNBits(&rc, newOffset, MAX_OFFSET_BITS);
			qsupdate(&offsetModel, longOffsetSymbol);
		}
		// zapis dlugosci slowa
		int log = ceilLog2(offset);
		saveSymbol(lengthModel[log], length, sysfreq, ltfreq);
		for (int i = 0; i < length; i++)
		{
			addNewHash();
		}
	}

	void writePair(unsigned char letter)
	{
		int ch, sysfreq, ltfreq;
		int one = 1;
		// zapis flagi
		saveSymbol(flagModel, one, sysfreq, ltfreq);
		// zapis litery
		saveSymbol(letterModel, letter, sysfreq, ltfreq);
		addNewHash();
	}

	void saveSymbol(qsmodel & model, int symbol, int & sysfreq, int & ltfreq)
	{
		qsgetfreq(&model, symbol, &sysfreq, &ltfreq);
		encode_shift(&rc, sysfreq, ltfreq, LG_TOTF);
		qsupdate(&model, symbol);
	}

	void loadSymbol(qsmodel & model, int symbol, int & sysfreq, int & ltfreq)
	{
		qsgetfreq(&model, symbol, &sysfreq, &ltfreq);
		decode_update(&rc, sysfreq, ltfreq, 1 << LG_TOTF);
		qsupdate(&model, symbol);
	}

	void addNewHash()
	{
		long hash = getHash(bufPos);
		auto &positions = myMap[hash];
		positions.push_back(bufPos);
		++bufPos;
	}

	void deleteModels()
	{
		deleteqsmodel(&flagModel);
		deleteqsmodel(&letterModel);
		deleteqsmodel(&offsetModel);
		for (int i = 0; i < LENGTH_MODEL_SIZE; ++i)
		{
			deleteqsmodel(&lengthModel[i]);
		}
	}

	int ceilLog2(int val)
	{
		int result;
		if (val == 1)
			return 0;
		result = 1;
		val -= 1;
		while (val >>= 1)
			++result;
		return result;
	}

	void encodeNBits(rangecoder * rc, int b, int n)
	{
		encode_shift(rc, (freq)1, (freq)b, (freq)n);
	}

	unsigned short decodeNBits(rangecoder * rc, int n)
	{
		unsigned short tmp;
		tmp = decode_culshift(rc, n);
		decode_update_shift(rc, 1, tmp, n);
		return tmp;
	}
};