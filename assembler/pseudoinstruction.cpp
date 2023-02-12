#include "pseudoinstruction.h"

#include <iostream>

bool CPseudo_Instruction_Data::Parse_String(const std::string& mnemonic, const std::vector<std::string>& operands) {

	mMnemonic = mnemonic;

	// parse desired length, lower and upper limits
	size_t desiredOpLen = 1;
	int32_t lowerLimit = -0x7FFFFFFF;
	int32_t upperLimit = 0x7FFFFFFF;
	if (mnemonic == "db" || mnemonic == "asciz") {
		desiredOpLen = 1;
		lowerLimit = -128;
		upperLimit = 127;
	}
	else if (mnemonic == "dw") {
		desiredOpLen = 4;
	}

	// for each operand, store to binary data dump
	for (auto& op : operands) {

		// string - copy to data as-is
		if (mnemonic == "asciz") {

			if (!op.starts_with('\'') || !op.ends_with('\'') || op.size() < 2) {
				std::cerr << "Could not parse string identifier" << std::endl;
				return false;
			}

			for (size_t i = 1; i < op.size() - 1; i++) {
				Store<1>(op[i]);
			}

		}
		// byte or word
		else {

			auto r = Parse_Any(op);
			if (r.Is_Immediate()) {
				auto imm = r.Get_Immediate();

				// TODO: check limits

				if (desiredOpLen == 1)
					Store<1>(imm);
				else if (desiredOpLen == 4)
					Store<4>(imm);
			}
			else if (r.Is_Symbolic()) {

				if (operands.size() == 1) {
					if (desiredOpLen == 1)
						Store<1>(0);
					else if (desiredOpLen == 4)
						Store<4>(0);

					mSymbolic_Req = r.Get_Symbol();
				}
				else {
					std::cerr << "Could not include symbolic in multiparam context" << std::endl;
					return false;
				}

			}
		}
	}

	// asciz -> terminate string with zero character
	if (mnemonic == "asciz")
		mData.push_back('\0');

	return true;
}

std::string CPseudo_Instruction_Data::Generate_String(bool hexaFmt) const {

	// determine desired length
	size_t desiredOpLen = 1;
	if (mMnemonic == "db") {
		desiredOpLen = 1;
	}
	else if (mMnemonic == "dw") {
		desiredOpLen = 4;
	}
	else if (mMnemonic == "asciz") {

		// generating string has a different flow - just dump characters into single quotes
		std::string generated = mMnemonic + " '";
		for (size_t i = 0; i < mData.size(); i++) {
			generated += static_cast<char>(mData[i]);
		}
		generated += "'";
		return generated;
	}

	// start at mnemonic
	std::string generated = mMnemonic;
	bool firstArg = false;

	// generate data dump
	size_t cur = 0;
	while (cur < mData.size()) {

		int32_t imm;

		if (desiredOpLen == 1)
			imm = Restore<1>(cur);
		else if (desiredOpLen == 4)
			imm = Restore<4>(cur);

		if (firstArg)
			generated += ",";
		else
			firstArg = true;

		generated += " #" + std::to_string(imm);

		cur += desiredOpLen;
	}

	return generated;
}

void CPseudo_Instruction_Data::Resolve_Symbol(int32_t value) {

	if (mSymbolic_Req.size() > 0) {
		mData.clear();

		if (mMnemonic == "db") {
			Store<1>(value);
		}
		else if (mMnemonic == "dw") {
			Store<4>(value);
		}
	}
}
