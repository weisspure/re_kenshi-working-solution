using FCS_extended;
using HarmonyLib;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;

namespace WorldStates_FCS
{
	public class WorldStatesPlugin : IPlugin
	{
		private enum itemType_extended { VARIABLE=1000 };

		public int Init(Assembly assembly)
		{

			Console.WriteLine("TESAT");
			Harmony harmony = new Harmony("WorldStates_FCS");

			harmony.PatchAll();
			/*
			MethodInfo enumToStringPrefix = typeof(WorldStatesPlugin).GetMethod("Enum_ToString_Prefix")
				;//.MakeGenericMethod(AccessTools.TypeByName("forgotten_construction_set.itemType"));
			Console.WriteLine(enumToStringPrefix);


			MethodInfo Enum_ToString = typeof(Enum).Method("ToString");
			Console.WriteLine(Enum_ToString);

			harmony.Patch(Enum_ToString, new HarmonyMethod(enumToStringPrefix));
			*/
			Console.WriteLine("WorldStates plugin loaded.");
			return 0;
		}

		// patch to allow merging of FCS layout sections
		[HarmonyPatch("forgotten_construction_set.navigation", "clearCategories")]
		public static class navigation_clearCategories_Patch
		{
			[HarmonyPrefix]
			static bool Prefix()
			{
				// drop ClearCategories calls so layout sections are merged together
				return false;
			}
		}
		
		// generic so we can set the correct arg type without statically linking to the FCS, which breaks simultaneous Steam + GOG compatibility
		public static bool Enum_TryParse_itemType<T>(string value, out T result) where T : struct
		{
			// we have to set the value regardless so always do it
			result = (T)Enum.ToObject(AccessTools.TypeByName("forgotten_construction_set.itemType"), 1000);
			Console.WriteLine(value);
			if (value == "VARIABLE")
				return true;

			// use standard implementation via reflection
			MethodInfo Enum_TryParse = typeof(Enum).GetMethods(BindingFlags.Public | BindingFlags.Static)
						.Where(method => method.Name == "TryParse" && method.GetParameters().Length == 2).First()
						.MakeGenericMethod(AccessTools.TypeByName("forgotten_construction_set.itemType"));

			return Enum.TryParse(value, out result);//(bool)Enum_TryParse.Invoke(null, new object[] { value, result });
		}

		// generics in Harmony are completely fucked so we patch the caller IL to redirect calls instead of hooking the called function
		[HarmonyPatch("forgotten_construction_set.Definitions", "ParseLayout")]
		public static class Definitions_ParseLayout_Patch
		{
			static IEnumerable<CodeInstruction> Transpiler(IEnumerable<CodeInstruction> instructions)
			{
				MethodInfo targetMethod = typeof(Enum).GetMethods(BindingFlags.Public | BindingFlags.Static)
							.Where(method => method.Name == "TryParse" && method.GetParameters().Length == 2).First()
							.MakeGenericMethod(AccessTools.TypeByName("forgotten_construction_set.itemType"));

				foreach (var instruction in instructions)
					if (instruction.Calls(targetMethod))
						instruction.operand = typeof(WorldStatesPlugin).GetMethod("Enum_TryParse_itemType")
							.MakeGenericMethod(AccessTools.TypeByName("forgotten_construction_set.itemType"));

				return instructions;
			}
		}
		
		// generics in Harmony are completely fucked so we patch the caller IL to redirect calls instead of hooking the called function
		[HarmonyPatch("forgotten_construction_set.Definitions", "ParseItem")]
		public static class Definitions_ParseItem_Patch
		{
			static IEnumerable<CodeInstruction> Transpiler(IEnumerable<CodeInstruction> instructions)
			{
				MethodInfo targetMethod = typeof(Enum).GetMethods(BindingFlags.Public | BindingFlags.Static)
							.Where(method => method.Name == "TryParse" && method.GetParameters().Length == 2).First()
							.MakeGenericMethod(AccessTools.TypeByName("forgotten_construction_set.itemType"));

				foreach (var instruction in instructions)
					if (instruction.Calls(targetMethod))
						instruction.operand = typeof(WorldStatesPlugin).GetMethod("Enum_TryParse_itemType")
							.MakeGenericMethod(AccessTools.TypeByName("forgotten_construction_set.itemType"));

				return instructions;
			}
		}

		// generics in Harmony are completely fucked so we patch the caller IL to redirect calls instead of hooking the called function
		[HarmonyPatch("forgotten_construction_set.Definitions", "Load")]
		public static class Definitions_Load_Patch
		{
			static IEnumerable<CodeInstruction> Transpiler(IEnumerable<CodeInstruction> instructions)
			{
				MethodInfo targetMethod = typeof(Enum).GetMethods(BindingFlags.Public | BindingFlags.Static)
							.Where(method => method.Name == "TryParse" && method.GetParameters().Length == 2).First()
							.MakeGenericMethod(AccessTools.TypeByName("forgotten_construction_set.itemType"));

				foreach (var instruction in instructions)
					if (instruction.Calls(targetMethod))
						instruction.operand = typeof(WorldStatesPlugin).GetMethod("Enum_TryParse_itemType")
							.MakeGenericMethod(AccessTools.TypeByName("forgotten_construction_set.itemType"));

				return instructions;
			}
		}

		// make enum name show up correctly in UI
		[HarmonyPatch(typeof(Enum), "ToString", new Type[] { })]
		public static class itemType_ToString_Patch
		{
			[HarmonyPrefix]
			static bool Prefix(Enum __instance, ref string __result)
			{
				if (__instance.GetType() == AccessTools.TypeByName("forgotten_construction_set.itemType")
					&& (int)(object)__instance == (int)itemType_extended.VARIABLE)
				{
					// override value
					__result = "VARIABLE";
					return false;
				}
				return true;
			}
		}
		
	}
}
