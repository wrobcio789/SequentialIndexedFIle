#pragma once
#include "io/BufferedFile.h"
#include "../model/Record.h"
#include <vector>

class IndexedSequentialFile{
private:

	class Iterator {
	private:
		IndexedSequentialFile& _file;

		size_t index;
		size_t pageIndex;
		size_t overflowPosition;

		const Record* _nextOverflow();

	public:
		Iterator(IndexedSequentialFile& file);

		const Record* next();

	};

	std::vector<RecordKeyType> index;
	std::unique_ptr<BufferedFile> mainFile;


	void _createEmptySequentialFile(std::string mainFilename, std::string indexFilename);
	int _findRecordPositionOnPage(RecordKeyType key, Record* records);
	size_t _findPageNumberForKey(RecordKeyType key);
	Record* _findInOverflowArea(RecordKeyType key, size_t startPosition);

public:
	IndexedSequentialFile(std::string mainFilename, std::string indexFilename);
	~IndexedSequentialFile() {};

	std::unique_ptr<Record> find(RecordKeyType key);
	bool add(const Record& record);
	bool deleteRecord(RecordKeyType key);
	bool update(RecordKeyType key, const Record& record);
	void reorganise();

	void print(std::ostream& stream);
};
