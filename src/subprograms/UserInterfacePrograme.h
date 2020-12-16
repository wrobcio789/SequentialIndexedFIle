#pragma once
#include <string>
#include <iostream>
#include "../common/config.h"
#include "../ind_seq_struc/IndexedSequentialFile.h"

class UserInterfaceProgram {

private:
	IndexedSequentialFile _file;

	void _insert();
	void _find();
	void _print();
	void _delete();
	void _update();
	void _reorganise();
	void _statistics();
	void _resetStatistics();

public:
	UserInterfaceProgram();
	~UserInterfaceProgram();

	int run();
};