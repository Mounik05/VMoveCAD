#include "vcCadTranslator.h"
#include "vcUtils.h"

#if defined(__WXMSW__)
	#ifdef __BORLANDC__
	#pragma hdrstop
	#endif

	#ifndef WX_PRECOMP
	#include "wx/wx.h"
	#endif

	#include <wx/encconv.h>
#endif
#include <vector>
#include <sstream>
#include "dirent.h"
#include "vcCad2Cax.h"
#include "vcCad2Cgr.h"

extern DtkErrorStatus g_dtkErrorStatus;

//static variable initialization
std::string vcUtils::m_sVMoveCADTmpDir;
// 
int g_iErrorCode=0;//NOT_ERROR 
std::string sOutputFormat;
std::string sSupportedOutputFormat;
int g_iInputFilesCount = 0;
int g_iOutputFilesCount = 0;
extern bool g_bValidInputFormat;
extern void ClearMemory();

vcCadTranslator::vcCadTranslator(void)
{
}

vcCadTranslator::~vcCadTranslator(void) 
{
}

bool vcCadTranslator::CheckForValidUnlistedFormat(std::string sModelFileFormat)
{
	if(vcUtils::StrCompare(sModelFileFormat,std::string("v4_model")) ||
	   vcUtils::StrCompare(sModelFileFormat,std::string("v5_prt")) ||
	   vcUtils::StrCompare(sModelFileFormat,std::string("v5_asm")) ||
	   vcUtils::StrCompare(sModelFileFormat,std::string("v6_3dxml")) ||
	   vcUtils::StrCompare(sModelFileFormat,std::string("proe_asm")) ||
	   vcUtils::StrCompare(sModelFileFormat,std::string("proe_prt")) ||
	   vcUtils::StrCompare(sModelFileFormat,std::string("ug_prt")) ||
	   vcUtils::StrCompare(sModelFileFormat,std::string("sw_asm")) ||
	   vcUtils::StrCompare(sModelFileFormat,std::string("sw_prt")) ||
	   vcUtils::StrCompare(sModelFileFormat,std::string("se_prt")) ||
	   vcUtils::StrCompare(sModelFileFormat,std::string("se_asm")) ||
	   vcUtils::StrCompare(sModelFileFormat,std::string("step_stp")) ||
	   vcUtils::StrCompare(sModelFileFormat,std::string("iges_igs")) ||
	   vcUtils::StrCompare(sModelFileFormat,std::string("catia_cgr")) ||
	   vcUtils::StrCompare(sModelFileFormat,std::string("acis_asm")) ||
	   vcUtils::StrCompare(sModelFileFormat,std::string("acis_prt")) ||
	   vcUtils::StrCompare(sModelFileFormat,std::string("inventor_asm")) ||
	   vcUtils::StrCompare(sModelFileFormat,std::string("inventor_prt")) ||
	   vcUtils::StrCompare(sModelFileFormat,std::string("parasolid_xmt")) 
	   )
	{
		return true;
	}
	return false;
}

#include <boost/algorithm/string.hpp>
bool vcCadTranslator::IsSupportedInputFormat(std::string _sInputPath)
{
	if(g_bValidInputFormat)
		return true;	

	std::string InputPath = _sInputPath;
	boost::algorithm::to_lower(InputPath);

	if(InputPath.rfind(".asm.")!= -1)
		return true;
	else if(InputPath.rfind(".prt.")!= -1)
		return true;

	std::string ext;
	ext = vcUtils::GetFileExt(InputPath);
	if(!vcUtils::StrCompare(ext,std::string("CATPart")) &&
	   !vcUtils::StrCompare(ext,std::string("CATProduct")) &&    
	   !vcUtils::StrCompare(ext,std::string("model")) &&    
	   !vcUtils::StrCompare(ext,std::string("prt")) &&    
	   !vcUtils::StrCompare(ext,std::string("asm")) &&    
	   !vcUtils::StrCompare(ext,std::string("step")) &&     
	   !vcUtils::StrCompare(ext,std::string("stp")) &&     
	   !vcUtils::StrCompare(ext,std::string("iges")) &&     
	   !vcUtils::StrCompare(ext,std::string("igs")) &&     
	   !vcUtils::StrCompare(ext,std::string("cgr")) &&     
	   !vcUtils::StrCompare(ext,std::string("ipt")) &&     
	   !vcUtils::StrCompare(ext,std::string("iam")) &&           
	   !vcUtils::StrCompare(ext,std::string("asm.")) &&           
	   !vcUtils::StrCompare(ext,std::string("prt.")) &&
	   !vcUtils::StrCompare(ext,std::string("psm")) &&           
	   !vcUtils::StrCompare(ext,std::string("par")) &&           
	   !vcUtils::StrCompare(ext,std::string("sldasm")) &&           
	   !vcUtils::StrCompare(ext,std::string("sldprt")) &&           
	   !vcUtils::StrCompare(ext,std::string("3dxml")) &&           
	   !vcUtils::StrCompare(ext,std::string("asat")) &&           
	   !vcUtils::StrCompare(ext,std::string("sat")) &&           
	   !vcUtils::StrCompare(ext,std::string("sab")) &&           
	   !vcUtils::StrCompare(ext,std::string("xmt")) &&           
	   !vcUtils::StrCompare(ext,std::string("x_b")) &&           
	   !vcUtils::StrCompare(ext,std::string("x_t")) &&           
	   !vcUtils::StrCompare(ext,std::string("xmt_bin")) /*&&           
	   !vcUtils::StrCompare(ext,std::string("xmt_txt")) &&
		!vcUtils::StrCompare(ext, std::string("rvt")) &&
		!vcUtils::StrCompare(ext, std::string("rfa"))*/
	   )		
	{
		return false;
	}
	return true;
}

bool vcCadTranslator::IsSupportedOutputFormat(std::string OutputPath)
{
	std::string ext = vcUtils::GetFileExt(OutputPath);
	if(!vcUtils::StrCompare(ext,std::string("CAX")) &&
	   !vcUtils::StrCompare(ext,std::string("CGR"))
		) 
	{
		return false;
	}
	return true;
}


bool vcCadTranslator::ValidateInputs(std::string InputPath,std::string OutputPath,bool bFileMode)
{
	if(bFileMode)
	{
		if(InputPath.length()==0)
		{
#if defined(VMOVECAD_BATCH)
			printf("\nPlease specify the input file.\n");
			vcUtils::LogMsg("Please specify the input file.");
#elif defined(__WXMSW__)	
			wxMessageBox(_("Please specify the input file."), wxT("Error"),wxICON_ERROR, NULL);
#else
			::MessageBox(NULL,"Please specify the input file.","VMoveCAD",MB_ICONERROR);
#endif
		return false;
 		}
		if(OutputPath.length()==0)
		{
#if defined(VMOVECAD_BATCH)
			printf("\nPlease specify the output file.\n");
			vcUtils::LogMsg("Please specify the output file.");
#elif defined(__WXMSW__)			
			wxMessageBox(_("Please specify the output file."), wxT("Error"),wxICON_ERROR, NULL);
#else
			::MessageBox(NULL,"Please specify the output file.","VMoveCAD",MB_ICONERROR);
#endif
			return false;
 		}
		if(!IsSupportedInputFormat(InputPath))
		{
#if defined(VMOVECAD_BATCH)
			printf("\nError:Unsupported input format.\n");
			vcUtils::LogMsg("Error:Unsupported input format.");
#elif defined(__WXMSW__)		
			wxMessageBox(_("Unsupported input format."), wxT("Error"),wxICON_ERROR, NULL);
#else
			::MessageBox(NULL,"Unsupported input format.","VMoveCAD",MB_ICONERROR);
#endif
			g_iErrorCode = vcUtils::FORMAT_ERROR;
			return false;
		}
		if(!IsSupportedOutputFormat(OutputPath))
		{
#if defined(VMOVECAD_BATCH)
			printf("\nError:Unsupported output format.\n");
			vcUtils::LogMsg("Error:Unsupported output format.");
#elif defined(__WXMSW__)	
			wxMessageBox(_("Unsupported output format."), wxT("Error"),wxICON_ERROR, NULL);
#else
			::MessageBox(NULL,"Unsupported output format.","VMoveCAD",MB_ICONERROR);
#endif
			g_iErrorCode = vcUtils::FORMAT_ERROR;
			return false;
		}
		if(!vcUtils::IsFileAvailable(InputPath))
		{
#if defined(VMOVECAD_BATCH)
			printf("\nError:Input file is not found.\n");
			vcUtils::LogMsg("Error:Input file is not found.");
#elif defined(__WXMSW__)		
			wxMessageBox(_("Input file is not found."), wxT("Error"),wxICON_ERROR, NULL);
#else
			::MessageBox(NULL,"Input file is not found.","VMoveCAD",MB_ICONERROR);
#endif
			g_iErrorCode = vcUtils::FILE_ACCESS_ERROR;
			return false;
		}
		int index = -1;
#if defined(_LINUX64_)		
		index = OutputPath.find_last_of("/");
#else
		index = OutputPath.find_last_of("\\");
#endif
		if(index!= -1)
		{
			std::string outputdir = OutputPath.substr(0,index);
			if(!vcUtils::IsValidDirectory(outputdir))
			{
#if defined(VMOVECAD_BATCH)
				printf("\nError:Output directory is not available.\n");
				vcUtils::LogMsg("Error:Output directory is not available.");
#elif defined(__WXMSW__)		
				wxMessageBox(_("Output directory is not available."), wxT("Error"),wxICON_ERROR, NULL);
#else
			::MessageBox(NULL,"Output directory is not available.","VMoveCAD",MB_ICONERROR);
#endif
			g_iErrorCode = vcUtils::FILE_ACCESS_ERROR;
			return false;
			}
		}

		FILE *fp = fopen(OutputPath.c_str(),"w");
		if(fp==NULL)
		{
#if defined(VMOVECAD_BATCH)
			vcUtils::LogMsg("Permission denied for writing the CAX file. Please verify whether you have the write permission.");
			printf("\nError:Permission denied for writing the CAX file. Please verify whether you have the write permission.\n");
#else
			::MessageBox(NULL,"Permission denied for writing the CAX file.\nPlease verify whether you have the write permission.","VMoveCAD",MB_ICONERROR);
#endif	
			g_iErrorCode = vcUtils::FILE_ACCESS_ERROR;
			return false;
		}
		else
		{
			fclose(fp);
			remove(OutputPath.c_str());
		}
	}
	else
	{
		if(InputPath.length()==0)
		{
#if defined(VMOVECAD_BATCH)
			vcUtils::LogMsg("Please specify the input directory.");
			printf("\nPlease specify the input directory..\n");
#elif defined(__WXMSW__)	
			wxMessageBox(_("Please specify the input directory."), wxT("Error"),wxICON_ERROR, NULL);
#else
			::MessageBox(NULL,"Please specify the input directory.","VMoveCAD",MB_ICONERROR);
#endif
			return false;
 		}
		if(OutputPath.length()==0)
		{
#if defined(VMOVECAD_BATCH)
			vcUtils::LogMsg("Please specify the output directory.");
			printf("\nPlease specify the output directory.\n");
#elif defined(__WXMSW__)	
			wxMessageBox(_("Please specify the output directory."), wxT("Error"),wxICON_ERROR, NULL);
#else
			::MessageBox(NULL,"Please specify the output directory.","VMoveCAD",MB_ICONERROR);
#endif
			return false;
 		}
		if(!vcUtils::IsValidDirectory(InputPath))
		{
#if defined(VMOVECAD_BATCH)
			vcUtils::LogMsg("Error:Input directory is not available.");
			printf("\nError:Input directory is not available.\n");
#elif defined(__WXMSW__)	
			wxMessageBox(_("Input directory is not available."), wxT("Error"),wxICON_ERROR, NULL);
#else
			::MessageBox(NULL,"Input directory is not available.","VMoveCAD",MB_ICONERROR);
#endif
			g_iErrorCode = vcUtils::FILE_ACCESS_ERROR;
			return false;
		}
		if(!vcUtils::IsValidDirectory(OutputPath))
		{
#if defined(VMOVECAD_BATCH)
			vcUtils::LogMsg("Error:Output directory is not available.");
			printf("\nError:Output directory is not available.\n");
#elif defined(__WXMSW__)	
			wxMessageBox(_("Output directory is not available."), wxT("Error"),wxICON_ERROR, NULL);
#else
			::MessageBox(NULL,"Output directory is not available.","VMoveCAD",MB_ICONERROR);
#endif
			g_iErrorCode = vcUtils::FILE_ACCESS_ERROR;
			return false;
		}
	}
	return true;
}

bool vcCadTranslator::IsFileNameExist(std::vector<std::string> InputFileNamesStrArray,std::string InputFilePath)
{
	std::string	FileName = vcUtils::GetFileNameFromFullPath(InputFilePath);
	for(int i=0;i<InputFileNamesStrArray.size();i++)
	{
		std::string ext;
		ext = vcUtils::GetFileExt(InputFileNamesStrArray[i]);
		
		if(vcUtils::StrCompare(ext,std::string("CGR")))
			continue;

		std::string	TmpFileName = vcUtils::GetFileNameFromFullPath(InputFileNamesStrArray[i]);
		if(vcUtils::StrCompare(FileName,TmpFileName))
			return true;
	}
	return false;
}
extern bool g_bInfoAboutCAD;
bool vcCadTranslator::Translate(std::string InputPath,std::string OutputPath,bool bFileMode,bool &bSuccess)
{
	g_iInputFilesCount = 0;
	g_iOutputFilesCount = 0;

	if(!ValidateInputs(InputPath,OutputPath,bFileMode))
	{
		return false;
	}
	std::vector<std::string> InputFileNamesStrArray,OutputFileNamesStrArray;
	if(bFileMode)
	{
		InputFileNamesStrArray.push_back(InputPath);
		OutputFileNamesStrArray.push_back(OutputPath);
	}
	else//Directory Mode
	{
		vcUtils::GetFilesFromDirectory(InputPath,InputFileNamesStrArray);
		std::string InputFilePath,FileName,OutputFilePath;
		for(int i=0;i<InputFileNamesStrArray.size();i++)
		{
			InputFilePath = InputFileNamesStrArray[i];
			FileName = vcUtils::GetFileNameFromFullPath(InputFilePath);

			if(FileName.find('.') != std::string::npos)
				FileName = FileName.substr(0,FileName.find('.'));

			std::string ext;
			ext = vcUtils::GetFileExt(InputFilePath);
			
			if(vcUtils::StrCompare(ext,std::string("CGR"))&& IsFileNameExist(InputFileNamesStrArray,InputFilePath) )
			{
				OutputFilePath = OutputPath + "\\"+FileName + "_cgr."+sOutputFormat; 
			}
			else
			{
				OutputFilePath = OutputPath + "\\"+FileName + "."+sOutputFormat; 
			}
			OutputFileNamesStrArray.push_back(OutputFilePath);
		}
	}

	if(!InputFileNamesStrArray.size())
	{
#if defined(VMOVECAD_BATCH)
			vcUtils::LogMsg("Input directory doesn't have supported CAD files");
			printf("\nInput directory doesn't have supported CAD files.\n");
#elif defined(__WXMSW__)		
		wxMessageBox(_("Input directory doesn't have supported CAD files!!!"), wxT("Error"),wxICON_ERROR, NULL);
#else
		::MessageBox(NULL,"Input directory doesn't have supported CAD files!!!.","VMoveCAD",MB_ICONERROR);
#endif
		return false;
	}

	std::string sTmpDir = vcUtils::m_sVMoveCADTmpDir;

	bSuccess = true;
	std::stringstream ss;

	ss.str("");
	ss<<"VMoveCAD:: No of Files : "<<InputFileNamesStrArray.size();
	vcUtils::LogMsg(ss.str());

#if defined(__WXMSW__)
	wxBeginBusyCursor();
#endif
	for(int i=0;i<InputFileNamesStrArray.size();i++)
	{
		std::string InputFile,OutputFile;
		InputFile = InputFileNamesStrArray[i];
		OutputFile = OutputFileNamesStrArray[i];

		ss.str("");
		ss<<"VMoveCAD:: Input File #"<<i+1<<": "<<InputFile;
		vcUtils::LogMsg(ss.str());

		ss.str("");
		ss<<"VMoveCAD:: Output File #"<<i+1<<": "<<OutputFile;
		vcUtils::LogMsg(ss.str());

		std::string ext;
		ext = vcUtils::GetFileExt(OutputFile);
		g_bInfoAboutCAD = false; 

		ClearMemory();

		std::string memusage = vcUtils::GetMemoryUsage();
		std::stringstream ss;
		ss.str("");
		ss<<"VMoveCAD::OnTranslateButtonClick : Memory Usage b4 conversion: "<<memusage;
		vcUtils::LogMsg(ss.str());
		ss.str("");

		try
		{
			if(vcUtils::StrCompare(ext,std::string("CAX")))
			{
				vcCad2Cax datakit(const_cast<char *>(InputFile.c_str()),const_cast<char *>(OutputFile.c_str()),const_cast<char *>(sTmpDir.c_str()),bSuccess);
				if(g_dtkErrorStatus == dtkErrorVersionNotSupported)
				{
					g_iErrorCode = vcUtils::VERSION_ERROR;
				}
				else if(g_dtkErrorStatus == dtkErrorLicence)
				{
					g_iErrorCode = vcUtils::LICENSE_ERROR;
				}
				else if(g_dtkErrorStatus == dtkErrorUnavailableReader)
				{
					g_iErrorCode = vcUtils::FORMAT_ERROR;
				}
				else if(datakit.m_iErrorCode == vcCad2Cax::DATAKIT_ERROR)
				{
					g_iErrorCode = vcUtils::READER_API_ERROR;
				}
				else if(datakit.m_iErrorCode == vcCad2Cax::CAX_WRITER_ERROR)
				{
					g_iErrorCode = vcUtils::WRITER_API_ERROR;
				}
			}
			else if(vcUtils::StrCompare(ext,std::string("CGR")))
			{
				vcCad2Cgr datakit(const_cast<char *>(InputFile.c_str()),const_cast<char *>(OutputFile.c_str()),const_cast<char *>(sTmpDir.c_str()),bSuccess);
			}
		}
		catch(...)
		{
			g_iErrorCode = vcUtils::UNKNOWN_ERROR;
			return false;
		}
		memusage = vcUtils::GetMemoryUsage();
		ss.str("");
		ss<<"VMoveCAD::OnTranslateButtonClick : Memory Usage after conversion: "<<memusage;
		vcUtils::LogMsg(ss.str());
		ss.str("");

		ClearMemory();
 
		memusage = vcUtils::GetMemoryUsage();
		ss.str("");
		ss<<"VMoveCAD::OnTranslateButtonClick : Memory Usage after memory clean: "<<memusage;
		vcUtils::LogMsg(ss.str());
		ss.str("");

	}
	g_iInputFilesCount = InputFileNamesStrArray.size();
	for(int i=0;i<OutputFileNamesStrArray.size();i++)
	{
		std::string OutputFile;
		OutputFile = OutputFileNamesStrArray[i];
		if(vcUtils::IsFileAvailable(OutputFile))
			g_iOutputFilesCount++;
	}

#if defined(__WXMSW__)
	wxEndBusyCursor();
#endif

	return true;
}

