#include <windows.h>
#include <ios>
#include <fstream>
#include <sys/stat.h>
#include <sstream>
#include <time.h>
#include <iostream>
#include <vector>
#include "version.h"
#include "vcUtils.h"
#include "vcCadTranslator.h"
#include "vcLicense.h"
#include "vcCad2Cax.h"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/format.hpp>

#include "SuiteVersion_V2.h"
char* g_sAppName = NULL;

#define VMOVECAD_BATCH 1

extern int g_iParseMode;
bool g_bMesh = true;
bool g_b2DElements = true;
bool g_bPointSet = false;
bool g_bCombinePartsInGroup = true;
extern bool g_bTestResults;
std::string g_sTestResultsFileName;
bool bFileMode = true;
std::string InputPath,OutputPath;
std::vector<float *> FloatArrays;
std::vector<uint32_t *> IntArrays;
extern std::string sOutputFormat;
bool g_bIgnoreTransparency = false;
extern int g_iInputFilesCount;
extern int g_iOutputFilesCount;
extern char g_sVersion[20];
extern double g_fTessTolerance;
extern bool g_bIgnoreTransparency;
bool g_bUserLogPath = false;
std::string g_sLogFilePath;
extern int g_iUnit;
extern bool g_bUserUnits;
extern bool g_bDetailedLog;
extern bool g_bDataLoss;
extern int g_iErrorCode;
extern bool g_bValidInputFormat;

void PrintSyntax()
{
	printf("Usage: VMoveCAD_Batch.exe [option=value] <input_CAD_file/input_directory> <output_CAX_file/output_directory>\n");
	/*printf("Options:\n");
	printf("--dir\n");
	printf("--output-format=cax/cgr\n");
	printf("--tess-tolerance=0.05\n");*/
}

bool ParseCmdLineArguments(int argc, char *argv[])
{
	if(argc<3)
	{
		g_iErrorCode = vcUtils::ARGUMENT_ERROR;
		return false;
	}

	sOutputFormat = "cax";
	bFileMode = true;
#ifdef PART_WISE_TESSELLATION
	double g_fTessTolerance = 0.0005f;
#else
	double g_fTessTolerance = 0.05f;
#endif
	g_iParseMode = 1;
	g_bIgnoreTransparency = false;
	g_bDetailedLog = false;
	g_bTestResults = false;
	g_bCombinePartsInGroup = true;
	for(int i = 0; i < argc-2; i++ )
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
		if(vcUtils::StrCompare(static_cast<std::string>(argv[i]),std::string("--dir")))
		{
			bFileMode = false;
		}
		else if(strstr(argv[i],"--output-format")!=NULL)
		{
			std::string sOption = argv[i];
			std::string sOptionTag = "--output-format="; 
			size_t found;
			found = sOption.find(sOptionTag);
			if(found != std::string::npos)
			{
				sOption.erase(0,sOptionTag.length());
			}
			for(int j=sOption.length()-1;j>=0;j--)
			{
				if(sOption[j]=='\"')
					sOption.erase(j,1);
			}
			if(vcUtils::StrCompare(sOption,std::string("cax")))
				sOutputFormat = "cax";
			else if (vcUtils::StrCompare(sOption,std::string("cgr")))
				sOutputFormat = "cgr";
		}
		else if(strstr(argv[i],"--tess-tolerance")!=NULL)
		{
			std::string sOption = argv[i];
			std::string sOptionTag = "--tess-tolerance="; 
			size_t found;
			found = sOption.find(sOptionTag);
			if(found != std::string::npos)
			{
				sOption.erase(0,sOptionTag.length());
			}
			for(int j=sOption.length()-1;j>=0;j--)
			{
				if(sOption[j]=='\"')
					sOption.erase(j,1);
			}
			g_fTessTolerance = atof(sOption.c_str());
		}
		else if (strstr(argv[i], "--set-parse-mode") != NULL)
		{
			std::string sOption = argv[i];
			std::string sOptionTag = "--set-parse-mode=";
			size_t found;
			found = sOption.find(sOptionTag);
			if (found != std::string::npos)
			{
				sOption.erase(0, sOptionTag.length());
			}
			for (int j = sOption.length() - 1; j >= 0; j--)
			{
				if (sOption[j] == '\"')
					sOption.erase(j, 1);
			}
			g_iParseMode = boost::lexical_cast<int>(sOption.c_str());
		}
		else if(strstr(argv[i],"--ignore-transparency")!=NULL)
		{
			g_bIgnoreTransparency = true;
		}
		else if (strstr(argv[i], "--disable-combine-parts") != NULL)
		{
			g_bCombinePartsInGroup = false;;
		}
		else if(strstr(argv[i],"--detailed-log")!=NULL)
		{
			g_bDetailedLog = true;
		}
		else if(strstr(argv[i],"--export-surface")!=NULL)
		{
			g_bMesh = true;
			std::string sOption = argv[i];
			std::string sOptionTag = "--export-surface="; 
			size_t found;
			found = sOption.find(sOptionTag);
			if(found != std::string::npos)
			{
				sOption.erase(0,sOptionTag.length());
				if(vcUtils::StrCompare(sOption,std::string("false")))
					g_bMesh = false;
			}
		}
		else if(strstr(argv[i],"--export-lines")!=NULL)
		{
			g_b2DElements = true;
			std::string sOption = argv[i];
			std::string sOptionTag = "--export-lines="; 
			size_t found;
			found = sOption.find(sOptionTag);
			if(found != std::string::npos)
			{
				sOption.erase(0,sOptionTag.length());
				if(vcUtils::StrCompare(sOption,std::string("false")))
					g_b2DElements = false;
			}
		}
		else if(strstr(argv[i],"--export-pointset")!=NULL)
		{
			g_bPointSet = true;
			std::string sOption = argv[i];
			std::string sOptionTag = "--export-pointset="; 
			size_t found;
			found = sOption.find(sOptionTag);
			if(found != std::string::npos)
			{
				sOption.erase(0,sOptionTag.length());
				if(vcUtils::StrCompare(sOption,std::string("false")))
					g_bPointSet = false;
			}
		}
		else if(strstr(argv[i],"--create-report")!=NULL)
		{
			g_bTestResults = true;
			std::string sOption = argv[i];
			std::string sOptionTag = "--create-report="; 
			size_t found;
			found = sOption.find(sOptionTag);
			if(found != std::string::npos)
			{
				sOption.erase(0,sOptionTag.length());
				if(vcUtils::StrCompare(sOption,std::string("false")))
					g_bTestResults = false;
			}
		}	
		else if(strstr(argv[i],"--units")!=NULL)
		{
			std::string sOption = argv[i];
			std::string sOptionTag = "--units="; 
			size_t found;
			found = sOption.find(sOptionTag);
			if(found != std::string::npos)
			{
				sOption.erase(0,sOptionTag.length());
			}
			for(int j=sOption.length()-1;j>=0;j--)
			{
				if(sOption[j]=='\"')
					sOption.erase(j,1);
			}
			if(vcUtils::StrCompare(sOption,std::string("meter")))
				g_iUnit = vcCad2Cax::METER;
			else if(vcUtils::StrCompare(sOption,std::string("millimeter")))
				g_iUnit = vcCad2Cax::MILLIMETER;
			else if(vcUtils::StrCompare(sOption,std::string("inch")))
				g_iUnit = vcCad2Cax::INCH;
			else if(vcUtils::StrCompare(sOption,std::string("feet")))
				g_iUnit = vcCad2Cax::FEET;
			else if(vcUtils::StrCompare(sOption,std::string("centimeter")))
				g_iUnit = vcCad2Cax::CENTIMETER;
			else
				g_iUnit = vcCad2Cax::MILLIMETER;

			g_bUserUnits = true;
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
				printf("\n\nError: Invalid log file.Please provide the proper log file path!!!\n");
				vcUtils::LogMsg("Error: Invalid log file.");
				return false;
			}

			int index = sOption.find_last_of("\\");
			if(index!= -1)
			{
				std::string outputdir = sOption.substr(0,index);
				if(!vcUtils::IsValidDirectory(outputdir))
				{
					g_iErrorCode = vcUtils::LOG_ERROR;
					printf("\n\nError:Log file directory is not available. Please provide the proper log file path!!!\n");
					vcUtils::LogMsg("Error:Log file directory is not available.");
					return false;
				}
			}
			index = sOption.find_last_of('.');
			if(index == -1 || sOption.length()<=4)
			{
				g_iErrorCode = vcUtils::LOG_ERROR;
				printf("\n\nError: Invalid log file.Please provide the proper log file path!!!\n");
				vcUtils::LogMsg("Error: Invalid log file.");
				return false;
			}
			std::string ext = vcUtils::GetFileExt(sOption);
			if(ext.length()<3)
			{
				g_iErrorCode = vcUtils::LOG_ERROR;
				printf("\n\nError: Invalid log file extension.Please provide the proper log file path!!!\n");
				vcUtils::LogMsg("Error: Invalid log file extension.");
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
		else
		{
			g_iErrorCode = vcUtils::ARGUMENT_ERROR;
			printf("\n\nError: Invalid option %d. Please refer VMoveCAD online help.\n",i);
			vcUtils::LogMsg("Error: Invalid option(s).");
			return false;
		}
	}
	InputPath = argv[argc-2];
	OutputPath = argv[argc-1];
}


bool CheckOutLicense()
{
	return vcLicense::CheckOutLicense(); 
}

void WriteAppVersionIntoLogFile()
{
	std::stringstream ss;
	ss<<"VMoveCAD_Batch :Version: "<<g_sVersion;
	vcUtils::LogMsg(ss.str());

#ifdef _WIN64
	vcUtils::LogMsg("VMoveCAD_Batch: Win64 Application");
#else
	vcUtils::LogMsg("VMoveCAD_Batch: Win32 Application");
#endif	
	
	ss.str("");
	ss<<"VMoveCAD_Batch : DTK Version: "<<g_sDatakitVersion;
	vcUtils::LogMsg(ss.str());

}

void WriteOSVersionIntoLogFile()
{
	wchar_t *ver = new wchar_t[1024];
	vcUtils::GetWindowsVersionName(ver,1024);
	std::stringstream ss;
	ss<<"VMoveCAD_Batch :OS Version: "<<vcUtils::ToNarrow(ver);
	vcUtils::LogMsg(ss.str());
	delete ver;
}

void WriteSystemInfo()
{
	MEMORYSTATUSEX statex;
    statex.dwLength = sizeof (statex);
    GlobalMemoryStatusEx (&statex);

	std::stringstream ss;
	ss<<"VMoveCAD_Batch: System Memory size: "<<statex.ullTotalPhys/(1024*1024)<<" MB";
	vcUtils::LogMsg(ss.str());
}
int main(int argc,char *argv[])
{ 
	g_iErrorCode=vcUtils::NOT_ERROR;
	g_bMesh = true;
	g_b2DElements = true;
	g_bPointSet = false;	

	CSuiteVersion sv;
	g_sAppName = sv.GetAppName("vmovecad");
	
	if(!ParseCmdLineArguments(argc,argv))
	{
		PrintSyntax();
		//vcLicense::CheckInLicense(); 
		return g_iErrorCode;
	}

	bool bLog=true;
	if(g_bUserLogPath)
		bLog = vcUtils::InitLog(g_sLogFilePath,g_bUserLogPath);
	else
		bLog = vcUtils::InitLog("VMoveCADBatch_Log.txt");
	
	if(!bLog)
	{
		printf("\nPermission denied for writing the Log file. Please verify whether you have the write permission!\n");
		return vcUtils::LOG_ERROR;
	}
	printf("\nSetting Log file to '%s'", vcUtils::m_sLogFileName.c_str());

	WriteAppVersionIntoLogFile();

	WriteOSVersionIntoLogFile();
	
	WriteSystemInfo();

	std::stringstream ss;
	ss<<"VMoveCAD_Batch::Arguments:";
	for(int i=0;i<argc;i++)
	{
		ss<<"\""<<argv[i]<<"\" ";
	}
	vcUtils::LogMsg(ss.str());
	if(!CheckOutLicense())
	{
		return vcUtils::LICENSE_ERROR;
	}

	vcUtils::LogMsg(InputPath.c_str());
	vcUtils::LogMsg(OutputPath.c_str());

	bool bSuccess = true;
	int clo = clock();
	try
	{
		if(!vcCadTranslator::Translate(InputPath,OutputPath,bFileMode,bSuccess))
		{
			vcLicense::CheckInLicense(); 
			return g_iErrorCode;
		}
	}
	catch(std::exception& ex) 
	{
		std::stringstream ss;
		ss.str("");
		ss<<"Exception: "<<ex.what();
		vcUtils::LogMsg(ss.str());
		printf("\n%s.\n",ss.str().c_str());
		return 0;
	}

	if(bFileMode) 
	{
		if(bSuccess)
		{
			if(g_bDataLoss)
			{
				printf("\nTranslated successfully, but there might be a data loss in the output file.\nPlease contact support@vcollab.com for support\n\n");
			}
			else
			{
				printf("\nTranslated successfully!!!\n\n");
			}
			int diff = clock() - clo;
			int hrs = diff/(60*60*1000);
			diff = diff%(60*60*1000);
			int min = diff/(60*1000);
			diff = diff%(60*1000);
			int seconds = diff/1000;
			int ms = diff%(1000);
			if(hrs==0 && min==0 && seconds==0)
				std::cout <<"Time taken for translation = "<<ms<<"millisecs "<< std::endl;
			else if(hrs==0 && min==0)
				std::cout <<"Time taken for translation = "<<seconds<<"secs "<<ms<<"millisecs "<< std::endl;
			else if(hrs==0)
				std::cout <<"Time taken for translation = "<<min<<"mins "<<seconds<<"secs "<<ms<<"millisecs "<< std::endl;
			else
				std::cout <<"Time taken for translation = "<<hrs<<"hrs "<<min<<"mins "<<seconds<<"secs "<<ms<<"millisecs "<< std::endl;
		}
		else
		{
			remove(OutputPath.c_str());
			printf("\nTranslation failed!!!\n");
		}
	}
	else
	{
		printf("\nTranslated %d of %d files successfully!!!\n",g_iOutputFilesCount,g_iInputFilesCount);
	}

 	vcLicense::CheckInLicense(); 
	vcUtils::LogMsg("VMoveCAD_Batch:: Exit");
	return g_iErrorCode;
}

