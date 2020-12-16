#pragma once
#include "IndexedSequentialFile.h"
#include "../config/config.h"
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <string>

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

	if (recordPageIndex < Config::get().blockingFactor - 1 && records[recordPageIndex + 1].type == RecordType::EMPTY) {
		records[recordPageIndex + 1] = record;
		mainFile.get()->writeMainPage();
		return true;
	}

	if (candidate.next == 0) {
		candidate.next = mainFile.get()->getNextOverflowPosition();
		mainFile.get()->writeMainPage();
		mainFile.get()->appendRecordInOverflow(record);
		return true;
	}

	size_t currentPosition = candidate.next;
	while(true){
		Record* currentRecord = mainFile.get()->readSingleFromOverflow(currentPosition);

		if (currentRecord->key == key)
			return false;

		if (currentRecord->key > key) {
			Record oldRecordCopy = *currentRecord;
			*currentRecord = record;
			currentRecord->next = mainFile.get()->getNextOverflowPosition();
			mainFile.get()->writeOverflowRecord();
			mainFile.get()->appendRecordInOverflow(oldRecordCopy);
			return true;
		}

		if (currentRecord->next == 0) {
			currentRecord->next = mainFile.get()->getNextOverflowPosition();
			mainFile.get()->writeOverflowRecord();
			mainFile.get()->appendRecordInOverflow(record);
			return true;
		}

		currentPosition = currentRecord->next;
	}

	//TODO: checkForReorganization;
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
		if (key == record.key)
			*candidate = record;
		else
			candidate->type = RecordType::DELETED;

		mainFile.get()->writeMainPage();
		return add(record);
	}

	Record* recordInOverflow = _findInOverflowArea(key, candidate->next);
	if (recordInOverflow != nullptr) {

		if (key == record.key)
			*recordInOverflow = record;
		else
			recordInOverflow->type = RecordType::DELETED;

		mainFile.get()->writeOverflowRecord();
		return add(record);
	}

	return false;
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

	std::fstream mainStream(mainFilename, std::fstream::out | std::fstream::binary);
	if (!mainStream.is_open()) {
		std::cerr << "File " + mainFilename + " could not be opened" << std::endl;
		exit(-1);
	}

	std::fstream indexStream(indexFilename, std::fstream::out | std::fstream::binary);
	if (!indexStream.is_open()) {
		std::cerr << "File " + indexFilename + " could not be opened" << std::endl;
		exit(-1);
	}

	//At the beginning we have only 1 page
	index.push_back(0);
	indexStream.write(reinterpret_cast<char*>(index.data()), index.size() * sizeof(RecordKeyType));

	Record* mainBuffer = new Record[config.blockingFactor];
	mainBuffer[0] = Record(RecordType::DELETED, MINIMUM_KEY_VALUE);
	mainStream.write(reinterpret_cast<char*>(mainBuffer), config.blockingFactor * sizeof(Record));

	indexStream.close();
	mainStream.close();
	delete[] mainBuffer;
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
	return index.begin() - foundPos - 1;
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
