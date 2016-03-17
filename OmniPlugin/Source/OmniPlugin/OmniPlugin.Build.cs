// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
    using System.IO;

    public class OmniPlugin : ModuleRules
    {
        private string ModulePath
        {
            get { return Path.GetDirectoryName(RulesCompiler.GetModuleFilename(this.GetType().Name)); }
        }

        private string ThirdPartyPath
        {
            get { return Path.GetFullPath(Path.Combine(ModulePath, "../../ThirdParty/")); }
        }

        private string ThirdPartyBinariesPath
        {
            get { return Path.Combine(ThirdPartyPath, "OpenHaptics", "lib"); }
        }

        public OmniPlugin(TargetInfo Target)
        {
            PublicIncludePaths.AddRange(
                new string[] {
                    "OmniPlugin/Public",
                }
                );

            PrivateIncludePaths.AddRange(
                new string[] {
                    "OmniPlugin/Private",
                }
                );

            PublicDependencyModuleNames.AddRange(
                new string[]
                {
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "InputCore",
                    "InputDevice",
                    "HeadMountedDisplay",
                    "Projects",
                    "Slate",
                    "SlateCore"
					// ... add other public dependencies that you statically link with here ...
				}
                );

            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
					// ... add private dependencies that you statically link with here ...
				}
                );

            DynamicallyLoadedModuleNames.AddRange(
                new string[]
                {
					// ... add any modules that your module loads dynamically here ...
				}
                );

            LoadOmniLib(Target);

            //Add DLL for packaging

        }

        public bool LoadOmniLib(TargetInfo Target)
        {
            bool isLibrarySupported = false;

            if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32))
            {
                isLibrarySupported = true;

                
                string PlatformString = (Target.Platform == UnrealTargetPlatform.Win64) ? "x64" : "Win32";
                string LibrariesPath = Path.Combine(ThirdPartyPath, "OpenHaptics", "Lib");

                PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, "hd.lib"));

                if (Target.Platform == UnrealTargetPlatform.Win64)
                {
                    PublicDelayLoadDLLs.Add(Path.Combine(ThirdPartyBinariesPath, "hd.dll"));
                    RuntimeDependencies.Add(new RuntimeDependency(Path.Combine(ThirdPartyBinariesPath,"hd.dll")));
                }
           /*     else
                {
                    PublicDelayLoadDLLs.Add(Path.Combine(ThirdPartyBinariesPath, "Win32", "hd.dll"));
                    RuntimeDependencies.Add(new RuntimeDependency(Path.Combine(ThirdPartyBinariesPath, "Win32", "hd.dll")));
                }   */
                
            }

            if (isLibrarySupported)
            {
                // Include path
                PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "OpenHaptics", "Include"));
            }


            return isLibrarySupported;
        }
    }

}