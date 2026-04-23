#include <Debug.h>
#include <core/Functions.h>

#include <kenshi/Dialogue.h>
#include <kenshi/Character.h>
#include <kenshi/CharStats.h>
#include <kenshi/GameData.h>
#include <kenshi/Enums.h>
#include <kenshi/Faction.h>

#include <string>
#include <sstream>

// ========================================
// CONFIGURATION
// ========================================
#define VERBOSE_LOGGING true     // Set to false to disable verbose exploration logs
#define LOG_EVERY_HOOK false     // Set to true to log every _doActions call (very spammy!)

void (*_doActions_orig)(Dialogue* thisptr, DialogLineData* dialogLine);

// Helper to conditionally log
static void VerboseLog(const std::string& message)
{
    if (VERBOSE_LOGGING)
        ErrorLog(message);  // Use ErrorLog to ensure it appears
}

static std::string ToStringInt(int value)
{
    std::ostringstream ss;
    ss << value;
    return ss.str();
}

static std::string ToStringFloat(float value)
{
    std::ostringstream ss;
    ss << value;
    return ss.str();
}

static std::string PtrToString(void* ptr)
{
    if (ptr == 0)
        return "null";

    std::ostringstream ss;
    ss << "0x" << std::hex << reinterpret_cast<unsigned long long>(ptr);
    return ss.str();
}

static std::string SafeGetStringOrEmpty(const std::string& str)
{
    return str.empty() ? "(empty)" : str;
}

static void LogCharacterSummary(const std::string& label, Character* character)
{
    VerboseLog("=== Character Summary: " + label + " ===");

    if (character == 0)
    {
        VerboseLog("  Character ptr: null");
        return;
    }

    VerboseLog("  Character ptr: " + PtrToString(character));

    CharStats* stats = character->getStats();
    VerboseLog("  Stats ptr: " + PtrToString(stats));

    if (stats != 0)
    {
        VerboseLog("  STAT_STRENGTH: " + ToStringFloat(stats->getStat(STAT_STRENGTH, false)));
        VerboseLog("  STAT_ATHLETICS: " + ToStringFloat(stats->getStat(STAT_ATHLETICS, false)));
        VerboseLog("  STAT_MELEE_ATTACK: " + ToStringFloat(stats->getStat(STAT_MELEE_ATTACK, false)));
        VerboseLog("  STAT_TOUGHNESS: " + ToStringFloat(stats->getStat(STAT_TOUGHNESS, false)));
    }

    VerboseLog("  isPlayerCharacter: " + std::string(character->isPlayerCharacter() ? "YES" : "NO"));

    VerboseLog("  Race ptr: " + PtrToString(character->getRace()));

    Faction* faction = character->getFaction();
    if (faction != 0)
    {
        VerboseLog("  Faction ptr: " + PtrToString(faction));
    }
    else
    {
        VerboseLog("  Faction: null");
    }

    ActivePlatoon* platoon = character->getPlatoon();
    if (platoon != 0)
    {
        VerboseLog("  Platoon ptr: " + PtrToString(platoon));
    }
    else
    {
        VerboseLog("  Platoon: null");
    }
}

static void LogGameDataSummary(const std::string& label, GameData* data)
{
    VerboseLog("  --- GameData: " + label + " ---");

    if (data == 0)
    {
        VerboseLog("    ptr: null");
        return;
    }

    VerboseLog("    ptr: " + PtrToString(data));
    VerboseLog("    type: " + ToStringInt((int)data->type));
    VerboseLog("    stringID: " + SafeGetStringOrEmpty(data->stringID));
    VerboseLog("    name: " + SafeGetStringOrEmpty(data->name));

    VerboseLog("    sdata entries: " + ToStringInt((int)data->sdata.size()));
    for (ogre_unordered_map<std::string, std::string>::type::iterator it = data->sdata.begin();
         it != data->sdata.end(); ++it)
    {
        VerboseLog("      sdata[" + it->first + "] = " + SafeGetStringOrEmpty(it->second));
    }

    VerboseLog("    idata entries: " + ToStringInt((int)data->idata.size()));
    for (ogre_unordered_map<std::string, int>::type::iterator it = data->idata.begin();
         it != data->idata.end(); ++it)
    {
        VerboseLog("      idata[" + it->first + "] = " + ToStringInt(it->second));
    }

    VerboseLog("    fdata entries: " + ToStringInt((int)data->fdata.size()));
    for (ogre_unordered_map<std::string, float>::type::iterator it = data->fdata.begin();
         it != data->fdata.end(); ++it)
    {
        VerboseLog("      fdata[" + it->first + "] = " + ToStringFloat(it->second));
    }

    VerboseLog("    bdata entries: " + ToStringInt((int)data->bdata.size()));
    for (ogre_unordered_map<std::string, bool>::type::iterator it = data->bdata.begin();
         it != data->bdata.end(); ++it)
    {
        VerboseLog("      bdata[" + it->first + "] = " + std::string(it->second ? "true" : "false"));
    }
}

static void LogGameDataReference(const std::string& label, int index, GameDataReference& ref)
{
    VerboseLog("  [" + ToStringInt(index) + "] " + label);
    VerboseLog("    sid: " + SafeGetStringOrEmpty(ref.sid));
    VerboseLog("    ptr: " + PtrToString(ref.ptr));
    VerboseLog("    values[0]: " + ToStringInt(ref.values[0]));
    VerboseLog("    values[1]: " + ToStringInt(ref.values[1]));
    VerboseLog("    values[2]: " + ToStringInt(ref.values[2]));

    if (ref.ptr != 0)
    {
        LogGameDataSummary("ref.ptr", ref.ptr);
    }
}

static void LogObjectReferences(GameData* lineData)
{
    if (lineData == 0)
    {
        VerboseLog("LogObjectReferences: lineData is null");
        return;
    }

    VerboseLog("=== objectReferences Map ===");
    VerboseLog("Total keys: " + ToStringInt((int)lineData->objectReferences.size()));

    int maxEntriesPerKey = 5;

    for (ogre_unordered_map<std::string, Ogre::vector<GameDataReference>::type>::type::iterator iter =
         lineData->objectReferences.begin();
         iter != lineData->objectReferences.end();
         ++iter)
    {
        VerboseLog("Key: [" + iter->first + "]");
        VerboseLog("  Vector size: " + ToStringInt((int)iter->second.size()));

        int entryCount = 0;
        for (Ogre::vector<GameDataReference>::type::iterator refIter = iter->second.begin();
             refIter != iter->second.end();
             ++refIter)
        {
            if (entryCount >= maxEntriesPerKey)
            {
                VerboseLog("  ... (truncated, " + ToStringInt((int)iter->second.size()) + " total entries)");
                break;
            }

            LogGameDataReference("Entry", entryCount, *refIter);
            entryCount++;
        }
    }
}

static StatsEnumerated SkillFromToken(const std::string& token)
{
    if (token == "STAT_STRENGTH")      return STAT_STRENGTH;
    if (token == "STAT_ATHLETICS")     return STAT_ATHLETICS;
    if (token == "STAT_DEXTERITY")     return STAT_DEXTERITY;
    if (token == "STAT_TOUGHNESS")     return STAT_TOUGHNESS;
    if (token == "STAT_MELEE_ATTACK")  return STAT_MELEE_ATTACK;
    if (token == "STAT_MELEE_DEFENCE") return STAT_MELEE_DEFENCE;
    if (token == "STAT_THIEVING")      return STAT_THIEVING;
    if (token == "STAT_STEALTH")       return STAT_STEALTH;
    if (token == "STAT_ASSASSINATION") return STAT_ASSASSINATION;
    if (token == "STAT_LABOURING")     return STAT_LABOURING;
    if (token == "STAT_SCIENCE")       return STAT_SCIENCE;
    if (token == "STAT_ENGINEERING")   return STAT_ENGINEERING;
    if (token == "STAT_ROBOTICS")      return STAT_ROBOTICS;
    if (token == "STAT_TURRETS")       return STAT_TURRETS;
    if (token == "STAT_FARMING")       return STAT_FARMING;
    if (token == "STAT_COOKING")       return STAT_COOKING;

    return STAT_NONE;
}

static Character* ResolveLearner(Dialogue* dlg)
{
    if (dlg == 0)
        return 0;

    Character* meChar = dlg->me;
    Character* targetChar = 0;

    if (dlg->getConversationTarget().getCharacter() != 0)
        targetChar = dlg->getConversationTarget().getCharacter();

    DebugLog("=== Learner Resolution ===");
    LogCharacterSummary("dlg->me", meChar);
    LogCharacterSummary("conversationTarget", targetChar);

    if (targetChar != 0)
    {
        DebugLog("DECISION: using conversation target as learner");
        return targetChar;
    }

    DebugLog("DECISION: using dlg->me as learner (fallback)");
    return meChar;
}

static std::string ResolveTokenFromReference(GameDataReference& ref)
{
    VerboseLog("=== Token Resolution ===");

    if (ref.ptr == 0)
    {
        VerboseLog("Token source: FAILED - ref.ptr is null");
        return "";
    }

    ogre_unordered_map<std::string, std::string>::type::iterator tokenIter =
        ref.ptr->sdata.find("stat token");

    if (tokenIter != ref.ptr->sdata.end() && !tokenIter->second.empty())
    {
        VerboseLog("Token source: custom field 'stat token'");
        VerboseLog("Token value: " + tokenIter->second);
        return tokenIter->second;
    }

    VerboseLog("Token source: stringID (fallback)");
    VerboseLog("Token value: " + SafeGetStringOrEmpty(ref.ptr->stringID));
    return ref.ptr->stringID;
}

static void ApplyGrantSkillLevels(Dialogue* dlg, DialogLineData* dialogLine)
{
    if (dlg == 0 || dialogLine == 0)
        return;

    if (dialogLine->getGameData() == 0)
        return;

    ogre_unordered_map<std::string, Ogre::vector<GameDataReference>::type>::type::iterator iter =
        dialogLine->getGameData()->objectReferences.find("grant skill levels");

    if (iter == dialogLine->getGameData()->objectReferences.end())
        return;

    DebugLog("========================================");
    DebugLog("=== APPLYING SKILL BOOST ===");

    Character* learner = ResolveLearner(dlg);

    if (learner == 0)
    {
        ErrorLog("SkillIncrease: learner was null");
        return;
    }

    CharStats* stats = learner->getStats();

    if (stats == 0)
    {
        ErrorLog("SkillIncrease: learner->getStats() was null");
        return;
    }

    Ogre::vector<GameDataReference>::type& refs = iter->second;

    if (refs.empty())
    {
        ErrorLog("SkillIncrease: grant skill levels exists but has no references");
        return;
    }

    DebugLog("Processing " + ToStringInt((int)refs.size()) + " skill boost references");

    for (Ogre::vector<GameDataReference>::type::iterator it = refs.begin(); it != refs.end(); ++it)
    {
        if (it->ptr == 0)
        {
            ErrorLog("SkillIncrease: grant skill levels ref ptr was null");
            continue;
        }

        std::string token = ResolveTokenFromReference(*it);
        StatsEnumerated stat = SkillFromToken(token);

        DebugLog("Resolved token '" + token + "' to enum " + ToStringInt((int)stat));

        if (stat == STAT_NONE)
        {
            ErrorLog("SkillIncrease: unsupported skill token: " + token);
            continue;
        }

        int levels = it->values[0];
        if (levels == 0)
            levels = 10;

        float& current = stats->getStatRef(stat);
        float before = current;
        current += (float)levels;
        float after = current;

        DebugLog(
            "APPLIED: " + token +
            " | before=" + ToStringFloat(before) +
            " | add=" + ToStringInt(levels) +
            " | after=" + ToStringFloat(after)
        );
    }

    DebugLog("=== SKILL BOOST COMPLETE ===");
    DebugLog("========================================");
}

void _doActions_hook(Dialogue* thisptr, DialogLineData* dialogLine)
{
    ErrorLog("########################################");
    ErrorLog("### _doActions_hook FIRED ###");

    ErrorLog("=== A. Hook Entry / Line Identity ===");
    ErrorLog("Dialogue* thisptr: " + PtrToString(thisptr));
    ErrorLog("DialogLineData* dialogLine: " + PtrToString(dialogLine));

    if (dialogLine != 0)
    {
        GameData* lineData = dialogLine->getGameData();
        DebugLog("dialogLine->getGameData(): " + PtrToString(lineData));

        if (lineData != 0)
        {
            DebugLog("Line stringID: " + SafeGetStringOrEmpty(lineData->stringID));
            DebugLog("Line name: " + SafeGetStringOrEmpty(lineData->name));
            DebugLog("Line type: " + ToStringInt((int)lineData->type));
        }

        DebugLog("dialogLine->speaker: " + ToStringInt((int)dialogLine->speaker));
        DebugLog("dialogLine->chancePermanent: " + ToStringFloat(dialogLine->chancePermanent));
        DebugLog("dialogLine->chanceTemporary: " + ToStringFloat(dialogLine->chanceTemporary));
        DebugLog("dialogLine->score: " + ToStringInt(dialogLine->score));
        DebugLog("dialogLine->unique: " + std::string(dialogLine->unique ? "true" : "false"));
        DebugLog("dialogLine->onceOnly: " + std::string(dialogLine->onceOnly ? "true" : "false"));

        DebugLog("=== B. Dialogue Actor/Context ===");
        if (thisptr != 0)
        {
            LogCharacterSummary("thisptr->me", thisptr->me);

            Character* convTarget = thisptr->getConversationTarget().getCharacter();
            LogCharacterSummary("conversationTarget", convTarget);
        }
        else
        {
            DebugLog("thisptr is null, cannot log actors");
        }

        DebugLog("=== C. Dialogue Line Metadata ===");
        if (lineData != 0)
        {
            DebugLog("Conditions count: " + ToStringInt((int)dialogLine->conditions.size()));
            DebugLog("Actions count: " + ToStringInt((int)dialogLine->actions.size()));

            DebugLog("=== D. objectReferences Deep Dump ===");
            LogObjectReferences(lineData);
        }
    }
    else
    {
        DebugLog("dialogLine is null");
    }

    DebugLog("=== E. Custom Action Path ===");
    ApplyGrantSkillLevels(thisptr, dialogLine);

    DebugLog("### Calling original _doActions ###");
    _doActions_orig(thisptr, dialogLine);
    DebugLog("### _doActions_hook COMPLETE ###");
    DebugLog("########################################");
}

__declspec(dllexport) void startPlugin()
{
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
        KenshiLib::GetRealAddress(&Dialogue::_doActions),
        &_doActions_hook,
        &_doActions_orig))
    {
        ErrorLog("SkillIncrease: could not hook Dialogue::_doActions");
        return;
    }

    DebugLog("SkillIncrease loaded and dialogue hook installed");
}