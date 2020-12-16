#pragma once
#include "UserInterfacePrograme.h"
#include "../model/Record.h"
#include "../common/statistics.h"

UserInterfaceProgram::UserInterfaceProgram() : _file(Config::get().mainFilename, Config::get().indexFilename) {}

UserInterfaceProgram::~UserInterfaceProgram(){
	_statistics();
}

int UserInterfaceProgram::run()
{
	std::string command;
	do {
		std::cin >> command;

		if (command == "insert")
			_insert();
		else if (command == "find")
			_find();
		else if (command == "print")
			_print();
		else if (command == "delete")
			_delete();
		else if (command == "update")
			_update();
		else if (command == "reorganise")
			_reorganise();
		else if (command == "statistics")
			_statistics();
		else if (command == "reset_statistics")
			_resetStatistics();
	} while (command != "quit");
	return 0;
}


void UserInterfaceProgram::_insert() {
	Record record;

	std::cin >> record.key;
	for (int i = 0; i < Record::ElementsCount; i++) {
		std::cin >> record.elements[i];
	}
	record.next = 0;
	record.type = RecordType::PRESENT;

	bool status = _file.add(record);
	if (status)
		std::cout << "Record inserted" << std::endl;
	else
		std::cout << "Record already present" << std::endl;
}

void UserInterfaceProgram::_find() {
	RecordKeyType key;
	std::cin >> key;

	std::unique_ptr<Record> recordPtr = _file.find(key);
	if (recordPtr) {
		std::cout << "Found record: ";
		for (int i = 0; i < Record::ElementsCount; i++) {
			std::cout << recordPtr.get()->elements[i] << " ";
		}
		std::cout << std::endl;
	}
	else {
		std::cout << "Record with key " << key << " not found" << std::endl;
	}
}

void UserInterfaceProgram::_delete() {
	RecordKeyType key;

	std::cin >> key;
	const bool status = _file.deleteRecord(key);
	if (status)
		std::cout << "Record deleted" << std::endl;
	else
		std::cout << "Record not found" << std::endl;
}

void UserInterfaceProgram::_update() {
	Record record;
	RecordKeyType key;

	std::cin >> key;
	std::cin >> record.key;
	for (int i = 0; i < Record::ElementsCount; i++) {
		std::cin >> record.elements[i];
	}
	record.next = 0;
	record.type = RecordType::PRESENT;

	bool status = _file.update(key, record);
	if (status)
		std::cout << "Record updated" << std::endl;
	else
		std::cout << "Record not found" << std::endl;
}

void UserInterfaceProgram::_print() {
	_file.print(std::cout);
}

void UserInterfaceProgram::_reorganise() {
	_file.reorganise();
}

void UserInterfaceProgram::_statistics() {
	Statistics::get().print(std::cout);
}

void UserInterfaceProgram::_resetStatistics(){
	Statistics::get().reset();
}
