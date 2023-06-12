#pragma once

#include <string>
#include <vector>

class vcCadTranslator
{
public:
	vcCadTranslator(void);
	~vcCadTranslator(void);

	static bool Translate(std::string InputPath,std::string OutputPath,bool bFileMode,bool &bSuccess);
	static bool IsSupportedInputFormat(std::string InputPath);
	static bool IsSupportedOutputFormat(std::string OutputPath);
	static bool ValidateInputs(std::string InputPath,std::string OutputPath,bool bFileMode);
	static bool IsFileNameExist(std::vector<std::string> InputFileNamesStrArray,std::string InputFilePath);
	static bool CheckForValidUnlistedFormat(std::string sModelFileFormat);

};
