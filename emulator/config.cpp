#include "config.h"

#include <regex>

CConfig::CConfig() {
	//
}

bool CConfig::Load_From_File(const std::string& path, std::string& error) {

	// try to open config file
	std::ifstream cfg(path);
	if (!cfg.is_open()) {
		error = "Could not open config file: " + path;
		return false;
	}

	// regexes for options and empty lines
	std::regex optionRegex{ "[\\s]*([a-zA-Z0-9]{1,})[\\s]*=[\\s]*([a-zA-Z0-9\\.:\\\\\\/_\\-]+)[\\s]*(;.*)?", std::regex::ECMAScript | std::regex::icase };
	std::regex emptyLineRegex{ "[\\s]*(;.*)?", std::regex::ECMAScript | std::regex::icase };

	// go through every line
	std::string line;
	while (std::getline(cfg, line)) {

		std::smatch sm;

		// try to match option
		if (std::regex_match(line, sm, optionRegex) && sm.size() >= 3) {

			const std::string key = sm[1];
			const std::string value = sm[2];

			if (key == "machine") {
				mMachine_Name = value;
			}
			else if (key == "memory") {

				// match memory string - valid strings are numerical ones, optionally suffixed with k, K, m, M, g or G to represent units
				std::regex memRegex{ "([0-9]{1,})([kmg]{1})?", std::regex::ECMAScript | std::regex::icase };

				if (std::regex_match(value, sm, memRegex) && sm.size() >= 2) {

					try {

						// either way parse number
						mMemory_Size = std::stoul(sm[1]);

						// if there is a suffix, apply it
						if (sm.size() > 2) {
							if (sm[2] == "k" || sm[2] == "K") {
								mMemory_Size *= 1024;
							}
							else if (sm[2] == "m" || sm[2] == "M") {
								mMemory_Size *= 1024 * 1024;
							}
							else if (sm[2] == "g" || sm[2] == "G") {
								mMemory_Size *= 1024 * 1024 * 1024;
							}
							else {
								error = "Failed to parse memory string (unrecognized suffix): " + value;
								return false;
							}
						}
					}
					catch (...) {
						error = "Failed to parse memory string: " + value;
						return false;
					}

				}

			}
			// TODO: modularize peripherals better
			else if (key == "display" || key == "gpio" || key == "timer") {
				mPeripherals[key] = value;
			}
			else if (key == "image") {
				mMemory_Image = value;
			}
			else {
				error = "Unknown key in config: " + key;
				return false;
			}

		}
		else if (std::regex_match(line, emptyLineRegex)) {
			continue;
		}
		else {
			error = "Unrecognized line in config: " + line;
			return false;
		}

	}

	return true;
}
