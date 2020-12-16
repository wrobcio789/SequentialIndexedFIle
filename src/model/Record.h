#pragma once

enum class RecordType : short {
	EMPTY = 0,
	DELETED,
	PRESENT
};

typedef unsigned int RecordKeyType;
static const RecordKeyType MINIMUM_KEY_VALUE = 0;
static const RecordKeyType MAXIMUM_KEY_VALUE = 50 * 1000 * 1000;


struct Record {
	static const int ElementsCount = 5;

	int elements[ElementsCount];
	size_t next;
	RecordType type;
	RecordKeyType key;
	
	Record(RecordType type = RecordType::EMPTY, unsigned short key = MINIMUM_KEY_VALUE) : elements() {
		this->type = type;
		this->key = key;
		this->next = 0;
	}
};
