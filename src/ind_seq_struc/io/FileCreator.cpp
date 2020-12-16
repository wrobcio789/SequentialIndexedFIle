#include "FileCreator.h"
#include "../../common/config.h"
#include <cassert>
#include "../../common/statistics.h"

FileCreator::FileCreator(std::string indexFilename, std::string mainFilename) : indexFilename(indexFilename),
	mainFilename(mainFilename), 
	mainStream(mainFilename, std::fstream::binary),
	indexStream(indexFilename, std::fstream::binary)
{
	buffer = new Record[Config::get().blockingFactor];
	std::memset(buffer, 0, Config::get().blockingFactor * sizeof(Record));
	nextRecordIndex = 0;
}

FileCreator::~FileCreator(){
	mainStream.close();
	indexStream.close();
	delete[] buffer;
}

void FileCreator::writeIndex(RecordKeyType* index, size_t size) {
	const size_t writeSizeInBytes = size * sizeof(RecordKeyType);
	indexStream.write(reinterpret_cast<char*>(index), writeSizeInBytes);

	const Config& config = Config::get();
	const size_t pageSizeInBytes = config.blockingFactor * sizeof(Record);
	const size_t pagesWritten = (writeSizeInBytes + pageSizeInBytes - 1) / pageSizeInBytes;
	Statistics::get().registerWrite(pagesWritten);
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

	Statistics::get().registerWrite();
}

size_t FileCreator::getRecordsCount() {
	return nextRecordIndex;
}

