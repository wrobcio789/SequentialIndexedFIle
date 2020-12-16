#pragma once
#include "BufferedFile.h"
#include <iostream>
#include "../../config/config.h"

BufferedFile::BufferedFile(const std::string & filename)
	: _stream(filename, std::fstream::in | std::fstream::out | std::fstream::binary | std::fstream::ate)
{
	if (!_stream.is_open()) {
		std::cerr << "File " + filename + " could not be opened" << std::endl;
	}

	const Config& config = Config::get();
	const size_t fileSizeInBytes = _stream.tellg();
	_pageSizeInBytes = config.blockingFactor * sizeof(Record);
	const size_t fileSizeInPages = fileSizeInBytes / _pageSizeInBytes; 

	_mainPagesCount = fileSizeInPages;
	_overflowPagesCount = std::max<size_t>(fileSizeInPages * config.overflowRatio, 1u);
	_nextOverflowPosition = fileSizeInBytes;

	
	assert((fileSizeInPages * _pageSizeInBytes == fileSizeInBytes));
}

BufferedFile::~BufferedFile() {
	_flushBuffer(_mainBuffer);
	_flushBuffer(_overflowBuffer);
	_stream.close();
}

Record* BufferedFile::readPageFromMain(size_t pageNumber) {
	assert((pageNumber < _mainPagesCount));

	const size_t streamPosition = pageNumber * _pageSizeInBytes;
	_readData(streamPosition, _mainBuffer);
	return reinterpret_cast<Record*>(_mainBuffer.data);
}


void BufferedFile::writeMainPage() {
	_mainBuffer.shouldBeFlushed = true;
}

Record* BufferedFile::readSingleFromOverflow(size_t position){
	const size_t pageNumber = position / (Config::get().blockingFactor * sizeof(Record));
	assert((pageNumber >= _mainPagesCount && pageNumber < _mainPagesCount + _overflowPagesCount));

	const size_t streamPosition = pageNumber * _pageSizeInBytes;
	_readData(streamPosition, _overflowBuffer);

	const size_t offset = position - streamPosition;
	const size_t recordIndex = offset / sizeof(Record);

	return reinterpret_cast<Record*>(_overflowBuffer.data) + recordIndex;
}

void BufferedFile::writeOverflowRecord(){
	_overflowBuffer.shouldBeFlushed = true;
}

void BufferedFile::appendRecordInOverflow(const Record& record){
	Record* newRecordPtr = readSingleFromOverflow(_nextOverflowPosition);
	*newRecordPtr = record;
	writeOverflowRecord();
	_nextOverflowPosition += sizeof(Record);
}

size_t BufferedFile::getNextOverflowPosition(){
	return _nextOverflowPosition;
}

size_t BufferedFile::getOverflowBeginning() {
	return _mainPagesCount * _pageSizeInBytes;
}

bool BufferedFile::isFull(){
	return _nextOverflowPosition / _pageSizeInBytes >= _mainPagesCount + _overflowPagesCount;
}

void BufferedFile::_readData(size_t position, Buffer& buffer) {
	if (buffer.associatedPosition == position)
		return;

	_flushBuffer(buffer);

	_stream.clear();
	_stream.seekg(position);
	_stream.read(buffer.data, _pageSizeInBytes);
	buffer.associatedPosition = position;
}

void BufferedFile::_flushBuffer(Buffer& buffer) {
	if (buffer.shouldBeFlushed) {
		_stream.clear();
		_stream.seekp(buffer.associatedPosition);
		_stream.write(buffer.data, _pageSizeInBytes);
		buffer.shouldBeFlushed = false;
	}
}