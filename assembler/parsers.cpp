#include "assembler.h"

#include <regex>

bool CAssembler::Parse_Section_Directive(const std::string& line, std::string& sectionName) const {

	// regex that matches a section directive (e.g., ".section abcd"), possibly containing a comment
	std::regex sectionregex{ "^[\\s]{0,}\\.section[\\s]*([a-zA-Z]+)[\\s]*(;.*)?$", std::regex::ECMAScript };

	std::smatch sm;
	if (!std::regex_match(line, sm, sectionregex) || sm.size() < 2)
		return false;

	sectionName = sm[1];
	return true;
}

bool CAssembler::Parse_Label_Directive(const std::string& line, std::string& labelName) const {

	// regex that matches a label directive (e.g., "$label_abcd:"), possibly containing a comment
	std::regex labelregex{ "^[\\s]{0,}\\$([a-zA-Z0-9_]+):[\\s]*(;.*)?$", std::regex::ECMAScript };

	std::smatch sm;
	if (!std::regex_match(line, sm, labelregex) || sm.size() < 2)
		return false;

	labelName = sm[1];
	return true;
}

std::unique_ptr<CInstruction> CAssembler::Parse_Pseudo_Instruction(const std::string& line) const {

	// regex that matches a data directive (e.g., "db #12", "dw #12345", "dw $otherlabel"), possibly containing a comment
	std::regex dbregex{ "^[\\s]{0,}(db|dw)[\\s]{0,}(#[\\-0x]{0,3}[0-9a-fA-F]+|\\$[a-zA-Z0-9_]+)[\\s]{0,}(;.*)?$", std::regex::ECMAScript };
	// regex that matches a string data directive (e.g., "asciz 'hello world'"), possibly containing a comment
	std::regex strregex{ "^[\\s]{0,}(asciz)[\\s]{0,}('[^']*')[\\s]{0,}(;.*)?$", std::regex::ECMAScript };

	std::smatch sm;

	// match data directive
	if (std::regex_match(line, sm, dbregex) && sm.size() >= 3) {

		std::unique_ptr<CPseudo_Instruction_Data> instr = std::make_unique<CPseudo_Instruction_Data>();
		if (!instr->Parse_String(sm[1], std::vector<std::string>{ sm[2] }))
			return nullptr;

		return instr;
	}

	// match string directive
	if (std::regex_match(line, sm, strregex) && sm.size() >= 3) {

		std::unique_ptr<CPseudo_Instruction_Data> instr = std::make_unique<CPseudo_Instruction_Data>();
		if (!instr->Parse_String(sm[1], std::vector<std::string>{ sm[2] }))
			return nullptr;

		return instr;
	}

	return nullptr;
}
