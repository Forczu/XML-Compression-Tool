#pragma once
#include <unordered_map>
#include <vector>
#include <fstream>
#include "rapidxml\rapidxml.hpp"
#include "rapidxml\rapidxml_print.hpp"

using namespace rapidxml;

/// <summary>
/// Kompresor plikow Xml
/// </summary>
class CompresorXml
{
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
	/// Mapa zawierajaca znaczniki i odpowiadajace im liczby id
	/// </summary>
	std::unordered_map<int, std::string> _markupMap;

	/// <summary>
	/// Pomocniczy licznik dla znacznikow przy kompresji
	/// </summary>
	int _markupCounter;

public:
	CompresorXml() : _contents(nullptr), _markupCounter(0)
	{
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
		saveCompressed(_doc.first_node());
	}

	void saveCompressed(xml_node<>* firstNode)
	{
		for (xml_node<>* node = firstNode; node; node = node->next_sibling())
		{
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
		for (xml_node<>* node = firstNode; node; node = node->next_sibling())
		{
			std::string nodeName = node->name();
			if (!nodeName.empty() && !mapHasValue(_markupMap, nodeName))
				_markupMap[_markupCounter++] = nodeName;
			auto firstChild = node->first_node();
			if (firstChild != NULL)
				allMarkupsToHashMap(firstChild);
		}
	}

	/// <summary>
	/// Sprawdza czy mapa zawiera wskazana wartosc.
	/// </summary>
	/// <param name="map">The map.</param>
	/// <param name="value">The value.</param>
	/// <returns>True, jesli wartosc istnieje w mapie, false, jesli nie</returns>
	bool mapHasValue(std::unordered_map<int, std::string> const & map, std::string const & value)
	{
		for (std::unordered_map<int, std::string>::const_iterator it = map.cbegin(); it != map.cend(); ++it)
		{
			if (it->second == value)
				return true;
		}
		return false;
	}

};