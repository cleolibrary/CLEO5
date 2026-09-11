workspace "CLEO5"
    configurations { "Release", "Debug" }
    architecture "x86"
    toolset "v143"
    language "C++"
    cppdialect "C++latest"
    staticruntime "On"
    multiprocessorcompile "On"
    rtti "Off"
    intrinsics "On"
    functionlevellinking "On"
    warnings "Default"
    fatalwarnings { "All" }

    defines {
        "NOMINMAX",
        "RW",
        "GTASA"
    }

    buildoptions {
        "/Zc:threadSafeInit-"
    }

    linkoptions {
        "/SAFESEH:NO"
    }

    filter "configurations:Debug"
        defines { "_DEBUG" }
        symbols "On"
        optimize "Off"

    filter "configurations:Release"
        defines { "_NDEBUG" }
        optimize "Speed"
        linktimeoptimization "On"
        symbols "Off"

    filter {}

-- =========================================================================
-- Common Include Directories
-- =========================================================================
local PLUGIN_SDK_INCLUDES = {
    "third-party/plugin-sdk/plugin_sa",
    "third-party/plugin-sdk/plugin_sa/game_sa",
    "third-party/plugin-sdk/plugin_sa/game_sa/rw",
    "third-party/plugin-sdk/plugin_sa/game_sa/enums",
    "third-party/plugin-sdk/shared",
    "third-party/plugin-sdk/shared/game"
}

-- =========================================================================
-- CLEO Core Project
-- =========================================================================
project "CLEO"
    kind "SharedLib"
    targetname "CLEO"
    targetextension ".asi"
    characterset "Unicode"

    targetdir ".output/%{cfg.buildcfg}"
    objdir ".output/.obj/%{cfg.buildcfg}/CLEO"

    includedirs {
        "source",
        "cleo_sdk"
    }

    externalincludedirs {
        PLUGIN_SDK_INCLUDES,
        "third-party/simdjson/singleheader",
        "third-party/simpleini"
    }

    defines {
        'TARGET_NAME=R"(CLEO)"'
    }

    pchheader "stdafx.h"
    pchsource "source/stdafx.cpp"

    files {
        "source/**.h",
        "source/**.cpp",
        "cleo_sdk/**.h",
        "source/CLEO5.rc",
        "source/cleo.def",
        "source/cleo_config.ini",
        "third-party/simdjson/singleheader/simdjson.h",
        "third-party/simdjson/singleheader/simdjson.cpp",
        "third-party/simpleini/SimpleIni.h",
        -- Plugin SDK sources
        "third-party/plugin-sdk/plugin_sa/game_sa/CFont.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/RenderWare.cpp",
        "third-party/plugin-sdk/shared/game/CRGBA.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CGame.cpp",
        "third-party/plugin-sdk/shared/DynAddress.cpp",
        "third-party/plugin-sdk/shared/GameVersion.cpp",
        "third-party/plugin-sdk/shared/Patch.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CTheScripts.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CTimer.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CMenuManager.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CSprite2d.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CRunningScript.cpp",
        "third-party/plugin-sdk/shared/extensions/Screen.cpp"
    }

    filter "files:source/crc32.cpp or files:source/OpcodeInfoDatabase.cpp or files:third-party/**"
        enablepch "Off"

    filter "files:third-party/**"
        warnings "Off"

    filter {}

    postbuildcommands {
        '{COPY} "%{cfg.targetdir}/CLEO.lib" "%{wks.location}/cleo_sdk/"',
        'if defined GTA_SA_DIR ( taskkill /IM gta_sa.exe /F /FI "STATUS eq RUNNING" & xcopy /Y "%{cfg.targetdir}/CLEO.asi" "%GTA_SA_DIR%\\" )'
    }

-- =========================================================================
-- CLEO Plugins
-- =========================================================================
group "cleo_plugins"

local function define_cleo_plugin(config)
    local name = config.name
    local target_name = "SA." .. name

    project(name)
        kind "SharedLib"
        targetname(target_name)
        targetextension ".cleo"
        characterset "MBCS"

        targetdir "cleo_plugins/.output"
        objdir("cleo_plugins/.output/.obj/%{cfg.buildcfg}/" .. name)

        dependson { "CLEO" }

        includedirs {
            "cleo_sdk",
            config.extra_includedirs or {}
        }

        externalincludedirs {
            PLUGIN_SDK_INCLUDES,
            config.extra_externalincludedirs or {}
        }

        resdefines {
            'TARGET_NAME=' .. target_name .. '.cleo'
        }

        filter "files:**.c or files:**.cpp"
            defines {
                'TARGET_NAME=R"(' .. target_name .. ')"'
            }

        filter {}

        libdirs {
            "cleo_sdk",
            ".output/%{cfg.buildcfg}",
            config.extra_libdirs or {}
        }

        links {
            "CLEO",
            config.extra_links or {}
        }

        files {
            "cleo_plugins/Resource.rc",
            "cleo_plugins/" .. name .. "/**.h",
            "cleo_plugins/" .. name .. "/**.cpp",
            config.extra_files or {}
        }

        filter "files:third-party/**"
            warnings "Off"

        filter {}

        postbuildcommands {
            'if defined GTA_SA_DIR ( taskkill /IM gta_sa.exe /F /FI "STATUS eq RUNNING" & xcopy /Y "%{cfg.targetdir}/' .. target_name .. '.cleo" "%GTA_SA_DIR%\\cleo\\cleo_plugins\\" )'
        }
end

-- 1. Audio
define_cleo_plugin {
    name = "Audio",
    extra_externalincludedirs = { "cleo_plugins/Audio/bass" },
    extra_libdirs = { "cleo_plugins/Audio/bass" },
    extra_links = { "bass.lib" },
    extra_files = {
        "cleo_plugins/Audio/bass/bass.h",
        "third-party/plugin-sdk/plugin_sa/game_sa/CPools.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CTheScripts.cpp",
        "third-party/plugin-sdk/shared/DynAddress.cpp",
        "third-party/plugin-sdk/shared/GameVersion.cpp",
        "third-party/plugin-sdk/shared/game/CRGBA.cpp",
        "third-party/plugin-sdk/shared/Patch.cpp",
        "third-party/plugin-sdk/shared/PluginBase.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CCamera.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CTimer.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/RenderWare.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CAEAudioHardware.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CPad.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CPlaceable.cpp"
    }
}

-- 2. DebugUtils
define_cleo_plugin {
    name = "DebugUtils",
    extra_includedirs = {
        "source"
    },
    extra_externalincludedirs = {
        "third-party/simdjson/singleheader"
    },
    extra_files = {
        "source/crc32.cpp",
        "source/OpcodeInfoDatabase.cpp",
        "third-party/simdjson/singleheader/simdjson.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CCheat.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CFont.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CGame.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CHud.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CMenuManager.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CRunningScript.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CSprite2d.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CTheScripts.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/RenderWare.cpp",
        "third-party/plugin-sdk/shared/DynAddress.cpp",
        "third-party/plugin-sdk/shared/GameVersion.cpp",
        "third-party/plugin-sdk/shared/game/CRGBA.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CTimer.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CMessages.cpp",
        "third-party/plugin-sdk/shared/Patch.cpp"
    }
}

-- 3. FileSystemOperations
define_cleo_plugin {
    name = "FileSystemOperations"
}

-- 4. GameEntities
define_cleo_plugin {
    name = "GameEntities",
    extra_files = {
        "third-party/plugin-sdk/plugin_sa/game_sa/CPools.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CMenuManager.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CRadar.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CSprite2d.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CWorld.cpp",
        "third-party/plugin-sdk/shared/game/CRGBA.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/common.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CBaseModelInfo.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CCheat.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CModelInfo.cpp",
        "third-party/plugin-sdk/shared/DynAddress.cpp",
        "third-party/plugin-sdk/shared/GameVersion.cpp",
        "third-party/plugin-sdk/shared/Patch.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CAESound.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CAEWeaponAudioEntity.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CPed.cpp"
    }
}

-- 5. IniFiles
define_cleo_plugin {
    name = "IniFiles"
}

-- 6. Input
define_cleo_plugin {
    name = "Input",
    extra_files = {
        "third-party/plugin-sdk/plugin_sa/game_sa/CCheat.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CControllerConfigManager.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CMessages.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CText.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CTimer.cpp",
        "third-party/plugin-sdk/shared/DynAddress.cpp",
        "third-party/plugin-sdk/shared/GameVersion.cpp",
        "third-party/plugin-sdk/shared/Patch.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/RenderWare.cpp"
    }
}

-- 7. Math
define_cleo_plugin {
    name = "Math"
}

-- 8. MemoryOperations
define_cleo_plugin {
    name = "MemoryOperations",
    extra_files = {
        "third-party/plugin-sdk/plugin_sa/game_sa/CPools.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CTheScripts.cpp",
        "third-party/plugin-sdk/shared/DynAddress.cpp",
        "third-party/plugin-sdk/shared/GameVersion.cpp",
        "third-party/plugin-sdk/shared/game/CRGBA.cpp",
        "third-party/plugin-sdk/shared/Patch.cpp",
        "third-party/plugin-sdk/shared/PluginBase.cpp"
    }
}

-- 9. Text
define_cleo_plugin {
    name = "Text",
    extra_links = { "Shlwapi.lib" },
    extra_files = {
        "third-party/plugin-sdk/plugin_sa/game_sa/CMenuManager.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CMissionCleanup.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CRect.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CGame.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CHud.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CMessages.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CModelInfo.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CBaseModelInfo.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CSprite2d.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CTxdStore.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/RenderWare.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CText.cpp",
        "third-party/plugin-sdk/shared/DynAddress.cpp",
        "third-party/plugin-sdk/shared/GameVersion.cpp",
        "third-party/plugin-sdk/shared/Patch.cpp",
        "third-party/plugin-sdk/plugin_sa/game_sa/CTheScripts.cpp",
        "third-party/plugin-sdk/shared/game/CRGBA.cpp"
    }
}
