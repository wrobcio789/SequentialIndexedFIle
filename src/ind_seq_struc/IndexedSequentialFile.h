#pragma once
#include "io/BufferedFile.h"
#include "../model/Record.h"
#include <vector>

struct _RecordCandidate{
	Record record;
	size_t position;

	_RecordCandidate(const Record& record, size_t position) : record(record), position(position) {}
};

class IndexedSequentialFile
{
private:
	std::vector<RecordKeyType> index;
	std::unique_ptr<BufferedFile> mainFile;


	void _createEmptySequentialFile(std::string mainFilename, std::string indexFilename);
	int _findRecordPositionOnPage(RecordKeyType key, Record* records);
	size_t _findPageNumberForKey(RecordKeyType key);
	std::unique_ptr<Record> _findInOverflowArea(RecordKeyType key, size_t startPosition);

public:
	IndexedSequentialFile(std::string mainFilename, std::string indexFilename);
	~IndexedSequentialFile() {};

	std::unique_ptr<Record> find(RecordKeyType key);
	bool add(const Record& record);

	void print(std::ostream& stream);
};
