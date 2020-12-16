#include <ostream>
class Statistics {
private:
	size_t _pagesWritten = 0;
	size_t _pagesRead = 0;

public:
	Statistics() = default;
	Statistics(const Statistics&) = default;

	static Statistics& get() {
		static Statistics _instance;
		return _instance;
	}

	void registerRead(int count = 1) {
		_pagesRead += count;
	}

	void registerWrite(int count = 1) {
		_pagesWritten += count;
	}

	void print(std::ostream& stream) const {
		stream << "STATISTICS\n"
			<< "Pages written: " << _pagesWritten << "\n"
			<< "Pages read: " << _pagesRead << "\n"
			<< "Total disk operations: " << _pagesWritten + _pagesRead << std::endl;
	}
};