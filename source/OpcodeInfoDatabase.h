#pragma once
#include <string>
#include <map>


class OpcodeInfoDatabase
{
	struct Extension
	{
		std::string name;
		std::map<uint16_t, std::string> opcodes;
	};

	bool ok = false;
	std::map<std::string, Extension> extensions;

public:
	OpcodeInfoDatabase() = default;

	void Clear();
	bool Load(const char* filepath);

	const char* GetExtensionName(uint16_t opcode) const; // nullptr if not found
	const char* GetExtensionName(const char* commandName) const; // nullptr if not found

	uint16_t GetOpcode(const char* commandName) const; // 0xFFFF if not found
	const char* GetCommandName(uint16_t opcode) const; // nullptr if not found

	std::string GetExtensionMissingMessage(uint16_t opcode) const; // extension "x" missing message if known, empty text otherwise
};

