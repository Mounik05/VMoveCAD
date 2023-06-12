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

//static variable initialization
std::string vcUtils::m_sVMoveCADTmpDir;
// 

std::string sOutputFormat;
std::string sSupportedOutputFormat;
int g_iInputFilesCount = 0;
int g_iOutputFilesCount = 0;

bool IsSupportedInputFormat(std::string InputPath)
{
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
	  !vcUtils::StrCompare(ext,std::string("sldprt"))            

	   )
	{
		return false;
	}
	return true;
}

bool IsSupportedOutputFormat(std::string OutputPath)
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


bool ValidateInputs(std::string InputPath,std::string OutputPath,bool bFileMode)
{
	if(bFileMode)
	{
		if(InputPath.length()==0)
		{
#if defined(VMOVECAD_BATCH)
			printf("\nPlease specify the input file.\n");
#elif defined(__WXMSW__)
			wxMessageBox(_("Please specify the input file."), wxT("Error"),wxICON_ERROR, NULL);
#else
			AfxMessageBox("Please specify the input file.",MB_ICONERROR);
#endif
			return false;
 		}
		if(OutputPath.length()==0)
		{
#if defined(VMOVECAD_BATCH)
			printf("\nPlease specify the output file.\n");
#elif defined(__WXMSW__)	
			wxMessageBox(_("Please specify the output file."), wxT("Error"),wxICON_ERROR, NULL);
#else
			AfxMessageBox("Please specify the output file.",MB_ICONERROR);
#endif
			return false;
 		}
		if(!IsSupportedInputFormat(InputPath))
		{
#if defined(VMOVECAD_BATCH)
			printf("\nError:Unsupported input format.\n");
#elif defined(__WXMSW__)		
			wxMessageBox(_("Unsupported input format."), wxT("Error"),wxICON_ERROR, NULL);
#else
			AfxMessageBox("Unsupported input format.",MB_ICONERROR);
#endif
			return false;
		}
		if(!IsSupportedOutputFormat(OutputPath))
		{
#if defined(VMOVECAD_BATCH)
			printf("\nError:Unsupported output format.\n");
#elif defined(__WXMSW__)			
			wxMessageBox(_("Unsupported output format."), wxT("Error"),wxICON_ERROR, NULL);
#else
			AfxMessageBox("Unsupported output format.",MB_ICONERROR);
#endif
			return false;
		}
		if(!vcUtils::IsFileAvailable(InputPath))
		{
#if defined(VMOVECAD_BATCH)
			printf("\nError:Input file is not found.\n");
#elif defined(__WXMSW__)			
			wxMessageBox(_("Input file is not found."), wxT("Error"),wxICON_ERROR, NULL);
#else
			AfxMessageBox("Input file is not found.",MB_ICONERROR);
#endif
			return false;
		}
		int index = OutputPath.find_last_of("\\");
		if(index!= -1)
		{
			std::string outputdir = OutputPath.substr(0,index);
			if(!vcUtils::IsValidDirectory(outputdir))
			{
#if defined(VMOVECAD_BATCH)
				printf("\nError:Output directory is not available.\n");
#elif defined(__WXMSW__)	
				wxMessageBox(_("Output directory is not available."), wxT("Error"),wxICON_ERROR, NULL);
#else
				AfxMessageBox("Output directory is not available.",MB_ICONERROR);
#endif
				return false;
			}
		}
	}
	else
	{
		if(InputPath.length()==0)
		{
#if defined(VMOVECAD_BATCH)
			printf("\nPlease specify the input directory..\n");
#elif defined(__WXMSW__)
			wxMessageBox(_("Please specify the input directory."), wxT("Error"),wxICON_ERROR, NULL);
#else
			AfxMessageBox("Please specify the input directory.",MB_ICONERROR);
#endif
			return false;
 		}
		if(OutputPath.length()==0)
		{
#if defined(VMOVECAD_BATCH)
			printf("\nPlease specify the output directory.\n");
#elif defined(__WXMSW__)
			wxMessageBox(_("Please specify the output directory."), wxT("Error"),wxICON_ERROR, NULL);
#else
			AfxMessageBox("Please specify the output directory.",MB_ICONERROR);
#endif
			return false;
 		}
		if(!vcUtils::IsValidDirectory(InputPath))
		{
#if defined(VMOVECAD_BATCH)
			printf("\nError:Input directory is not available.\n");
#elif defined(__WXMSW__)	
			wxMessageBox(_("Input directory is not available."), wxT("Error"),wxICON_ERROR, NULL);
#else
			AfxMessageBox("Input directory is not available.",MB_ICONERROR);
#endif
			return false;
		}
		if(!vcUtils::IsValidDirectory(OutputPath))
		{
#if defined(VMOVECAD_BATCH)
			printf("\nError:Output directory is not available.\n");
#elif defined(__WXMSW__)		
			wxMessageBox(_("Output directory is not available."), wxT("Error"),wxICON_ERROR, NULL);
#else
			AfxMessageBox("Output directory is not available.",MB_ICONERROR);
#endif
			return false;
		}
	}
	return true;
}

bool IsFileNameExist(std::vector<std::string> InputFileNamesStrArray,std::string InputFilePath)
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

bool Translate(std::string InputPath,std::string OutputPath,bool bFileMode,bool &bSuccess)
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
		printf("\nInput directory doesn't have supported CAD files.\n");
#elif defined(__WXMSW__)		
		wxMessageBox(_("Input directory doesn't have supported CAD files!!!"), wxT("Error"),wxICON_ERROR, NULL);
#else
		AfxMessageBox("Input directory doesn't have supported CAD files!!!",MB_ICONERROR);
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
		
		if(vcUtils::StrCompare(ext,std::string("CAX")))
		{
			vcCad2Cax datakit(const_cast<char *>(InputFile.c_str()),const_cast<char *>(OutputFile.c_str()),const_cast<char *>(sTmpDir.c_str()),bSuccess);
		}
		else if(vcUtils::StrCompare(ext,std::string("CGR")))
		{
			vcCad2Cgr datakit(const_cast<char *>(InputFile.c_str()),const_cast<char *>(OutputFile.c_str()),const_cast<char *>(sTmpDir.c_str()),bSuccess);
		}

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