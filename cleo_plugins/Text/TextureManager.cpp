#include "TextureManager.h"
#include "CLEO_Utils.h"
#include <CFileLoader.h>

void TextureManager::Clear()
{
	for (auto entry : m_dicts)
	{
		if (entry.second.txd != nullptr) RwTexDictionaryDestroy(entry.second.txd);
	}
	m_dicts.clear();
	m_currDict.clear();
}

void TextureManager::ScriptUnregister(CLEO::CRunningScript* script)
{
	FreeDictionaries(script);
	m_currDict.erase(script);
}

TextureManager::DictInfo* TextureManager::GetDictionary(CLEO::CRunningScript* script, const char* filename)
{
	std::string filepath, name;
	ResolveDictionaryFilename(script, filename, filepath, name);

	DictInfo* dict = nullptr;
	if (m_dicts.find(name) == m_dicts.end())
	{
		// not present, load from file
		auto tex = CFileLoader::LoadTexDictionary(filepath.c_str());
		if (tex == nullptr)
		{
			LOG_WARNING(script, "Invalid texture dictionary '%s'", name.c_str());
			return nullptr;
		}

		if (tex->texturesInDict.link.next == tex->texturesInDict.link.prev)
		{
			LOG_WARNING(script, "Failed to load texture dictionary '%s'", name.c_str());
			return nullptr;
		}

		dict = &m_dicts[name];
		dict->name = name;
		dict->txd = tex;
	}
	else
	{
		dict = &m_dicts[name];
	}

	auto ref = GetRefGroup(script);
	dict->references.insert(ref);
	m_currDict[ref] = dict;
	return dict;
}

TextureManager::DictInfo* TextureManager::GetCurrDictionary(CLEO::CRunningScript* script) const
{
	auto ref = GetRefGroup(script);
	return (m_currDict.find(ref) != m_currDict.end()) ? m_currDict.at(ref) : nullptr;
}

void TextureManager::FreeDictionaries(CLEO::CRunningScript* script)
{
	auto ref = GetRefGroup(script);
	
	std::unordered_set<std::string> unusedDicts;
	for (auto& entry : m_dicts)
	{
		entry.second.references.erase(ref);

		if (entry.second.references.empty())
		{
			// clear ref if used as current dictionary
			for (auto& curr : m_currDict)
			{
				if(curr.second == &entry.second)
				{
					curr.second = nullptr;
				}
			}

			unusedDicts.insert(entry.first);
		}
	}

	for (auto& name : unusedDicts)
	{
		if (m_dicts[name].txd != nullptr) RwTexDictionaryDestroy(m_dicts[name].txd);
		m_dicts.erase(name);
	}
}

void TextureManager::FreeDictionary(CLEO::CRunningScript* script, const char* filename)
{
	std::string filepath, name;
	ResolveDictionaryFilename(script, filename, filepath, name);

	auto ref = GetRefGroup(script);

	if (m_dicts.find(name) == m_dicts.end() || m_dicts.at(name).references.count(ref) == 0)
	{
		LOG_WARNING(script, "Failed to unload not loaded texture dictionary '%s'", name.c_str());
	}

	auto& dict = m_dicts.at(name);
	dict.references.erase(ref);
	if (dict.references.empty())
	{
		// clear ref if used as current dictionary
		for (auto& entry : m_currDict)
		{
			if(entry.second == &dict)
			{
				entry.second = nullptr;
			}
		}

		if (m_dicts[name].txd != nullptr) RwTexDictionaryDestroy(m_dicts[name].txd);
		m_dicts.erase(name);
	}
}

void TextureManager::ResolveDictionaryFilename(CLEO::CRunningScript* script, const char* filename, std::string& outFilepath, std::string& name)
{
	// game's original txd search location
	auto oriDir = std::string("root:\\models\\txd\\");
	oriDir.resize(MAX_PATH);
	CLEO_ResolvePath(script, oriDir.data(), oriDir.size());
	oriDir.resize(strlen(oriDir.c_str()));
	FilepathNormalize(oriDir, true);
	oriDir += '\\';

	// real full path
	if (!StringEndsWith(filename, ".txd", false)) // game's original behavior
	{
		// default game behavior
		outFilepath = oriDir;
		outFilepath += filename;
		outFilepath += ".txd";
	}
	else // support any other path
	{
		outFilepath = filename;
		outFilepath.resize(MAX_PATH);
		CLEO_ResolvePath(script, outFilepath.data(), outFilepath.size());
		outFilepath.resize(strlen(outFilepath.c_str()));
	}
	FilepathNormalize(outFilepath, true);

	// path for indexing/display purposes
	name = outFilepath;
	auto len = name.length();
	FilepathRemoveParent(name, CLEO_GetGameDirectory());
	if (name.length() < len)
	{
		name = "root:\\" + name;
	}
	else
	{
		FilepathRemoveParent(name, CLEO_GetUserDirectory());
		if (name.length() < len)
		{
			name = "user:\\" + name;
		}
	}

	// ModLoader support
	if (StringStartsWith(outFilepath, oriDir, false))
	{
		FilepathRemoveParent(outFilepath, CLEO_GetGameDirectory());
	}
}

CLEO::CRunningScript* TextureManager::GetRefGroup(CLEO::CRunningScript* script)
{
	if (script->IsMission()) return (CLEO::CRunningScript*)-1; // mission
	if (!script->IsCustom()) return (CLEO::CRunningScript*)-2; // native scripts
	return script;
}
