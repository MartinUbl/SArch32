#include "sobjfile.h"

#include <algorithm>
#include <iterator>
#include <fstream>
#include <iostream>

namespace SObj {

	CSObj_File::CSObj_File() {
		//
	}

	void CSObj_File::Put_To_Section(const std::string& sectionName, const std::vector<uint8_t>& data) {
		// if the section does not exist, create its record
		if (mSection.find(sectionName) == mSection.end()) {
			mSection[sectionName].startAddr = 0;
			mSection[sectionName].size = 0;
		}

		std::copy(data.begin(), data.end(), std::back_inserter(mSection[sectionName].data));
		mSection[sectionName].size += static_cast<uint32_t>(data.size());

		// TODO: check overflow
	}

	void CSObj_File::Relocate_Section(const std::string& sectionName, uint32_t startAddr) {
		// if the section does not exist, create its record
		if (mSection.find(sectionName) == mSection.end()) {
			mSection[sectionName].size = 0;
		}

		mSection[sectionName].startAddr = startAddr;
	}

	void CSObj_File::Clear_Section(const std::string& sectionName) {
		// erase section record, if exists
		if (mSection.find(sectionName) != mSection.end()) {
			mSection.erase(sectionName);
		}
	}

	bool CSObj_File::Save_To_File(const std::string& path) {

		// open file to store object dump to
		std::ofstream ofs(path, std::ios::out | std::ios::binary);
		if (!ofs.is_open()) {
			return false;
		}

		// serialize number of sections
		Serialize<uint32_t>(ofs, static_cast<uint32_t>(mSection.size()));
		// serialize all sections one after another
		for (auto& s : mSection) {

			// section name length
			Serialize<uint32_t>(ofs, static_cast<uint32_t>(s.first.size()));
			// section name
			Serialize<std::string>(ofs, s.first);
			// start address
			Serialize<uint32_t>(ofs, s.second.startAddr);
			// length of section
			Serialize<uint32_t>(ofs, s.second.size);
			// section data
			ofs.write(reinterpret_cast<char*>(s.second.data.data()), s.second.data.size());

			//std::cout << "Stored section " << s.first << " at " << s.second.startAddr << ", size " << s.second.size << std::endl;
		}

		return true;
	}

	bool CSObj_File::Load_From_File(const std::string& path) {

		// open object file
		std::ifstream ifs(path, std::ios::in | std::ios::binary);
		if (!ifs.is_open()) {
			return false;
		}

		// read section count
		auto sectionCount = Deserialize<uint32_t>(ifs);
		for (decltype(sectionCount) i = 0; i < sectionCount; i++) {

			// read section name length
			auto nameLen = Deserialize<uint32_t>(ifs);
			// read section name
			auto name = Deserialize_String(ifs, nameLen);

			// read starting address
			mSection[name].startAddr = Deserialize<uint32_t>(ifs);
			// read section size
			mSection[name].size = Deserialize<uint32_t>(ifs);
			// allocate space
			mSection[name].data.resize(mSection[name].size);

			// read section data
			ifs.read(reinterpret_cast<char*>(mSection[name].data.data()), mSection[name].size);

			//std::cout << "Loaded section " << name << " at " << mSection[name].startAddr << ", size " << mSection[name].size << std::endl;
		}

		return true;
	}

}
