#pragma once
#include <string>

class Config
{
public:
	unsigned int blockingFactor = 4;
	float overflowRatio = 0.2;
	float pageUtilisationFactor = 0.5;
	unsigned int initialPageCount = 3;

	std::string indexFilename = "data.indseq.index";
	std::string mainFilename = "data.indseq";

	std::string tempIndexFilename = "data.indseq.index.tmp";
	std::string tempMainFilename = "data.indseq.tmp";

	static Config& get() {
		static Config _instance;
		return _instance;
	}

private:
	Config() = default;
	Config(Config&) = default;
};

