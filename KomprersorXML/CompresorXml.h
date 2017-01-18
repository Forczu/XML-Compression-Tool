#pragma once
#include <unordered_map>
#include <vector>
#include <fstream>
#include <string> 
#include <iostream>
#include <algorithm>
#include <stack>
#include "rapidxml\rapidxml.hpp"
#include "rapidxml\rapidxml_print.hpp"
#include "LzssCoder.h"
#include "text_encoding_detect.h"
#include "ReadByteStrategyFactory.h"
#include "AbstractReadByteStrategy.h"

using namespace rapidxml;
using namespace AutoIt::Common;

/// <summary>
/// Kompresor plikow Xml
/// </summary>
class CompresorXml
{
protected:
	static const char STRING_FLAG = -0x37;
	static const char FLOAT_FLAG = -0x38;
	static const char SHORT_FLAG = -0x39;
	static const char INT_FLAG = -0x40;
	static const char CHAR_FLAG = -0x41;

	std::vector<char> valueTypes;

	static const char NEXT_ATTRIBUTE_SIGN = -0x44;
	static const char NEXT_CHILDREN_SIGN = -0x46;
	static const char NEXT_NODE_END_SIGN = -0x47;

	/// <summary>
	/// Struktura reprezentujaca oryginalny plik Xml
	/// </summary>
	xml_document <> _doc;

	/// <summary>
	/// Zwartosc pierwotnego pliku Xml w formie lancucha
	/// </summary>
	char * _contents;

	/// typ mapy haszujacej dla danych z xml wejsciowego
	typedef std::unordered_map<std::string, int> InputHashMap;

	/// typ mapy haszujacej dla danych ze skompresowanego pliku
	typedef std::unordered_map<int, std::string> OutputHashMap;

	/// <summary>
	/// Mapa zawierajaca nazwy znacznikow i odpowiadajace im liczby id przy kompresji
	/// </summary>
	InputHashMap _inputMarkupNameMap;

	/// <summary>
	/// Mapa zawierajaca nazwy atrybutow i odpowiadajace im liczby id przy kompresji
	/// </summary>
	InputHashMap _inputAttributeNameMap;

	/// <summary>
	/// Mapa zawierajaca nazwy znacznikow i odpowiadajace im liczby id przy dekompresji
	/// </summary>
	OutputHashMap _outputMarkupNameMap;

	/// <summary>
	/// Mapa zawierajaca nazwy atrybutow i odpowiadajace im liczby id przy dekompresji
	/// </summary>
	OutputHashMap _outputAttributeNameMap;

	/// <summary>
	/// Strategia zapisywania i odczytywania znacznikow
	/// </summary>
	AbstractReadByteStrategy * _markupStrategy;

	/// <summary>
	/// Strategia zapisywania i odczytywania atrybutow
	/// </summary>
	AbstractReadByteStrategy * _attributeStrategy;

public:
	/// <summary>
	/// Inicjalizuje obiekt klasy <see cref="CompresorXml"/>.
	/// </summary>
	CompresorXml() : _contents(nullptr)
	{
		valueTypes.push_back(STRING_FLAG);
		valueTypes.push_back(CHAR_FLAG);
		valueTypes.push_back(SHORT_FLAG);
		valueTypes.push_back(FLOAT_FLAG);
		valueTypes.push_back(INT_FLAG);
	}

	/// <summary>
	/// Niszczy odczytana zawartosc pliku Xml.
	/// </summary>
	~CompresorXml()
	{
		delete _contents;
	}

	/// <summary>
	/// Zakodowanie pliku xml do binarnej formy
	/// </summary>
	/// <param name="source">Sciezka do pliku zrodlowego.</param>
	/// <param name="target">Sciezka do pliku docelowego.</param>
	void encode(std::string const & source, std::string const & target)
	{
		try
		{
			parse(source);
			saveEncodedToBinaryFile(target);
		}
		catch (std::runtime_error const & ex)
		{
			std::cout << ex.what() << std::endl;
		}
	}

	/// <summary>
	/// Dekompresuje plik XMl z zrodla binarnego
	/// </summary>
	/// <param name="source">Sciezka do pliku zrodlowego.</param>
	/// <param name="target">Sciezka do pliku docelowego.</param>
	void decode(std::string const & source, std::string const & target)
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

		_markupStrategy = ReadByteStrategyFactory::Instance().create(markupNamesSaveStrategy);
		_attributeStrategy = ReadByteStrategyFactory::Instance().create(attributeNamesSaveStrategy);
		
		changePositionForRangeCoder(file, source, pos, '!');
		std::string markupNames = lzss.decode(source, pos);
		readMap(_outputMarkupNameMap, markupNames);

		changePositionForRangeCoder(file, source, pos, '!');
		std::string attributeNames = lzss.decode(source, pos);
		readMap(_outputAttributeNameMap, attributeNames);
		
		changePositionForRangeCoder(file, source, pos, '!');
		std::string markupValueSource = lzss.decode(source, pos);

		changePositionForRangeCoder(file, source, pos, '!');
		std::string attributeValueSource = lzss.decode(source, pos);
		
		changePositionForRangeCoder(file, source, pos, '!');
		std::string byteStr = lzss.decode(source, pos);
		int index = 0;
		readXml(byteStr, xml, index, markupValueSource, attributeValueSource, 0);

		std::ofstream out(target);
		out << xml;
		out.close();
	}

private:
	/// <summary>
	/// Zapsuje plik xml do skompresowanej, binarnej postaci
	/// </summary>
	/// <param name="filePath">Sciezka do pliku.</param>
	void saveEncodedToBinaryFile(std::string const & filePath)
	{
		std::ofstream file(filePath, std::ios::binary);
		LzssCoder lzss;

		auto root = _doc.first_node();
		allMarkupsToHashMap(root);

		ReadStrategy markupStr, attrStr;
		if (_inputMarkupNameMap.size() <= CHAR_MAX)
			markupStr = ReadStrategy::Char;
		else if (_inputMarkupNameMap.size() <= SHRT_MAX)
			markupStr = ReadStrategy::Short;
		else
			markupStr = ReadStrategy::Int;

		if (_inputAttributeNameMap.size() <= CHAR_MAX)
			attrStr = ReadStrategy::Char;
		else if (_inputAttributeNameMap.size() <= SHRT_MAX)
			attrStr = ReadStrategy::Short;
		else
			attrStr = ReadStrategy::Int;
		_markupStrategy = ReadByteStrategyFactory::Instance().create(markupStr);
		_attributeStrategy = ReadByteStrategyFactory::Instance().create(attrStr);

		file.write((char*)&markupStr, sizeof(char));
		file.write((char*)&attrStr, sizeof(char));
		file.close();

		std::vector<char> * xml = new std::vector<char>();
		std::string markupValues;
		std::string attributeValues;
		saveXml(root, *xml, markupValues, attributeValues);

		std::vector<std::string> markups;
		saveMap(_inputMarkupNameMap, markups);
		lzss.encode(markups, filePath);

		std::vector<std::string> attributes;
		saveMap(_inputAttributeNameMap, attributes);
		lzss.encode(attributes, filePath);

		lzss.encode(markupValues, filePath);
		lzss.encode(attributeValues, filePath);

		lzss.encode(*xml, filePath);

		delete xml;
		delete _markupStrategy, _attributeStrategy;
	}

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
		return out;
	}

	/// <summary>
	/// Zapisuje wszystkie identyfikatory znacznikow w xmlu jako liczby do mapy
	/// </summary>
	/// <param name="firstNode">Pierwszy wezel.</param>
	void allMarkupsToHashMap(xml_node<> * firstNode)
	{
		static int markupNameCounter = 0, attributeCounter = 0;
		// wezly
		for (xml_node<>* node = firstNode; node; node = node->next_sibling())
		{
			// znacznik
			std::string nodeName = node->name();
			if (!nodeName.empty() && _inputMarkupNameMap.find(nodeName) == _inputMarkupNameMap.end())
				_inputMarkupNameMap[nodeName] = markupNameCounter++;
			// atrybuty
			for (xml_attribute<>* atr = node->first_attribute(); atr; atr = atr->next_attribute())
			{
				// nazwa atrybutu
				std::string attrName = atr->name();
				if (!attrName.empty() && _inputAttributeNameMap.find(attrName) == _inputAttributeNameMap.end())
					_inputAttributeNameMap[attrName] = attributeCounter++;
			}
			// dzieci w wezle
			auto firstChild = node->first_node();
			if (firstChild != NULL)
				allMarkupsToHashMap(firstChild);
		}
	}

	/// <summary>
	/// Zapisuje mape do postaci wektora stringow.
	/// </summary>
	/// <param name="map">Mapa.</param>
	/// <param name="names">Wyjsciowy wektor.</param>
	void saveMap(InputHashMap const & map, std::vector<std::string> & names)
	{
		for (InputHashMap::const_iterator it = map.begin(); it != map.end(); ++it)
		{
			names.push_back(std::to_string(it->second));
			names.push_back(it->first);
		}
	}

	/// <summary>
	/// Na podstawie stringu wejsciowego tworzy mape, gdzie kluczem jest identyfikator znacznika, a wartoscia jest nazwa.
	/// </summary>
	/// <param name="map">Mapa wyjsciowa.</param>
	/// <param name="names">String stanowiacy zrodlo nazw.</param>
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

	/// <summary>
	/// Ustawia wskaznik pozycji na kolejne sekcje range codera
	/// </summary>
	/// <param name="file">Plik wejsciowy.</param>
	/// <param name="source">Sciezka do pliku.</param>
	/// <param name="pos">Pozycja w pliku, zostaje zmieniona.</param>
	/// <param name="endSign">Znak rozpoczynajacy blok range codera.</param>
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

	/// <summary>
	/// Proba zmiany stringu na wartosc typu float.
	/// </summary>
	/// <param name="str">String wejsciowy.</param>
	/// <param name="out">Float wyjsciowy.</param>
	/// <returns><c>true</c> jezeli konwersja jest mozliwe, <c>false<c> jezeli nie</returns>
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

	/// <summary>
	/// Zmienia wartosc typu float na ciag bajtow.
	/// </summary>
	/// <param name="paramFloat">Float wejsciowy.</param>
	/// <returns>Ciag bajtow reprezentujacy float</returns>
	std::vector<char> floatToBytes(float paramFloat)
	{
		unsigned char* p = reinterpret_cast<unsigned char*>(&paramFloat);
		std::vector<char> arrayOfBytes(p, p + 4);
		return arrayOfBytes;
	}

	/// <summary>
	/// Determinuje jaki typ wartosci posiada string i zmiana go na zbiot bajtow oraz zwraca odpowiednia flage do referencji flag.
	/// </summary>
	/// <param name="str">String wejsciowy.</param>
	/// <param name="flag">Zwracana flaga wartosci.</param>
	/// <returns></returns>
	std::vector<char> stringToValues(std::string const & str, char & flag)
	{
		std::vector<char> bytes;
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
			bytes = _markupStrategy->intToBytes(i);
		}
		else if (flag == FLOAT_FLAG)
		{
			bytes = floatToBytes(f);
		}
		else if (flag == SHORT_FLAG)
		{
			bytes = _markupStrategy->shortToBytes(i);
		}
		else if (flag == CHAR_FLAG)
		{
			auto byte = _markupStrategy->intToByte(i);
			bytes.push_back(byte);
		}
		else
		{
			int size = str.size();
			bytes = _markupStrategy->intToBytes(size);
		}
		return bytes;
	}

	/// <summary>
	/// Konwenrtuje ciag bajtow do postaci stringa.
	/// </summary>
	/// <param name="nextFlag">Flaga wartosci.</param>
	/// <param name="bytes">Zbior bajtow.</param>
	/// <param name="index">Indeks pierwszego bajtu.</param>
	/// <param name="strategy">Strategia odczytywania.</param>
	/// <param name="source">Miejsce skad odczytac wartosci stringowe.</param>
	/// <param name="sourcePos">Pozycja z ktorej odczytac wartosc stringowa.</param>
	/// <returns></returns>
	std::string bytesToString(char nextFlag, std::string const & bytes, int & index, AbstractReadByteStrategy * strategy, std::string const & source, int & sourcePos)
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

	/// <summary>
	/// Zapisuje zawartosc pliku XML jako binarna forme znacznikow i ich wartosci
	/// </summary>
	/// <param name="firstNode">Pierwszy wezel xml.</param>
	/// <param name="xml">The XML.</param>
	/// <param name="markupValues">Miejsce zapisu wszystkich wartosci znacznikow.</param>
	/// <param name="attributeValues">Miejsce zapisu wszystkich wartosci atrybutow.</param>
	void saveXml(xml_node<>* firstNode, std::vector<char> & xml, std::string & markupValues, std::string & attributeValues)
	{
		for (xml_node<>* node = firstNode; node; node = node->next_sibling())
		{
			// zapis nazwy znacznika
			std::string nodeName = node->name();
			if (!nodeName.empty())
			{
				int nodeId = _inputMarkupNameMap[nodeName];
				std::vector<char> bytes = _markupStrategy->writeToBytes(nodeId);
				//xml.push_back(NEXT_NEW_NODE_SIGN);
				xml.insert(xml.end(), bytes.begin(), bytes.end());
			}
			// zapis atrybutow wezla
			for (xml_attribute<>* atr = node->first_attribute(); atr; atr = atr->next_attribute())
			{
				xml.push_back(NEXT_ATTRIBUTE_SIGN);
				// nazwa atrybutu
				std::string attrName = atr->name();
				int attrId = _inputAttributeNameMap[attrName];
				std::vector<char> bytes = _attributeStrategy->writeToBytes(attrId);
				xml.insert(xml.end(), bytes.begin(), bytes.end());
				// wartosc atrybutu
				std::string attrValue = atr->value();
				char typeFlag;
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
				//xml.push_back(NEXT_VALUE_SIGN);
				char typeFlag;
				std::vector<char> bytes = stringToValues(value, typeFlag);
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
					saveXml(firstChild, xml, markupValues, attributeValues);
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
	/// <param name="index">The index.</param>
	/// <param name="markupValueSource">Zrodlo wartosci wszystkich znacznikow.</param>
	/// <param name="attributeValueSource">Zrodlo wartosci wszystkich atrybutow.</param>
	/// <param name="tabulators">Poziom tabulacji.</param>
	void readXml(std::string const & bytes, std::string & xml, int & index, std::string const & markupValueSource, std::string const & attributeValueSource, int tabulators)
	{
		static int markupValueSourcePos = 0, attributeValueSourcePos = 0;
		static std::stack<std::string> lastOpenedNodes;
		int id, markupSize = _markupStrategy->getSize(), attributeSize = _attributeStrategy->getSize();
		do
		{
			// poczatek linii
			char nextFlag = bytes[index];
			// flaga zamkniecia otwartego wezla
			if (nextFlag == NEXT_NODE_END_SIGN)
			{
				++index;
				std::string name = lastOpenedNodes.top();
				lastOpenedNodes.pop();
				for (int i = 0; i < tabulators - 1; ++i)
					xml += '\t';
				xml += "</" + name + ">\n";
				break;
			}
			id = _markupStrategy->read(bytes, index);
			index += markupSize;
			std::string nodeName = _outputMarkupNameMap[id];
			for (int i = 0; i < tabulators; ++i)
				xml += '\t';
			xml += '<' + nodeName;
			nextFlag = bytes[index];
			while (nextFlag == NEXT_ATTRIBUTE_SIGN)
			{
				++index;
				id = _attributeStrategy->read(bytes, index);
				index += attributeSize;
				std::string attrName = _outputAttributeNameMap[id];
				nextFlag = bytes[index]; ++index;
				std::string attrValue = bytesToString(nextFlag, bytes, index, _attributeStrategy, attributeValueSource, attributeValueSourcePos);
				xml += ' ' + attrName + "=\"" + attrValue + "\"";
				nextFlag = _attributeStrategy->read(bytes, index);
			}
			++index;
			if (nextFlag == NEXT_NODE_END_SIGN)
			{
				xml += "/>\n";
			}
			else if (isFlagValueType(nextFlag))
			{
				auto nodeValue = bytesToString(nextFlag, bytes, index, _markupStrategy, markupValueSource, markupValueSourcePos);
				xml += '>' + nodeValue + "</" + nodeName + ">\n";
			}
			else if (nextFlag == NEXT_CHILDREN_SIGN)
			{
				xml += ">\n";
				lastOpenedNodes.push(nodeName);
				readXml(bytes, xml, index, markupValueSource, attributeValueSource, tabulators + 1);
			}
		} while (!lastOpenedNodes.empty());
	}

	/// <summary>
	/// Sprawdza czy odczytana flaga nalezy do zbioru flag wartosci.
	/// </summary>
	/// <param name="flag">Flaga.</param>
	/// <returns>
	///   <c>true</c> jesli flaga nalezy do zbioru flag wartosci; w przeciwnym razie, <c>false</c>.
	/// </returns>
	bool isFlagValueType(char flag)
	{
		return std::find(valueTypes.begin(), valueTypes.end(), flag) != valueTypes.end();
	}
};