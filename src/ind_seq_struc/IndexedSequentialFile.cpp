#pragma once
#include "IndexedSequentialFile.h"
#include "../common/config.h"
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <string>
#include "io/FileCreator.h"

class _RecordByKeySorter {
public:
	bool operator()(const Record& a, const Record& b) {
		return a.key < b.key;
	}
};

IndexedSequentialFile::IndexedSequentialFile(std::string mainFilename, std::string indexFilename){
	_createEmptySequentialFile(mainFilename, indexFilename);
	mainFile = std::make_unique<BufferedFile>(mainFilename);
}

std::unique_ptr<Record> IndexedSequentialFile::find(RecordKeyType key){
	const size_t pageNumber = _findPageNumberForKey(key);
	Record* records = mainFile.get()->readPageFromMain(pageNumber);
	const int recordPageIndex = _findRecordPositionOnPage(key, records);

	const Record& record = records[recordPageIndex];
	if (record.key == key && record.type == RecordType::PRESENT)
		return std::make_unique<Record>(record);

	const Record* recordInOverflow = _findInOverflowArea(key, record.next);
	if (recordInOverflow != nullptr)
		return std::make_unique<Record>(*recordInOverflow); 
	return std::unique_ptr<Record>();
}

bool IndexedSequentialFile::add(const Record& record) {
	assert((record.type == RecordType::PRESENT && record.next == 0));

	const RecordKeyType& key = record.key;
	
	const size_t pageNumber = _findPageNumberForKey(key);
	Record* records = mainFile.get()->readPageFromMain(pageNumber);
	const int recordPageIndex = _findRecordPositionOnPage(key, records);

	Record& candidate = records[recordPageIndex];
	if (candidate.key == key) {
		if (candidate.type == RecordType::DELETED) {
			records[recordPageIndex] = record;
			mainFile.get()->writeMainPage();
			return true;
		}
		return false; 
	}

	for (int newPlaceToInsert = recordPageIndex + 1; newPlaceToInsert < Config::get().blockingFactor; newPlaceToInsert++) {
		if (records[newPlaceToInsert].type == RecordType::EMPTY) {
			records[newPlaceToInsert] = record;
			std::sort(records, records + newPlaceToInsert + 1, _RecordByKeySorter());
			mainFile.get()->writeMainPage();
			return true;
		}
	}

	if (candidate.next == 0) {
		candidate.next = mainFile.get()->getNextOverflowPosition();
		mainFile.get()->writeMainPage();
		mainFile.get()->appendRecordInOverflow(record);

		if (mainFile.get()->isFull()) {
			reorganise();
		}
		return true;
	}

	size_t currentPosition = candidate.next;

	bool result = false;
	while(true){
		Record* currentRecord = mainFile.get()->readSingleFromOverflow(currentPosition);

		if (currentRecord->key == key) {
			if (currentRecord->type == RecordType::PRESENT) {
				result = false;
				break;
			}
			else {
				*currentRecord = record;
				currentRecord->type = RecordType::PRESENT;
				mainFile.get()->writeOverflowRecord();
				result = true;
				break;
			}
		}

		if (currentRecord->key > key) {
			Record oldRecordCopy = *currentRecord;
			*currentRecord = record;
			currentRecord->next = mainFile.get()->getNextOverflowPosition();
			mainFile.get()->writeOverflowRecord();
			mainFile.get()->appendRecordInOverflow(oldRecordCopy);
			result = true;
			break;
		}

		if (currentRecord->next == 0) {
			currentRecord->next = mainFile.get()->getNextOverflowPosition();
			mainFile.get()->writeOverflowRecord();
			mainFile.get()->appendRecordInOverflow(record);
			result = true;
			break;
		}

		currentPosition = currentRecord->next;
	}

	if (mainFile.get()->isFull()) {
		reorganise();
	}

	return result;
}

bool IndexedSequentialFile::deleteRecord(RecordKeyType key)
{
	const size_t pageNumber = _findPageNumberForKey(key);
	Record* records = mainFile.get()->readPageFromMain(pageNumber);
	const int recordPageIndex = _findRecordPositionOnPage(key, records);

	Record& record = records[recordPageIndex];
	if (record.key == key && record.type == RecordType::PRESENT) {
		record.type = RecordType::DELETED;
		mainFile.get()->writeMainPage();
		return true;
	}

	Record* recordInOverflow = _findInOverflowArea(key, record.next);
	if (recordInOverflow != nullptr) {
		recordInOverflow->type = RecordType::DELETED;
		mainFile.get()->writeOverflowRecord();
		return true;
	}
	return false;
}

bool IndexedSequentialFile::update(RecordKeyType key, const Record& record){
	assert((record.type == RecordType::PRESENT && record.next == 0));

	const size_t pageNumber = _findPageNumberForKey(key);
	Record* records = mainFile.get()->readPageFromMain(pageNumber);
	const int recordPageIndex = _findRecordPositionOnPage(key, records);

	Record* candidate = &records[recordPageIndex];
	bool wasFound = false;
	if (candidate->key == key && candidate->type == RecordType::PRESENT) {
		if (key == record.key) {
			*candidate = record;
			mainFile.get()->writeMainPage();
			return true;
		}
		else {
			candidate->type = RecordType::DELETED;
			mainFile.get()->writeMainPage();
			return add(record);
		}

	}

	Record* recordInOverflow = _findInOverflowArea(key, candidate->next);
	if (recordInOverflow != nullptr) {

		if (key == record.key) {
			*recordInOverflow = record;
			mainFile.get()->writeOverflowRecord();
			return true;

		}
		else {
			recordInOverflow->type = RecordType::DELETED;
			mainFile.get()->writeOverflowRecord();
			return add(record);
		}

	}

	return false;
}

void IndexedSequentialFile::reorganise(){
	const Config& config = Config::get();
	const size_t maxRecordsCountOnPage = config.blockingFactor * config.pageUtilisationFactor;
	std::vector<RecordKeyType> newIndex = { MINIMUM_KEY_VALUE };

	{
		const Record initialValue(RecordType::DELETED, MINIMUM_KEY_VALUE);
		FileCreator fileCreator(config.tempIndexFilename, config.tempMainFilename);
		fileCreator.appendRecord(&initialValue);

		Iterator iterator(*this);
		while (const Record* record = iterator.next()) {
			if (fileCreator.getRecordsCount() == 0) {
				newIndex.push_back(record->key);
			}

			fileCreator.appendRecord(record);
			if (fileCreator.getRecordsCount() >= maxRecordsCountOnPage)
				fileCreator.flushPage();
		}

		if (fileCreator.getRecordsCount() > 0)
			fileCreator.flushPage();
	}

	index = newIndex;
	mainFile.reset();

	std::remove(config.indexFilename.c_str());
	std::remove(config.mainFilename.c_str());
	std::rename(config.tempIndexFilename.c_str(), config.indexFilename.c_str());
	std::rename(config.tempMainFilename.c_str(), config.mainFilename.c_str());

	mainFile = std::make_unique<BufferedFile>(config.mainFilename);
}

void IndexedSequentialFile::print(std::ostream& stream){
	stream << "Key" << std::setw(10) << "PageNo"<<std::endl;
	for (int i = 0; i < index.size(); i++) {
		stream << index[i] << std::setw(10) << i << std::endl;
	}
	stream << std::endl << std::endl;

	stream << std::setw(10)<< "Index" << std::setw(10) << "Key";
	for (int i = 0; i < Record::ElementsCount; i++)
		stream << std::setw(9) << "[" + std::to_string(i) + "]";
	stream<<std::setw(10) <<"pointer" << std::setw(10) << "Deleted" << std::endl;

	int indexInFile = 0;
	for (int i = 0; i < index.size(); i++) {
		stream << std::setw(10) << "Page nr " << i << std::endl;
		Record* records = mainFile.get()->readPageFromMain(i);
		for (int j = 0; j < Config::get().blockingFactor; j++) {
			const Record& record = records[j];
			stream << std::setw(10) << indexInFile++ << std::setw(10) << record.key;
			for (int k = 0; k < Record::ElementsCount; k++)
				stream << std::setw(9) << (record.type == RecordType::EMPTY ? "-" : std::to_string(record.elements[k]));
			stream << std::setw(10) << record.next / sizeof(Record);
			stream << std::setw(10) << (record.type == RecordType::DELETED);
			stream << std::endl;
		}
	}

	stream << "Overflow" << std::endl;
	size_t currentOverflowPosition = mainFile.get()->getOverflowBeginning();
	while (currentOverflowPosition != mainFile.get()->getNextOverflowPosition()) {
		const Record& record = *mainFile.get()->readSingleFromOverflow(currentOverflowPosition);
		stream << std::setw(10) << indexInFile++ << std::setw(10) << record.key;
		for (int k = 0; k < Record::ElementsCount; k++)
			stream << std::setw(9) << (record.type == RecordType::EMPTY ? "-" : std::to_string(record.elements[k]));
		stream << std::setw(10) << record.next / sizeof(Record);
		stream << std::setw(10) << (record.type == RecordType::DELETED);
		stream << std::endl;
		currentOverflowPosition += sizeof(Record);
	}
}


void IndexedSequentialFile::_createEmptySequentialFile(std::string mainFilename, std::string indexFilename){
	const Config& config = Config::get();

	FileCreator fileCreator(config.indexFilename, config.mainFilename);

	const size_t increment = (MAXIMUM_KEY_VALUE - MINIMUM_KEY_VALUE) / config.initialPageCount;
	for (int i = 0; i < config.initialPageCount; i++) {
		const size_t pageStartKey = MINIMUM_KEY_VALUE + increment * i;
		index.push_back(pageStartKey);

		Record initialRecord(RecordType::DELETED, pageStartKey);
		fileCreator.appendRecord(&initialRecord);
		fileCreator.flushPage();
	}

	fileCreator.writeIndex(index.data(), index.size());
}

Record* IndexedSequentialFile::_findInOverflowArea(RecordKeyType key, size_t position)
{
	while(position != 0) {
		Record* current = mainFile.get()->readSingleFromOverflow(position);
		if (current->key > key)
			break;

		if (current->key == key){
			if (current->type == RecordType::PRESENT)
				return current;
			else
				return nullptr;
		}

		position = current->next;
	}

	return nullptr;
}

size_t IndexedSequentialFile::_findPageNumberForKey(RecordKeyType key) {
	std::vector<RecordKeyType>::iterator foundPos = std::upper_bound(index.begin(), index.end(), key);
	if (foundPos == index.end())
		return index.size() - 1;
	return foundPos - index.begin() - 1;
}

int IndexedSequentialFile::_findRecordPositionOnPage(RecordKeyType key, Record* records) {
	const unsigned int recordsCount = Config::get().blockingFactor;
	for (int i = recordsCount - 1; i >= 0; i--) {
		const Record& record = records[i];
		if (record.type != RecordType::EMPTY && record.key <= key)
			return i;
	}
	return 0;
}

IndexedSequentialFile::Iterator::Iterator(IndexedSequentialFile& file) : _file(file){
	index = 0;
	pageIndex = 0;
	overflowPosition = 0;
}

const Record* IndexedSequentialFile::Iterator::_nextOverflow()
{
	while (overflowPosition != 0) {
		Record* current = _file.mainFile.get()->readSingleFromOverflow(overflowPosition);
		overflowPosition = current->next;
		if (current->type == RecordType::PRESENT)
			return current;
	}
	return nullptr;
}

const Record* IndexedSequentialFile::Iterator::next(){
	while (index < _file.index.size()) {

		const Record* nextOverflow = _nextOverflow();
		if (nextOverflow != nullptr)
			return nextOverflow;

		const Record* records = _file.mainFile.get()->readPageFromMain(index);
		while (pageIndex < Config::get().blockingFactor) {
			const Record* current = &records[pageIndex++];
			if (current->type == RecordType::PRESENT) {
				overflowPosition = current->next;
				return current;
			}
			else if (current->type == RecordType::DELETED) {
				overflowPosition = current->next;
				const Record* nextOverflow = _nextOverflow();
				if (nextOverflow != nullptr)
					return nextOverflow;
			}
		}

		index++;
		pageIndex = 0;
	}

	return nullptr;
}
