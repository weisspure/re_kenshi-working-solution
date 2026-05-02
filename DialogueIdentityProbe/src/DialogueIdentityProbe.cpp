#include <Debug.h>
#include <core/Functions.h>

#include <kenshi/Character.h>
#include <kenshi/Dialogue.h>
#include <kenshi/GameData.h>
#include <kenshi/RaceData.h>

#include <sstream>
#include <string>

static const char* PROBE_ACTION_KEY = "SM_PROBE_DIALOGUE_IDENTITY";
static const char* REF_CONDITIONS = "conditions";

static void (*_doActions_orig)(Dialogue* thisptr, DialogLineData* dialogLine) = 0;
static bool (*checkTags_orig)(DialogLineData* thisptr, Character* me, Character* target) = 0;

static std::string IntToString(int value)
{
	std::ostringstream ss;
	ss << value;
	return ss.str();
}

static std::string PtrToString(const void* ptr)
{
	std::ostringstream ss;
	ss << ptr;
	return ss.str();
}

static void LogInfo(const std::string& message)
{
	DebugLog("DialogueIdentityProbe: " + message);
}

static void LogError(const std::string& message)
{
	ErrorLog("DialogueIdentityProbe: " + message);
}

static const char* TalkerName(TalkerEnum value)
{
	switch (value)
	{
	case T_ME:
		return "T_ME";
	case T_TARGET:
		return "T_TARGET";
	case T_TARGET_IF_PLAYER:
		return "T_TARGET_IF_PLAYER";
	case T_INTERJECTOR1:
		return "T_INTERJECTOR1";
	case T_INTERJECTOR2:
		return "T_INTERJECTOR2";
	case T_INTERJECTOR3:
		return "T_INTERJECTOR3";
	case T_WHOLE_SQUAD:
		return "T_WHOLE_SQUAD";
	case T_TARGET_WITH_RACE:
		return "T_TARGET_WITH_RACE";
	}

	return "T_UNKNOWN";
}

static const char* EventName(EventTriggerEnum value)
{
	switch (value)
	{
	case EV_PLAYER_TALK_TO_ME:
		return "EV_PLAYER_TALK_TO_ME";
	case EV_I_SEE_NEUTRAL_SQUAD:
		return "EV_I_SEE_NEUTRAL_SQUAD";
	case EV_NONE:
		return "EV_NONE";
	default:
		return "EV_OTHER";
	}
}

static const char* CompareName(ComparisonEnum value)
{
	switch (value)
	{
	case CE_EQUALS:
		return "CE_EQUALS";
	case CE_LESS_THAN:
		return "CE_LESS_THAN";
	case CE_MORE_THAN:
		return "CE_MORE_THAN";
	}

	return "CE_UNKNOWN";
}

static std::string DescribeGameData(GameData* data)
{
	if (data == 0)
		return "null";

	return "name=\"" + data->name +
		"\" | stringID=\"" + data->stringID +
		"\" | id=" + IntToString(data->id) +
		" | type=" + IntToString((int)data->type);
}

static std::string BoolField(GameData* data, const std::string& key)
{
	if (data == 0)
		return "unavailable";

	auto it = data->bdata.find(key);
	if (it == data->bdata.end())
		return "missing";

	return it->second ? "1" : "0";
}

static std::string DescribeRace(RaceData* race)
{
	if (race == 0)
		return "race unavailable";

	return DescribeGameData(race->data);
}

static std::string DescribeCharacter(Character* character)
{
	if (character == 0)
		return "null";

	return "ptr=" + PtrToString(character) +
		" | name=\"" + character->getName() +
		"\" | data={" + DescribeGameData(character->getGameData()) +
		"} | race={" + DescribeRace(character->getRace()) + "}";
}

static std::string ProbeId(GameData* lineData)
{
	if (lineData == 0)
		return "null";
	if (!lineData->stringID.empty())
		return lineData->stringID;
	return IntToString(lineData->id);
}

static std::string CleanLogText(const std::string& value)
{
	std::string out = value;
	for (size_t i = 0; i < out.size(); ++i)
	{
		if (out[i] == '\r' || out[i] == '\n' || out[i] == '\t')
			out[i] = ' ';
	}
	return out;
}

static std::string FirstLineText(DialogLineData* line)
{
	if (line == 0 || line->texts == 0 || line->lineCount <= 0)
		return "";

	return CleanLogText(line->texts[0]);
}

static Ogre::vector<GameDataReference>::type* FindReferences(GameData* data, const std::string& key)
{
	if (data == 0)
		return 0;

	auto it = data->objectReferences.find(key);
	if (it == data->objectReferences.end() || it->second.empty())
		return 0;

	return &it->second;
}

static bool HasProbeMarker(GameData* lineData)
{
	return FindReferences(lineData, PROBE_ACTION_KEY) != 0;
}

static std::string LinePrefix(const char* phase, DialogLineData* line)
{
	GameData* data = line == 0 ? 0 : line->getGameData();
	return std::string("phase=") + phase +
		" | probeId=" + ProbeId(data) +
		" | lineName=\"" + (data == 0 ? std::string("null") : data->name) +
		"\" | text0=\"" + FirstLineText(line) +
		"\" | line={" + DescribeGameData(data) + "}";
}

static void LogReferenceList(const char* phase, const char* listName, Ogre::vector<GameDataReference>::type* refs)
{
	if (refs == 0)
	{
		LogInfo(std::string("phase=") + phase + " | list=" + listName + " | count=0");
		return;
	}

	LogInfo(std::string("phase=") + phase + " | list=" + listName + " | count=" + IntToString((int)refs->size()));
	for (int i = 0; i < (int)refs->size(); ++i)
	{
		const GameDataReference& ref = (*refs)[i];
		LogInfo(std::string("phase=") + phase +
			" | list=" + listName +
			" | index=" + IntToString(i) +
			" | sid=\"" + ref.sid +
			"\" | values=(" + IntToString(ref.values[0]) +
			"," + IntToString(ref.values[1]) +
			"," + IntToString(ref.values[2]) +
			") | ptr={" + DescribeGameData(ref.ptr) + "}");
	}
}

static void LogGameDataLektor(const char* phase, const char* listName, const lektor<GameData*>& items)
{
	LogInfo(std::string("phase=") + phase + " | list=" + listName + " | count=" + IntToString((int)items.size()));
	for (int i = 0; i < (int)items.size(); ++i)
	{
		LogInfo(std::string("phase=") + phase +
			" | list=" + listName +
			" | index=" + IntToString(i) +
			" | data={" + DescribeGameData(items[i]) + "}");
	}
}

static void LogLineLists(DialogLineData* line)
{
	if (line == 0)
		return;

	LogGameDataLektor("lineList", "isTargetRace/runtime target race", line->isTargetRace);
	LogGameDataLektor("lineList", "isTargetSubRace_specificallyTheTarget", line->isTargetSubRace_specificallyTheTarget);
	LogGameDataLektor("lineList", "isMyRace", line->isMyRace);
	LogGameDataLektor("lineList", "isMySubRace", line->isMySubRace);
}

static Character* ProbeSpeaker(Dialogue* dlg, DialogLineData* line, TalkerEnum who)
{
	if (dlg == 0 || line == 0)
		return 0;

	return dlg->getSpeaker(who, line, false);
}

static void LogSpeakerProbe(Dialogue* dlg, DialogLineData* line, TalkerEnum who)
{
	Character* result = ProbeSpeaker(dlg, line, who);
	LogInfo(LinePrefix("speakerProbe", line) +
		" | requested=" + TalkerName(who) +
		" | requestedInt=" + IntToString((int)who) +
		" | result={" + DescribeCharacter(result) + "}");
}

static void LogDialogueContext(Dialogue* dlg, DialogLineData* line)
{
	if (dlg == 0 || line == 0)
	{
		LogInfo("phase=context | dlg or dialogLine is null");
		return;
	}

	Character* conversationTarget = dlg->getConversationTarget().getCharacter();
	GameData* currentConversationData = dlg->currentConversation == 0 ? 0 : dlg->currentConversation->getGameData();
	LogInfo(LinePrefix("context", line) +
		" | event=" + EventName(dlg->currentConversationType) +
		" | eventInt=" + IntToString((int)dlg->currentConversationType) +
		" | parentForEnemies=" + BoolField(currentConversationData, "for enemies") +
		" | parentMonologue=" + BoolField(currentConversationData, "monologue") +
		" | parentLocked=" + BoolField(currentConversationData, "locked") +
		" | parentOneAtATime=" + BoolField(currentConversationData, "one at a time") +
		" | lineSpeakerEnum=" + IntToString((int)line->speaker) +
		" | lineSpeakerName=" + TalkerName(line->speaker) +
		" | getSpeaker(lineSpeaker)={" + DescribeCharacter(ProbeSpeaker(dlg, line, line->speaker)) +
		"} | conversationTarget={" + DescribeCharacter(conversationTarget) +
		"} | dlgMe={" + DescribeCharacter(dlg->me) +
		"} | currentConversation={" + DescribeGameData(currentConversationData) +
		"} | currentLine={" + DescribeGameData(dlg->currentLine == 0 ? 0 : dlg->currentLine->getGameData()) + "}");
}

static void LogAllDialogueProbes(Dialogue* dlg, DialogLineData* line)
{
	LogDialogueContext(dlg, line);
	LogLineLists(line);

	LogSpeakerProbe(dlg, line, T_ME);
	LogSpeakerProbe(dlg, line, T_TARGET);
	LogSpeakerProbe(dlg, line, T_TARGET_IF_PLAYER);
	LogSpeakerProbe(dlg, line, T_INTERJECTOR1);
	LogSpeakerProbe(dlg, line, T_WHOLE_SQUAD);
	LogSpeakerProbe(dlg, line, T_TARGET_WITH_RACE);
}

static void LogConditionRows(DialogLineData* line)
{
	if (line == 0)
		return;

	GameData* lineData = line->getGameData();
	Ogre::vector<GameDataReference>::type* refs = FindReferences(lineData, REF_CONDITIONS);
	LogReferenceList("conditionReference", REF_CONDITIONS, refs);

	for (int i = 0; i < (int)line->conditions.size(); ++i)
	{
		DialogLineData::DialogCondition* cond = line->conditions[i];
		if (cond == 0)
		{
			LogInfo(LinePrefix("conditionRow", line) + " | index=" + IntToString(i) + " | runtimeCondition=null");
			continue;
		}

		LogInfo(LinePrefix("conditionRow", line) +
			" | index=" + IntToString(i) +
			" | condition=" + IntToString((int)cond->key) +
			" | compare=" + CompareName(cond->compareBy) +
			" | compareInt=" + IntToString((int)cond->compareBy) +
			" | who=" + TalkerName(cond->who) +
			" | whoInt=" + IntToString((int)cond->who) +
			" | value=" + IntToString(cond->value));
	}
}

static void _doActions_hook(Dialogue* thisptr, DialogLineData* dialogLine)
{
	GameData* lineData = dialogLine == 0 ? 0 : dialogLine->getGameData();
	if (HasProbeMarker(lineData))
	{
		LogInfo(LinePrefix("doActions", dialogLine) + " | marker=" + PROBE_ACTION_KEY);
		LogAllDialogueProbes(thisptr, dialogLine);
	}

	if (_doActions_orig == 0)
	{
		LogError("phase=doActions | _doActions_orig is null");
		return;
	}

	_doActions_orig(thisptr, dialogLine);
}

static bool checkTags_hook(DialogLineData* thisptr, Character* me, Character* target)
{
	GameData* lineData = thisptr == 0 ? 0 : thisptr->getGameData();
	bool shouldLog = HasProbeMarker(lineData);

	if (shouldLog)
	{
		LogInfo(LinePrefix("checkTags", thisptr) +
			" | me={" + DescribeCharacter(me) +
			"} | target={" + DescribeCharacter(target) + "}");
		LogLineLists(thisptr);
		LogConditionRows(thisptr);
	}

	if (checkTags_orig == 0)
	{
		LogError("phase=checkTags | checkTags_orig is null");
		return false;
	}

	bool result = checkTags_orig(thisptr, me, target);

	if (shouldLog)
	{
		LogInfo(LinePrefix("originalResult", thisptr) +
			" | originalResult=" + IntToString(result ? 1 : 0) +
			" | me={" + DescribeCharacter(me) +
			"} | target={" + DescribeCharacter(target) + "}");
	}

	return result;
}

__declspec(dllexport) void startPlugin()
{
	if (KenshiLib::SUCCESS != KenshiLib::AddHook(
		KenshiLib::GetRealAddress(&Dialogue::_doActions),
		&_doActions_hook,
		&_doActions_orig))
	{
		LogError("phase=startup | failed to hook Dialogue::_doActions");
		return;
	}

	if (KenshiLib::SUCCESS != KenshiLib::AddHook(
		KenshiLib::GetRealAddress(&DialogLineData::checkTags),
		&checkTags_hook,
		&checkTags_orig))
	{
		LogError("phase=startup | failed to hook DialogLineData::checkTags");
		return;
	}

	LogInfo("phase=startup | loaded and hooks installed");
}
