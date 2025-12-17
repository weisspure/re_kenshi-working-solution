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
		private enum itemType_extended { VARIABLE=900 };

		public int Init(Assembly assembly)
		{
			Harmony harmony = new Harmony("WorldStates_FCS");
			harmony.PatchAll();

			Console.WriteLine("WorldStates plugin loaded.");
			return 0;
		}
		// generic so we can set the correct arg type without statically linking to the FCS, which breaks simultaneous Steam + GOG compatibility
		public static bool Enum_TryParse_itemType<T>(string value, out T result) where T : struct
		{
			// we have to set the value regardless so always do it
			result = (T)Enum.ToObject(AccessTools.TypeByName("forgotten_construction_set.itemType"), itemType_extended.VARIABLE);

			if (value == "VARIABLE")
				return true;

			// use standard implementation via reflection
			MethodInfo Enum_TryParse = typeof(Enum).GetMethods(BindingFlags.Public | BindingFlags.Static)
						.Where(method => method.Name == "TryParse" && method.GetParameters().Length == 2).First()
						.MakeGenericMethod(AccessTools.TypeByName("forgotten_construction_set.itemType"));

			return Enum.TryParse(value, out result);
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
	}
}
