#pragma once
#include "plugin.h"
#include "CLEO.h"
#include "CLEO_Utils.h"
#include <unordered_set>
#include <unordered_map>


using namespace CLEO;
using namespace plugin;

class TextureManager
{
public:
	struct DictInfo
	{
		std::string name;
		RwTexDictionary* txd;
		std::unordered_set<CLEO::CRunningScript*> references;
	};

	TextureManager() = default;
	~TextureManager() = default;
	void Clear();
	void ScriptUnregister(CLEO::CRunningScript* script);

	DictInfo* GetDictionary(CLEO::CRunningScript* script, const char* filename);
	DictInfo* GetCurrDictionary(CLEO::CRunningScript* script) const;

	void FreeDictionaries(CLEO::CRunningScript* script);
	void FreeDictionary(CLEO::CRunningScript* script, const char* filename);

private:
	std::unordered_map<std::string, DictInfo> m_dicts; // loaded dictionaries
	std::unordered_map<CLEO::CRunningScript*, DictInfo*> m_currDict; // script's most recently loaded dictionary

	static void ResolveDictionaryFilename(CLEO::CRunningScript* script, const char* filename, std::string& outFilepath, std::string& name);

	static CLEO::CRunningScript* GetRefGroup(CLEO::CRunningScript* script);
};
