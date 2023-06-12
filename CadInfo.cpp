#include <windows.h>
#include "vcLicense.h" 
#include <ios>
#include "dirent.h"
#include <sys/stat.h>
#include "version.h"
#include "vcCad2Xml.h"
#include "vcUtils.h"
#include "vcCadTranslator.h"

#include "SuiteVersion_V2.h"
char* g_sAppName = NULL;

#include "VctCaxScene.h"
#include "VctException.h"
using namespace Vct;
typedef Cax::uint32_t uint32_t;


std::string InputPath,OutputPath; 
std::vector<float *> FloatArrays;
std::vector<uint32_t *> IntArrays;
bool g_bOutputXml = false;
bool g_bIgnoreTransparency = false;
int g_iXMLFormat = vcCad2Xml::XML_FORMAT::GENERIC;
bool g_bUserLogPath = false;
std::string g_sLogFilePath;
extern int g_iErrorCode;


bool g_bMesh = true;
bool g_b2DElements = true;
bool g_bPointSet = false;
bool g_bCombinePartsInGroup = true;

extern bool g_bValidInputFormat;
//static variable initialization
std::string vcUtils::m_sVMoveCADTmpDir;

std::map<std::string,int>  g_sOutputXMLFilesFormatMap;

void PrintSyntax()
{
	printf("\nusage : CadInfo.exe [options] <cad_file> \n");
}

bool ParseCmdLineArguments(int argc, char *argv[])
{
	if(argc<2)
	{
		g_iErrorCode = vcUtils::ARGUMENT_ERROR;
		return false;
	}

	for(int i = 0; i < argc; i++ )
    {
		if(i==0)
		{
			if(argc == i+1)
			{
				g_iErrorCode = vcUtils::ARGUMENT_ERROR;
				return false;
			}
			continue;
		}
		else if(strstr(argv[i],"--ekm")!=NULL)
		{
			if(strcmp(argv[i],"--ekm-meta-data")!=0 && strcmp	(argv[i],"--ekm-report")!=0 && strcmp(argv[i], "--ekm-asm-dep") != 0)
			{
				printf("\nError: Invalid EKM option");
				g_iErrorCode = vcUtils::ARGUMENT_ERROR;
				return false;
			}
			if(argc == i+1 && InputPath.length()==0)
			{
				g_iErrorCode = vcUtils::ARGUMENT_ERROR;
				return false;
			}
			g_bOutputXml = true;
			if(strcmp(argv[i],"--ekm-meta-data")==0 )
			{
				OutputPath = std::string("ekm-meta-data.xml");
				g_iXMLFormat = vcCad2Xml::XML_FORMAT::EKM_META_DATA;
				g_sOutputXMLFilesFormatMap[OutputPath] = g_iXMLFormat;
			}
			else if(strcmp(argv[i],"--ekm-report")==0)
			{
				OutputPath = std::string("ekm-report.xml");
				g_iXMLFormat = vcCad2Xml::XML_FORMAT::EKM_REPORT;
				g_sOutputXMLFilesFormatMap[OutputPath] = g_iXMLFormat;
			}
			else if (strcmp(argv[i], "--ekm-asm-dep") == 0)
			{
				OutputPath = std::string("minerva-dependency.xml");
				g_iXMLFormat = vcCad2Xml::XML_FORMAT::EKM_ASM_DEP_EXTRACT;
				g_sOutputXMLFilesFormatMap[OutputPath] = g_iXMLFormat;
			}
		}
		else if(strstr(argv[i],"--key")!=NULL)
		{
			if(strcmp(argv[i],"--key-value-pairs")!=0 )
			{
				g_iErrorCode = vcUtils::ARGUMENT_ERROR;
				printf("\nError: Invalid key-value-pair option");
				return false;
			}
			g_bOutputXml = true;
			g_iXMLFormat = vcCad2Xml::XML_FORMAT::KEY_VALUE_PAIRS;
		}
		else if(strstr(argv[i],"--output")!=NULL)
		{
			if(strstr(argv[i],"--output=")==NULL)
			{
				g_iErrorCode = vcUtils::ARGUMENT_ERROR;
				printf("\nError: Invalid Output option!");
				return false;
			}
			g_bOutputXml = true;
			OutputPath = argv[i];
			std::string sOutputOption = "--output="; 
			size_t found;
			found = OutputPath.find(sOutputOption);
			if(found != std::string::npos)
			{
				OutputPath.erase(0,sOutputOption.length());
			}
			g_sOutputXMLFilesFormatMap[OutputPath] = g_iXMLFormat;
		}
		else if(strstr(argv[i],"--log-file-path")!=NULL)
		{
			std::string sOption = argv[i];
			std::string sOptionTag = "--log-file-path="; 
			size_t found;
			found = sOption.find(sOptionTag);
			if(found != std::string::npos)
			{
				sOption.erase(0,sOptionTag.length());
			}
			if(sOption.length()==0)
			{
				g_iErrorCode = vcUtils::LOG_ERROR;
				printf("Error: Invalid log file.Please provide the proper log file path!!!\n");
				return false;
			}

			int index = sOption.find_last_of("\\");
			if(index!= -1)
			{
				std::string outputdir = sOption.substr(0,index);
				if(!vcUtils::IsValidDirectory(outputdir))
				{
					g_iErrorCode = vcUtils::LOG_ERROR;
					printf("\nError:Log file directory is not available. Please provide the proper log file path!!!\n");
					return false;
				}
			}
			index = sOption.find_last_of('.');
			if(index == -1 || sOption.length()<=4)
			{
				g_iErrorCode = vcUtils::LOG_ERROR;
				printf("Error: Invalid log file.Please provide the proper log file path!!!\n");
				return false;
			}
			std::string ext = vcUtils::GetFileExt(sOption);
			if(ext.length()<3)
			{
				g_iErrorCode = vcUtils::LOG_ERROR;
				printf("Error: Invalid log file extension.Please provide the proper log file path!!!\n");
				return false;
			}
			g_bUserLogPath = true;
			g_sLogFilePath = sOption;
		}
		else  if(strstr(argv[i],"--model-file-format")!=NULL)
		{
			std::string sOption = argv[i];
			std::string sOptionTag = "--model-file-format="; 
			size_t found;
			found = sOption.find(sOptionTag);
			if(found != std::string::npos)
			{
				sOption.erase(0,sOptionTag.length());
			}
			if(sOption.length()==0)
			{
				g_iErrorCode = vcUtils::FORMAT_ERROR;;
				printf("\n\nError: Invalid model file format.Please provide the proper model file!!!\n");
				vcUtils::LogMsg("Error: model file format.");
				return false;
			}
			g_bValidInputFormat = vcCadTranslator::CheckForValidUnlistedFormat(sOption);
		}
		else if (strcmp(argv[i], "-o") == 0)
		{
			if (strstr(argv[1], "--ekm") == NULL)
			{
				printf("\nError: Invalid Input option");
				g_iErrorCode = vcUtils::ARGUMENT_ERROR;
				return false;
			}
			if (argc < 5)
			{
				printf("\nError: Invalid EKM -o option"); 
				g_iErrorCode = vcUtils::ARGUMENT_ERROR;
				return false;
			}
		}
		else if (i == 4 && strstr(argv[1], "--ekm") != NULL && strcmp(argv[3], "-o") == 0 && argc >= 5)
		{
			OutputPath = argv[4];
			g_sOutputXMLFilesFormatMap[OutputPath] = g_iXMLFormat;
		}
		else 
		{
				InputPath = argv[i]; 
			//printf("Error: Invalid option %d. Please refer VMoveCAD online help.\n",i-1);
			//return false;
		}
	}
	//InputPath = argv[argc-1]; 
	return true;
}
bool ValidateInput()
{
	if(!vcCadTranslator::IsSupportedInputFormat(InputPath))
	{
		vcUtils::LogMsg("CadInfo::Error: Unsupported input format"); 
		printf("\nError: Unsupported input format!!!\n");
		g_iErrorCode = vcUtils::FORMAT_ERROR;
		return 0;
	}
	if(!vcUtils::IsFileAvailable(InputPath))
	{
		vcUtils::LogMsg("CadInfo::Error: Input file is not found"); 
		printf("\nError: Input file is not found.\n");
		g_iErrorCode = vcUtils::FILE_ACCESS_ERROR;
		return false;
	}
	return true;
}

bool ValidateOutput()
{
	if(g_bOutputXml)
	{
		std::string ext;
		ext = vcUtils::GetFileExt(OutputPath);
		if(!vcUtils::StrCompare(ext,std::string("xml")))
		{
			vcUtils::LogMsg("CadInfo::Error: Unsupported output format"); 
			printf("\nError: Unsupported output format!!!\n");
			g_iErrorCode = vcUtils::FORMAT_ERROR;
			return false;
		}
		size_t found = OutputPath.find_last_of("\\");
		if(found != std::string::npos)
		{
			std::string outputdir = OutputPath.substr(0,found);
			if(!vcUtils::IsValidDirectory(outputdir))
			{
				printf("\nError: Output directory is not available.\n");
				vcUtils::LogMsg("CadInfo::Error: Output directory is not available"); 
				g_iErrorCode = vcUtils::FILE_ACCESS_ERROR;
				return false;
			}
		}
		if(!vcUtils::CheckWritePermission(OutputPath))
		{
			vcUtils::LogMsg("CadInfo::Error: Permission denied for writing the xml file"); 
			g_iErrorCode = vcUtils::FILE_ACCESS_ERROR;
			return false;
		}
	}
	return true;
}

void WriteAppVersionIntoLogFile()
{
	std::stringstream ss;
	ss<<"CadInfo::Version:: "<<g_sVersion;
	vcUtils::LogMsg(ss.str());

#ifdef _WIN64
	vcUtils::LogMsg("CadInfo: Win64 Application");
#else
	vcUtils::LogMsg("CadInfo: Win32 Application");
#endif

	ss.str("");
	ss<<"CadInfo: DTK Version: "<<g_sDatakitVersion;
	vcUtils::LogMsg(ss.str());
}

void WriteOSVersionIntoLogFile()
{
	wchar_t *ver = new wchar_t[1024];
	vcUtils::GetWindowsVersionName(ver,1024);
	std::stringstream ss;
	ss<<"CadInfo::OS Version:: "<<vcUtils::ToNarrow(ver);
	vcUtils::LogMsg(ss.str());
	delete ver;
}
void WriteSystemInfo()
{
	MEMORYSTATUSEX statex;
    statex.dwLength = sizeof (statex);
    GlobalMemoryStatusEx (&statex);

	std::stringstream ss;
	ss<<"CadInfo: System Memory size: "<<statex.ullTotalPhys/(1024*1024)<<" MB";
	vcUtils::LogMsg(ss.str());
}
bool CheckOutLicense()
{
	return vcLicense::CheckOutLicense();
}

int main(int argc,char *argv[])
{
	g_iErrorCode=vcUtils::NOT_ERROR;

	CSuiteVersion sv;
	g_sAppName = sv.GetAppName("vmovecad");

    if(argc > 1 && strstr(argv[1],"--version")!=NULL)
	{
		printf("\nCadInfo (64-bit) %s\n",g_sVersion);
		printf("Visual Collaboration Technologies Inc. Copyright(C) 2021 \n\n");
		return 1;
    }
	if(!ParseCmdLineArguments(argc,argv))
	{
		PrintSyntax();
		return g_iErrorCode;//vcUtils::ARGUMENT_ERROR;
	}

	bool bLog=true;
	if(g_bUserLogPath)
		bLog = vcUtils::InitLog(g_sLogFilePath,g_bUserLogPath);
	else
		bLog = vcUtils::InitLog("CadInfo_Log.txt");

	if(!bLog)
	{
		printf("\nPermission denied for writing the Log file. Please verify whether you have the write permission!\n");
		return vcUtils::LOG_ERROR;
	}

	printf("\nSetting Log file to '%s'\n\n",vcUtils::m_sLogFileName.c_str());

	WriteAppVersionIntoLogFile();

	WriteOSVersionIntoLogFile();

	WriteSystemInfo();

	std::stringstream ss;
	ss<<"CadInfo::Arguments:";
	for(int i=0;i<argc;i++)
	{
		ss<<"\""<<argv[i]<<"\" ";
	}
	vcUtils::LogMsg(ss.str());

	if(!CheckOutLicense())
		return vcUtils::LICENSE_ERROR;

	bool bSuccess = true;

	if(!ValidateInput())
	{
		vcLicense::CheckInLicense(); 
		return g_iErrorCode;
	}
	if(!ValidateOutput())
	{
		vcLicense::CheckInLicense(); 
		return g_iErrorCode;
	}

	std::string sTmpDir = vcUtils::m_sVMoveCADTmpDir;
	try
	{
		vcCad2Xml datakit(const_cast<char *>(InputPath.c_str()),const_cast<char *>(OutputPath.c_str()),const_cast<char *>(sTmpDir.c_str()),bSuccess);

		if(bSuccess)
		{
			g_iErrorCode=vcUtils::NOT_ERROR;
			vcUtils::LogMsg("CadInfo::Meta data extracted successfully"); 
			printf("\nMeta data extracted successfully!!!\n");
		}
		else
		{
			if(datakit.m_dtkErrorStatus == dtkErrorVersionNotSupported)
			{
				g_iErrorCode = vcUtils::VERSION_ERROR;
			}
			else if(datakit.m_dtkErrorStatus == dtkErrorLicence)
			{
				g_iErrorCode = vcUtils::LICENSE_ERROR;
			}
			else if(datakit.m_dtkErrorStatus == dtkErrorUnavailableReader)
			{
				g_iErrorCode = vcUtils::FORMAT_ERROR;
			}
			else if(datakit.m_iErrorCode == vcCad2Xml::FILE_ACCESS_ERROR)
			{
				g_iErrorCode = vcUtils::FILE_ACCESS_ERROR;
			}
			else if(datakit.m_iErrorCode == vcCad2Xml::DATAKIT_ERROR)
			{
				g_iErrorCode = vcUtils::READER_API_ERROR; 
			}
			else if(datakit.m_iErrorCode == vcCad2Xml::XML_WRITER_ERROR)
			{
				g_iErrorCode = vcUtils::WRITER_API_ERROR;
			}	
			vcUtils::LogMsg("CadInfo::Meta data extraction failed"); 
			printf("\nMeta data extraction failed!!!\n");
		}
	}
	catch(...)
	{
		g_iErrorCode=vcUtils::UNKNOWN_ERROR;
		vcUtils::LogMsg("CadInfo::Meta data extraction failed"); 
		printf("\nMeta data extraction failed!!!\n");
	}
	
	vcLicense::CheckInLicense(); 
	return g_iErrorCode;
}



