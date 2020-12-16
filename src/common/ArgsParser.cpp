#include "ArgsParser.h"
#include <stdexcept>
#include "ArgsParser.h"

ArgsParser::ArgsParser(int argsCount, char* args[]) {

	for (int i = 1; i < argsCount; i++) {
		std::string currentArgument(args[i]);
		if (_isArgumentLabel(currentArgument)) {
			if (i + 1 < argsCount && _isArgumentValue(args[i + 1]))
			{
				std::string argumentValue(args[i + 1]);
				_argsMap.emplace(_truncatePreffix(currentArgument), ArgumentMarshaller(argumentValue));
				i++;
			}
			else
				_setFlags.insert(_truncatePreffix(currentArgument));
		}
		else
			throw(ArgsParserError("Unrecognized symbol: " + std::string(currentArgument)));
	}
}

bool ArgsParser::isFlagSet(const std::string& arg) const
{
	return _setFlags.find(arg) != _setFlags.end();
}

bool ArgsParser::isArgumentSpecified(const std::string& arg) const
{
	return _argsMap.find(arg) != _argsMap.end();
}

const ArgumentMarshaller& ArgsParser::getValue(const std::string& arg) const
{
	const auto& argPosition = _argsMap.find(arg);
	if (argPosition == _argsMap.end())
		return ArgumentMarshaller::empty;
	return argPosition->second;
}

bool ArgsParser::_isArgumentValue(const std::string& argument) const {
	return argument.front() != '-';
}

bool ArgsParser::_isArgumentLabel(const std::string& argument) const {
	return argument.front() == '-';
}

std::string ArgsParser::_truncatePreffix(const std::string& argument) const
{
	return argument.substr(1);
}

const ArgumentMarshaller ArgumentMarshaller::empty = ArgumentMarshaller("");

ArgumentMarshaller::ArgumentMarshaller(std::string value)
	: _value(value)
{}

bool ArgumentMarshaller::operator==(const ArgumentMarshaller& other) const {
	return _value == other._value;
}

bool ArgumentMarshaller::asBool(bool _default) const {
	if (_value == "1" || _value == "true")
		return true;
	else if (_value == "0" || _value == "false")
		return false;
	return _default;

}

int ArgumentMarshaller::asInt(int _default) const {
	try {
		return std::stoi(_value);
	}
	catch (const std::exception&) {
		return _default;
	}
}

long long ArgumentMarshaller::asLongLong(long long _default) const {
	try {
		return std::stoll(_value);
	}
	catch (const std::exception&) {
		return _default;
	}
}

std::string ArgumentMarshaller::asString(std::string _default) const
{
	return _value.empty() ? _default : _value;
}

float ArgumentMarshaller::asFloat(float _default) const{
	try {
		return std::stof(_value);
	}
	catch (const std::exception&) {
		return _default;
	}
}

