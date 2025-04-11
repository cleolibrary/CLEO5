#include "stdafx.h"
#include "OpcodeInfoDatabase.h"


using namespace std;
using namespace simdjson;

bool OpcodeInfoDatabase::Load(const char* filepath)
{
	Clear();

	dom::parser parser;
	dom::element root;

	auto error = parser.load(filepath).get(root);
	if (error) 
	{ 
		TRACE("Failed to parse opcodes database '%s' file. Code %d", filepath, error);
		return false;
	}

	const char* version;
	if (root["meta"]["version"].get_c_str().get(version))
	{
		TRACE("Invalid opcodes database '%s' file.", filepath);
		return false;
	}

	dom::array ext;
	if (root["extensions"].get_array().get(ext))
	{
		TRACE("Invalid opcodes database '%s' file.", filepath);
		return false;
	}

	for (auto& e : ext)
	{
		Extension extension;

		const char* name;
		if (e["name"].get_c_str().get(name))
		{
			continue; // invalid extension
		}
		extension.name = name;

		dom::array commands;
		if (e["commands"].get_array().get(commands))
		{
			continue; // invalid extension
		}

		for (auto& c : commands)
		{
			bool unsupported;
			if (!c["attrs"]["is_unsupported"].get_bool().get(unsupported) && unsupported)
			{
				continue; // command defined as unsupported
			}

			const char* commandId;
			if (c["id"].get_c_str().get(commandId))
			{
				continue; // invalid command
			}

			const char* commandName;
			if (c["name"].get_c_str().get(commandName))
			{
				continue; // invalid command
			}

			auto idLong = stoul(commandId, nullptr, 16);
			if (idLong > 0x7FFF)
			{
				continue; // opcode out of bounds
			}
			auto id = (uint16_t)idLong;

			extension.opcodes.emplace(piecewise_construct, make_tuple(id), make_tuple(id, commandName));
			auto& opcode = extension.opcodes.at(id);

			// read arguments info
			dom::array inputArgs;
			if (!c["input"].get_array().get(inputArgs))
			{
				for (auto& p : inputArgs)
				{
					const char* argName;
					if (!p["name"].get_c_str().get(argName))
					{
						opcode.arguments.emplace_back(argName);
					}
				}
			}
		}

		if (!extension.opcodes.empty())
		{
			extensions[extension.name] = move(extension);
		}
	}

	TRACE("Opcodes database version %s loaded from '%s'", version, filepath);
	ok = true;
	return true;
}

void OpcodeInfoDatabase::Clear()
{
	ok = false;
	extensions.clear();
}

const char* OpcodeInfoDatabase::GetExtensionName(uint16_t opcode) const
{
	if (ok)
	{
		for (auto& entry : extensions)
		{
			auto& extension = entry.second;
			auto& opcodes = extension.opcodes;

			if (opcodes.find(opcode) != opcodes.end())
			{
				return extension.name.c_str();
			}
		}
	}

	return nullptr;
}

const char* OpcodeInfoDatabase::GetExtensionName(const char* commandName) const
{
	if (ok)
	{
		for (auto& entry : extensions)
		{
			auto& extension = entry.second;
			auto& opcodes = extension.opcodes;

			for (auto& opcode : opcodes)
			{
				if (_strcmpi(commandName, opcode.second.name.c_str()) == 0)
				{
					return extension.name.c_str();
				}
			}
		}
	}

	return nullptr;
}

uint16_t OpcodeInfoDatabase::GetOpcode(const char* commandName) const
{
	if (ok)
	{
		for (auto& entry : extensions)
		{
			auto& extension = entry.second;
			auto& opcodes = extension.opcodes;

			for (auto& opcode : opcodes)
			{
				if (_strcmpi(commandName, opcode.second.name.c_str()) == 0)
				{
					return opcode.first;
				}
			}
		}
	}

	return 0xFFFF;
}

const char* OpcodeInfoDatabase::GetCommandName(uint16_t opcode) const
{
	if (ok)
	{
		for (auto& entry : extensions)
		{
			auto& opcodes = entry.second.opcodes;

			if (opcodes.find(opcode) != opcodes.end())
			{
				return opcodes.at(opcode).name.c_str();
			}
		}
	}

	return nullptr;
}

const char* OpcodeInfoDatabase::GetArgumentName(uint16_t opcode, size_t paramIdx) const
{
	if (ok)
	{
		for (auto& entry : extensions)
		{
			auto& opcodes = entry.second.opcodes;

			if (opcodes.find(opcode) != opcodes.end())
			{
				if(paramIdx < opcodes.at(opcode).arguments.size())
				{
					return opcodes.at(opcode).arguments[paramIdx].name.c_str();
				}
			}
		}
	}

	return nullptr;
}

std::string OpcodeInfoDatabase::GetExtensionMissingMessage(uint16_t opcode) const
{
	auto extensionName = GetExtensionName(opcode);
	if (extensionName == nullptr)
	{
		return {};
	}

	return CLEO::StringPrintf("CLEO extension plugin \"%s\" is missing!", extensionName);
}

