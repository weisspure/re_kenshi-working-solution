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
				
				// AccessTools.TypeByName("forgotten_construction_set.DialogConditionEnum")
				Type condDefType = typeof(Dictionary<,>).MakeGenericType(new Type[] { AccessTools.TypeByName("forgotten_construction_set.DialogConditionEnum"), typeof(object) });
				MethodInfo method = condDefType.Method("Add", new Type[] { AccessTools.TypeByName("forgotten_construction_set.DialogConditionEnum"), typeof(object) });
				condDefType.Method("Add", new Type[] { AccessTools.TypeByName("forgotten_construction_set.DialogConditionEnum"), typeof(object) })
					.Invoke(conditionDefaults, new object[]{
					Enum.ToObject(AccessTools.TypeByName("forgotten_construction_set.DialogConditionEnum"),(int)DialogConditionEnum_extended.DC_HAS_SHORT_TERM_TAG),
					(AccessTools.TypeByName("forgotten_construction_set.CharacterPerceptionTags_ShortTerm").GetMember("ST_NONE").First() as FieldInfo).GetValue(null) });
			}
		}
		/*
		// patch to enable the condition tag box
		[HarmonyPatch("forgotten_construction_set.dialog.ConditionControl", "refreshGrid")]
		public static class ConditionControl_refreshGrid_Patch
		{
			[HarmonyPostfix]
			static void Postfix(object __instance, object dialogLine)
			{
				if (dialogLine != null)
				{
					//ListViewItem listViewItem = this.listView1conditions.Items;
					ListView listView1conditions = Traverse.Create(__instance).Field("listView1conditions").GetValue() as ListView;
					foreach(ListViewItem item in listView1conditions.Items)
					{
						if(item.SubItems.Count >= 5)
						{
							// fix enum
							IDictionary conditionDefaults = (IDictionary)Traverse.Create(AccessTools.TypeByName("forgotten_construction_set.dialog.ConditionControl")).Field("conditionDefaults").GetValue();
							object conditionEnum = Enum.Parse(AccessTools.TypeByName("forgotten_construction_set.DialogConditionEnum"), item.SubItems[1].Text);
							if(conditionDefaults.Contains(conditionEnum))
							{
								int result;
								// if the value is an int instead of an enum name, convert it to it's enum
								if(int.TryParse(item.SubItems[4].Text, out result))
								{
									item.SubItems.RemoveAt(4);

									// use type of default value for enum parsing
									object defaultVal = conditionDefaults[conditionEnum];
									item.SubItems.Add(Enum.ToObject(defaultVal.GetType(), result).ToString());
								}
							}
							else
							{
								item.SubItems.RemoveAt(4);
							}

						}
					}
				}
			}
			// this is fucked

			enum NUMBER
			{

			}
			// fixes parsing of dialogue condition tags
			[HarmonyTranspiler]
			static IEnumerable<CodeInstruction> Transpiler(IEnumerable<CodeInstruction> instructions)
			{
				Type personaltyTags = AccessTools.TypeByName("forgotten_construction_set.PersonalityTags");

				foreach (var instruction in instructions)
				{
					if (instruction.opcode == OpCodes.Constrained)
					{
						Console.WriteLine(instruction);

						if ((Type)instruction.operand == personaltyTags)
						{
							//instruction.opcode = OpCodes.Nop;
							instruction.operand = typeof(NUMBER);// typeof(object);
						}
					}
				}
				return instructions;
			}
		}
		*/
	}
}
