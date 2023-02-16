#pragma once

#include <string>
#include <map>
#include <vector>
#include <fstream>

/*
 * Emulator config class - loader and container
 */
class CConfig {
	
	private:
		// machine name
		std::string mMachine_Name{};
		// size of memory (bytes)
		uint32_t mMemory_Size = 2*1024*1024;
		// memory image (SObj file)
		std::string mMemory_Image{};
		// connected peripherals
		std::map<std::string, std::string> mPeripherals;

	public:
		CConfig();
		virtual ~CConfig() = default;

		// loads config from given file; if any error occurs, the error string is filled and false is returned
		bool Load_From_File(const std::string& path, std::string& error);

		// retrieve machine name from config
		const std::string& Get_Machine_Name() const {
			return mMachine_Name;
		}

		// retrieve memory size in bytes from config
		const uint32_t Get_Memory_Size() const {
			return mMemory_Size;
		}

		// retrieve memory image file from config
		const std::string& Get_Memory_Image() const {
			return mMemory_Image;
		}

		// retrieve peripheral map from config
		const std::map<std::string, std::string>& Get_Peripherals() const {
			return mPeripherals;
		}
};
