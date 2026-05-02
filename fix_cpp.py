p = r"C:\Git\KenshiLib_Examples\StatModification_Extension\StatModification_Extension.cpp"
t = open(p, encoding="utf-8").read()

old_enum = (
    ' * @brief Extended dialogue condition type IDs for stat checks.\n'
    ' *\n'
    ' * DC_STAT_UNMODIFIED checks the character\'s raw base stat value,\n'
    ' * ignoring any equipment or modifier bonuses. Use this for trainer\n'
    ' * checks ("does this NPC actually know this skill?").\n'
    ' *\n'
    ' * DC_STAT_MODIFIED checks the effective stat value including all bonuses.\n'
    ' *\n'
    ' * These must match the values in your fcs.def DialogConditionEnum extension.\n'
    ' */\n'
    'enum ExtendedDialogConditionEnum\n'
    '{\n'
    '\tDC_STAT_UNMODIFIED = 3004,\t\t   ///< Checks base stat value against a fixed threshold\n'
    '\tDC_STAT_MODIFIED = 3005,\t\t   ///< Checks effective stat value against a fixed threshold\n'
    '\tDC_STAT_COMPARE_UNMODIFIED = 3006, ///< Compares two characters\' base stat values\n'
    '\tDC_STAT_COMPARE_MODIFIED = 3007\t   ///< Compares two characters\' effective stat values\n'
    '};'
)
new_enum = (
    ' * @brief Condition IDs for two-character stat comparisons.\n'
    ' *\n'
    ' * Compares the same stat on owner vs target rather than one character vs a\n'
    ' * fixed threshold. Single-value threshold checks are provided by BFrizzle\'s\n'
    ' * Dialogue plugin (BFrizz Extra Extensions) and are not duplicated here.\n'
    ' *\n'
    ' * Pass 2: add T_WHOLE_SQUAD threshold checks at 3008/3009.\n'
    ' * Use GetModuleHandle("BFrizz_Extra_Extensions.dll") in startPlugin to\n'
    ' * warn when Dialogue is absent.\n'
    ' */\n'
    'enum ExtendedDialogConditionEnum\n'
    '{\n'
    '\tDC_STAT_COMPARE_UNMODIFIED = 3006, ///< Compares two characters\' base stat values\n'
    '\tDC_STAT_COMPARE_MODIFIED   = 3007  ///< Compares two characters\' effective stat values\n'
    '};'
)
assert old_enum in t, "ENUM NOT FOUND"; t = t.replace(old_enum, new_enum)

import re
t = re.sub(
    r'/\*\*\n \* @brief Evaluates a single stat condition against a character\.\n.*?^static bool EvaluateStatCondition\(.*?^\}\n\n',
    '', t, flags=re.DOTALL | re.MULTILINE
)

old_scan = (
    '\t\tif (key == DC_STAT_UNMODIFIED || key == DC_STAT_MODIFIED ||\n'
    '\t\t\tkey == DC_STAT_COMPARE_UNMODIFIED || key == DC_STAT_COMPARE_MODIFIED)'
)
new_scan = '\t\tif (key == DC_STAT_COMPARE_UNMODIFIED || key == DC_STAT_COMPARE_MODIFIED)'
assert old_scan in t, "SCAN NOT FOUND"; t = t.replace(old_scan, new_scan)

old_branch = (
    '\t\t\t\t\tbool isSingleCheck = (conditionInt == DC_STAT_UNMODIFIED ||\n'
    '\t\t\t\t\t\tconditionInt == DC_STAT_MODIFIED);\n'
    '\t\t\t\t\tbool isComparison = (conditionInt == DC_STAT_COMPARE_UNMODIFIED ||\n'
    '\t\t\t\t\t\tconditionInt == DC_STAT_COMPARE_MODIFIED);\n'
    '\n'
    '\t\t\t\t\tif (!isSingleCheck && !isComparison)\n'
    '\t\t\t\t\t\tcontinue; // Different plugin\'s condition, skip\n'
    '\n'
    '\t\t\t\t\tExtendedDialogConditionEnum conditionType =\n'
    '\t\t\t\t\t\t(ExtendedDialogConditionEnum)conditionInt;\n'
    '\t\t\t\t\tComparisonEnum compareBy = (ComparisonEnum)compareByIt->second;\n'
    '\t\t\t\t\tTalkerEnum who = (TalkerEnum)whoIt->second;\n'
    '\t\t\t\t\tStatsEnumerated stat = (StatsEnumerated)tagIt->second;\n'
    '\n'
    '\t\t\t\t\t// who controls the left-hand side:\n'
    '\t\t\t\t\t//   T_ME    = owner is left,  target is right\n'
    '\t\t\t\t\t//   other   = target is left, owner is right\n'
    '\t\t\t\t\tCharacter* left = (who == T_ME) ? me : target;\n'
    '\t\t\t\t\tCharacter* right = (who == T_ME) ? target : me;\n'
    '\n'
    '\t\t\t\t\tif (isSingleCheck)\n'
    '\t\t\t\t\t{\n'
    '\t\t\t\t\t\tint threshold = iter->second[i].values[0];\n'
    '\t\t\t\t\t\tif (!EvaluateStatCondition(conditionType, left, compareBy, stat, threshold))\n'
    '\t\t\t\t\t\t\treturn false;\n'
    '\t\t\t\t\t}\n'
    '\t\t\t\t\telse\n'
    '\t\t\t\t\t{\n'
    '\t\t\t\t\t\tif (!EvaluateStatComparison(conditionType, left, right, compareBy, stat))\n'
    '\t\t\t\t\t\t\treturn false;\n'
    '\t\t\t\t\t}'
)
new_branch = (
    '\t\t\t\t\tif (conditionInt != DC_STAT_COMPARE_UNMODIFIED &&\n'
    '\t\t\t\t\t    conditionInt != DC_STAT_COMPARE_MODIFIED)\n'
    '\t\t\t\t\t\tcontinue;\n'
    '\n'
    '\t\t\t\t\tExtendedDialogConditionEnum conditionType =\n'
    '\t\t\t\t\t\t(ExtendedDialogConditionEnum)conditionInt;\n'
    '\t\t\t\t\tComparisonEnum compareBy = (ComparisonEnum)compareByIt->second;\n'
    '\t\t\t\t\tTalkerEnum who = (TalkerEnum)whoIt->second;\n'
    '\t\t\t\t\tStatsEnumerated stat = (StatsEnumerated)tagIt->second;\n'
    '\n'
    '\t\t\t\t\tCharacter* left = (who == T_ME) ? me : target;\n'
    '\t\t\t\t\tCharacter* right = (who == T_ME) ? target : me;\n'
    '\n'
    '\t\t\t\t\tif (!EvaluateStatComparison(conditionType, left, right, compareBy, stat))\n'
    '\t\t\t\t\t\treturn false;'
)
assert old_branch in t, "BRANCH NOT FOUND"; t = t.replace(old_branch, new_branch)

open(p, 'w', encoding='utf-8').write(t)
print("Done. 3004:", "3004" in t, "| EvalStatCond:", "EvaluateStatCondition" in t, "| isSingleCheck:", "isSingleCheck" in t)
