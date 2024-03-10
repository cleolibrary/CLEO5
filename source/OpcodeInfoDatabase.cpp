#include "stdafx.h"
#include "OpcodeInfoDatabase.h"
#include "json.hpp"
#include <fstream>
#include <vector>
#include <thread>

using namespace std;
using namespace json;

bool OpcodeInfoDatabase::_Load(const std::string filepath)
{
	Clear();

	ifstream file(filepath.c_str());
	if (file.fail())
	{
		TRACE("Failed to open opcodes database '%s' file.", filepath.c_str());
		return false;
	}

	file.seekg(0, ifstream::end);
	auto size = file.tellg();
	file.seekg(0, ifstream::beg);

	if (size > 8 * 1024 * 1024) // 8MB is reasonable json file size upper limit
	{
		TRACE("Opcodes database '%s' file too large to load.", filepath.c_str());
		return false;
	}

	string text;
	text.resize((size_t)size);
	file.read(text.data(), size);
	file.close();

	if (file.fail())
	{
		TRACE("Error while reading opcodes database '%s' file.", filepath.c_str());
		return false;
	}

	JSON root;
	try
	{
		root = JSON::Load(text.c_str());
	}
	catch (const exception& ex)
	{
		TRACE("Error while parsing opcodes database '%s' file:\n%s", filepath.c_str(), ex.what());
		return false;
	}

	if (root.IsNull() || root["extensions"].JSONType() != JSON::Class::Array)
	{
		TRACE("Invalid opcodes database '%s' file.", filepath.c_str());
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

			auto idLong = stoul(commandId.ToString(), nullptr, 16);
			if (idLong > 0x7FFF)
			{
				continue; // opcode out of bounds
			}
			auto id = (uint16_t)idLong;

			extension.opcodes.emplace(piecewise_construct, make_tuple(id), make_tuple(id, commandName.ToString()));
			auto& opcode = extension.opcodes.at(id);

			// read arguments info
			auto inputArgs = c["input"];
			if (inputArgs.JSONType() == JSON::Class::Array)
			{
				for (auto& p : inputArgs.ArrayRange())
				{
					if (p.JSONType() == JSON::Class::Object && p["name"].JSONType() == JSON::Class::String)
					{
						opcode.arguments.emplace_back(p["name"].ToString().c_str());
					}
				}
			}

			auto outputArgs = c["output"];
			if (outputArgs.JSONType() == JSON::Class::Array)
			{
				for (auto& p : outputArgs.ArrayRange())
				{
					if (p.JSONType() == JSON::Class::Object && p["name"].JSONType() == JSON::Class::String)
					{
						opcode.arguments.emplace_back(p["name"].ToString().c_str());
					}
				}
			}
		}

		if (!extension.opcodes.empty())
		{
			extensions[extension.name] = move(extension);
		}
	}

	ok = true;
	return true;
}

void OpcodeInfoDatabase::Clear()
{
	ok = false;
	extensions.clear();
}

void OpcodeInfoDatabase::Load(const char* filepath)
{
	thread(&OpcodeInfoDatabase::_Load, this, std::string(filepath)).detach(); // asynchronic execute
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

