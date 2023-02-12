#pragma once

#include "../core/isa.h"

#include <vector>
#include <string>
#include <iterator>

/*
 * Data pseudoinstruction - contains only text-defined data (bytes, strings, ...)
 */
class CPseudo_Instruction_Data : public CInstruction
{
	private:
		// stored data
		std::vector<uint8_t> mData;
		// pseudo-mnemonic
		std::string mMnemonic;
		// request for a symbol (in case this holds e.g., an address of another variable -> is a pointer)
		std::string mSymbolic_Req;

	protected:
		// stores a value with desired precision
		template<size_t precision>
		void Store(const int32_t& t) {
			union {
				uint8_t bytes[precision];
				int32_t orig;
			} dump;

			dump.orig = t;
			std::copy_n(&dump.bytes[0], precision, std::back_inserter(mData));
		}

		// restores (loads) a value with desired precision
		template<size_t precision>
		int32_t Restore(const size_t ptr) const {
			union {
				uint8_t bytes[precision];
				int32_t orig;
			} dump;

			std::copy_n(&mData[ptr], precision, &dump.bytes[0]);

			return dump.orig;
		}

	public:
		using CInstruction::CInstruction;

		virtual bool Parse_String(const std::string& mnemonic, const std::vector<std::string>& operands) override;

		virtual bool Parse_Binary(const uint32_t instruction) {
			// this instruction is never parsed from binary
			return true;
		};

		virtual std::string Generate_String(bool hexaFmt) const override;

		virtual void Resolve_Symbol(int32_t value) override;

		virtual bool Get_Resolve_Request(std::string& str) const override {
			if (mSymbolic_Req.size() > 0) {
				str = mSymbolic_Req;
				return true;
			}
			return false;
		}

		virtual bool Is_Pseudo_Instruction() const override {
			return true;
		}

		virtual bool Generate_Additional_Data(std::vector<uint8_t>& data) override {
			// held data for dump
			if (mData.size() > 0) {
				std::copy(mData.begin(), mData.end(), std::back_inserter(data));
				return true;
			}

			return false;
		}

		virtual uint32_t Get_Length() const {
			return static_cast<uint32_t>(mData.size());
		}
};
