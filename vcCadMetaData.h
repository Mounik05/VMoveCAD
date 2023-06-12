#pragma once

#include <ios>
#include<vector>

class vcCadMetaData
{
public:
	std::string m_sAssemblyName;
	std::string m_sUnitsName;
	std::string m_sNativeCadFileName;
	std::string m_sNativeCadPackageName;
	std::string m_sNativeCadPackageVersion;

	std::string m_sAuthorName;
	std::string m_sOrganizationName;
	std::string m_sDate;

	int m_iNumberOfAssemblies;
	std::vector<std::string> m_AssemblyNamesStrArray;
	int m_iNumberOfParts;
	std::vector<std::string> m_PartNamesStrArray;

	std::vector<std::string> m_MaterialNamesStrArray;



	vcCadMetaData(void);
	~vcCadMetaData(void);
};
