#pragma once
#include <unordered_map>
#include <vector>
#include <fstream>
#include <string> 
#include <iostream>
#include <stack>
#include "rapidxml\rapidxml.hpp"
#include "rapidxml\rapidxml_print.hpp"
#include "LzssCoder.h"
#include "text_encoding_detect.h"
#include "ReadFourBytesStrategy.h"
#include "ReadTwoBytesStrategy.h"
#include "ReadOneByteStrategy.h"

using namespace rapidxml;
using namespace AutoIt::Common;

enum class ReadStrategy : char
{
	Char = 0x30, Short = 0x31, Int = 0x32
};

/// <summary>
/// Kompresor plikow Xml
/// </summary>
class CompresorXml
{
protected:
	static const unsigned char STRING_FLAG = 0x37;
	static const unsigned char FLOAT_FLAG = 0x38;
	static const unsigned char SHORT_FLAG = 0x39;
	static const unsigned char INT_FLAG = 0x40;
	static const unsigned char CHAR_FLAG = 0x41;

	static const unsigned char NEXT_ATTRIBUTE_SIGN = 0x44;
	static const unsigned char NEXT_VALUE_SIGN = 0x45;
	static const unsigned char NEXT_CHILDREN_SIGN = 0x46;
	static const unsigned char NEXT_NODE_END_SIGN = 0x47;
	static const unsigned char NEXT_NEW_NODE_SIGN = 0x48;

	/// <summary>
	/// Struktura reprezentujaca oryginalny plik Xml
	/// </summary>
	xml_document <> _doc;

	/// <summary>
	/// Zwartosc pierwotnego pliku Xml w formie lancucha
	/// </summary>
	char * _contents;

	// typ mapy haszujacej dla danych z xml wejsciowego
	typedef std::unordered_map<std::string, int> InputHashMap;

	// typ mapy haszujacej dla danych ze skompresowanego pliku
	typedef std::unordered_map<int, std::string> OutputHashMap;

	/// <summary>
	/// Mapa zawierajaca nazwy znacznikow i odpowiadajace im liczby id
	/// </summary>
	InputHashMap _markupNameMap;

	/// <summary>
	/// Mapa zawierajaca wartosci znacznikow i odpowiadajace im liczby id
	/// </summary>
	InputHashMap _markupValueMap;

	/// <summary>
	/// Mapa zawierajaca nazwy atrybutow i odpowiadajace im liczby id
	/// </summary>
	InputHashMap _attributeNameMap;

	/// <summary>
	/// Mapa zawierajaca wartosci atrybutow i odpowiadajace im liczby id
	/// </summary>
	InputHashMap _attributeValueMap;

	/// <summary>
	/// Pomocniczy licznik dla nazw znacznikow przy kompresji
	/// </summary>
	int _markupNameCounter;

	/// <summary>
	/// Pomocniczy licznik dla wartosci znacznikow przy kompresji
	/// </summary>
	int _markupValueCounter;

	/// <summary>
	/// Pomocniczy licznik dla atrybutow przy kompresji
	/// </summary>
	int _attributeCounter;

	/// <summary>
	/// Pomocniczy licznik dla atrybutow przy kompresji
	/// </summary>
	int _attributeValueCounter;

	OutputHashMap _markupNameMap2;
	OutputHashMap _attributeNameMap2;

	std::stack<std::string> _lastOpenedNodes;

	std::string markupValues;
	std::string attributeValues;

	std::string markupValueSource;
	std::string attributeValueSource;

	int markupValueSourcePos;
	int attributeValueSourcePos;

	AbstractReadByteStrategy * markupStrategy;
	AbstractReadByteStrategy * attributeStrategy;

public:
	CompresorXml() : _contents(nullptr)
	{
		_markupNameCounter = _markupValueCounter = _attributeCounter = _attributeValueCounter = 0;
		markupValueSourcePos = attributeValueSourcePos = 0;
	}

	~CompresorXml()
	{
		delete _contents;
	}

	/// <summary>
	/// Zakodowanie pliku xml
	/// </summary>
	/// <param name="filePath">Sciezka do pliku.</param>
	void encode(std::string const & filePath)
	{
		try
		{
			parse(filePath);
			saveEncodedToBinaryFile(filePath + ".bin");
		}
		catch (std::runtime_error const & ex)
		{
			std::cout << ex.what() << std::endl;
		}
	}

	void saveEncodedToBinaryFile(std::string const & filePath)
	{
		std::ofstream file(filePath, std::ios::binary);
		LzssCoder lzss;

		auto root = _doc.first_node();
		allMarkupsToHashMap(root);

		ReadStrategy markupStr, attrStr;
		if (_markupNameMap.size() <= UCHAR_MAX - 2)
		{
			markupStrategy = new ReadOneBytesStrategy;
			markupStr = ReadStrategy::Char;
		}
		else if (_markupNameMap.size() <= USHRT_MAX - 2)
		{
			markupStrategy = new ReadTwoBytesStrategy;
			markupStr = ReadStrategy::Short;
		}
		else
		{
			markupStrategy = new ReadFourBytesStrategy;
			markupStr = ReadStrategy::Int;
		}

		if (_attributeNameMap.size() <= UCHAR_MAX - 2)
		{
			attributeStrategy = new ReadOneBytesStrategy;
			attrStr = ReadStrategy::Char;
		}
		else if (_attributeNameMap.size() <= USHRT_MAX - 2)
		{
			attributeStrategy = new ReadTwoBytesStrategy;
			attrStr = ReadStrategy::Short;
		}
		else
		{
			attributeStrategy = new ReadFourBytesStrategy;
			attrStr = ReadStrategy::Int;
		}

		file.write((char*)&markupStr, sizeof(char));
		file.write((char*)&attrStr, sizeof(char));
		file.close();

		std::vector<unsigned char> * xml = new std::vector<unsigned char>();
		saveXml(root, *xml);

		std::vector<std::string> markups;
		saveMap(_markupNameMap, markups);
		lzss.encode(markups, filePath);

		std::vector<std::string> attributes;
		saveMap(_attributeNameMap, attributes);
		lzss.encode(attributes, filePath);

		lzss.encode(markupValues, filePath);
		lzss.encode(attributeValues, filePath);

		lzss.encode(*xml, filePath);

		delete xml;
		delete markupStrategy, attributeStrategy;
	}

	void readEncodedBinaryFile(std::string const & source, std::string const & target)
	{
		std::ifstream file;
		std::string xml;
		int pos = 0;
		LzssCoder lzss;

		file.open(source, std::ios::binary | std::ios::in);
		ReadStrategy markupNamesSaveStrategy, attributeNamesSaveStrategy;
		file.read((char*)&markupNamesSaveStrategy, sizeof(char));
		file.read((char*)&attributeNamesSaveStrategy, sizeof(char));
		pos = file.tellg();
		file.close();
		switch (markupNamesSaveStrategy)
		{
		case ReadStrategy::Char:
			markupStrategy = new ReadOneBytesStrategy;
			break;
		case ReadStrategy::Short:
			markupStrategy = new ReadTwoBytesStrategy;
			break;
		case ReadStrategy::Int: default:
			markupStrategy = new ReadFourBytesStrategy;
			break;
		}
		switch (attributeNamesSaveStrategy)
		{
		case ReadStrategy::Char:
			attributeStrategy = new ReadOneBytesStrategy;
			break;
		case ReadStrategy::Short:
			attributeStrategy = new ReadTwoBytesStrategy;
			break;
		case ReadStrategy::Int: default:
			attributeStrategy = new ReadFourBytesStrategy;
			break;
		}
		
		changePositionForRangeCoder(file, source, pos, '!');
		std::string markupNames = lzss.decode(source, pos);
		readMap(_markupNameMap2, markupNames);

		changePositionForRangeCoder(file, source, pos, '!');
		std::string attributeNames = lzss.decode(source, pos);
		readMap(_attributeNameMap2, attributeNames);
		
		changePositionForRangeCoder(file, source, pos, '!');
		markupValueSource = lzss.decode(source, pos);

		changePositionForRangeCoder(file, source, pos, '!');
		attributeValueSource = lzss.decode(source, pos);
		
		changePositionForRangeCoder(file, source, pos, '!');
		std::string byteStr = lzss.decode(source, pos);
		int index = 0;
		readXml(byteStr, xml, index, 0);

		std::ofstream out(target);
		out << xml;
		out.close();
	}

private:
	/// <summary>
	/// Sparoswanie wejsciowego pliku xml do pomocniczej struktury
	/// </summary>
	/// <param name="filePath">Sciezka do pliku.</param>
	/// <returns></returns>
	bool parse(std::string const & filePath)
	{
		_contents = xmlToChar(filePath);
		if (_contents == nullptr)
			return false;
		try
		{
			_doc.parse<0>(_contents);
			return true;
		}
		catch (rapidxml::parse_error const & e)
		{
			return true;
		}
	}

	/// <summary>
	/// Zapis pliku xml do lancucha znakow, konieczny dla biblioteki rapidxml
	/// </summary>
	/// <param name="stageFile">Sciezka do pliku.</param>
	/// <returns></returns>
	char * xmlToChar(std::string const & stageFile)
	{
		std::ifstream file(stageFile);
		if (file.fail())
			return nullptr;
		std::filebuf * pbuf = file.rdbuf();
		long fileLength = static_cast<long>(pbuf->pubseekoff(0, std::ios::end, std::ios::in));
		file.seekg(0);
		char * out = new char[fileLength + 1];
		file.read(out, fileLength);

		TextEncodingDetect detect;
		const unsigned char * inDetect = (unsigned char*)out;
		TextEncodingDetect::Encoding xmlEncoding = detect.DetectEncoding(inDetect, fileLength);
		switch (xmlEncoding)
		{
		case AutoIt::Common::TextEncodingDetect::None:
		case AutoIt::Common::TextEncodingDetect::ANSI:
		case AutoIt::Common::TextEncodingDetect::ASCII:
			break;
		default:
			delete out;
			std::string message("XML is not encoded in ANSI or ASCII.");
			throw std::runtime_error(message);
			break;
		}
		return out;
	}

	/// <summary>
	/// Zapisuje wszystkie identyfikatory znacznikow w xmlu jako liczby do mapy
	/// </summary>
	/// <param name="firstNode">Pierwszy wezel.</param>
	void allMarkupsToHashMap(xml_node<> * firstNode)
	{
		// wezly
		for (xml_node<>* node = firstNode; node; node = node->next_sibling())
		{
			// znacznik
			std::string nodeName = node->name();
			if (!nodeName.empty() && _markupNameMap.find(nodeName) == _markupNameMap.end())
				_markupNameMap[nodeName] = _markupNameCounter++;
			// atrybuty
			for (xml_attribute<>* atr = node->first_attribute(); atr; atr = atr->next_attribute())
			{
				// nazwa atrybutu
				std::string attrName = atr->name();
				if (!attrName.empty() && _attributeNameMap.find(attrName) == _attributeNameMap.end())
					_attributeNameMap[attrName] = _attributeCounter++;
			}
			// dzieci w wezle
			auto firstChild = node->first_node();
			if (firstChild != NULL)
				allMarkupsToHashMap(firstChild);
		}
	}

	void saveMap(InputHashMap const & map, std::vector<std::string> & names)
	{
		for (InputHashMap::const_iterator it = map.begin(); it != map.end(); ++it)
		{
			names.push_back(std::to_string(it->second));
			names.push_back(it->first);
		}
	}

	void readMap(OutputHashMap & map, std::string const & names)
	{
		enum State { Id, Value };
		State state = Id;
		std::string str;
		int id;
		for (std::string::const_iterator it = names.cbegin(); it != names.end(); ++it)
		{
			if (*it != ' ')
				str += *it;
			else
			{
				switch (state)
				{
				case Id:
					id = std::stoi(str);
					state = Value;
					break;
				case Value:
					map[id] = str;
					state = Id;
					break;
				}
				str = "";
			}
		}
	}

	void changePositionForRangeCoder(std::ifstream & file, std::string const & source, int & pos, char endSign)
	{
		file.open(source, std::ios::binary | std::ios::app);
		file.seekg(std::ios_base::beg);
		file.seekg(pos);
		char a;
		std::streampos lastPos;
		while (true)
		{
			lastPos = file.tellg();
			file.read(&a, sizeof(char));
			if (a == endSign)
				break;
		}
		file.close();
		pos = lastPos;
	}

	bool tryParseToFloat(std::string const & str, float & out)
	{
		char *end;
		float  f;
		const char * cStr = str.c_str();
		if (*cStr == '\0')
			return false;
		f = strtof(cStr, &end);
		if (*end != '\0')
			return false;
		out = f;
		return true;
	}

	std::vector<unsigned char> floatToBytes(float paramFloat)
	{
		unsigned char* p = reinterpret_cast<unsigned char*>(&paramFloat);
		std::vector<unsigned char> arrayOfBytes(p, p + 4);
		return arrayOfBytes;
	}

	std::vector<unsigned char> stringToValues(std::string const & str, unsigned char & flag)
	{
		std::vector<unsigned char> bytes;
		flag = STRING_FLAG;
		float f;
		int i;
		bool isFloat = tryParseToFloat(str, f);
		if (isFloat)
		{
			bool isInteger = ceilf(f) == f;
			if (isInteger)
			{
				i = f;
				if (i >= -256 && i <= 255)
					flag = CHAR_FLAG;
				else if (i >= -32768 && i <= 32767)
					flag = SHORT_FLAG;
				else
				{
					int digits = i > 0 ? (int)log10((double)i) + 1 : 1;
					if (digits > 5)
						flag = INT_FLAG;
				}
			}
			else
			{
				flag = FLOAT_FLAG;
			}
		}
		if (flag == INT_FLAG)
		{
			bytes = markupStrategy->intToBytes(i);
		}
		else if (flag == FLOAT_FLAG)
		{
			bytes = floatToBytes(f);
		}
		else if (flag == SHORT_FLAG)
		{
			bytes = markupStrategy->shortToBytes(i);
		}
		else if (flag == CHAR_FLAG)
		{
			auto byte = markupStrategy->intToByte(i);
			bytes.push_back(byte);
		}
		else
		{
			int size = str.size();
			bytes = markupStrategy->intToBytes(size);
		}
		return bytes;
	}

	std::string bytesToString(char nextFlag, std::string const & bytes, int & index, AbstractReadByteStrategy * strategy, std::string & source, int & sourcePos)
	{
		std::string value;
		if (nextFlag == INT_FLAG)
		{
			int valueInt = strategy->bytesToInt(bytes, index);
			value = std::to_string(valueInt);
			index += 4;
		}
		else if (nextFlag == FLOAT_FLAG)
		{
			float valueFloat = strategy->bytesToFloat(bytes, index);
			value = std::to_string(valueFloat);
			index += 4;
		}
		else if (nextFlag == SHORT_FLAG)
		{
			short valueShort = strategy->bytesToShort(bytes, index);
			value = std::to_string(valueShort);
			index += 2;
		}
		else if (nextFlag == CHAR_FLAG)
		{
			unsigned char charValue = bytes[index];
			value = std::to_string(charValue);
			index += 1;
		}
		else if (nextFlag == STRING_FLAG)
		{
			int sizeStr = strategy->bytesToInt(bytes, index);
			value = source.substr(sourcePos, sizeStr);
			index += 4;
			sourcePos += sizeStr;
		}
		return value;
	}

	void saveXml(xml_node<>* firstNode, std::vector<unsigned char> & xml)
	{
		for (xml_node<>* node = firstNode; node; node = node->next_sibling())
		{
			// zapis nazwy znacznika
			std::string nodeName = node->name();
			if (!nodeName.empty())
			{
				int nodeId = _markupNameMap[nodeName];
				std::vector<unsigned char> bytes = markupStrategy->writeToBytes(nodeId);
				xml.push_back(NEXT_NEW_NODE_SIGN);
				xml.insert(xml.end(), bytes.begin(), bytes.end());
			}
			// zapis atrybutow wezla
			for (xml_attribute<>* atr = node->first_attribute(); atr; atr = atr->next_attribute())
			{
				xml.push_back(NEXT_ATTRIBUTE_SIGN);
				// nazwa atrybutu
				std::string attrName = atr->name();
				int attrId = _attributeNameMap[attrName];
				std::vector<unsigned char> bytes = attributeStrategy->writeToBytes(attrId);
				xml.insert(xml.end(), bytes.begin(), bytes.end());
				// wartosc atrybutu
				std::string attrValue = atr->value();
				unsigned char typeFlag;
				bytes = stringToValues(attrValue, typeFlag);
				if (typeFlag == STRING_FLAG)
					attributeValues += attrValue;
				xml.push_back(typeFlag);
				xml.insert(xml.end(), bytes.begin(), bytes.end());
			}
			// zapis wartosci wezla
			const std::string value = node->value();
			bool hasValue = !value.empty() && value.size() != 0;
			if (hasValue)
			{
				xml.push_back(NEXT_VALUE_SIGN);
				unsigned char typeFlag;
				std::vector<unsigned char> bytes = stringToValues(value, typeFlag);
				if (typeFlag == STRING_FLAG)
					markupValues += value;
				xml.push_back(typeFlag);
				xml.insert(xml.end(), bytes.begin(), bytes.end());
			}
			else
			{
				// zapis dzieci wezla
				auto firstChild = node->first_node();
				bool hasChildren = firstChild != NULL && strlen(firstChild->name()) != 0;
				if (hasChildren)
				{
					xml.push_back(NEXT_CHILDREN_SIGN);
					saveXml(firstChild, xml);
				}
				// zapis znaku konca wezla
				xml.push_back(NEXT_NODE_END_SIGN);
			}
		}
	}
	/// <summary>
	/// Funkcja rekurencyjnie wczytujaca plik XML. W pojedynczej iteracji wczytywana jest pojdyncza linia XML,
	/// w pojedynczej rekurencji wylacznie wezly o jednakowym zagniezdzeniu. 
	/// </summary>
	/// <param name="bytes">The bytes.</param>
	/// <param name="xml">The XML.</param>
	/// <param name="tabulators">The tabulators.</param>
	/// <param name="markupStrategy">The markup strategy.</param>
	/// <param name="attributeStrategy">The attribute strategy.</param>
	void readXml(std::string const & bytes, std::string & xml, int & index, int tabulators)
	{
		int id;
		int markupSize = markupStrategy->getSize(), attributeSize = attributeStrategy->getSize();
		do
		{
			// poczatek linii
			char nextFlag = bytes[index]; ++index;
			// flaga zamkniecia otwartego wezla
			if (nextFlag == NEXT_NODE_END_SIGN)
			{
				std::string name = _lastOpenedNodes.top();
				_lastOpenedNodes.pop();
				for (int i = 0; i < tabulators - 1; ++i)
					xml += '\t';
				xml += "</" + name + ">\n";
				break;
			}
			else if (nextFlag == NEXT_NEW_NODE_SIGN)
			{
				id = markupStrategy->read(bytes, index);
				index += markupSize;
				std::string nodeName = _markupNameMap2[id];
				for (int i = 0; i < tabulators; ++i)
					xml += '\t';
				xml += '<' + nodeName;
				nextFlag = bytes[index];
				while (nextFlag == NEXT_ATTRIBUTE_SIGN)
				{
					++index;
					id = attributeStrategy->read(bytes, index);
					index += attributeSize;
					std::string attrName = _attributeNameMap2[id];
					nextFlag = bytes[index]; ++index;
					std::string attrValue = bytesToString(nextFlag, bytes, index, attributeStrategy, attributeValueSource, attributeValueSourcePos);
					xml += ' ' + attrName + "=\"" + attrValue + "\"";
					nextFlag = attributeStrategy->read(bytes, index);
				}
				++index;
				if (nextFlag == NEXT_NODE_END_SIGN)
				{
					xml += "/>\n";
				}
				else if (nextFlag == NEXT_VALUE_SIGN)
				{
					nextFlag = bytes[index]; ++index;
					auto nodeValue = bytesToString(nextFlag, bytes, index, markupStrategy, markupValueSource, markupValueSourcePos);
					xml += '>' + nodeValue + "</" + nodeName + ">\n";
				}
				else if (nextFlag == NEXT_CHILDREN_SIGN)
				{
					xml += ">\n";
					_lastOpenedNodes.push(nodeName);
					readXml(bytes, xml, index, tabulators + 1);
				}
			}
		} while (!_lastOpenedNodes.empty());
	}
};