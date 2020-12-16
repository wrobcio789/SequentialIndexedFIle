#include "FileCreator.h"
#include "../../config/config.h"
#include <cassert>

FileCreator::FileCreator(std::string indexFilename, std::string mainFilename) : indexFilename(indexFilename),
	mainFilename(mainFilename), 
	mainStream(mainFilename, std::fstream::binary),
	indexStream(indexFilename, std::fstream::binary)
{
	buffer = new Record[Config::get().blockingFactor];
	std::memset(buffer, 0, Config::get().blockingFactor *sizeof(Record));
	nextRecordIndex = 0;
}

FileCreator::~FileCreator(){
	mainStream.close();
	indexStream.close();
	delete[] buffer;
}

void FileCreator::writeIndex(RecordKeyType* index, size_t size) {
	indexStream.write(reinterpret_cast<char*>(index), size * sizeof(index));
}

void FileCreator::appendRecord(const Record* record) {
	assert((nextRecordIndex < Config::get().blockingFactor));
	buffer[nextRecordIndex] = *record;
	buffer[nextRecordIndex++].next = 0;
}

void FileCreator::flushPage(){
	mainStream.write(reinterpret_cast<char*>(buffer), Config::get().blockingFactor * sizeof(Record));
	nextRecordIndex = 0;
	std::memset(buffer, 0, Config::get().blockingFactor * sizeof(Record));
}

size_t FileCreator::getRecordsCount() {
	return nextRecordIndex;
}

