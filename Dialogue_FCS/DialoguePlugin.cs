using FCS_extended;
using HarmonyLib;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Reflection.Emit;
using System.Windows.Forms;
using static System.Windows.Forms.ListViewItem;

namespace Dialogue_FCS
{	
	enum DialogConditionEnum_extended
	{
		DC_IS_SLEEPING = 1000,
		DC_HAS_SHORT_TERM_TAG,
		DC_IS_ALLY_BECAUSE_OF_DISGUISE
	}

	public class DialoguePlugin : IPlugin
	{
		public int Init(Assembly assembly)
		{
			Harmony harmony = new Harmony("Dialogue_FCS");
			harmony.PatchAll();

			Console.WriteLine("Dialogue plugin loaded.");
			return 0;
		}

		// patch to enable the condition tag box/add default condition tag value
		[HarmonyPatch("forgotten_construction_set.dialog.ConditionControl", "createDefaults")]
		public static class ConditionControl_createDefaults_Patch
		{
			[HarmonyPostfix]
			static void Postfix()
			{

				object conditionDefaults = Traverse.Create(AccessTools.TypeByName("forgotten_construction_set.dialog.ConditionControl")).Field("conditionDefaults").GetValue();
				
				Type condDefType = typeof(Dictionary<,>).MakeGenericType(new Type[] { AccessTools.TypeByName("forgotten_construction_set.DialogConditionEnum"), typeof(object) });
				MethodInfo method = condDefType.Method("Add", new Type[] { AccessTools.TypeByName("forgotten_construction_set.DialogConditionEnum"), typeof(object) });
				condDefType.Method("Add", new Type[] { AccessTools.TypeByName("forgotten_construction_set.DialogConditionEnum"), typeof(object) })
					.Invoke(conditionDefaults, new object[]{
					Enum.ToObject(AccessTools.TypeByName("forgotten_construction_set.DialogConditionEnum"),(int)DialogConditionEnum_extended.DC_HAS_SHORT_TERM_TAG),
					(AccessTools.TypeByName("forgotten_construction_set.CharacterPerceptionTags_ShortTerm").GetMember("ST_NONE").First() as FieldInfo).GetValue(null) });
			}
		}
	}
}
