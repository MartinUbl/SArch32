#pragma once

#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <iostream>

#include "../core/isa.h"
#include "../core/sobjfile.h"

#include "pseudoinstruction.h"

/*
 * Log level of assembler
 */
enum class NLog_Level {
	None,
	Basic,
	Extended,
	Full,
};

/*
 * Structure representing inputs to the assembler process
 */
struct TAssembly_Input {
	// files to be assembled
	std::vector<std::string> Input_Files;
	// linker file for section relocation
	std::string Linker_File;
	// output binary object file
	std::string Output_File;
	// desired log level
	NLog_Level Log_Level = NLog_Level::Basic;
};

/*
 * The main assembler class
 */
class CAssembler {

	private:
		// input parameters
		TAssembly_Input mInput;

		// label reference for linkage
		struct TLabel_Ref {
			std::string section{};
			size_t byteOffset = 0;
		};

		// assembled instructions and data, sorted to sections
		std::map<std::string, std::vector<std::unique_ptr<CInstruction>>> mSections;
		// cached section offsets for linkage
		std::map<std::string, size_t> mSection_Offsets;
		// cached label references for linkage
		std::map<std::string, TLabel_Ref> mLabel_Refs;

		// resolve request for linkage - the linker then uses this request to resolve each symbol
		struct TResolve_Request {
			std::string symbol;
			std::string section;
			size_t instructionIndex;
		};

		// stored resolve requests
		std::vector<TResolve_Request> mResolve_Requests;

		// current section the assembler is assembling into
		std::string mCurrent_Section = "data";

		// definition of linker section (from the linker file)
		struct TLinker_Section_Def {
			std::string section{};
			uint32_t startAddr = 0;
			uint32_t limit = 0;
		};

		// stored linker sections from linker file
		std::map<std::string, TLinker_Section_Def> mLinker_Section_Defs;

	protected:

		// assembles a given file
		bool Assemble_File(const std::string& path);

		// loads a linker file
		bool Load_Linker_File(const std::string& path);

		// resolves symbols in all assembled files and sections
		bool Resolve_Symbols();

		// generates output binary given all assembling went OK
		bool Generate_Binary();

		// parses section directive in assembly file
		bool Parse_Section_Directive(const std::string& line, std::string& sectionName) const;
		// parses label directive in assembly file
		bool Parse_Label_Directive(const std::string& line, std::string& labelName) const;
		// parses a pseudoinstruction (data - db, dw, asciz)
		std::unique_ptr<CInstruction> Parse_Pseudo_Instruction(const std::string& line) const;

	protected:
		// internal log method - single parameter output
		template<typename T1>
		void Log_Internal(const T1& val) {
			std::cout << val << " ";
		}

		// internal log method - variadic template
		template<typename T1, typename... TArgs>
		void Log_Internal(const T1& val, TArgs... args) {
			Log_Internal(val);
			Log_Internal(args...);
		}

		// log entry method
		template<typename... Args>
		void Log(NLog_Level ll, Args... args) {

			if (static_cast<int>(ll) > static_cast<int>(mInput.Log_Level))
				return;

			Log_Internal(args...);
			std::cout << std::endl;
		}

	public:
		CAssembler(TAssembly_Input& input);
		virtual ~CAssembler() = default;

		// assemble given inputs into output binary
		bool Assemble();
};
