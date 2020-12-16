#pragma once
#include <string>
#include <map>
#include <set>

class ArgumentMarshaller {
private:
	std::string _value;

public:
	static const ArgumentMarshaller empty;

	ArgumentMarshaller(std::string value);

	bool operator==(const ArgumentMarshaller& other) const;

	bool asBool(bool = false) const;
	int asInt(int = 0) const;
	long long asLongLong(long long = 0) const;
	std::string asString(std::string = "") const;
	float asFloat(float = 0.0) const;
};

class ArgsParserError : public std::exception {
	std::string _message;
public:
	ArgsParserError(std::string message) : _message(message) {}

	const char* what() const override {
		return _message.c_str();
	}
};

class ArgsParser
{
private:
	std::map<std::string, ArgumentMarshaller> _argsMap;
	std::set<std::string> _setFlags;

	bool _isArgumentValue(const std::string& argument) const;

	bool _isArgumentLabel(const std::string& argument) const;

	std::string _truncatePreffix(const std::string& argument) const;

public:
	ArgsParser(int argsCount, char* args[]);

	bool isFlagSet(const std::string& arg) const;

	bool isArgumentSpecified(const std::string& arg) const;

	const ArgumentMarshaller& getValue(const std::string& arg) const;
};

