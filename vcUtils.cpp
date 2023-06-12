#if defined(__WXMSW__)
	#ifndef WX_PRECOMP
		#include "wx/wx.h"
	#endif
#endif

#include "vcUtils.h"
#include "vcCadTranslator.h"
#include "dirent.h"
#include <sys/stat.h> 
#include <direct.h>
#include <stdlib.h>
#include <stdio.h>
#include "time.h"
#include "TempFileFolder.h"
//static variable initialization
std::string vcUtils::m_sLogFileName;
std::ofstream vcUtils::m_Logfile;
std::string vcUtils::m_sVMoveCADTmpDir;
//
extern char* g_sAppName;
vcUtils::vcUtils(void)
{
}

vcUtils::~vcUtils(void)
{
}

std::string vcUtils::CreateTempFolder()
{
	//Vct::Util::TempFileFolder temp_file_folder("VMoveCAD-%time%-%pid%");
	std::stringstream ss;
	ss << g_sAppName << "-%time%-%pid%";
	Vct::Util::TempFileFolder temp_file_folder(ss.str());

	try
	{
		temp_file_folder.create();
	}
	catch(...)
	{
		return std::string("");
	}
	std::string sTempFolder = temp_file_folder.path().string()+std::string("/");

#if defined(BOOST_OS_WINDOWS)
        //Sleep(5000);    // wait for 5 sec = 5000 ms
#else
        //sleep(5);       // wait for 5 sec
#endif

    //temp_file_folder.clearTemporaryFiles();
    //temp_file_folder.clearOldFolders(5, 0);
#if 1
	try
	{
		unsigned int num_log_folders_to_retain = 5;
		const char* num_log_folders_env = getenv("VCOLLAB_LOG_FOLDERS_TO_RETAIN");
		if(num_log_folders_env  != NULL)
				num_log_folders_to_retain = boost::lexical_cast<unsigned int>(num_log_folders_env);
		unsigned int days_to_elapse = 2;
		const char* days_to_elapse_log_env = getenv("VCOLLAB_LOG_DAYS_TO_ELAPSE");
		if(days_to_elapse_log_env  != NULL)
				days_to_elapse = boost::lexical_cast<unsigned int>(days_to_elapse_log_env);
		temp_file_folder.clearOldFolders(num_log_folders_to_retain, days_to_elapse);
	}
	catch(...)
	{
	}
#else
	try
	{
		temp_file_folder.clearOldFolders(3, 0);
	}
	catch(...)
	{
		//::MessageBox(NULL,"Clear Log folder - crash","VMoveCAD",MB_ICONERROR);
	}
#endif

	return sTempFolder;
}

bool vcUtils::InitLog(std::string sLogFileName,bool bUserLogPath)
{
	if(bUserLogPath)
	{
		int index = sLogFileName.find_last_of("\\");
		if(index!= -1)
		{
			m_sVMoveCADTmpDir = sLogFileName.substr(0,index);
			if(sLogFileName.length()>index+1)
				m_sVMoveCADTmpDir += std::string("\\");
		}
		else
			m_sVMoveCADTmpDir = ".\\";

		m_sLogFileName = sLogFileName;
	}
	else
	{
//#ifdef _DEBUG
	//	m_sVMoveCADTmpDir = GetVMoveCADTempDir(GetVCollabTempPath());
//#else
		m_sVMoveCADTmpDir = CreateTempFolder();
		if(m_sVMoveCADTmpDir.empty())
		{
			return false;
		}
			
//#endif
		m_sLogFileName = m_sVMoveCADTmpDir+std::string("\\")+sLogFileName;
	}

	if(!CheckWritePermission(m_sLogFileName))
		return false;

	struct stat stFileInfo; 
	int intStat; 
	intStat = stat(m_sLogFileName.c_str(),&stFileInfo); 
	if(intStat == 0) 
	{ 	
		m_Logfile.open(m_sLogFileName.c_str(),std::ios::out);
	}


	std::stringstream ss;
	const char* num_log_folders_env = getenv("VCOLLAB_LOG_FOLDERS_TO_RETAIN");
	if(num_log_folders_env  != NULL)
	{
		unsigned int num_log_folders_to_retain = boost::lexical_cast<unsigned int>(num_log_folders_env);
		ss<<"VCOLLAB_LOG_FOLDERS_TO_RETAIN : "<<num_log_folders_to_retain;
		LogMsg(ss.str());
	}
	else
	{
		ss<<"VCOLLAB_LOG_FOLDERS_TO_RETAIN is default";
		LogMsg(ss.str());
	}
	ss.str("");
	const char* days_to_elapse_log_env = getenv("VCOLLAB_LOG_DAYS_TO_ELAPSE");
	if(days_to_elapse_log_env  != NULL)
	{
		unsigned int days_to_elapse = boost::lexical_cast<unsigned int>(days_to_elapse_log_env);
		ss<<"VCOLLAB_LOG_DAYS_TO_ELAPSE : "<<days_to_elapse;
		LogMsg(ss.str());
	}
	else
	{
		ss<<"VCOLLAB_LOG_DAYS_TO_ELAPSE is default";
		LogMsg(ss.str());
	}

	return true;
}

#define WINVER 0x0501
#define _WIN32_WINNT 0x0501
#include <psAPI.h>
#include <sstream>
std::string vcUtils::GetMemoryUsage()
{
#if defined (_MSC_VER) || defined (__MINGW32__)
	std::ostringstream oss;

    PROCESS_MEMORY_COUNTERS_EX *pMemCountr = new PROCESS_MEMORY_COUNTERS_EX;
    if( GetProcessMemoryInfo(GetCurrentProcess(),
        reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(pMemCountr), 
        sizeof(PROCESS_MEMORY_COUNTERS_EX))
	  )
		//oss << pMemCountr->PeakWorkingSetSize/1024/1024 << " Mb";
		oss <<pMemCountr->WorkingSetSize/1024 << " Kb		"<< "Peak Memory Usage :"<<pMemCountr->PeakWorkingSetSize/1024 << " Kb	";

    delete pMemCountr;
    return oss.str();
#else
    return std::string("Unknown");
#endif
}

std::string vcUtils::GetVirtualMemorySize()
{
#if defined (_MSC_VER) || defined (__MINGW32__)
    std::ostringstream oss;

    PROCESS_MEMORY_COUNTERS_EX *pMemCountr = new PROCESS_MEMORY_COUNTERS_EX;
    if( GetProcessMemoryInfo(GetCurrentProcess(),
							 reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(pMemCountr), 
							 sizeof(PROCESS_MEMORY_COUNTERS_EX)
							 )
	  )
		oss << pMemCountr->WorkingSetSize/1024/1024 << " Mb";
 
    delete pMemCountr;
    return oss.str();
#else
    return std::string("Unknown");
#endif
}

char* vcUtils::GetVCollabTempPath()
{
	char *profile_dir = NULL;
	profile_dir = getenv("VCOLLAB_TEMP_PATH");
	if(profile_dir)
	{
		mkdir(profile_dir);
	}
	else
	{
		profile_dir = getenv("TEMP");
		if(profile_dir)
			return profile_dir;

		profile_dir = getenv("TMP");
		if(profile_dir)
			return profile_dir;

		profile_dir = getenv("USERPROFILE");
	}
	return profile_dir;
}
#if defined(__WXMSW__)
char* vcUtils::wxStringToChar(wxString input)
{
#if (wxUSE_UNICODE)
        size_t size = input.size() + 1;
        char *buffer = new char[size];//No need to multiply by 4, converting to 1 byte char only.
        memset(buffer, 0, size); //Good Practice, Can use buffer[0] = '&#65533;' also.
        wxEncodingConverter wxec;
        wxec.Init(wxFONTENCODING_ISO8859_1, wxFONTENCODING_ISO8859_1, wxCONVERT_SUBSTITUTE);
        wxec.Convert(input.mb_str(), buffer);
        return buffer; //To free this buffer memory is user responsibility.
#else
        return (char *)(input.c_str());
#endif
}
#endif

bool vcUtils::StrCompare(std::string& String1, std::string& String2 )
{
	if ( String1.size() != String2.size() )
		return false;

	for ( std::string::size_type i = 0; i < String1.size(); ++i )
	{
		if ( tolower(String1[i]) != tolower(String2[i]) )
			return false;
	}
	return true;
}

void vcUtils::LogMsg(std::string msg,bool bMsgBox)
{
	try
	{
		if(!m_Logfile.is_open())
			m_Logfile.open(m_sLogFileName.c_str(),std::ios::app);

		time_t curr;
		tm local;
		time(&curr); 
		local=*(localtime(&curr));
		
		m_Logfile<<"["<<local.tm_hour<<":"<<local.tm_min<<":"<<local.tm_sec<<", "<<local.tm_mday<<"-"<<local.tm_mon+1<<"-"<<local.tm_year+1900<<"] "<<msg<<"\n";

		if(bMsgBox)
#if defined(VMOVECAD_BATCH)
			printf("\n%s\n",msg.c_str());
#else
			::MessageBox(NULL,msg.c_str(),"VMoveCAD",MB_ICONERROR);
#endif

		m_Logfile.close();
	}
	catch(...)
	{
		::MessageBox(NULL,"Log crash","VMoveCAD",MB_ICONERROR);
	}
}

std::string vcUtils::GetFileExt(std::string Path)
{
	std::string ext;
	int res = Path.find_last_of(".");
	if(res==-1)
		return std::string();
	ext = Path.substr(Path.find_last_of(".") + 1) ;
	return ext;
}

std::string vcUtils::GetFileNameFromFullPath(std::string Path)
{
	std::string FileName,tmp;
	if(Path.find_last_of("\\") != -1)
		tmp = Path.substr(Path.find_last_of("\\")+1);
	else
		tmp = Path.substr(Path.find_last_of("/")+1);

	FileName = tmp.substr(0,tmp.find_last_of("."));
	return FileName;
}

std::string vcUtils::GetFileNameWithExtn(std::string Path)
{
	std::string FileName;
	if(Path.find_last_of("\\")!=-1)
		FileName = Path.substr(Path.find_last_of("\\")+1);
	else
		FileName = Path.substr(Path.find_last_of("/")+1);

	return FileName;
}


std::string vcUtils::GetPathFromFileName(std::string FileName)
{
	std::string Path;
	
	if (FileName.find_last_of("\\") != -1)
		Path = FileName.substr(0, FileName.find_last_of("\\"));
	else
		Path = FileName.substr(0, FileName.find_last_of("/"));;
	return Path;
}

bool vcUtils::IsFileAvailable(std::string InputPath)
{
	FILE *fp=fopen(InputPath.c_str(),"r");
	if(fp==NULL)
	{
		return false;
	} 
	else
		fclose(fp);

	return true;
}

bool vcUtils::IsValidDirectory(std::string outputdir)
{
	long size;
	char *buf;
	char *ptr;
	size = 2048;
	if ((buf = (char *)malloc((size_t)size)) != NULL)
		ptr = getcwd(buf, (size_t)size);
	if(ptr)
	{
		if(chdir(outputdir.c_str()) == -1)
		{
			if(buf)
				delete buf;
			return false;
		}
		else
		{
			chdir(buf);
		}
	}
	if(buf)
		delete buf;

	return true;
}

void vcUtils::GetFilesFromDirectory(std::string sDirName, std::vector<std::string> &FileNamesStrArray)
{
	std::string filepath;
	DIR *dp;
	struct dirent *dirp;
	struct stat filestat;

	dp = opendir( sDirName.c_str() );
	if (dp == NULL)
	{
		return ;//errno;
	}

	while ((dirp = readdir( dp )))
	{
		filepath = sDirName + "\\" + dirp->d_name;

		// If the file is a directory (or is in some way invalid) we'll skip it 
		if (stat( filepath.c_str(), &filestat )) 
			continue;

		//if (S_ISDIR( filestat.st_mode ))
		//if ( filestat.st_attr == S_IADIR)         
			//continue;
		if(strcmp(dirp->d_name,".")==0 || strcmp(dirp->d_name,"..")==0)
			continue;

		if(GetFileExt(filepath).empty())
			continue;

		if(!vcCadTranslator::IsSupportedInputFormat(filepath))
		{
			continue;
		}
		FileNamesStrArray.push_back(filepath);
	}
	closedir( dp );
}

std::string vcUtils::Integer2String(int iVal)
{
	char str[10];
	itoa(iVal,str,10);
	return std::string(str);
}

std::string vcUtils::GetVMoveCADTempDir(std::string sTmpPath)
{
	time_t curr;
	tm local;
	time(&curr); 
	local=*(localtime(&curr));
	
	std::string sTmpDir;
	sTmpDir = sTmpPath + std::string("\\VMoveCAD-")+Integer2String(local.tm_year+1900)+std::string("-")+Integer2String(local.tm_mon+1)+std::string("-")+Integer2String(local.tm_mday)+std::string("-")+Integer2String(local.tm_hour)+std::string("-")+Integer2String(local.tm_min)+std::string("-")+Integer2String(local.tm_sec);
	if(_mkdir(sTmpDir.c_str()) == -1)
	{
	}
	return sTmpDir;
}




#include <locale>
#include <sstream>
#include <string>

std::string vcUtils::ToNarrow( const wchar_t *s, char dfault,const std::locale& loc)
{
  std::ostringstream stm;

  while( *s != L'\0' ) {
    stm << std::use_facet< std::ctype<wchar_t> >( loc ).narrow( *s++, dfault );
  }
  return stm.str();
}

#include <windows.h>
#include <string>
#include <sstream>
typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
typedef BOOL (WINAPI *PGPI)(DWORD, DWORD, DWORD, DWORD, PDWORD);
#define PRODUCT_PROFESSIONAL	0x00000030
#define VER_SUITE_WH_SERVER	0x00008000
bool vcUtils::GetWindowsVersionName(wchar_t* str, int bufferSize)
{
	OSVERSIONINFOEX osvi;
	SYSTEM_INFO si;
	BOOL bOsVersionInfoEx;
	DWORD dwType; ZeroMemory(&si, sizeof(SYSTEM_INFO));
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX)); osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	bOsVersionInfoEx = GetVersionEx((OSVERSIONINFO*) &osvi); 
	if(bOsVersionInfoEx == 0)
		return false; // Call GetNativeSystemInfo if supported or GetSystemInfo otherwise.
	PGNSI pGNSI = (PGNSI) GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GetNativeSystemInfo");
	if(NULL != pGNSI)
		pGNSI(&si);
	else 
		GetSystemInfo(&si); // Check for unsupported OS
	if (VER_PLATFORM_WIN32_NT != osvi.dwPlatformId || osvi.dwMajorVersion <= 4 ) 
	{
		return false;
	} 
	std::wstringstream os;
	os << L"Microsoft "; // Test for the specific product. 
	if ( osvi.dwMajorVersion == 6 )
	{
		if( osvi.dwMinorVersion == 0 )
		{
			if( osvi.wProductType == VER_NT_WORKSTATION )
				os << "Windows Vista ";
			else 
				os << "Windows Server 2008 ";
		} 
		if ( osvi.dwMinorVersion == 1 )
		{
			if( osvi.wProductType == VER_NT_WORKSTATION )
				os << "Windows 7 ";
			else 
				os << "Windows Server 2008 R2 ";
		} 
		PGPI pGPI = (PGPI) GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GetProductInfo");
		/*pGPI( osvi.dwMajorVersion, osvi.dwMinorVersion, 0, 0, &dwType);  
		switch( dwType )
		{
			case PRODUCT_ULTIMATE:
			os << "Ultimate Edition";
			break;
			case PRODUCT_PROFESSIONAL:
			os << "Professional";
			break;
			case PRODUCT_HOME_PREMIUM:
			os << "Home Premium Edition";
			break;
			case PRODUCT_HOME_BASIC:
			os << "Home Basic Edition";
			break;
			case PRODUCT_ENTERPRISE:
			os << "Enterprise Edition";
			break;
			case PRODUCT_BUSINESS:
			os << "Business Edition";
			break;
			case PRODUCT_STARTER:
			os << "Starter Edition";
			break;
			case PRODUCT_CLUSTER_SERVER:
			os << "Cluster Server Edition";
			break;
			case PRODUCT_DATACENTER_SERVER:
			os << "Datacenter Edition";
			break;
			case PRODUCT_DATACENTER_SERVER_CORE:
			os << "Datacenter Edition (core installation)";
			break;
			case PRODUCT_ENTERPRISE_SERVER:
			os << "Enterprise Edition";
			break;
			case PRODUCT_ENTERPRISE_SERVER_CORE:
			os << "Enterprise Edition (core installation)";
			break;
			case PRODUCT_ENTERPRISE_SERVER_IA64:
			os << "Enterprise Edition for Itanium-based Systems";
			break;
			case PRODUCT_SMALLBUSINESS_SERVER:
			os << "Small Business Server";
			break;
			case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM:
			os << "Small Business Server Premium Edition";
			break;
			case PRODUCT_STANDARD_SERVER:
			os << "Standard Edition";
			break;
			case PRODUCT_STANDARD_SERVER_CORE:
			os << "Standard Edition (core installation)";
			break;
			case PRODUCT_WEB_SERVER:
			os << "Web Server Edition";
			break;
		}*/ 
	 }
	if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
	{
		/*if( GetSystemMetrics(SM_SERVERR2) )
		os <<  "Windows Server 2003 R2, ";
		else*/ 
		if ( osvi.wSuiteMask & VER_SUITE_STORAGE_SERVER )
			os <<  "Windows Storage Server 2003";
		else if ( osvi.wSuiteMask & VER_SUITE_WH_SERVER )
			os <<  "Windows Home Server";
		else if( osvi.wProductType == VER_NT_WORKSTATION && si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
		{
			os <<  "Windows XP Professional x64 Edition";
		}
		else 
			os << "Windows Server 2003, ";  // Test for the server type.

		if ( osvi.wProductType != VER_NT_WORKSTATION )
		{
			if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_IA64 )
			{
				if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
					os <<  "Datacenter Edition for Itanium-based Systems";
				else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
					os <<  "Enterprise Edition for Itanium-based Systems";
			}  
			else if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 )
			{
				if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
					os <<  "Datacenter x64 Edition";
				else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
					os <<  "Enterprise x64 Edition";
				else os <<  "Standard x64 Edition";
			}
			else
			{
				if ( osvi.wSuiteMask & VER_SUITE_COMPUTE_SERVER )
					os <<  "Compute Cluster Edition";
				else if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
					os <<  "Datacenter Edition";
				else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
					os <<  "Enterprise Edition";
				else if ( osvi.wSuiteMask & VER_SUITE_BLADE )
					os <<  "Web Edition";
				else 
					os <<  "Standard Edition";
			}
		}
	} 
	if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
	{
		os << "Windows XP ";
		if( osvi.wSuiteMask & VER_SUITE_PERSONAL )
			os <<  "Home Edition";
		else 
			os <<  "Professional";
	}
	if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
	{
		os << "Windows 2000 ";  
		if ( osvi.wProductType == VER_NT_WORKSTATION )
		{
			os <<  "Professional";
		}
		else 
		{
			if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
				os <<  "Datacenter Server";
			else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
				os <<  "Advanced Server";
			else os <<  "Server";
		}
	} // Include service pack (if any) and build number. 
	if(wcslen((const wchar_t*)osvi.szCSDVersion) > 0)
	{
		os << " " << osvi.szCSDVersion;
	} 
	os << L" (build " << osvi.dwBuildNumber << L")"; 
	//if ( osvi.dwMajorVersion >= 6 ) 
	{
		if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 )
			os <<  ", 64-bit";
		else if (si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_INTEL )
			os << ", 32-bit";
	}
	wcscpy_s(str, bufferSize, os.str().c_str());
	 return true; 
}


bool vcUtils::CheckWritePermission(std::string sFilePath)
{
	FILE *fp = fopen(sFilePath.c_str(),"w");
	if(fp==NULL)
	{
		return false;
	}
	else
	{
		fclose(fp);
		remove(sFilePath.c_str());
	}
	return true;
}

