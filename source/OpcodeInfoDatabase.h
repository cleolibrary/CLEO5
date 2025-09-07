#pragma once
#include <atomic>
#include <map>
#include <string>

class OpcodeInfoDatabase
{
    struct OpcodeArgument
    {
        std::string name;

        OpcodeArgument() = default;
        OpcodeArgument(const char* name) : name(name) {}
    };

    struct Opcode
    {
        uint16_t id;
        std::string name;
        std::vector<OpcodeArgument> arguments;

        Opcode() = default;
        Opcode(uint16_t id, std::string name) : id(id), name(name) {}
    };

    struct Extension
    {
        std::string name;
        std::map<uint16_t, Opcode> opcodes;
    };

    std::atomic<bool> ok = false;
    std::map<std::string, Extension> extensions;

  public:
    OpcodeInfoDatabase() = default;

    void Clear();
    bool Load(const char* filepath); // triggers asynchronic load

    const char* GetExtensionName(uint16_t opcode) const;         // nullptr if not found
    const char* GetExtensionName(const char* commandName) const; // nullptr if not found

    uint16_t GetOpcode(const char* commandName) const; // 0xFFFF if not found
    const char* GetCommandName(uint16_t opcode) const; // nullptr if not found

    const char* GetArgumentName(uint16_t opcode, size_t paramIdx) const; // nullptr if not found

    // extension "x" missing message if known, empty text otherwise
    std::string GetExtensionMissingMessage(uint16_t opcode) const;
};
