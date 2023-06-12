#pragma once
#include <string>
#include <vector>
#ifdef __WXMSW__
#include <wx/string.h>
#endif
#include <fstream>

class vcUtils
{
public:
	vcUtils(void);
	~vcUtils(void);

public:
	enum ERROR_CODE
	{
		NOT_ERROR=0, UNKNOWN_ERROR=1,FILE_ACCESS_ERROR=2,FORMAT_ERROR=3,
		VERSION_ERROR=4,LICENSE_ERROR=5,MEMORY_ALLOCATION_ERROR=6,
		ZERO_ELEMENT_MODEL=7,NOT_IMPLEMENTED=8,ARGUMENT_ERROR=9,READER_API_ERROR=10,
		WRITER_API_ERROR=11,LOG_ERROR=12
	};
	static std::string m_sLogFileName;
	static std::ofstream m_Logfile;
	static std::string m_sVMoveCADTmpDir;

	static bool InitLog(std::string sAppName,bool bUserLogPath=false);
	static std::string GetMemoryUsage();
	static inline std::string GetVirtualMemorySize();
	static char *GetVCollabTempPath();
#ifdef __WXMSW__
	static char* wxStringToChar(wxString input);
#endif
	static bool StrCompare( std::string& String1, std::string& String2);
	static void LogMsg(std::string msg,bool bMsgBox=false);
	static std::string GetFileExt(std::string Path);
	static std::string GetFileNameFromFullPath(std::string Path);
	static std::string GetFileNameWithExtn(std::string Path);
	static std::string GetPathFromFileName(std::string FileName);
	static bool IsFileAvailable(std::string InputPath);
	static bool IsValidDirectory(std::string outputdir);
	static void GetFilesFromDirectory(std::string sDirName, std::vector<std::string> &FileNamesStrArray);
	static std::string Integer2String(int iVal);
	static std::string GetVMoveCADTempDir(std::string sTmpPath);
	static std::string ToNarrow( const wchar_t *s, char dfault = '?',const std::locale& loc = std::locale() );
	static bool GetWindowsVersionName(wchar_t* str, int bufferSize);

	static std::string CreateTempFolder();
	static bool CheckWritePermission(std::string sFilePath);



};
