#define X64 1

#define RELEASE 1

char g_sAppTitle[256] = "VMoveCAD";

std::string g_sDatakitVersion = "v2023.2";

#define COPY_RIGHT_STR "Copyright (C) 2023"
char g_sCopyright[256] =  "Copyright (C) 2023";

#define FLEXLM_VER_STR "FLEXlm V11.14.0.0"


#ifdef RELEASE
	#ifdef X64
		#define COMMENTS_STR "Release, Win64"
		#define FILE_DESCRIPTION_STR "VMoveCAD, Release, Win64"
	#else
		#define COMMENTS_STR "Release, Win32"
		#define FILE_DESCRIPTION_STR "VMoveCAD, Release, Win32"
	#endif
#else
	#ifdef X64
		#define COMMENTS_STR "Beta, Win64"
		#define FILE_DESCRIPTION_STR "VMoveCAD, Beta, Win64"
	#else
		#define COMMENTS_STR "Beta, Win32"
		#define FILE_DESCRIPTION_STR "VMoveCAD, Beta, Win32"
	#endif
#endif

#define VERSION 3,0,523,426
#define VERSION_STR "3,0,523,426"

#ifdef RELEASE
	char g_sVersion[20] =  "3.0.523.426";
#else
	char g_sVersion[20] =  "3.0.523.426 (Beta)"; 
#endif
 
