#pragma once
#include <string>
#include <iostream>
#include "../config/config.h"
#include "../ind_seq_struc/IndexedSequentialFile.h"

class UserInterfaceProgram {

private:
	IndexedSequentialFile _file;

	void _insert();
	void _find();
	void _print();
	void _delete();
	void _update();

public:
	UserInterfaceProgram();

	int run();
};