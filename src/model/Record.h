#pragma once

enum class RecordType : char {
	EMPTY = 0,
	DELETED,
	PRESENT
};

typedef unsigned short RecordKeyType;
static const RecordKeyType MINIMUM_KEY_VALUE = 0;


struct Record {
	static const int ElementsCount = 5;

	RecordType type;
	unsigned short key;
	int elements[ElementsCount];
	size_t next;
	
	Record(RecordType type = RecordType::EMPTY, unsigned short key = MINIMUM_KEY_VALUE) : elements() {
		this->type = type;
		this->key = key;
		this->next = 0;
	}
};
