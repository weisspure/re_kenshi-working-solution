using FCS_extended;
using HarmonyLib;
using System;
using System.Collections.Generic;
using System.Reflection;

namespace StatModification_FCS
{
    /**
     * Keep these IDs in sync with StatModification_Extension/src/Constants.h
     * and StatModification_Extension/StatModification_Extension/fcs.def.
     */
    enum StatModificationDialogConditionEnum
    {
        DC_STAT_LEVEL_COMPARE_UNMODIFIED = 3004,
        DC_STAT_LEVEL_COMPARE_MODIFIED = 3005,
        DC_STAT_LEVEL_UNMODIFIED = 3006,
        DC_STAT_LEVEL_MODIFIED = 3007,
    }

    /**
     * FCS renders the condition tag picker from the enum type of the default
     * value registered in ConditionControl.conditionDefaults.
     *
     * These integer values are still Kenshi StatsEnumerated values at runtime,
     * but this editor-facing enum intentionally exposes only the skills this
     * mod publishes as supported authoring targets.
     */
    enum StatModificationSkillEnum
    {
        STAT_STRENGTH = 1,
        STAT_MELEE_ATTACK = 2,
        STAT_LABOURING = 3,
        STAT_SCIENCE = 4,
        STAT_ENGINEERING = 5,
        STAT_ROBOTICS = 6,
        STAT_SMITHING_WEAPON = 7,
        STAT_SMITHING_ARMOUR = 8,
        STAT_MEDIC = 9,
        STAT_THIEVING = 10,
        STAT_TURRETS = 11,
        STAT_FARMING = 12,
        STAT_COOKING = 13,
        STAT_STEALTH = 16,
        STAT_ATHLETICS = 17,
        STAT_DEXTERITY = 18,
        STAT_MELEE_DEFENCE = 19,
        STAT_TOUGHNESS = 21,
        STAT_ASSASSINATION = 22,
        STAT_SWIMMING = 23,
        STAT_PERCEPTION = 24,
        STAT_KATANAS = 25,
        STAT_SABRES = 26,
        STAT_HACKERS = 27,
        STAT_HEAVY_WEAPONS = 28,
        STAT_BLUNT = 29,
        STAT_MARTIAL_ARTS = 30,
        STAT_DODGE = 32,
        STAT_POLEARMS = 34,
        STAT_CROSSBOWS = 35,
        STAT_FRIENDLY_FIRE = 36,
        STAT_LOCKPICKING = 37,
        STAT_SMITHING_BOW = 38,
    }

    public class StatModificationFcsPlugin : IPlugin
    {
        public int Init(Assembly assembly)
        {
            Harmony harmony = new Harmony("StatModification_FCS");
            harmony.PatchAll();

            Console.WriteLine("StatModification FCS plugin loaded.");
            return 0;
        }

        /**
         * Teach FCS that our condition tag field is a stat enum.
         *
         * Without this editor helper, the runtime still works, but authors see
         * a plain integer field and must manually type stat IDs. Registering a
         * StatModificationSkillEnum value makes FCS render our narrower picker
         * instead of the full built-in StatsEnumerated dropdown.
         */
        [HarmonyPatch("forgotten_construction_set.dialog.ConditionControl", "createDefaults")]
        public static class ConditionControlCreateDefaultsPatch
        {
            [HarmonyPostfix]
            static void Postfix()
            {
                object conditionDefaults = Traverse
                    .Create(AccessTools.TypeByName("forgotten_construction_set.dialog.ConditionControl"))
                    .Field("conditionDefaults")
                    .GetValue();

                Type dialogConditionEnumType = AccessTools.TypeByName("forgotten_construction_set.DialogConditionEnum");
                if (dialogConditionEnumType == null)
                {
                    Console.WriteLine("StatModification FCS plugin could not find DialogConditionEnum.");
                    return;
                }

                Type dictionaryType = typeof(Dictionary<,>).MakeGenericType(dialogConditionEnumType, typeof(object));
                MethodInfo addMethod = dictionaryType.GetMethod("Add", new Type[] { dialogConditionEnumType, typeof(object) });
                MethodInfo containsKeyMethod = dictionaryType.GetMethod("ContainsKey", new Type[] { dialogConditionEnumType });
                if (addMethod == null || containsKeyMethod == null)
                {
                    Console.WriteLine("StatModification FCS plugin could not access condition defaults methods.");
                    return;
                }

                object defaultStat = StatModificationSkillEnum.STAT_STRENGTH;

                // Add-only registration lets compatibility helpers that load
                // before us keep their combined/custom condition tag enum.
                AddStatsTagDefault(conditionDefaults, dialogConditionEnumType, addMethod, containsKeyMethod, defaultStat, StatModificationDialogConditionEnum.DC_STAT_LEVEL_COMPARE_UNMODIFIED);
                AddStatsTagDefault(conditionDefaults, dialogConditionEnumType, addMethod, containsKeyMethod, defaultStat, StatModificationDialogConditionEnum.DC_STAT_LEVEL_COMPARE_MODIFIED);

                // Reserved for the planned self-contained threshold conditions.
                // Harmless if these IDs are not present in fcs.def yet; useful once added.
                AddStatsTagDefault(conditionDefaults, dialogConditionEnumType, addMethod, containsKeyMethod, defaultStat, StatModificationDialogConditionEnum.DC_STAT_LEVEL_UNMODIFIED);
                AddStatsTagDefault(conditionDefaults, dialogConditionEnumType, addMethod, containsKeyMethod, defaultStat, StatModificationDialogConditionEnum.DC_STAT_LEVEL_MODIFIED);
            }

            static void AddStatsTagDefault(
                object conditionDefaults,
                Type dialogConditionEnumType,
                MethodInfo addMethod,
                MethodInfo containsKeyMethod,
                object defaultStat,
                StatModificationDialogConditionEnum condition)
            {
                object conditionKey = Enum.ToObject(dialogConditionEnumType, (int)condition);
                bool alreadyRegistered = (bool)containsKeyMethod.Invoke(conditionDefaults, new object[] { conditionKey });
                if (alreadyRegistered)
                    return;

                addMethod.Invoke(conditionDefaults, new object[] { conditionKey, defaultStat });
            }
        }
    }
}
