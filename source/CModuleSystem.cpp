#include "stdafx.h"
#include "CleoBase.h"
#include "CModuleSystem.h"
#include "FileEnumerator.h"

#include <chrono>
#include <fstream>

using namespace CLEO;

void CModuleSystem::Clear()
{
	modules.clear();
}

const ScriptDataRef CModuleSystem::GetExport(std::string modulePath, std::string_view exportName)
{
	NormalizePath(modulePath);

	auto& it = modules.find(modulePath);
	if (it == modules.end()) // module not loaded yet?
	{
		if (!LoadFile(modulePath.c_str()))
		{
			return {};
		}

		// check if available now
		it = modules.find(modulePath);
		if (it == modules.end())
		{
			return {};
		}
	}
	auto& module = it->second;

	auto e = module.GetExport(std::string(exportName));
	if (e.Valid())
	{
		module.refCount++;
	}
	return e;
}

bool CModuleSystem::LoadFile(const char* path)
{
	std::string normalizedPath(path);
	NormalizePath(normalizedPath);

	if (!modules[normalizedPath].LoadFromFile(normalizedPath.c_str()))
	{
		return false;
	}

	return true;
}

bool CModuleSystem::LoadDirectory(const char* path)
{
	bool result = true;
	FilesWalk(path, ".s", [&](const char* fullPath, const char* filename)
	{
		result &= LoadFile(fullPath);
	});

	return result;
}

bool CModuleSystem::LoadCleoModules()
{
	const auto path = FS::path(Filepath_Cleo).append("cleo_modules").string();
	return LoadDirectory(path.c_str());
}

void CLEO::CModuleSystem::AddModuleRef(const char* baseIP)
{
	for (auto& it : modules)
	{
		auto& module = it.second;

		if (module.data.data() == baseIP)
		{
			module.refCount++;
			return;
		}
	}
}

void CLEO::CModuleSystem::ReleaseModuleRef(const char* baseIP)
{
	for (auto& it : modules)
	{
		auto& module = it.second;

		if (module.data.data() == baseIP)
		{
			module.refCount--;
			return;
		}
	}
}

void CModuleSystem::NormalizePath(std::string& path)
{
	for (char& c : path)
	{
		// standarize path separators
		if (c == '/')
			c = '\\';

		// lower case
		c = std::tolower(c);
	};
}

void CModuleSystem::CModule::Update()
{
	while (updateActive)
	{
		if (!updateNeeded)
		{
			FS::file_time_type time;
			try
			{
				time = FS::last_write_time(filepath);
			}
			catch (...)
			{
				time = {};
			}

			// file not exists or up to date
			if (time == FS::file_time_type{} || time == fileTime)
			{
				// query files once a second
				for(size_t i = 0; i < 100 && updateActive; i++)
					std::this_thread::sleep_for(std::chrono::milliseconds(10));

				continue; 
			}

			updateNeeded = true;
		}

		if (refCount != 0)
		{
			continue; // module currently in use
		}

		auto file = filepath;
		auto result = LoadFromFile(file.c_str());
		updateNeeded = false;
		Debug.Trace(eLogLevel::Debug, "Module reload %s '%s'", result ? "OK" : "FAILED", file.c_str());
	}
}

CModuleSystem::CModule::CModule() :
	updateThread(&CModuleSystem::CModule::Update, this)
{
}

CModuleSystem::CModule::~CModule()
{
	updateActive = false;
	updateThread.join();
}

void CModuleSystem::CModule::Clear()
{
	if (refCount != 0)
	{
		TRACE("Warning! Module '%s' cleared despite in use %d time(s)", filepath.c_str(), refCount.load());
	}

	std::lock_guard<std::mutex> guard(updateMutex);

	filepath.clear();
	data.clear();
	exports.clear();

	refCount = 0;
	fileTime = {};
}

const char* CModuleSystem::CModule::GetFilepath() const
{
	return filepath.c_str();
}

bool CModuleSystem::CModule::LoadFromFile(const char* path)
{
	Clear();

	std::lock_guard<std::mutex> guard(updateMutex);

	filepath = path;

	try
	{
		fileTime = FS::last_write_time(path);
	}
	catch(...)
	{
		fileTime = {};
	}

	std::ifstream file(path, std::ios::binary);
	if (!file.good())
	{
		LOG_WARNING("Failed to open module file '%s'", path);
		return false;
	}

#pragma warning ( push )
#pragma warning ( disable: 4838 )
#pragma warning ( disable: 4309 )
	const char Segment_First_Instruction[] = { 0x02, 0x00, 0x01 }; // jump, param type
	const char Segment_Magic[] = { 0xFF, 0x7F, 0xFE, 0x00, 0x00 }; // Rockstar custom header magic
	const char Header_Signature_Module_Exports[] = { 'E', 'X', 'P', 'T' }; // CLEO's module header signature
#pragma warning ( pop )

	// read first instruction
#pragma pack(push, 1)
	struct
	{
		char firstInstruction[3];
		int jumpAddress;
		char magic[5];
	} segment;
#pragma pack(pop)

	file.read((char*)&segment, sizeof(segment));
	if (file.fail())
	{
		LOG_WARNING("Module '%s' file header read error", path);
		return false;
	}

	// verify segment data
	if (std::memcmp(segment.firstInstruction, Segment_First_Instruction, sizeof(Segment_First_Instruction)) != 0 ||
		segment.jumpAddress >= 0 || // jump labels should be negative values
		std::memcmp(segment.magic, Segment_Magic, sizeof(Segment_Magic)) != 0) // not a custom header
	{
		LOG_WARNING("Module '%s' load error. Custom segment not present", path);
		return false;
	}
	segment.jumpAddress = abs(segment.jumpAddress); // turn label into actual file offset

	// process custom headers
#pragma pack(push, 1)
	struct
	{
		char signature[4];
		int size;
	} header;
#pragma pack(pop)

	bool result = false; // no custom header found yet
	while (file.tellg() < segment.jumpAddress)
	{
		file.read((char*)&header, sizeof(header));
		if (file.fail() ||
			file.tellg() > segment.jumpAddress) // read past the segment end
		{ 
			LOG_WARNING("Module '%s' load error. Invalid custom header", path);
			return false;
		}

		auto headerEndPos = file.tellg();
		headerEndPos += header.size;

		// CLEO Module Exports
		if (std::memcmp(header.signature, Header_Signature_Module_Exports, sizeof(Header_Signature_Module_Exports)) == 0)
		{
			if (headerEndPos > segment.jumpAddress)
			{
				LOG_WARNING("Module '%s' load error. Invalid size of exports header", path);
				return false;
			}

			while (true)
			{
				ModuleExport e;

				if (!e.LoadFromFile(file) ||
					!file.good() ||
					file.tellg() > headerEndPos)
				{
					if (e.name.empty())
					{
						LOG_WARNING("Module '%s' export load error.", path);
					}
					else
					{
						LOG_WARNING("Module's '%s' export '%s' load error.", path, e.name.c_str());
					}
					return false;
				}

				exports[e.name] = std::move(e); // move to container
				result = true; // something useful loaded

				if (file.tellg() == headerEndPos)
				{
					break; // all exports done
				}
			}
		}
		else // other unknown header type
		{
			file.seekg(headerEndPos, file.beg);
			if (file.fail())
			{
				LOG_WARNING("Module '%s' load error. Error while skipping unknown header type", path);
				return false;
			}
		}
	}

	if (!file.good())
	{
		LOG_WARNING("Module '%s' read error", path);
		return false;
	}

	if (!result) // no usable elements found. No point to keeping this module
	{
		LOG_WARNING("Module '%s' skipped. Nothing found", path);
		return false;
	}

	// get file size
	file.seekg(0, file.end);
	auto size = (size_t)file.tellg();
	file.seekg(0, file.beg);

	// store file data
	data.resize(size);
	file.read(data.data(), size);
	if (file.fail())
	{
		return false;
	}

	return true;
}

const ScriptDataRef CModuleSystem::CModule::GetExport(std::string name)
{
	ModuleExport::NormalizeName(name);

	auto& it = exports.find(name);
	if (it == exports.end())
	{
		return {};
	}
	auto& exp = it->second;

	return { data.data(), exp.offset };
}

void CModuleSystem::CModule::ModuleExport::Clear()
{
	name.clear();
	offset = 0;
}

bool CModuleSystem::CModule::ModuleExport::LoadFromFile(std::ifstream& file)
{
	if (!file.good())
	{
		return false;
	}

	// name
	std::getline(file, name, '\0');
	if (file.fail() || name.length() >= 0xFF)
	{
		return false;
	}
	NormalizeName(name);

	// address
	file.read((char*)&offset, 4);
	if (file.fail())
	{
		return false;
	}

	// input arg count
	unsigned char inParamCount;
	file.read((char*)&inParamCount, 1);
	if (file.fail())
	{
		return false;
	}

	// skip input argument types info
	file.seekg(inParamCount, file.cur);
	if (file.fail())
	{
		return false;
	}

	// return value count
	unsigned char outParamCount;
	file.read((char*)&outParamCount, 1);
	if (file.fail())
	{
		return false;
	}

	// skip return value types info
	file.seekg(outParamCount, file.cur);
	if (file.fail())
	{
		return false;
	}

	return true; // done
}

void CModuleSystem::CModule::ModuleExport::NormalizeName(std::string& name)
{
	for (auto& ch : name)
	{
		ch = std::tolower(ch);
	}
}
