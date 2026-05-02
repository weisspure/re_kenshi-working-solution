#include "FcsData.h"

#include "Constants.h"
#include "Logging.h"

/** @brief Reads an integer field from GameData::idata with a safe fallback. */
int GetIntField(GameData* data, const std::string& key, int fallback)
{
	if (data == 0)
		return fallback;

	// auto hides the engine-specific iterator typedef while preserving exact behavior.
	auto it = data->idata.find(key);
	if (it != data->idata.end())
		return it->second;

	return fallback;
}

/** @brief Returns a non-empty GameData reference list by FCS key, or null. */
Ogre::vector<GameDataReference>::type* FindReferences(GameData* data, const std::string& key)
{
	if (data == 0)
		return 0;

	auto it = data->objectReferences.find(key);
	if (it == data->objectReferences.end() || it->second.empty())
		return 0;

	return &it->second;
}

/**
 * @brief Resolves the STAT_DEFINITION record linked from an action record.
 *
 * This is the core "FCS picker, not free text" stat resolution path.
 */
GameData* GetStatDefinitionRecord(GameData* actionRecord, const std::string& context)
{
	if (actionRecord == 0)
		return 0;

	Ogre::vector<GameDataReference>::type* refs = FindReferences(actionRecord, REF_STAT);
	if (refs == 0)
		return 0;

	GameData* ptr = (*refs)[0].ptr;
	if (ptr == 0)
		return 0;

	if ((int)ptr->type != STAT_DEFINITION)
	{
		LogError(
			"wrong item type for stat reference"
			" | context=" +
			context +
			" | expected=" +
			IntToString(STAT_DEFINITION) +
			" | got=" + IntToString((int)ptr->type));
		return 0;
	}

	return ptr;
}

/**
 * @brief Reads the StatsEnumerated value from a STAT_DEFINITION record.
 *
 * This plugin intentionally supports third-party/custom stat enum values by
 * casting the authored integer directly. Runtime rejects only STAT_NONE later.
 */
StatsEnumerated ReadStatEnum(GameData* statDef)
{
	if (statDef == 0)
		return STAT_NONE;

	return (StatsEnumerated)GetIntField(statDef, FIELD_ENUM_VALUE, (int)STAT_NONE);
}
