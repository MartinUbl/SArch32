#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <fstream>
#include <map>

namespace SObj {

	// structure of a section (loaded into memory)
	struct TSection {
		uint32_t startAddr;
		uint32_t size;
		std::vector<uint8_t> data;
	};

	/*
	 * Class encapsulating object file
	 */
	class CSObj_File {

		private:
			// section map
			std::map<std::string, TSection> mSection;

			// serializes given POD type to file
			template<typename T>
			void Serialize(std::ostream& file, const T& t) {
				union {
					char bytes[sizeof(T)];
					T raw;
				} un;

				un.raw = t;
				file.write(un.bytes, sizeof(T));
			}

			// serializes string to file (without the null terminator)
			template<>
			void Serialize(std::ostream& file, const std::string& t) {
				file.write(t.c_str(), t.size());
			}

			// deserializes given POD type from stream
			template<typename T>
			T Deserialize(std::istream& file) {
				union {
					char bytes[sizeof(T)];
					T raw;
				} un;

				file.read(un.bytes, sizeof(T));
				return un.raw;
			}

			// deserializes string from stream
			std::string Deserialize_String(std::istream& file, size_t expectedLen) {
				std::string ret(expectedLen + 1, '\0');

				file.read(ret.data(), expectedLen);

				return ret;
			}

		public:
			CSObj_File();

			// put given data dump into section
			void Put_To_Section(const std::string& sectionName, const std::vector<uint8_t>& data);
			// relocate section to given address
			void Relocate_Section(const std::string& sectionName, uint32_t startAddr);
			// clears given section
			void Clear_Section(const std::string& sectionName);

			// saves the loaded state to file
			bool Save_To_File(const std::string& path);
			// loads contents from file
			bool Load_From_File(const std::string& path);

			// retrieves read-only map of sections
			const std::map<std::string, TSection>& Get_Sections() const {
				return mSection;
			}
	};

}
