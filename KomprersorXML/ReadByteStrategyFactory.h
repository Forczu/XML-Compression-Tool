#pragma once
#include <map>
#include "ReadStrategyEnum.h"

class AbstractReadByteStrategy;

/// <summary>
/// Fabryka tworząca nowe obiekty klas zarządzających odczytywaniem pliku binarnego
/// </summary>
class ReadByteStrategyFactory
{
	typedef AbstractReadByteStrategy * (*CreateStrategyCallback)();
	typedef std::map<ReadStrategy, CreateStrategyCallback> CallbackMap;

	static ReadByteStrategyFactory * _pInstance;

	CallbackMap _callbackMap;

	ReadByteStrategyFactory();

	static void destroy();

public:
	static ReadByteStrategyFactory & Instance();

	bool registerStrategy(ReadStrategy name, CreateStrategyCallback callback);

	bool unregister(ReadStrategy name);

	AbstractReadByteStrategy * create(ReadStrategy name);
};