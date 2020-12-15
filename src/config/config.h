#pragma once

class Config
{
public:
	unsigned int blockingFactor = 4;
	unsigned int mainToOverflowRatioNumerator = 4;
	unsigned int mainToOverflowRatioDenominator = 5;

	std::string indexFilename = "data.indseq.index";
	std::string mainFilename = "data.indseq";

	static Config& get() {
		static Config _instance;
		return _instance;
	}

private:
	Config() = default;
	Config(Config&) = default;
};

