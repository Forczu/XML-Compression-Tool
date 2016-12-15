#pragma once
#include <unordered_map>
#include <vector>
#include <fstream>
#include <string> 
#include <iostream>
#include <stack>
#include "rapidxml\rapidxml.hpp"
#include "rapidxml\rapidxml_print.hpp"

using namespace rapidxml;

/// <summary>
/// Kompresor plikow Xml
/// </summary>
class CompresorXml
{
protected:
	static const int NODE_END_SIGN = -1;
	static const int NODE_HEAD_END_SIGN = -2;

	static const int MARKUP_NAMES_ID = -3;
	static const int MARKUP_VALUES_ID = -4;
	static const int ATTRIBUTE_NAMES_ID = -5;
	static const int ATTRIBUTE_VALUES_ID = -6;
	static const int MAPS_END = -7;

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
	OutputHashMap _markupValueMap2;
	OutputHashMap _attributeNameMap2;
	OutputHashMap _attributeValueMap2;

	std::stack<std::string> _lastOpenedNodes;

public:
	CompresorXml() : _contents(nullptr)
	{
		_markupNameCounter = _markupValueCounter = _attributeCounter = _attributeValueCounter = 0;
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
		parse(filePath);
		allMarkupsToHashMap(_doc.first_node());
	}

	/// <summary>
	/// Zapisanie skompresowanego pliku xml do czytelnej formy
	/// Funkcja ma na celu ulatwienie testowania
	/// </summary>
	/// <param name="filePath">Sciezka do pliku.</param>
	void saveEncodedToFile(std::string const & filePath)
	{
		std::string output;
		int tabulators = 0;
		saveMaps(output);
		saveCompressed(_doc.first_node(), output, tabulators);
		std::ofstream file(filePath);
		file << output.c_str();
		file.close();
	}

	/// <summary>
	/// Zapis wezla i jego dzieci do czytelnej formy
	/// </summary>
	/// <param name="firstNode">Wezel.</param>
	/// <param name="output">Miejsce, gdzie wezel jako string zostanie zapisany.</param>
	/// <param name="tabulators">Wielkosc tabulacji.</param>
	void saveCompressed(xml_node<>* firstNode, std::string & output, int tabulators)
	{
		for (xml_node<>* node = firstNode; node; node = node->next_sibling())
		{
			// zapis nazwy wezla
			std::string nodeName = node->name();
			if (!nodeName.empty())
			{
				for (int i = 0; i < tabulators; ++i)
					output += '\t';
				// zapis id zamiast nazwy
				std::string nodeId = std::to_string(_markupNameMap[nodeName]);
				output += nodeId;
			}
			// zapis atrybutow wezla
			for (xml_attribute<>* atr = node->first_attribute(); atr; atr = atr->next_attribute())
			{
				std::string attribute;
				attribute += ' ';
				std::string attrId = std::to_string(_attributeNameMap[atr->name()]);
				attribute += attrId;
				attribute += ' ';// '=';
				std::string attrValueId = std::to_string(_attributeValueMap[atr->value()]);
				attribute += attrValueId;
				output += attribute;
			}
			// zapis wartosci wezla
			auto value = node->value();
			bool hasValue = value != NULL && strlen(value) != 0;
			if (hasValue)
			{
				output += ' ';
				std::string valueId = std::to_string(_markupValueMap[value]);
				output += valueId;
			}
			// zapis dzieci wezla
			auto firstChild = node->first_node();
			bool hasChildren = firstChild != NULL && strlen(firstChild->name()) != 0;
			if (hasChildren)
			{
				output += ' ';
				output += std::to_string(NODE_HEAD_END_SIGN);
				output += '\n';
				saveCompressed(firstChild, output, tabulators + 1);
			}
			// zapis znaku konca wezla
			if (hasChildren)
				for (int i = 0; i < tabulators; ++i)
					output += '\t';
			else
				output += ' ';
			output += std::to_string(NODE_END_SIGN);
			output += '\n';
		}
	}

	void saveEncodedToBinaryFile(std::string const & filePath)
	{
		std::ofstream file(filePath, std::ios::binary);
		saveMaps(file);
		saveXml(_doc.first_node(), file);
	}

	void readEncodedBinaryFile(std::string const & source, std::string const & target)
	{
		std::ifstream file(source, std::ios::binary);
		std::string xml;
		readMaps(file);
		readXml(file, xml, 0);
		file.close();
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
			return false;
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
		// wezly
		for (xml_node<>* node = firstNode; node; node = node->next_sibling())
		{
			// nazwa znacznika
			std::string nodeName = node->name();
			if (!nodeName.empty() && _markupNameMap.find(nodeName) == _markupNameMap.end())
				_markupNameMap[nodeName] = _markupNameCounter++;
			// wartosc znacznika
			std::string nodeValue = node->value();
			if (!nodeValue.empty() && _markupValueMap.find(nodeName) == _markupValueMap.end())
				_markupValueMap[nodeValue] = _markupValueCounter++;
			// atrybuty
			for (xml_attribute<>* atr = node->first_attribute(); atr; atr = atr->next_attribute())
			{
				// nazwa atrybutu
				std::string attrName = atr->name();
				if (!attrName.empty() && _attributeNameMap.find(attrName) == _attributeNameMap.end())
					_attributeNameMap[attrName] = _attributeCounter++;
				std::string attrValue = atr->value();
				// wartosc atrybutu
				if (!attrValue.empty() && _attributeValueMap.find(attrValue) == _attributeValueMap.end())
					_attributeValueMap[attrValue] = _attributeValueCounter++;
			}
			// dzieci w wezle
			auto firstChild = node->first_node();
			if (firstChild != NULL)
				allMarkupsToHashMap(firstChild);
		}
	}

	void saveMaps(std::string & output)
	{
		saveMap(MARKUP_NAMES_ID, _markupNameMap, output);
		saveMap(MARKUP_VALUES_ID, _markupValueMap, output);
		saveMap(ATTRIBUTE_NAMES_ID, _attributeNameMap, output);
		saveMap(ATTRIBUTE_VALUES_ID, _attributeValueMap, output);
		output += '\n';
		output += std::to_string(MAPS_END) + '\n';
	}

	void saveMap(int id, InputHashMap const & map, std::string & output)
	{
		output += std::to_string(id) + '\n';
		for (InputHashMap::const_iterator it = map.begin(); it != map.end(); ++it)
		{
			output += std::to_string(it->second);
			output += ' ';
			output += ':';
			output += ' ';
			output += std::to_string(it->first.size());
			output += ' ';
			output += ':';
			output += ' ';
			output += it->first;
			output += '\n';
		}
	}

	void saveMaps(std::ofstream & file)
	{
		saveMap(MARKUP_NAMES_ID, _markupNameMap, file);
		saveMap(MARKUP_VALUES_ID, _markupValueMap, file);
		saveMap(ATTRIBUTE_NAMES_ID, _attributeNameMap, file);
		saveMap(ATTRIBUTE_VALUES_ID, _attributeValueMap, file);
		file.write((char*)&MAPS_END, sizeof(int));
	}

	void saveMap(int id, InputHashMap const & map, std::ofstream & file)
	{
		size_t size;
		file.write((char*)&id, sizeof(int));
		for (InputHashMap::const_iterator it = map.begin(); it != map.end(); ++it)
		{
			file.write((char*)&it->second, sizeof(int));
			size = it->first.size();
			file.write((char*)&size, sizeof(size_t));
			file.write((char*)it->first.c_str(), size + 1);
		}
	}

	void readMaps(std::ifstream & file)
	{
		int * id = new int;
		file.read((char*)id, sizeof(int));
		int id2 = *id;
		delete id;
		if (id2 != MARKUP_NAMES_ID)
			return;
		readMap(file, _markupNameMap2, MARKUP_VALUES_ID);
		readMap(file, _markupValueMap2, ATTRIBUTE_NAMES_ID);
		readMap(file, _attributeNameMap2, ATTRIBUTE_VALUES_ID);
		readMap(file, _attributeValueMap2, MAPS_END);
	}

	void readMap(std::ifstream & file, OutputHashMap & map, int endSign)
	{
		int * id = new int;
		size_t * size = new size_t;
		char * value;
		// odczytanie wartosci do mapy
		file.read((char*)id, sizeof(int));
		do
		{
			file.read((char*)size, sizeof(size_t));
			value = new char[*size + 1];
			file.read(value, *size + 1);
			map[*id] = value;
			delete[] value;
			file.read((char*)id, sizeof(int));
		} while (*id != endSign);
		delete id, delete size;
	}

	void saveXml(xml_node<>* firstNode, std::ofstream & file)
	{
		for (xml_node<>* node = firstNode; node; node = node->next_sibling())
		{
			// zapis nazwy wezla
			std::string nodeName = node->name();
			if (!nodeName.empty())
			{
				int nodeId = _markupNameMap[nodeName];
				file.write((char*)&nodeId, sizeof(int));
			}
			// zapis atrybutow wezla
			for (xml_attribute<>* atr = node->first_attribute(); atr; atr = atr->next_attribute())
			{
				int attrId = _attributeNameMap[atr->name()];
				file.write((char*)&attrId, sizeof(int));
				int attrValueId = _attributeValueMap[atr->value()];
				file.write((char*)&attrValueId, sizeof(int));
			}
			// zapis wartosci wezla
			auto value = node->value();
			bool hasValue = value != NULL && strlen(value) != 0;
			if (hasValue)
			{
				int valueId = _markupValueMap[value];
				file.write((char*)&valueId, sizeof(int));
			}
			// zapis dzieci wezla
			auto firstChild = node->first_node();
			bool hasChildren = firstChild != NULL && strlen(firstChild->name()) != 0;
			if (hasChildren)
			{
				file.write((char*)&NODE_HEAD_END_SIGN, sizeof(int));
				saveXml(firstChild, file);
			}
			// zapis znaku konca wezla
			file.write((char*)&NODE_END_SIGN, sizeof(int));
		}
	}

	void readXml(std::ifstream & file, std::string & xml, int tabulators)
	{
		int * id = new int;
		int * value = new int;
		do
		{
			file.read((char*)id, sizeof(int));
			if (*id == NODE_END_SIGN)
			{
				std::string name = _lastOpenedNodes.top();
				_lastOpenedNodes.pop();
				for (int i = 0; i < tabulators - 1; ++i)
					xml += '\t';
				xml += "</" + name + ">\n";
				break;
			}
			std::string nodeName = _markupNameMap2[*id];
			for (int i = 0; i < tabulators; ++i)
				xml += '\t';
			xml += '<' + nodeName;
			file.read((char*)id, sizeof(int));
			if (*id == NODE_END_SIGN)
			{
				xml += "/>\n";
			}
			else if (*id == NODE_HEAD_END_SIGN)
			{
				xml += ">\n";
				_lastOpenedNodes.push(nodeName);
				readXml(file, xml, tabulators + 1);
			}
			else
			{
				std::streampos oldpos = file.tellg();
				file.read((char*)value, sizeof(int));
				if (*value == NODE_END_SIGN)
				{
					std::string nodeValue = _markupValueMap2[*id];
					xml += '>' + nodeValue + "</" + nodeName + ">\n";
				}
				else
				{
					file.seekg(oldpos);
					do
					{
						file.read((char*)value, sizeof(int));
						std::string attrName = _attributeNameMap2[*id];
						std::string attrValue = _attributeValueMap2[*value];
						xml += ' ' + attrName + "=\"" + attrValue + "\"";
						file.read((char*)id, sizeof(int));
					} while (*id != NODE_END_SIGN && *id != NODE_HEAD_END_SIGN);
					if (*id == NODE_END_SIGN)
					{
						xml += "/>\n";
					}
					else
					{
						xml += ">\n";
						_lastOpenedNodes.push(nodeName);
						readXml(file, xml, tabulators + 1);
					}
				}
			}
			if (_lastOpenedNodes.empty())
				break;
		} while (true);
		delete id, value;
	}
};