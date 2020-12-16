#include "Configurator.h"
#include "ArgsParser.h"
#include "config.h"

void Configurator::configure(int argsCount, char* args[]){
	const ArgsParser argsParser(argsCount, args);
	Config& config = Config::get();


	config.blockingFactor = argsParser.getValue("blocking").asInt(4);
	config.overflowRatio = argsParser.getValue("overflowRatio").asFloat(0.2);
	config.initialPageCount = argsParser.getValue("pageCount").asInt(10);
	config.pageUtilisationFactor = argsParser.getValue("utilFactor").asFloat(0.5);
}
