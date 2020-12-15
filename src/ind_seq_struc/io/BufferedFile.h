#pragma once
#include <fstream>
#include "../../model/Record.h"
#include "../../config/config.h"
#include <cassert>

class Buffer {
public:
	char* data;
	bool shouldBeFlushed;
	size_t associatedPosition;

	Buffer() {
		shouldBeFlushed = false;
		associatedPosition = SIZE_MAX;
		data = new char[Config::get().blockingFactor & sizeof(Record)];
	}

	~Buffer() {
		delete[] data;
	}
};

class BufferedFile
{
private:
	std::fstream _stream;
	std::size_t _mainPagesCount;
	std::size_t _overflowPagesCount;
	std::size_t _pageSizeInBytes;

	Buffer _mainBuffer;
	Buffer _overflowBuffer;

	void _flushBuffer(Buffer& buffer);
	void _readData(size_t position, Buffer& buffer);

public:

	BufferedFile(const std::string & filename);
	~BufferedFile();

	Record* readPageFromMain(size_t pageNumber);
	void writePageToMain();

	Record readFromOverflowArea(size_t position);
};
