#include "assembler.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>

CAssembler::CAssembler(TAssembly_Input& input)
	: mInput(input) {
	//
}

bool CAssembler::Assemble_File(const std::string& path) {

	Log(NLog_Level::Basic, "Assembling", path, "...");

	// regex to detect empty lines (possibly containing comments)
	std::regex emptylineregex{ "^[\\s]{0,}(;.*)?$", std::regex::ECMAScript };
	std::string tmp;

	std::string line;
	std::ifstream iss(path);
	// for each line...
	while (std::getline(iss, line)) {
		try
		{
			// is this an empty line? ignore
			std::smatch sm;
			if (std::regex_match(line, sm, emptylineregex)) {
				continue;
			}

			// is this a section directive? (.section abcd)
			if (Parse_Section_Directive(line, tmp)) {
				Log(NLog_Level::Extended, "Switch to section", tmp);
				mCurrent_Section = tmp;
				continue;
			}
			// is this a label directive? ($labelname:)
			else if (Parse_Label_Directive(line, tmp)) {
				Log(NLog_Level::Extended, "Found label", tmp, "at offset", mSection_Offsets[mCurrent_Section], "in section", mCurrent_Section);
				mLabel_Refs[tmp].section = mCurrent_Section;
				mLabel_Refs[tmp].byteOffset = mSection_Offsets[mCurrent_Section];
				continue;
			}

			std::unique_ptr<CInstruction> r;

			// try to parse pseudoinstruction (has precedence over machine instructions)
			r = Parse_Pseudo_Instruction(line);
			// if it's not a pseudoinstruction, parse it as a machine instruction
			if (!r) {
				r = CInstruction::Build_From_String(line);
			}

			// on success - store it
			if (r) {
				auto instrLen = r->Get_Length(); // size of instruction or data

				// does this line need a symbol resolution?
				if (r->Get_Resolve_Request(tmp)) {
					mResolve_Requests.push_back({ tmp, mCurrent_Section, mSections[mCurrent_Section].size() });
				}

				// add instrction and move section offset
				mSections[mCurrent_Section].push_back(std::move(r));
				mSection_Offsets[mCurrent_Section] += instrLen;
			}
		}
		catch (std::exception& ex) {
			std::cerr << "Exception: " << ex.what() << std::endl;
		}
	}

	return true;
}

bool CAssembler::Load_Linker_File(const std::string& path) {

	Log(NLog_Level::Full, "Loading linker file:", path);

	// open linker file
	std::ifstream lf(path);
	if (!lf.is_open()) {
		return false;
	}

	// section definition regex
	std::regex sdefregex("[\\s]{0,}(section)[\\s]{0,}([a-zA-Z0-9_]+)\\((0x[0-9a-zA-Z]+|[0-9])\\)", std::regex::ECMAScript | std::regex::icase);

	// for each line...
	std::string line;
	while (std::getline(lf, line)) {

		Log(NLog_Level::Full, "Parsing line:", line);

		std::smatch sm;
		if (std::regex_match(line, sm, sdefregex) && sm.size() == 4) {

			const auto& sname = sm[2];
			const auto& reloc = sm[3];

			Log(NLog_Level::Extended, "Section", sname, "relocate to", reloc);

			uint32_t relocAddr = 0;

			try {

				// hexa format
				if (reloc.str().starts_with("0x") || reloc.str().starts_with("0X")) {
					relocAddr = std::stol(reloc.str(), nullptr, 16);
				}
				// dec format
				else {
					relocAddr = std::stol(reloc.str(), nullptr, 10);
				}

				Log(NLog_Level::Full, "Section definition of", sname, "loaded");

				mLinker_Section_Defs[sname.str()] = {
					sname.str(),
					relocAddr,
					0 // TODO: limit
				};
			}
			catch (std::exception& /*ex*/) {
				std::cerr << "Cannot parse relocation address: " << reloc << std::endl;
			}
		}

	}

	return true;
}

bool CAssembler::Resolve_Symbols() {

	Log(NLog_Level::Basic, "Resolving symbols...");

	// go through all resolution requests
	for (auto& rr : mResolve_Requests) {

		// find the respective label reference (actual locations)
		auto sym = mLabel_Refs.find(rr.symbol);

		// no such label? report unresolved symbol
		if (sym == mLabel_Refs.end()) {
			std::cerr << "Unresolved symbol: " << rr.symbol << std::endl;
			return false;
		}

		// find a section in which the symbol resides
		auto slink = mLinker_Section_Defs.find(sym->second.section);
		if (slink == mLinker_Section_Defs.end()) {
			std::cerr << "Unknown section '" << sym->second.section << "' for symbol: " << rr.symbol << std::endl;
			return false;
		}

		// actual address = section starting address + symbol offset
		auto addr = static_cast<int32_t>(slink->second.startAddr + sym->second.byteOffset);

		Log(NLog_Level::Extended, "Resolving symbol", rr.symbol, "to", addr);

		mSections[rr.section][rr.instructionIndex]->Resolve_Symbol(addr);
	}

	return true;
}

bool CAssembler::Generate_Binary() {

	Log(NLog_Level::Basic, "Generating output...");

	// create object file
	SObj::CSObj_File output;

	// put all sections
	for (auto& s : mSections) {

		std::vector<uint8_t> dest;

		// generate all instruction and pseudoinstruction-related data
		for (auto& instr : s.second) {

			// is pseudo-instruction? ask for the additional data (as it might not be aligned to 4 bytes)
			if (instr->Is_Pseudo_Instruction()) {
				instr->Generate_Additional_Data(dest);
			}
			else {
				Word_To_Bytes(instr->Generate_Binary(), dest);
			}

		}

		// place into section
		output.Put_To_Section(s.first, dest);

		// TODO: check section limit
	}

	// relocate all sections (just set starting address to allow loader to put it to a correct place in memory)
	for (auto& sr : mLinker_Section_Defs) {
		output.Relocate_Section(sr.second.section, sr.second.startAddr);
	}

	return output.Save_To_File(mInput.Output_File);
}

bool CAssembler::Assemble() {

	// 1) load linker file
	if (!Load_Linker_File(mInput.Linker_File)) {
		std::cerr << "Could not load linker file" << std::endl;
		return false;
	}

	// 2) assemble all input files
	for (auto& inputFile : mInput.Input_Files) {
		bool result = Assemble_File(inputFile);
		if (!result)
			return false;
	}

	// 3) resolve all symbols (basically "link")
	if (!Resolve_Symbols()) {
		return false;
	}

	// 4) generate output binary
	if (!Generate_Binary()) {
		return false;
	}

	return true;
}
