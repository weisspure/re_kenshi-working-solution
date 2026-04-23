using FCS_extended;
using System.Reflection;

namespace SkillIncrease_FCS
{
    public class SkillIncreasePlugin : IPlugin
    {
        public int Init(Assembly assembly)
        {
            System.Console.WriteLine("SkillIncrease FCS plugin loaded.");
            return 0;
        }
    }
}