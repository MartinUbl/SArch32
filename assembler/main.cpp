#include <iostream>
#include <filesystem>

#include "../core/isa.h"

#include "assembler.h"

/*
 * Parses CLI arguments and puts them to a container
 */
static bool Parse_CLI_Args(int argc, char** argv, TAssembly_Input& target) {

	// convert to strings
	std::vector<std::string> args;
	for (int i = 1; i < argc; i++) {
		args.push_back(argv[i]);
	}

	target.Input_Files.clear();
	target.Linker_File.clear();
	target.Output_File.clear();

	// current mode enumerator
	enum class NMode {
		none,
		ifile,
		lfile,
		ofile,
		loglevel,
	};

	// current mode
	NMode mode = NMode::none;

	for (size_t i = 0; i < args.size(); i++) {

		// input file switch
		if (args[i] == "-i") {
			mode = NMode::ifile;
		}
		// linker file switch
		else if (args[i] == "-l") {
			mode = NMode::lfile;
		}
		// output file switch
		else if (args[i] == "-o") {
			mode = NMode::ofile;
		}
		// verbosity
		else if (args[i] == "-ll") {
			mode = NMode::loglevel;
		}
		// we have some mode set
		else if (mode != NMode::none) {

			// according to mode, sort the next argument to a given place
			switch (mode) {
				case NMode::ifile:
					target.Input_Files.push_back(args[i]);
					break;
				case NMode::lfile:
					target.Linker_File = args[i];
					mode = NMode::none;
					break;
				case NMode::ofile:
					target.Output_File = args[i];
					mode = NMode::none;
					break;
				case NMode::loglevel:
					if (args[i] == "none") {
						target.Log_Level = NLog_Level::None;
					}
					else if (args[i] == "basic") {
						target.Log_Level = NLog_Level::Basic;
					}
					else if (args[i] == "extended") {
						target.Log_Level = NLog_Level::Extended;
					}
					else if (args[i] == "full") {
						target.Log_Level = NLog_Level::Full;
					}
					else {
						std::cerr << "Invalid log level: " << args[i] << "; use one of following: none, basic, extended, full" << std::endl;
						return false;
					}
					mode = NMode::none;
					break;
			}

		}
		else {
			std::cerr << "Invalid command line parameter: " << args[i] << std::endl;
			return false;
		}
	}

	// validate number of input files
	if (target.Input_Files.empty()) {
		std::cerr << "No input files specified" << std::endl;
		return false;
	}

	// validate existence of input files
	for (auto& f : target.Input_Files) {
		if (!std::filesystem::exists(f)) {
			std::cerr << "Input file " << f << " does not exist" << std::endl;
			return false;
		}

		if (!std::filesystem::is_regular_file(f)) {
			std::cerr << "Input file " << f << " is not a file" << std::endl;
			return false;
		}
	}

	// there must be an output file specified
	if (target.Output_File.empty()) {
		std::cerr << "No output file specified" << std::endl;
		return false;
	}

	// there must be a linker file specified
	if (target.Linker_File.empty()) {
		std::cerr << "No linker file specified" << std::endl;
		return false;
	}

	return true;
}

int main(int argc, char** argv) {

	// parse requests from CLI args
	TAssembly_Input input;
	if (!Parse_CLI_Args(argc, argv, input)) {
		return 1;
	}

	// assemble!
	CAssembler assembler(input);
	if (!assembler.Assemble()) {
		return 2;
	}

	return 0;
}
