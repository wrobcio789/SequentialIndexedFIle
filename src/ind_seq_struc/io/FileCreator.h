#pragma once
#include <string>
#include <fstream>
#include "../../model/Record.h"

class FileCreator{
	std::string indexFilename;
	std::string mainFilename;
	std::ofstream mainStream;
	std::ofstream indexStream;
	Record* buffer;
	size_t nextRecordIndex;

public:
	FileCreator(std::string indexFilename, std::string mainFilename);
	~FileCreator();

	void writeIndex(RecordKeyType* index, size_t size);
	void appendRecord(const Record* record);
	void flushPage();
	size_t getRecordsCount();
};

