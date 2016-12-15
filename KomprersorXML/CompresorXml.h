#pragma once
#include <unordered_map>
#include <vector>
#include <fstream>
#include <string> 
#include <iostream>
#include "rapidxml\rapidxml.hpp"
#include "rapidxml\rapidxml_print.hpp"

using namespace rapidxml;

/// <summary>
/// Kompresor plikow Xml
/// </summary>
class CompresorXml
{
	static const int NODE_END_SIGN = -1;
	static const int NODE_HEAD_END_SIGN = -2;

protected:
	/// <summary>
	/// Struktura reprezentujaca oryginalny plik Xml
	/// </summary>
	xml_document <> _doc;

	/// <summary>
	/// Zwartosc pierwotnego pliku Xml w formie lancucha
	/// </summary>
	char * _contents;

	/// <summary>
	/// Mapa zawierajaca nazwy znacznikow i odpowiadajace im liczby id
	/// </summary>
	std::unordered_map<std::string, int> _markupNameMap;

	/// <summary>
	/// Mapa zawierajaca wartosci znacznikow i odpowiadajace im liczby id
	/// </summary>
	std::unordered_map<std::string, int> _markupValueMap;

	/// <summary>
	/// Mapa zawierajaca nazwy atrybutow i odpowiadajace im liczby id
	/// </summary>
	std::unordered_map<std::string, int> _attributeMap;

	/// <summary>
	/// Mapa zawierajaca wartosci atrybutow i odpowiadajace im liczby id
	/// </summary>
	std::unordered_map<std::string, int> _attributeValueMap;

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

public:
	CompresorXml() : _contents(nullptr)
	{
		_markupNameCounter = _attributeCounter = 0;
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
				std::string attrId = std::to_string(_attributeMap[atr->name()]);
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
				std::string valueId = std::to_string(_attributeValueMap[value]);
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
				if (!attrName.empty() && _attributeMap.find(attrName) == _attributeMap.end())
					_attributeMap[attrName] = _attributeCounter++;
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

};