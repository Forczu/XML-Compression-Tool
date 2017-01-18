#include "ReadByteStrategyFactory.h"

ReadByteStrategyFactory * ReadByteStrategyFactory::_pInstance = 0;

ReadByteStrategyFactory::ReadByteStrategyFactory()
{
}

void ReadByteStrategyFactory::destroy()
{
	delete _pInstance;
}

ReadByteStrategyFactory & ReadByteStrategyFactory::Instance()
{
	static bool __initialized = false;
	if (!__initialized)
	{
		_pInstance = new ReadByteStrategyFactory();
		atexit(destroy);
		__initialized = true;
	}
	return *_pInstance;
}

bool ReadByteStrategyFactory::registerStrategy(ReadStrategy name, CreateStrategyCallback callback)
{
	std::pair<CallbackMap::iterator, bool> ret;
	ret = _callbackMap.insert(std::pair<ReadStrategy, CreateStrategyCallback>(name, callback));
	return ret.second;
}

bool ReadByteStrategyFactory::unregister(ReadStrategy name)
{
	return _callbackMap.erase(name) == 1;
}

AbstractReadByteStrategy * ReadByteStrategyFactory::create(ReadStrategy name)
{
	CallbackMap::const_iterator it = _callbackMap.find(name);
	AbstractReadByteStrategy * result = (*it).second();
	return result;
}