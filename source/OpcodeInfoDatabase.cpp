#include "stdafx.h"
#include "OpcodeInfoDatabase.h"
#include <algorithm>
#include <fstream>


using namespace std;
using namespace simdjson;

bool OpcodeInfoDatabase::LoadCommands(const char* filepath)
{
	m_commands.clear();

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
		const char* extensionName;
		if (e["name"].get_c_str().get(extensionName))
		{
			continue; // invalid extension
		}

		dom::array commands;
		if (e["commands"].get_array().get(commands))
		{
			continue; // invalid extension
		}

		for (auto& c : commands)
		{
			// unsupported?
			bool unsupported;
			if (!c["attrs"]["is_unsupported"].get_bool().get(unsupported) && unsupported)
			{
				continue; // command defined as unsupported
			}

			// opcode
			const char* commandId;
			if (c["id"].get_c_str().get(commandId))
			{
				continue; // invalid command
			}
			auto idLong = stoul(commandId, nullptr, 16);
			if (idLong > 0x7FFF)
			{
				continue; // opcode out of bounds
			}
			auto id = (uint16_t)idLong;

			// command already present?
			if (m_commands.count(id))
			{
				m_commands[id].extension.push_back('/');
				m_commands[id].extension.append(extensionName);
				continue;
			}

			// name
			const char* commandName;
			if (c["name"].get_c_str().get(commandName))
			{
				continue; // invalid command
			}

			// store
			Command& command = m_commands[id];
			command.extension = extensionName;
			command.id = id;
			command.name = commandName;
			command.nameLower = commandName;
			CLEO::StringToLower(command.nameLower);

			// operator
			const char* commandOperator = nullptr;
			if(!c["operator"].get_c_str().get(commandOperator))
			{
				command.oper = commandOperator;
			}

			// attributes
			dom::element attr;
			if (!c["attrs"].get(attr))
			{
				bool state;
				if (!attr["is_condition"].get_bool().get(state)) { command.isCondition = state; }
				if (!attr["is_constructor"].get_bool().get(state)) { command.isConstructor = state; }
				if (!attr["is_nop"].get_bool().get(state)) { command.isNop = state; }
			}

			// command params
			auto LoadParams = [&](const char* obj, bool isInput)
			{
				dom::array args;
				if (!c[obj].get_array().get(args))
				{
					for (auto& p : args)
					{
						const char* argName;
						if (!p["name"].get_c_str().get(argName))
						{
							auto& arg = command.arguments.emplace_back();
							if (isInput) command.inputArguments++;

							arg.name = argName;

							const char* argType;
							if (!p["type"].get_c_str().get(argType))
							{
								arg.typeName = argType;
								arg.typeNameLower = argType;
								CLEO::StringToLower(arg.typeNameLower);
								arg.type = TypeFromName(arg.typeName);
							}

							const char* argSource;
							if (!p["source"].get_c_str().get(argSource))
							{
								arg.source = argSource;
								CLEO::StringToLower(arg.source);
							}
						}
					}
				}
			};
			LoadParams("input", true);
			LoadParams("output", false);
		}
	}

	TRACE("Commands database version %s loaded from '%s'", version, filepath);
	return true;
}

bool OpcodeInfoDatabase::LoadEnums(const char* filepath)
{
	m_enums.clear();

	std::ifstream file(filepath, std::ios::in);
	if (!file.is_open())
	{
		TRACE("Failed to open enums database '%s' file", filepath);
		return false;
	}

	bool inEnum = false;
	Enum currEnum;
	std::string line;
	size_t lineCount = 0;
	while (std::getline(file, line))
	{
		lineCount++;

		CLEO::StringTrim(line);

		if (line.empty()) continue;

		if (!inEnum)
		{
			if (CLEO::StringStartsWith(line, "enum ", false))
			{
				currEnum = Enum();
				currEnum.name = line.c_str() + 5; // + strlen("enum ")

				inEnum = true;
				continue;
			}
			else
			{
				TRACE("Failed to parse enums database '%s' file. Unexpected keyword in line %d", filepath, lineCount);
				return false;
			}
		}
		else
		{
			if (CLEO::StringStartsWith(line, "enum ", false))
			{
				TRACE("Failed to parse enums database '%s' file. Unexpected 'enum' keyword in line %d", filepath, lineCount);
				return false;
			}

			if (_stricmp(line.c_str(), "end") == 0)
			{
				if (!currEnum.valuesNum.empty() && !currEnum.valuesTxt.empty())
				{
					TRACE("Failed to parse enums database '%s' file. Enum with mixed data type in line %d", filepath, lineCount);
					return false;
				}

				if (!currEnum.IsEmpty())
				{
					auto name = currEnum.name;
					CLEO::StringToLower(name);
					m_enums.emplace(std::move(name), std::move(currEnum));
				}

				inEnum = false;
				continue;
			}

			std::vector<std::string> elements;
			CLEO::StringSplit(line, "=", elements);
			if (elements.size() > 2)
			{
				TRACE("Failed to parse enums database '%s' file. Too many '=' characters in line %d", filepath, lineCount);
				return false;
			}

			std::string& name = elements[0];
			std::string value = elements.size() > 1 ? elements[1] : "";

			if (value.find_first_not_of("-0123456789") != std::string::npos)
			{
				if (value.front() != '"' || value.back() != '"')
				{
					TRACE("Failed to parse enums database '%s' file. Invalid format of value in line %d", filepath, lineCount);
					return false;
				}
				value.pop_back();
				value.erase(0, 1);
				currEnum.valuesTxt.emplace(std::move(value), std::move(name));
			}
			else // numeric or empty
			{
				int numValue;
				if (value.empty())
				{
					if (currEnum.valuesNum.empty()) numValue = 0;
					else numValue = std::prev(currEnum.valuesNum.end())->first + 1; // value of last entry + 1
				}
				else
				{
					char* end;
					long i = strtol(value.c_str(), &end, 10);

					if (i > std::numeric_limits<int>::max() || i < std::numeric_limits<int>::min())
					{
						TRACE("Failed to parse enums database '%s' file. Numeric value out of range in line %d", filepath, lineCount);
						return false;
					}

					if (end != value.c_str() + value.length())
					{
						TRACE("Failed to parse enums database '%s' file. Unexpected character in numeric value in line %d", filepath, lineCount);
						return false;
					}

					numValue = (int)i;
				}

				if (currEnum.valuesNum.count(numValue) != 0)
				{
					continue; // do not store duplicates
				}

				currEnum.valuesNum.emplace(numValue, std::move(name));
			}
		}
	}

	TRACE("Enums database loaded from '%s'", filepath);
	return true;
}

bool OpcodeInfoDatabase::HasCommand(uint16_t opcode) const
{
	return m_commands.count(opcode);
}

bool OpcodeInfoDatabase::HasCommand(const char* name) const
{
	return GetCommand(name) != nullptr;
}

const OpcodeInfoDatabase::Command* OpcodeInfoDatabase::GetCommand(uint16_t opcode) const
{
	return HasCommand(opcode) ? &m_commands.at(opcode) : nullptr;
}

const OpcodeInfoDatabase::Command* OpcodeInfoDatabase::GetCommand(const char* name) const
{
	for (auto& [opcode, command] : m_commands)
	{
		if (command.NameEqual(name))
		{
			return &command;
		}
	}

	return nullptr;
}

const char* OpcodeInfoDatabase::GetExtensionName(uint16_t opcode) const
{
	auto command = GetCommand(opcode);
	return command ? command->extension.c_str() : nullptr;
}

const char* OpcodeInfoDatabase::GetExtensionName(const char* name) const
{
	auto command = GetCommand(name);
	return command ? command->extension.c_str() : nullptr;
}

uint16_t OpcodeInfoDatabase::GetOpcode(const char* name) const
{
	auto command = GetCommand(name);
	return command ? command->id : 0xFFFF;
}

const char* OpcodeInfoDatabase::GetCommandName(uint16_t opcode) const
{
	auto command = GetCommand(opcode);
	return command ? command->name.c_str() : nullptr;
}

const char* OpcodeInfoDatabase::GetArgumentName(uint16_t opcode, size_t paramIdx) const
{
	auto command = GetCommand(opcode);
	
	if (command == nullptr || paramIdx >= command->arguments.size())
	{
		return "";
	}

	return command->arguments[paramIdx].name.c_str();
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

bool OpcodeInfoDatabase::HasEnum(const char* name) const
{
	return m_enums.count(name);
}

const OpcodeInfoDatabase::Enum* OpcodeInfoDatabase::GetEnum(const char* name) const
{
	return HasEnum(name) ? &m_enums.at(name) : nullptr;
}

OpcodeInfoDatabase::CommandArgumentType OpcodeInfoDatabase::TypeFromName(const std::string_view& name)
{
	auto str = name.data();
	auto len = name.length();

	if (!_strnicmp(str, "Any", len)) return CommandArgumentType::Any;
	if (!_strnicmp(str, "Arguments", len)) return CommandArgumentType::Arguments;
	if (!_strnicmp(str, "Bool", len)) return CommandArgumentType::Bool;
	if (!_strnicmp(str, "Float", len)) return CommandArgumentType::Float;
	if (!_strnicmp(str, "GxtKey", len)) return CommandArgumentType::GxtKey;
	if (!_strnicmp(str, "Int", len)) return CommandArgumentType::Int;
	if (!_strnicmp(str, "Label", len)) return CommandArgumentType::Label;
	if (!_strnicmp(str, "ModelAny", len)) return CommandArgumentType::ModelAny;
	if (!_strnicmp(str, "ModelChar", len)) return CommandArgumentType::ModelChar;
	if (!_strnicmp(str, "ModelObject", len)) return CommandArgumentType::ModelObject;
	if (!_strnicmp(str, "ModelVehicle", len)) return CommandArgumentType::ModelVehicle;
	if (!_strnicmp(str, "ScriptId", len)) return CommandArgumentType::ScriptId;
	if (!_strnicmp(str, "String", len)) return CommandArgumentType::String;
	if (!_strnicmp(str, "String128", len)) return CommandArgumentType::String128;
	if (!_strnicmp(str, "ZoneGxt", len)) return CommandArgumentType::ZoneGxt;
	return CommandArgumentType::Other;
}

