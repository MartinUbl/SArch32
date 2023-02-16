#include "isa.h"

#include <iostream>
#include <regex>
#include <algorithm>
#include <cctype>
#include <string>
#include <variant>
#include <sstream>

// conversion map from string to register
const std::unordered_map<std::string, NRegister> Mnemonic_To_Register = {
	{ "r0",  NRegister::R0 },
	{ "r1",  NRegister::R1 },
	{ "r2",  NRegister::R2 },
	{ "r3",  NRegister::R3 },
	{ "r4",  NRegister::R4 },
	{ "r5",  NRegister::R5 },
	{ "r6",  NRegister::R6 },
	{ "r7",  NRegister::R7 },
	{ "r8",  NRegister::R8 },
	{ "r9",  NRegister::R9 },
	{ "r10", NRegister::R10 },
	{ "r11", NRegister::R11 },
	{ "sp",  NRegister::SP },
	{ "ra",  NRegister::RA },
	{ "flg", NRegister::FLG },
	{ "pc",  NRegister::PC },
};

// conversion map for register to string mnemonic
const std::unordered_map<NRegister, std::string> Register_To_Mnemonic = {
	{ NRegister::R0, "r0" },
	{ NRegister::R1, "r1" },
	{ NRegister::R2, "r2" },
	{ NRegister::R3, "r3" },
	{ NRegister::R4, "r4" },
	{ NRegister::R5, "r5" },
	{ NRegister::R6, "r6" },
	{ NRegister::R7, "r7" },
	{ NRegister::R8, "r8" },
	{ NRegister::R9, "r9" },
	{ NRegister::R10, "r10" },
	{ NRegister::R11, "r11" },
	{ NRegister::SP, "sp" },
	{ NRegister::RA, "ra" },
	{ NRegister::FLG, "flg" },
	{ NRegister::PC, "pc" },
};

// conversion map from condition mnemonic to condition enum
const std::unordered_map<std::string, NCondition> Mnemonic_To_Cond = {
	{ "al", NCondition::always },
	{ "eq", NCondition::equal },
	{ "ne", NCondition::not_equal },
	{ "gt", NCondition::greater },
	{ "ge", NCondition::greater_equal },
	{ "lt", NCondition::less },
	{ "le", NCondition::less_equal },
};

// conversion map from condition enum value to mnemonic
const std::unordered_map<NCondition, std::string> Cond_To_Mnemonic = {
	{ NCondition::always, "al" },
	{ NCondition::equal, "eq" },
	{ NCondition::not_equal, "ne" },
	{ NCondition::greater, "gt" },
	{ NCondition::greater_equal, "ge" },
	{ NCondition::less, "lt" },
	{ NCondition::less_equal, "le" },
};

// conversion map from opcode mnemonic to enum value
const std::unordered_map<std::string, NOpcode> Mnemonic_To_Opcode = {
	{ "nop",  NOpcode::nop },
	{ "mov",  NOpcode::mov },
	{ "movi", NOpcode::movi },
	{ "add",  NOpcode::add },
	{ "addi", NOpcode::addi },
	{ "sub",  NOpcode::sub },
	{ "subi", NOpcode::subi },
	{ "mul",  NOpcode::mul },
	{ "muli", NOpcode::muli },
	{ "div",  NOpcode::div },
	{ "divi", NOpcode::divi },
	{ "and",  NOpcode::and_ },
	{ "andi", NOpcode::andi },
	{ "or",   NOpcode::or_ },
	{ "ori",  NOpcode::ori },
	{ "slr",  NOpcode::slr },
	{ "sli",  NOpcode::sli },
	{ "srr",  NOpcode::srr },
	{ "sri",  NOpcode::sri },
	{ "lw",   NOpcode::lw },
	{ "li",   NOpcode::li },
	{ "sw",   NOpcode::sw },
	{ "si",   NOpcode::si },
	{ "cmpr", NOpcode::cmpr },
	{ "cmpi", NOpcode::cmpi },
	{ "br",   NOpcode::br },
	{ "bi",   NOpcode::bi },
	{ "brr",   NOpcode::br },
	{ "bir",   NOpcode::bi },
	{ "push", NOpcode::push },
	{ "pop",  NOpcode::pop },
	{ "fw",   NOpcode::fw },
	{ "svc",  NOpcode::svc },
	{ "aps",  NOpcode::aps },
};

// conversion map from opcode enum value to mnemonic string
const std::unordered_map<NOpcode, std::string> Opcode_To_Mnemonic = {
	{ NOpcode::nop, "nop" },
	{ NOpcode::mov, "mov" },
	{ NOpcode::movi, "movi" },
	{ NOpcode::add, "add" },
	{ NOpcode::addi, "addi" },
	{ NOpcode::sub, "sub" },
	{ NOpcode::subi, "subi" },
	{ NOpcode::mul, "mul" },
	{ NOpcode::muli, "muli" },
	{ NOpcode::div, "div" },
	{ NOpcode::divi, "divi" },
	{ NOpcode::and_, "and" },
	{ NOpcode::andi, "andi" },
	{ NOpcode::or_, "or" },
	{ NOpcode::ori, "ori" },
	{ NOpcode::slr, "slr" },
	{ NOpcode::sli, "sli" },
	{ NOpcode::srr, "srr" },
	{ NOpcode::sri, "sri" },
	{ NOpcode::lw, "lw" },
	{ NOpcode::li, "li" },
	{ NOpcode::sw, "sw" },
	{ NOpcode::si, "si" },
	{ NOpcode::cmpr, "cmpr" },
	{ NOpcode::cmpi, "cmpi" },
	{ NOpcode::br, "br" },
	{ NOpcode::bi, "bi" },
	// NOTE: brr and bir are generated from br and bi dynamically as they are the same instructions with different flags
	{ NOpcode::push, "push" },
	{ NOpcode::pop, "pop" },
	{ NOpcode::fw, "fw" },
	{ NOpcode::svc, "svc" },
	{ NOpcode::aps, "aps" },
};

// retrieves register name using its enum value
std::string Get_Register_Name(NRegister reg) {
	auto itr = Register_To_Mnemonic.find(reg);
	return itr != Register_To_Mnemonic.end() ? itr->second : "";
}

// retrieves register name using its index
std::string Get_Register_Name(size_t regIdx) {
	return Get_Register_Name(static_cast<NRegister>(regIdx));
}

// validates argument count in instruction parser context
inline bool Validate_Argc(const std::vector<std::string>& operands, size_t cnt) {
	if (operands.size() == cnt)
		return true;
	throw sarch32_parser_exception{ "Invalid number of operands" };
}

// parse register name from string, throw exception if not found
static NRegister Parse_Register(const std::string& str) {
	auto itr = Mnemonic_To_Register.find(str);
	if (itr == Mnemonic_To_Register.end())
		throw sarch32_parser_exception{ "Unknown register: " + str };

	return itr->second;
}

// attempt to parse register, return invalid value if not found
static NRegister Try_Parse_Register(const std::string& str) {
	auto itr = Mnemonic_To_Register.find(str);
	if (itr == Mnemonic_To_Register.end())
		return NRegister::count;

	return itr->second;
}

// attempt to parse immediate value (dec/hex)
static bool Try_Parse_Immediate(const std::string& str, int32_t& imm) {
	// must start with the # character
	if (!str.starts_with('#'))
		return false;

	// cut off the #
	auto sub = str.substr(1);
	try {

		// is hexa?
		if (sub.starts_with("0x") || sub.starts_with("-0x")) {

			// is negative? (contains minus sign?)
			bool neg = false;
			if (sub[0] == '-') {
				neg = true;
				sub = sub.substr(3);
			}
			else {
				sub = sub.substr(2);
			}

			// try to parse hex number
			imm = static_cast<int32_t>(std::stoull(sub, nullptr, 16));
			// if negative, switch sign
			if (neg)
				imm *= -1;
		}
		// is dec
		else {
			imm = static_cast<int32_t>(std::stol(sub));
		}
	}
	// this means the string does not represent a number --> error
	catch (...)
	{
		return false;
	}

	return true;
}

// tries to parse symbolic reference from string
static bool Try_Parse_Symbolic(const std::string& str, std::string& sym) {
	// must start with $ sign and must contain at least 1 other character
	if (!str.starts_with('$') || str.size() < 2)
		return false;

	sym = str.substr(1);

	return true;
}

// parses an operand from string (immediate, symbolic, register)
COperand Parse_Any(const std::string& str) {
	NRegister reg = Try_Parse_Register(str);
	if (reg != NRegister::count)
		return reg;

	int32_t imm;
	if (Try_Parse_Immediate(str, imm))
		return imm;

	std::string sym;
	if (Try_Parse_Symbolic(str, sym))
		return sym;

	throw sarch32_parser_exception{ "Could not parse value: " + str };
}

// converts a single word to byte dump
void Word_To_Bytes(uint32_t word, std::vector<uint8_t>& res) {

	union {
		uint8_t bytes[sizeof(word)];
		uint32_t orig;
	} dump;

	dump.orig = word;
	std::copy_n(&dump.bytes[0], sizeof(word), std::back_inserter(res));
}

// formats number for output - this should be compliant with the parser (strings generated by this should be parsed fine)
std::string formatNum(int32_t num, bool hexaFmt) {
	std::ostringstream oss;
	if (hexaFmt)
		oss << std::hex << (num < 0 ? "-" : "") << "0x" << std::abs(num);
	else
		oss << std::dec << num;
	return oss.str();
}

/*
 * NOP instruction class
 */
class CInstruction_Nop : public CInstruction
{
	public:
		using CInstruction::CInstruction;

		virtual bool Parse_String(const std::string& mnemonic, const std::vector<std::string>& operands) override {
			return Validate_Argc(operands, 0);
		};
		virtual bool Parse_Binary(const uint32_t instruction) override {
			return true; // opcode was parsed by parser, rest is ignored
		};
		virtual std::string Generate_String(bool hexaFmt) const override {
			return Opcode_To_Mnemonic.find(Get_Opcode())->second;
		};
		virtual uint32_t Generate_Binary() const override {
			return Encode_From_Bytes({ Encode_MSB(), 0, 0, 0 });
		};

		virtual bool Execute(CCPU_Context& cpu) const override {
			// NOP = literally do nothing
			return true;
		}

		virtual void Resolve_Symbol(int32_t value) override {
			//
		}
};

/*
 * Generic instruction with 2 parameters
 * 
 * First attribute is always a register, second is either immediate argument, symbolic (later resolved to immediate) or register
 */
class CInstruction_Generic_2Param : public CInstruction
{
	protected:
		// two parameters - dst (first) and src (second)
		COperand mDst, mSrc;

	public:
		using CInstruction::CInstruction;

		virtual bool Parse_String(const std::string& mnemonic, const std::vector<std::string>& operands) override {
			Validate_Argc(operands, 2);

			// dst is always a register
			mDst = Parse_Register(operands[0]);
			// src is any of supported ones
			mSrc = Parse_Any(operands[1]);

			// validate type

			if (mSrc.Is_Register() && Is_Immediate_Instruction() && mOpcode != NOpcode::aps)
				throw sarch32_parser_exception{ "Invalid parameter for instruction " + mnemonic + " - should be immediate value or symbol" };

			if (!mSrc.Is_Register() && !Is_Immediate_Instruction() && mOpcode != NOpcode::aps)
				throw sarch32_parser_exception{ "Invalid parameter for instruction " + mnemonic + " - should be register" };

			if (mOpcode == NOpcode::aps && mSrc.Is_Register())
				throw sarch32_parser_exception{ "Invalid parameter for instruction " + mnemonic + " - should be immediate value or symbol" };

			return true;
		};

		virtual bool Parse_Binary(const uint32_t instruction) override {
			auto bytes = Decode_From_Bytes(instruction);

			if (!Is_Immediate_Instruction() && mOpcode != NOpcode::aps) {
				auto pr = Decode_Register_Pair(bytes[1]);
				mDst = pr.first;
				mSrc = pr.second;
			}
			else {
				auto pr = Decode_Register_Pair(bytes[1]);
				mDst = pr.first;
				mSrc = Decode_Immediate(instruction);
			}

			return true;
		};

		virtual std::string Generate_String(bool hexaFmt) const override {
			return Opcode_To_Mnemonic.find(Get_Opcode())->second + (mCondition == NCondition::always ? "" : ("."+Cond_To_Mnemonic.find(Get_Condition())->second)) + " " + Register_To_Mnemonic.find(mDst.Get_Register())->second + ", " +
				(mSrc.Is_Immediate()
					?
					"#" + formatNum(mSrc.Get_Immediate(), hexaFmt)
					:
					(mSrc.Is_Symbolic() ? "$" + mSrc.Get_Symbol() : Register_To_Mnemonic.find(mSrc.Get_Register())->second));
		};

		virtual uint32_t Generate_Binary() const override {

			if (mSrc.Is_Immediate()) {
				if (mSrc.Get_Immediate() > std::numeric_limits<int16_t>::max() || mSrc.Get_Immediate() < std::numeric_limits<int16_t>::min())
					throw sarch32_generator_exception{ "Immediate argument out of range" };
				return Encode_From_Byte_Half(Encode_MSB(), Encode_Register_Pair(mDst.Get_Register(), NRegister::ignored), mSrc.Get_Immediate());
			}
			else if (mSrc.Is_Symbolic()) {
				return Encode_From_Bytes({ Encode_MSB(), Encode_Register_Pair(NRegister::ignored, NRegister::ignored), 0, 0 });
			}
			else {
				return Encode_From_Bytes({ Encode_MSB(), Encode_Register_Pair(mDst.Get_Register(), mSrc.Get_Register()), 0, 0 });
			}
		};

		virtual void Resolve_Symbol(int32_t value) override {
			if (mSrc.Is_Symbolic()) {
				mSrc.Resolve_Symbol(value);
			}
		}

		virtual bool Get_Resolve_Request(std::string& str) const override {
			if (mSrc.Is_Symbolic()) {
				str = mSrc.Get_Symbol();
				return true;
			}

			return false;
		}
};

/*
 * Generic instruction with a single parameter
 */
class CInstruction_Generic_1Param : public CInstruction
{
	protected:
		COperand mSrc;

	public:
		using CInstruction::CInstruction;

		virtual bool Parse_String(const std::string& mnemonic, const std::vector<std::string>& operands) override {
			Validate_Argc(operands, 1);

			mSrc = Parse_Any(operands[0]);

			if (mSrc.Is_Register() && Is_Immediate_Instruction())
				throw sarch32_parser_exception{ "Invalid parameter for instruction " + mnemonic + " - should be immediate value or symbol" };

			if (!mSrc.Is_Register() && !Is_Immediate_Instruction())
				throw sarch32_parser_exception{ "Invalid parameter for instruction " + mnemonic + " - should be register" };

			return true;
		};

		virtual bool Parse_Binary(const uint32_t instruction) override {
			auto bytes = Decode_From_Bytes(instruction);

			if (!Is_Immediate_Instruction()) {
				auto pr = Decode_Register_Pair(bytes[1]);
				mSrc = pr.second;
			}
			else {
				auto pr = Decode_Register_Pair(bytes[1]);
				mSrc = Decode_Immediate(instruction);
			}

			return true;
		};

		virtual std::string Generate_String(bool hexaFmt) const override {
			return Opcode_To_Mnemonic.find(Get_Opcode())->second + (mCondition == NCondition::always ? "" : ("." + Cond_To_Mnemonic.find(Get_Condition())->second)) + " " +
				(mSrc.Is_Immediate()
					?
					"#" + formatNum(mSrc.Get_Immediate(), hexaFmt)
					:
					(mSrc.Is_Symbolic() ? "$" + mSrc.Get_Symbol() : Register_To_Mnemonic.find(mSrc.Get_Register())->second));
		};

		virtual uint32_t Generate_Binary() const override {
			if (mSrc.Is_Immediate()) {
				if (mSrc.Get_Immediate() > std::numeric_limits<int16_t>::max() || mSrc.Get_Immediate() < std::numeric_limits<int16_t>::min())
					throw sarch32_generator_exception{ "Immediate argument out of range" };
				return Encode_From_Byte_Half(Encode_MSB(), Encode_Register_Pair(NRegister::ignored, NRegister::ignored), mSrc.Get_Immediate());
			}
			else if (mSrc.Is_Symbolic()) {
				return Encode_From_Bytes({ Encode_MSB(), Encode_Register_Pair(NRegister::ignored, NRegister::ignored), 0, 0 });
			}
			else {
				return Encode_From_Bytes({ Encode_MSB(), Encode_Register_Pair(NRegister::ignored, mSrc.Get_Register()), 0, 0 });
			}
		};

		virtual void Resolve_Symbol(int32_t value) override {
			if (mSrc.Is_Symbolic()) {
				mSrc.Resolve_Symbol(value);
			}
		}

		virtual bool Get_Resolve_Request(std::string& str) const override {
			if (mSrc.Is_Symbolic()) {
				str = mSrc.Get_Symbol();
				return true;
			}

			return false;
		}
};

/*
 * Generic instruction with a single parameter, but specified parameter type
 */
template<NOperand_Type NType>
class CInstruction_1Param : public CInstruction
{
	protected:
		COperand mSrc;

	public:
		using CInstruction::CInstruction;

		virtual bool Parse_String(const std::string& mnemonic, const std::vector<std::string>& operands) override {
			Validate_Argc(operands, 1);

			mSrc = Parse_Any(operands[0]);

			if (mSrc.Is_Register() && NType != NOperand_Type::Register)
				throw sarch32_parser_exception{ "Invalid parameter for instruction " + mnemonic + " - should be immediate value or symbol" };

			if (!mSrc.Is_Register() && NType == NOperand_Type::Register)
				throw sarch32_parser_exception{ "Invalid parameter for instruction " + mnemonic + " - should be register" };

			return true;
		};

		virtual bool Parse_Binary(const uint32_t instruction) override {
			auto bytes = Decode_From_Bytes(instruction);

			if (NType == NOperand_Type::Register) {
				auto pr = Decode_Register_Pair(bytes[1]);
				mSrc = pr.second;
			}
			else {
				mSrc = Decode_Immediate_24b(instruction);
			}

			return true;
		};

		virtual std::string Generate_String(bool hexaFmt) const override {
			return Opcode_To_Mnemonic.find(Get_Opcode())->second + (mCondition == NCondition::always ? "" : ("." + Cond_To_Mnemonic.find(Get_Condition())->second)) + " " +
				(mSrc.Is_Immediate()
					?
					"#" + formatNum(mSrc.Get_Immediate(), hexaFmt)
					:
					(mSrc.Is_Symbolic() ? "$" + mSrc.Get_Symbol() : Register_To_Mnemonic.find(mSrc.Get_Register())->second));
		};

		virtual uint32_t Generate_Binary() const override {
			if (mSrc.Is_Immediate()) {

				constexpr int32_t toplimit24b = 0x7FFFFF;
				constexpr int32_t bottomlimit24b = -0x800000;

				if (mSrc.Get_Immediate() > toplimit24b || mSrc.Get_Immediate() < bottomlimit24b)
					throw sarch32_generator_exception{ "Immediate argument out of range" };
				return Encode_From_Byte_Extended(Encode_MSB(), mSrc.Get_Immediate());
			}
			else if (mSrc.Is_Symbolic()) {
				return Encode_From_Bytes({ Encode_MSB(), Encode_Register_Pair(NRegister::ignored, NRegister::ignored), 0, 0 });
			}
			else {
				return Encode_From_Bytes({ Encode_MSB(), Encode_Register_Pair(NRegister::ignored, mSrc.Get_Register()), 0, 0 });
			}
		};

		virtual void Resolve_Symbol(int32_t value) override {
			if (mSrc.Is_Symbolic()) {
				mSrc.Resolve_Symbol(value);
			}
		}

		virtual bool Get_Resolve_Request(std::string& str) const override {
			if (mSrc.Is_Symbolic()) {
				str = mSrc.Get_Symbol();
				return true;
			}

			return false;
		}
};

/*
 * MOV instruction class
 */
class CInstruction_Mov : public CInstruction_Generic_2Param {
	public:
		using CInstruction_Generic_2Param::CInstruction_Generic_2Param;

		virtual bool Execute(CCPU_Context& cpu) const override {

			if (!mDst.Is_Register() || !(mSrc.Is_Immediate() || mSrc.Is_Register()))
				return false;

			if (!Check_Condition(mCondition, cpu))
				return true;

			cpu.Reg(mDst.Get_Register()) =
				(mSrc.Is_Immediate() ?
					mSrc.Get_Immediate()
					:
					cpu.Reg(mSrc.Get_Register()));

			return true;
		}
};

/*
 * ADD instruction class
 */
class CInstruction_Add : public CInstruction_Generic_2Param {
	public:
		using CInstruction_Generic_2Param::CInstruction_Generic_2Param;

		virtual bool Execute(CCPU_Context& cpu) const override {

			if (!mDst.Is_Register() || !(mSrc.Is_Immediate() || mSrc.Is_Register()))
				return false;

			if (!Check_Condition(mCondition, cpu))
				return true;

			cpu.Reg(mDst.Get_Register()) =
				cpu.Reg(mDst.Get_Register()) +
				(mSrc.Is_Immediate() ?
					mSrc.Get_Immediate()
					:
					cpu.Reg(mSrc.Get_Register()));

			return true;
		}
};

/*
 * SUB instruction class
 */
class CInstruction_Sub : public CInstruction_Generic_2Param {
	public:
		using CInstruction_Generic_2Param::CInstruction_Generic_2Param;

		virtual bool Execute(CCPU_Context& cpu) const override {

			if (!mDst.Is_Register() || !(mSrc.Is_Immediate() || mSrc.Is_Register()))
				return false;

			if (!Check_Condition(mCondition, cpu))
				return true;

			cpu.Reg(mDst.Get_Register()) =
				cpu.Reg(mDst.Get_Register()) -
				(mSrc.Is_Immediate() ?
					mSrc.Get_Immediate()
					:
					cpu.Reg(mSrc.Get_Register()));

			return true;
		}
};

/*
 * MUL instruction class
 */
class CInstruction_Mul : public CInstruction_Generic_2Param {
	public:
		using CInstruction_Generic_2Param::CInstruction_Generic_2Param;

		virtual bool Execute(CCPU_Context& cpu) const override {

			if (!mDst.Is_Register() || !(mSrc.Is_Immediate() || mSrc.Is_Register()))
				return false;

			if (!Check_Condition(mCondition, cpu))
				return true;

			cpu.Reg(mDst.Get_Register()) =
				cpu.Reg(mDst.Get_Register()) *
				(mSrc.Is_Immediate() ?
					mSrc.Get_Immediate()
					:
					cpu.Reg(mSrc.Get_Register()));

			return true;
		}
};

/*
 * DIV instruction class
 */
class CInstruction_Div : public CInstruction_Generic_2Param {
	public:
		using CInstruction_Generic_2Param::CInstruction_Generic_2Param;

		virtual bool Execute(CCPU_Context& cpu) const override {

			if (!mDst.Is_Register() || !(mSrc.Is_Immediate() || mSrc.Is_Register()))
				return false;

			if (!Check_Condition(mCondition, cpu))
				return true;

			const auto op2 = mSrc.Is_Immediate() ?
				mSrc.Get_Immediate()
				:
				cpu.Reg(mSrc.Get_Register());

			// division by zero
			if (op2 == 0) {
				// TODO: exception
				return false;
			}

			cpu.Reg(mDst.Get_Register()) =
				cpu.Reg(mDst.Get_Register()) /
				op2;

			return true;
		}
};

/*
 * AND instruction class
 */
class CInstruction_And : public CInstruction_Generic_2Param {
	public:
		using CInstruction_Generic_2Param::CInstruction_Generic_2Param;

		virtual bool Execute(CCPU_Context& cpu) const override {

			if (!mDst.Is_Register() || !(mSrc.Is_Immediate() || mSrc.Is_Register()))
				return false;

			if (!Check_Condition(mCondition, cpu))
				return true;

			cpu.Reg(mDst.Get_Register()) =
				cpu.Reg(mDst.Get_Register()) &
				(mSrc.Is_Immediate() ?
					mSrc.Get_Immediate()
					:
					cpu.Reg(mSrc.Get_Register()));

			return true;
		}
};

/*
 * OR instruction class
 */
class CInstruction_Or : public CInstruction_Generic_2Param {
	public:
		using CInstruction_Generic_2Param::CInstruction_Generic_2Param;

		virtual bool Execute(CCPU_Context& cpu) const override {

			if (!mDst.Is_Register() || !(mSrc.Is_Immediate() || mSrc.Is_Register()))
				return false;

			if (!Check_Condition(mCondition, cpu))
				return true;

			cpu.Reg(mDst.Get_Register()) =
				cpu.Reg(mDst.Get_Register()) |
				(mSrc.Is_Immediate() ?
					mSrc.Get_Immediate()
					:
					cpu.Reg(mSrc.Get_Register())
				);

			return true;
		}
};

/*
 * SHL instruction class
 */
class CInstruction_ShiftLeft : public CInstruction_Generic_2Param {
	public:
		using CInstruction_Generic_2Param::CInstruction_Generic_2Param;

		virtual bool Execute(CCPU_Context& cpu) const override {

			if (!mDst.Is_Register() || !(mSrc.Is_Immediate() || mSrc.Is_Register()))
				return false;

			if (!Check_Condition(mCondition, cpu))
				return true;

			cpu.Reg(mDst.Get_Register()) =
				cpu.Reg(mDst.Get_Register()) <<
				(mSrc.Is_Immediate() ?
					mSrc.Get_Immediate()
					:
					cpu.Reg(mSrc.Get_Register()));

			return true;
		}
};

/*
 * SHR instruction class
 */
class CInstruction_ShiftRight : public CInstruction_Generic_2Param {
	public:
		using CInstruction_Generic_2Param::CInstruction_Generic_2Param;

		virtual bool Execute(CCPU_Context& cpu) const override {

			if (!mDst.Is_Register() || !(mSrc.Is_Immediate() || mSrc.Is_Register()))
				return false;

			if (!Check_Condition(mCondition, cpu))
				return true;

			cpu.Reg(mDst.Get_Register()) =
				cpu.Reg(mDst.Get_Register()) >>
				(mSrc.Is_Immediate() ?
					mSrc.Get_Immediate()
					:
					cpu.Reg(mSrc.Get_Register()));

			return true;
		}
};

/*
 * LW instruction class
 */
class CInstruction_LoadWord : public CInstruction_Generic_2Param {
	public:
		using CInstruction_Generic_2Param::CInstruction_Generic_2Param;

		virtual bool Execute(CCPU_Context& cpu) const override {

			if (!mDst.Is_Register() || !(mSrc.Is_Immediate() || mSrc.Is_Register()))
				return false;

			if (!Check_Condition(mCondition, cpu))
				return true;

			cpu.Reg(mDst.Get_Register()) =
				cpu.Mem_Read_Scalar<uint32_t>(mSrc.Is_Immediate() ?
					mSrc.Get_Immediate()
					:
					cpu.Reg(mSrc.Get_Register()));

			return true;
		}
};

/*
 * SW instruction class
 */
class CInstruction_StoreWord : public CInstruction_Generic_2Param {
	public:
		using CInstruction_Generic_2Param::CInstruction_Generic_2Param;

		virtual bool Execute(CCPU_Context& cpu) const override {

			if (!mDst.Is_Register() || !(mSrc.Is_Immediate() || mSrc.Is_Register()))
				return false;

			if (!Check_Condition(mCondition, cpu))
				return true;

			cpu.Mem_Write_Scalar(mSrc.Is_Immediate() ?
				mSrc.Get_Immediate()
				:
				cpu.Reg(mSrc.Get_Register()), cpu.Reg(mDst.Get_Register()));

			return true;
		}
};

/*
 * CMP instruction class
 */
class CInstruction_Compare : public CInstruction_Generic_2Param {
	public:
		using CInstruction_Generic_2Param::CInstruction_Generic_2Param;

		virtual bool Execute(CCPU_Context& cpu) const override {

			if (!mDst.Is_Register() || !(mSrc.Is_Immediate() || mSrc.Is_Register()))
				return false;

			if (!Check_Condition(mCondition, cpu))
				return true;

			const int32_t r1 = std::bit_cast<int32_t>(cpu.Reg(mDst.Get_Register()));
			const int32_t r2 = std::bit_cast<int32_t>(mSrc.Is_Immediate() ? mSrc.Get_Immediate() : cpu.Reg(mSrc.Get_Register()));

			const int32_t result = r1 - r2;

			auto& flg = cpu.Reg(NRegister::FLG);
			auto setFlag = [&flg](NFlags flag, bool set = true) {
				if (set)
					flg |= static_cast<uint32_t>(flag);
				else
					flg &= ~static_cast<uint32_t>(flag);
			};

			setFlag(NFlags::Zero, (result == 0));
			setFlag(NFlags::Sign, (result < 0));
			setFlag(NFlags::Overflow, (r1 > r2 && result > r1));  // ?

			return true;
		}
};

/*
 * B instruction class
 */
class CInstruction_Branch : public CInstruction_Generic_1Param {

	private:
		bool mIs_Relative = false;

	public:
		using CInstruction_Generic_1Param::CInstruction_Generic_1Param;

		virtual bool Parse_String(const std::string& mnemonic, const std::vector<std::string>& operands) override {

			if (!CInstruction_Generic_1Param::Parse_String(mnemonic, operands)) {
				return false;
			}

			mIs_Relative = (mnemonic == "brr" || mnemonic == "bir");
			return true;
		}

		virtual bool Parse_Binary(const uint32_t instruction) override {

			if (!CInstruction_Generic_1Param::Parse_Binary(instruction)) {
				return false;
			}

			mIs_Relative = ((instruction >> 8) & 0xFF) == 0xFF;

			return true;
		}

		virtual std::string Generate_String(bool hexaFmt) const override {

			auto opMnemonic = Opcode_To_Mnemonic.find(Get_Opcode())->second + (mIs_Relative ? "r" : "");

			return opMnemonic + (mCondition == NCondition::always ? "" : ("." + Cond_To_Mnemonic.find(Get_Condition())->second)) + " " +
				(mSrc.Is_Immediate()
					?
					"#" + formatNum(mSrc.Get_Immediate(), hexaFmt)
					:
					(mSrc.Is_Symbolic() ? "$" + mSrc.Get_Symbol() : Register_To_Mnemonic.find(mSrc.Get_Register())->second));
		};

		virtual uint32_t Generate_Binary() const override {
			uint32_t bin = CInstruction_Generic_1Param::Generate_Binary();

			if (mIs_Relative)
				bin |= (0xFF << 8);

			return bin;
		}

		virtual bool Execute(CCPU_Context& cpu) const override {

			if (!mSrc.Is_Immediate() && !mSrc.Is_Register())
				return false;

			if (!Check_Condition(mCondition, cpu))
				return true;

			const auto to = mSrc.Is_Immediate() ? mSrc.Get_Immediate() : cpu.Reg(mSrc.Get_Register());

			cpu.Reg(NRegister::PC) = (mIs_Relative ? (cpu.Reg(NRegister::PC) + to) : to);

			return true;
		}
};

/*
 * PUSH instruction class
 */
class CInstruction_Push : public CInstruction_1Param<NOperand_Type::Register> {
	public:
		using CInstruction_1Param<NOperand_Type::Register>::CInstruction_1Param;

		virtual bool Execute(CCPU_Context& cpu) const override {

			if (!mSrc.Is_Register())
				return false;

			if (!Check_Condition(mCondition, cpu))
				return true;

			cpu.Mem_Write_Scalar<uint32_t>(cpu.Reg(NRegister::SP) - 4, cpu.Reg(mSrc.Get_Register()));
			cpu.Reg(NRegister::SP) -= 4;

			return true;
		}
};

/*
 * POP instruction class
 */
class CInstruction_Pop : public CInstruction_1Param<NOperand_Type::Register> {
	public:
		using CInstruction_1Param<NOperand_Type::Register>::CInstruction_1Param;

		virtual bool Execute(CCPU_Context& cpu) const override {

			if (!mSrc.Is_Register())
				return false;

			if (!Check_Condition(mCondition, cpu))
				return true;

			cpu.Reg(mSrc.Get_Register()) = cpu.Mem_Read_Scalar<uint32_t>(cpu.Reg(NRegister::SP));
			cpu.Reg(NRegister::SP) += 4;

			return true;
		}
};

/*
 * FW instruction class
 */
class CInstruction_Fw : public CInstruction_1Param<NOperand_Type::Immediate> {
	public:
		using CInstruction_1Param<NOperand_Type::Immediate>::CInstruction_1Param;

		virtual bool Execute(CCPU_Context& cpu) const override {

			if (!mSrc.Is_Immediate())
				return false;

			if (!Check_Condition(mCondition, cpu))
				return true;

			// fetch 24b immediate to register R0
			cpu.Reg(NRegister::R0) = mSrc.Get_Immediate();

			return true;
		}
};

/*
 * SVC instruction class
 */
class CInstruction_Svc : public CInstruction_1Param<NOperand_Type::Immediate> {
	public:
		using CInstruction_1Param<NOperand_Type::Immediate>::CInstruction_1Param;

		virtual bool Execute(CCPU_Context& cpu) const override {

			if (!mSrc.Is_Immediate())
				return false;

			if (!Check_Condition(mCondition, cpu))
				return true;

			throw supervisor_call_exception{ mSrc.Get_Immediate() };
		}
};

/*
 * APS instruction class
 */
class CInstruction_Aps : public CInstruction_Generic_2Param {
	public:
		using CInstruction_Generic_2Param::CInstruction_Generic_2Param;

		virtual bool Execute(CCPU_Context& cpu) const override {

			// should not happen due to encoding, but check anyways
			if (!mDst.Is_Register() || !mSrc.Is_Immediate())
				return false;

			if (!Check_Condition(mCondition, cpu))
				return true;

			auto checkPrivileged = [&]() {
				if (cpu.State<NCPU_Mode>(NProcessor_State_Register::Mode) != NCPU_Mode::System)
					throw undefined_instruction_exception();
			};

			NAPS_Request_Code reqCode = static_cast<NAPS_Request_Code>(mSrc.Get_Immediate());
			switch (reqCode) {

				case NAPS_Request_Code::None:
					return true;

				case NAPS_Request_Code::Get_Mode:
					cpu.Reg(mDst.Get_Register()) = cpu.State(NProcessor_State_Register::Mode);
					return true;

				case NAPS_Request_Code::Set_Mode:
					checkPrivileged();
					cpu.State(NProcessor_State_Register::Mode) = cpu.Reg(mDst.Get_Register());
					return true;
			}

			// unknown request code - ignore
			return true;
		}
};

// factor function type definition
using TFactory_Fnc = std::unique_ptr<CInstruction>(*)(NOpcode, NCondition);

// instruction factor definition
template<typename T>
std::unique_ptr<CInstruction> InstrFactory(NOpcode op, NCondition cnd) {
	return std::make_unique<T>(op, cnd);
}

// map of factories per opcode (and instruction type)
const std::unordered_map<NOpcode, TFactory_Fnc> Instruction_Factory_Map = {
	{ NOpcode::nop, &InstrFactory<CInstruction_Nop> },
	{ NOpcode::mov, &InstrFactory<CInstruction_Mov> },
	{ NOpcode::movi, &InstrFactory<CInstruction_Mov> },
	{ NOpcode::add, &InstrFactory<CInstruction_Add> },
	{ NOpcode::addi, &InstrFactory<CInstruction_Add> },
	{ NOpcode::sub, &InstrFactory<CInstruction_Sub> },
	{ NOpcode::subi, &InstrFactory<CInstruction_Sub> },
	{ NOpcode::mul, &InstrFactory<CInstruction_Mul> },
	{ NOpcode::muli, &InstrFactory<CInstruction_Mul> },
	{ NOpcode::div, &InstrFactory<CInstruction_Div> },
	{ NOpcode::divi, &InstrFactory<CInstruction_Div> },
	{ NOpcode::and_, &InstrFactory<CInstruction_And> },
	{ NOpcode::andi, &InstrFactory<CInstruction_And> },
	{ NOpcode::or_, &InstrFactory<CInstruction_Or> },
	{ NOpcode::ori, &InstrFactory<CInstruction_Or> },
	{ NOpcode::slr, &InstrFactory<CInstruction_ShiftLeft> },
	{ NOpcode::sli, &InstrFactory<CInstruction_ShiftLeft> },
	{ NOpcode::srr, &InstrFactory<CInstruction_ShiftRight> },
	{ NOpcode::sri, &InstrFactory<CInstruction_ShiftRight> },
	{ NOpcode::lw, &InstrFactory<CInstruction_LoadWord> },
	{ NOpcode::li, &InstrFactory<CInstruction_LoadWord> },
	{ NOpcode::sw, &InstrFactory<CInstruction_StoreWord> },
	{ NOpcode::si, &InstrFactory<CInstruction_StoreWord> },
	{ NOpcode::cmpr, &InstrFactory<CInstruction_Compare> },
	{ NOpcode::cmpi, &InstrFactory<CInstruction_Compare> },
	{ NOpcode::br, &InstrFactory<CInstruction_Branch> },
	{ NOpcode::bi, &InstrFactory<CInstruction_Branch> },
	{ NOpcode::push, &InstrFactory<CInstruction_Push> },
	{ NOpcode::pop, &InstrFactory<CInstruction_Pop> },
	{ NOpcode::fw, &InstrFactory<CInstruction_Fw> },
	{ NOpcode::svc, &InstrFactory<CInstruction_Svc> },
	{ NOpcode::aps, &InstrFactory<CInstruction_Aps> },
};

// builds an instruction class from string
std::unique_ptr<CInstruction> CInstruction::Build_From_String(const std::string& line) {

	// transform the input to lowercase
	std::string input = "";
	std::transform(line.begin(), line.end(), std::back_inserter(input), [](unsigned char c) { return std::tolower(c); });

	// regular expressions for instruction parsing
	std::regex instr_2op{ "^[\\s]{0,}([a-z]+[a-z0-9]*[\\.]{0,1}[a-z]{0,2})[\\s]+([a-z]+[a-z0-9]*)[\\s]*,[\\s]*([a-z#\\$]{1}[\\-a-z0-9]*)[\\s]*(;.*)?$", std::regex::ECMAScript };
	std::regex instr_1op{ "^[\\s]{0,}([a-z]+[a-z0-9]*[\\.]{0,1}[a-z]{0,2})[\\s]+([a-z#\\$]{1}[\\-a-z0-9]*)[\\s]*(;.*)?$", std::regex::ECMAScript };
	std::regex instr_0op{ "^[\\s]{0,}([a-z]+[a-z0-9]*[\\.]{0,1}[a-z]{0,2})[\\s]*(;.*)?$", std::regex::ECMAScript };
	std::regex cond_parse{ "^([a-z0-9]+)(\\.[a-z]+)?$", std::regex::ECMAScript };

	std::string mnemonic;
	std::vector<std::string> operands;

	NOpcode opcode = NOpcode::nop;
	NCondition cond = NCondition::always;

	std::smatch sm;
	// match 2 parametric
	if (std::regex_match(input, sm, instr_2op))
	{
		mnemonic = sm[1];
		operands.push_back(sm[2]);
		operands.push_back(sm[3]);
	}
	// match single parametric
	else if (std::regex_match(input, sm, instr_1op))
	{
		mnemonic = sm[1];
		operands.push_back(sm[2]);
	}
	// match no parametric
	else if (std::regex_match(input, sm, instr_0op))
	{
		mnemonic = sm[1];
	}
	else
		throw sarch32_parser_exception{ "Unable to parse line: " + line };

	// parse mnemonic and conditional, if present
	if (std::regex_match(mnemonic, sm, cond_parse))
	{
		// if matched and corresponds to condition type, use it
		if (sm[2].matched) {
			auto itr = Mnemonic_To_Cond.find(sm[2].str().substr(1));

			if (itr == Mnemonic_To_Cond.end())
				throw sarch32_parser_exception{ "Invalid condition mnemonic: " + mnemonic };

			cond = itr->second;
		}

		// parse mnemonic - find opcode enum value
		auto itr = Mnemonic_To_Opcode.find(sm[1]);
		if (itr == Mnemonic_To_Opcode.end())
			throw sarch32_parser_exception{ "Invalid opcode mnemonic: " + mnemonic };

		mnemonic = sm[1];

		opcode = itr->second;
	}
	else
		throw sarch32_parser_exception{ "Invalid instruction mnemonic format: " + mnemonic };

	// find factory function
	if (!Instruction_Factory_Map.contains(opcode))
		throw sarch32_generator_exception{ "No instruction factory hooked for: " + mnemonic };

	// build instruction class instance
	auto instr = Instruction_Factory_Map.find(opcode)->second(opcode, cond);
	instr->Parse_String(mnemonic, operands);

	// TEST!
	/*
	std::cout << instr->Generate_String() << std::endl;

	auto binary = instr->Generate_Binary();

	std::unique_ptr<CInstruction> testInstr = Build_From_Binary(binary);
	testInstr->Parse_Binary(instr->Generate_Binary());
	std::cout << testInstr->Generate_String() << std::endl;
	*/
	// TEST!

	return instr;
}

// build instruction class instance from binary representation
std::unique_ptr<CInstruction> CInstruction::Build_From_Binary(const uint32_t instruction) {

	// parse last byte (opcode and condition)
	const uint32_t lastByte = instruction & 0xFF;
	const NOpcode opcode = static_cast<NOpcode>(lastByte & 0b11111);
	const NCondition cond = static_cast<NCondition>(lastByte >> 5);

	// find factory by opcode
	auto instr = Instruction_Factory_Map.find(opcode)->second(opcode, cond);
	// parse binary
	if (!instr->Parse_Binary(instruction))
		return nullptr;

	return instr;
}
