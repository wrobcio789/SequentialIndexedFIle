#pragma once
#include "BufferedFile.h"
#include <iostream>
#include "../../config/config.h"

BufferedFile::BufferedFile(const std::string & filename)
	: _stream(filename, std::fstream::in | std::fstream::out | std::fstream::binary | std::fstream::ate)
{
	if (!_stream.is_open()) {
		std::cerr << "File " + filename + " could not be opened" << std::endl;
		exit(-1);
	}

	const Config& config = Config::get();
	const size_t fileSizeInBytes = _stream.tellg();
	_pageSizeInBytes = config.blockingFactor * sizeof(Record);
	const size_t fileSizeInPages = fileSizeInBytes / _pageSizeInBytes; 

	_mainPagesCount = std::max(fileSizeInPages * config.mainToOverflowRatioNumerator / config.mainToOverflowRatioDenominator, 1u);
	_overflowPagesCount = std::max(fileSizeInPages * (config.mainToOverflowRatioDenominator - config.mainToOverflowRatioNumerator) / config.mainToOverflowRatioDenominator, 1u);

	
	assert((fileSizeInPages * _pageSizeInBytes == fileSizeInBytes));
}

BufferedFile::~BufferedFile() {
	_flushBuffer(_mainBuffer);
	_flushBuffer(_overflowBuffer);
	_stream.close();
}

Record* BufferedFile::readPageFromMain(size_t pageNumber) {
	assert((pageNumber < _mainPagesCount));

	const size_t streamPosition = pageNumber * _pageSizeInBytes; //TODO change buffering
	_readData(streamPosition, _mainBuffer);
	return reinterpret_cast<Record*>(_mainBuffer.data);
}


void BufferedFile::writePageToMain() {
	_mainBuffer.shouldBeFlushed = true;
}

Record BufferedFile::readFromOverflowArea(size_t position){
	const size_t pageNumber = (position) / Config::get().blockingFactor;
	assert((pageNumber >= _mainPagesCount && pageNumber < _mainPagesCount + _overflowPagesCount));

	const size_t streamPosition = pageNumber * Config::get().blockingFactor;
	_readData(streamPosition, _overflowBuffer);

	const size_t offset = position - streamPosition;
	const size_t recordIndex = offset / sizeof(Record);

	return reinterpret_cast<Record*>(_overflowBuffer.data)[recordIndex];
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