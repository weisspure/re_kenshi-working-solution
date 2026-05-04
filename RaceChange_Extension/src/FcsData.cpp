#include "FcsData.h"

#include "Logging.h"

Ogre::vector<GameDataReference>::type* FindReferences(GameData* data, const std::string& key)
{
	if (data == 0)
		return 0;

	auto it = data->objectReferences.find(key);
	if (it == data->objectReferences.end() || it->second.empty())
		return 0;

	return &it->second;
}

std::string GetFcsStringField(GameData* data, const std::string& key)
{
	if (data == 0)
		return "";

	auto it = data->sdata.find(key);
	if (it == data->sdata.end())
		return "";

	return it->second;
}

std::string DescribeFcsObjectReferenceKeys(GameData* data)
{
	if (data == 0)
		return "lineData=null";

	if (data->objectReferences.empty())
		return "none";

	std::string result;
	for (auto it = data->objectReferences.begin(); it != data->objectReferences.end(); ++it)
	{
		if (!result.empty())
			result += "; ";

		result += it->first + "[" + IntToString((int)it->second.size()) + "]";

		if (!it->second.empty())
		{
			const GameDataReference& first = it->second[0];
			result += " firstSid=\"" + first.sid + "\"";
			result += " firstVal0=" + IntToString(first.values[0]);
			result += " firstPtr={" + DescribeGameData(first.ptr) + "}";
		}
	}

	return result;
}

std::string DescribeFirstFcsReference(GameData* data, const std::string& key)
{
	if (data == 0)
		return "data=null";

	auto it = data->objectReferences.find(key);
	if (it == data->objectReferences.end())
		return "missing";

	if (it->second.empty())
		return "empty";

	const GameDataReference& first = it->second[0];
	return "sid=\"" + first.sid + "\"" +
		   " | values=(" + IntToString(first.values[0]) + "," + IntToString(first.values[1]) + "," + IntToString(first.values[2]) + ")" +
		   " | ptr={" + DescribeGameData(first.ptr) + "}";
}

