#pragma once

namespace CLEO
{
	struct ScriptDataRef
	{
		char* base = nullptr; // script's base data
		DWORD size = 0; // available code block size
		int offset = 0; // address within the script

		bool Valid() const
		{
			return base != nullptr && size >= sizeof(WORD); // at least one opcode
		}
	};

	class CModuleSystem
	{
	public:
		CModuleSystem() = default;
		CModuleSystem(const CModuleSystem&) = delete; // no copying

		void Clear();

		// registers module reference. Needs to be released with ReleaseModuleRef
		const ScriptDataRef GetExport(std::string modulePath, std::string_view exportName);

		bool LoadFile(const char* const path); // single file
		bool LoadDirectory(const char* const path); // all modules in directory
		bool LoadCleoModules(); // all in cleo\cleo_modules

	private:
		static void NormalizePath(std::string& path);

		class CModule
		{
			friend class CModuleSystem;

			struct ModuleExport
			{
				std::string name;
				int offset = 0; // address within module's data

				void Clear();
				bool LoadFromFile(std::ifstream& file);

				static void NormalizeName(std::string& name);
			};

			std::string filepath; // source file
			std::vector<char> data;
			std::map<std::string, ModuleExport> exports;

		public:
			CModule() = default;
			CModule(const CModule&) = delete; // no copying
			~CModule() = default;

			void Clear();
			const char* GetFilepath() const;
			bool LoadFromFile(const char* path);
			const ScriptDataRef GetExport(std::string name);
		};

		std::map<std::string, CModule> modules;
	};
}

