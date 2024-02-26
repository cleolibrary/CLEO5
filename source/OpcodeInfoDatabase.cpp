#include "stdafx.h"
#include "OpcodeInfoDatabase.h"
#include "json.hpp"
#include <fstream>

using namespace std;
using namespace json;

void OpcodeInfoDatabase::Clear()
{
	ok = false;
	extensions.clear();
}

bool OpcodeInfoDatabase::Load(const char* filepath)
{
	Clear();

	ifstream file(filepath);
	if (file.fail())
	{
		TRACE("Failed to open opcodes database '%s' file.", filepath);
		return false;
	}

	file.seekg(0, ifstream::end);
	auto size = file.tellg();
	file.seekg(0, ifstream::beg);

	if (size > 8 * 1024 * 1024) // 8MB is reasonable json file size upper limit
	{
		TRACE("Opcodes database '%s' file too large to load.", filepath);
		return false;
	}

	std::string text;
	text.resize((size_t)size);
	file.read(text.data(), size);
	file.close();

	if (file.fail())
	{
		TRACE("Error while reading opcodes database '%s' file.", filepath);
		return false;
	}

	JSON root;
	try
	{
		root = JSON::Load(text.c_str());
	}
	catch (const std::exception& ex)
	{
		TRACE("Error while parsing opcodes database '%s' file:\n%s", filepath, ex.what());
		return false;
	}

	if (root.IsNull() || root["extensions"].JSONType() != JSON::Class::Array)
	{
		TRACE("Invalid opcodes database '%s' file.", filepath);
		return false;
	}

	for (auto& e : root["extensions"].ArrayRange())
	{
		auto name = e["name"];
		auto commands = e["commands"];
		if (name.JSONType() != JSON::Class::String || commands.JSONType() != JSON::Class::Array)
		{
			continue; // invalid extension
		}

		Extension extension;
		extension.name = name.ToString();

		for (auto& c : commands.ArrayRange())
		{
			auto commandId = c["id"];
			auto commandName = c["name"];
			if (commandId.JSONType() != JSON::Class::String || commandName.JSONType() != JSON::Class::String)
			{
				continue; // invalid command
			}

			auto id = std::stoul(commandId.ToString(), nullptr, 16);
			if (id > 0x7FFF)
			{
				continue; // opcode out of bounds
			}

			extension.opcodes[(uint16_t)id] = commandName.ToString();
		}

		if (!extension.opcodes.empty())
		{
			extensions[extension.name] = std::move(extension);
		}
	}

	ok = true;
	return true;
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
				if (_strcmpi(commandName, opcode.second.c_str()) == 0)
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
				if (_strcmpi(commandName, opcode.second.c_str()) == 0)
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
				return opcodes.at(opcode).c_str();
			}
		}
	}

	return nullptr;
}
