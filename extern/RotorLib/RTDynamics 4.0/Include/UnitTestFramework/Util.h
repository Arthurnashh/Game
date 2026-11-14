#pragma once
#include <vector>

std::string getReferenceDataDir();

void compareLogsAndPrintResults( const std::string& scenarioName , 
										 const std::string& testLog, const std::string& referenceLog,
										 const std::vector<std::string>& ignoredChannels,
										 const std::string& commandToExecuteForError,
										 bool suggestReplacingFile );

