#pragma once
#include "src/subprograms/UserInterfacePrograme.h"
#include "src/common/Configurator.h"

int main(int argc, char* args[]) {
	try {
		Configurator::configure(argc, args);
		return UserInterfaceProgram().run();
	}catch (std::exception& e) {
		std::cout << e.what() << std::endl;
		return -1;
	}
}