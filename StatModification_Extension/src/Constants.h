#pragma once

/**
 * @brief Custom item type IDs used by this plugin's FCS data items.
 *        These must match the item type values defined in fcs.def.
 */
enum ItemTypeExtended
{
	STAT_DEFINITION = 3000,	   ///< FCS item type for stat definition records
	CLAMP_PROFILE = 3001,	   ///< FCS item type for reusable clamp policy records
	ADJUST_SKILL_LEVEL = 3002, ///< FCS item type for train/untrain actions
	SET_SKILL_LEVEL = 3003	   ///< FCS item type for train/untrain-until actions
};

/**
 * @brief Custom dialogue condition IDs used by this plugin.
 */
enum ExtendedDialogConditionEnum
{
	DC_STAT_LEVEL_COMPARE_UNMODIFIED = 3004, ///< Compares two characters' base stat values
	DC_STAT_LEVEL_COMPARE_MODIFIED = 3005	 ///< Compares two characters' effective stat values
};

// FCS objectReferences keys for dialogue actions. These must match fcs.def exactly.
static const char *ACTION_TRAIN_SKILL_LEVELS = "train skill levels";
static const char *ACTION_UNTRAIN_SKILL_LEVELS = "untrain skill levels";
static const char *ACTION_TRAIN_OTHER_SKILL_LEVELS = "train other skill levels";
static const char *ACTION_UNTRAIN_OTHER_SKILL_LEVELS = "untrain other skill levels";
static const char *ACTION_TRAIN_SQUAD_SKILL_LEVELS = "train squad skill levels";
static const char *ACTION_UNTRAIN_SQUAD_SKILL_LEVELS = "untrain squad skill levels";
static const char *ACTION_TRAIN_OTHER_SQUAD_SKILL_LEVELS = "train other squad skill levels";
static const char *ACTION_UNTRAIN_OTHER_SQUAD_SKILL_LEVELS = "untrain other squad skill levels";

static const char *ACTION_TRAIN_SKILL_LEVELS_UNTIL = "train skill levels until";
static const char *ACTION_TRAIN_OTHER_SKILL_LEVELS_UNTIL = "train other skill levels until";
static const char *ACTION_UNTRAIN_SKILL_LEVELS_UNTIL = "untrain skill levels until";
static const char *ACTION_UNTRAIN_OTHER_SKILL_LEVELS_UNTIL = "untrain other skill levels until";
static const char *ACTION_TRAIN_SQUAD_SKILL_LEVELS_UNTIL = "train squad skill levels until";
static const char *ACTION_UNTRAIN_SQUAD_SKILL_LEVELS_UNTIL = "untrain squad skill levels until";
static const char *ACTION_TRAIN_OTHER_SQUAD_SKILL_LEVELS_UNTIL = "train other squad skill levels until";
static const char *ACTION_UNTRAIN_OTHER_SQUAD_SKILL_LEVELS_UNTIL = "untrain other squad skill levels until";

// FCS field and objectReferences keys. These must match fcs.def / FCS condition data.
static const char *REF_STAT = "stat";
static const char *REF_CLAMP_PROFILE = "clamp profile";
static const char *REF_CONDITIONS = "conditions";
static const char *FIELD_ENUM_VALUE = "enum value";
static const char *FIELD_CLAMP_MIN = "clamp min";
static const char *FIELD_CLAMP_MAX = "clamp max";
static const char *FIELD_CONDITION_NAME = "condition name";
static const char *FIELD_COMPARE_BY = "compare by";
static const char *FIELD_WHO = "who";
static const char *FIELD_TAG = "tag";

static const char *STAT_ACTION_KEYS[] = {
	ACTION_TRAIN_SKILL_LEVELS,
	ACTION_UNTRAIN_SKILL_LEVELS,
	ACTION_TRAIN_OTHER_SKILL_LEVELS,
	ACTION_UNTRAIN_OTHER_SKILL_LEVELS,
	ACTION_TRAIN_SQUAD_SKILL_LEVELS,
	ACTION_UNTRAIN_SQUAD_SKILL_LEVELS,
	ACTION_TRAIN_OTHER_SQUAD_SKILL_LEVELS,
	ACTION_UNTRAIN_OTHER_SQUAD_SKILL_LEVELS,
	ACTION_TRAIN_SKILL_LEVELS_UNTIL,
	ACTION_TRAIN_OTHER_SKILL_LEVELS_UNTIL,
	ACTION_UNTRAIN_SKILL_LEVELS_UNTIL,
	ACTION_UNTRAIN_OTHER_SKILL_LEVELS_UNTIL,
	ACTION_TRAIN_SQUAD_SKILL_LEVELS_UNTIL,
	ACTION_UNTRAIN_SQUAD_SKILL_LEVELS_UNTIL,
	ACTION_TRAIN_OTHER_SQUAD_SKILL_LEVELS_UNTIL,
	ACTION_UNTRAIN_OTHER_SQUAD_SKILL_LEVELS_UNTIL};

static const int STAT_ACTION_KEY_COUNT = 16;
