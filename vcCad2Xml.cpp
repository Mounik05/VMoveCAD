#include <Windows.h>
#include "vcCad2Xml.h"
#include "vcUtils.h"
#include "vcLicense.h"
#include <algorithm>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/format.hpp>

#define GRAVITY 9.80665f
extern int g_iErrorCode;
//"E:\Users\Senthil\Work\Official\models\ug\assembly2.prt" --output="E:\Users\Senthil\Work\Official\models\ug\assembly2.xml"
//"F:\Program Files\VCollabDependencyAPI\DATAKIT\2013.3\SampleFiles\v5\assembly.catproduct" --output="F:\Program Files\VCollabDependencyAPI\DATAKIT\2013.3\SampleFiles\v5\assembly.xml"

bool g_bXml = false;
bool g_bImgGenProcess = true;
std::string g_sMinervaReportDir("reportFiles");
std::string vcdir_env="";
extern std::map<std::string, int>  g_sOutputXMLFilesFormatMap;

       std::pair<std::string, std::string> getExePaths()
        {
            namespace bfs = boost::filesystem;
            
#if defined (_MSC_VER) || defined (__MINGW32__)
            HMODULE hModule = GetModuleHandleA(NULL);
            CHAR path[MAX_PATH];
            GetModuleFileNameA(hModule, path, MAX_PATH);

            bfs::path info_path(path);
            vcdir_env = getenv("VCOLLAB_DIR");
            if(vcdir_env.empty())
                vcdir_env = "C:\\Program Files\\VCollab";

            bfs::path vcdir(vcdir_env);
            
            bfs::path vmb_path1(info_path);
            vmb_path1.remove_filename();
            vmb_path1 /= "VMoveCADBatch.exe";

            bfs::path vcimg_path1(info_path);
            vcimg_path1.remove_filename();
            vcimg_path1.remove_filename();
            vcimg_path1 /= "VCollabPro64";
            vcimg_path1 /= "VCollabImageGenerator.exe";
            
            bfs::path vmcaefolder_path (vcdir/"VMoveCAD64");
            bfs::path vcprofolder_path (vcdir/"VCollabPro64");

            bfs::path vmb_path2(vmcaefolder_path/"VMoveCADBatch.exe");
            bfs::path vmb_path;
            if(bfs::exists(vmb_path2))
                vmb_path = vmb_path2;
            else if(bfs::exists(vmb_path1))
                vmb_path = vmb_path1;

            bfs::path vcimg_path2(vcprofolder_path/"VCollabImageGenerator.exe");
            bfs::path vcimg_path;
            if(bfs::exists(vcimg_path2))
                vcimg_path = vcimg_path2;
            else if(bfs::exists(vcimg_path1))
                vcimg_path = vcimg_path1;

            return std::make_pair(vmb_path.string(), vcimg_path.string());
#else
            std::string path("/usr/local/bin/cadinfo");
            bfs::path info_path(path);
            bfs::path vmb_path1(info_path);
            vmb_path1.remove_filename();
            vmb_path1 /= "vmovecad";

            bfs::path vcimg_path1(info_path);
            vcimg_path1.remove_filename();
            vcimg_path1 /= "VCollabImageGenerator";
            
            const char* vcenv_cstr = getenv("VCOLLAB_DIR");
            //std::string vcdir_env;
            if(vcenv_cstr != NULL)
                vcdir_env = vcenv_cstr;
            if(vcdir_env.empty())
                vcdir_env = "/usr/local";

            bfs::path vcdir(vcdir_env);
            bfs::path vmcaefolder_path (vcdir/"bin");
            bfs::path vcprofolder_path (vcdir/"bin");

            bfs::path vmb_path2(vmcaefolder_path/"vmovecad");
            bfs::path vmb_path;
            if(bfs::exists(vmb_path2))
                vmb_path = vmb_path2;
            else if(bfs::exists(vmb_path1))
                vmb_path = vmb_path1;

            bfs::path vcimg_path2(vcprofolder_path/"VCollabImageGenerator");
            bfs::path vcimg_path;
            if(bfs::exists(vcimg_path2))
                vcimg_path = vcimg_path2;
            else if(bfs::exists(vcimg_path1))
                vcimg_path = vcimg_path1;

            //TRACE_LOG << "Translator path : \"" << vmb_path.string() << "\"";
            //TRACE_LOG << "ImageGenerator path : \"" << vcimg_path.string() << "\"";
            return std::make_pair(vmb_path.string(), vcimg_path.string());
#endif
        }

        std::string getCaxGeneratorCommand(const std::string& vmb_exe_path, const std::vector<std::string>& cae_files,
                                            const std::string& cax_path, 
                                            std::string res_name = std::string(), 
                                            int id1 = -1, int id2 = -1)
        {
            /*if(res_name.empty())
                res_name = "x";*/
            std::string all_files_str(cae_files[0]);
            for(size_t i = 1; i < cae_files.size(); ++ i)
            {
                all_files_str += " ";
                all_files_str += cae_files[i];
            }

            /*boost::format inst_fmt("--instances=%1%:%2%");
            inst_fmt % id1 % id2;
            std::string inst_str;
            if(id1 != -1 && id2 != -1)
                inst_str = inst_fmt.str();*/
            
#if defined (_MSC_VER) || defined (__MINGW32__)
            //boost::format fmter("\"\"%1%\" --results=\"%2%\" %3% \"%4%\" \"%5%\"\"");
			boost::format fmter(" \"\"%1%\"  \"%2%\" \"%3%\"\"");
#else
            //boost::format fmter("\"%1%\" --results=\"%2%\" %3% \"%4%\" \"%5%\"");
			boost::format fmter(" \"\"%1%\"  \"%2%\" \"%3%\"\"");
#endif
            //fmter % vmb_exe_path % res_name % inst_str % all_files_str % cax_path;
			fmter % vmb_exe_path  % all_files_str % cax_path;
            std::string ret_val = fmter.str();

            return ret_val;
        }

        //std::string getImgGeneratorCommand(const std::string& vcimg_exe_path, const std::string& cax_path, const std::string& png_path)
		std::string getImgGeneratorCommand(const std::string& vcimg_exe_path, const std::string& input_path, const std::string& png_path)
        {
			if(g_bXml)
			{
	#if defined (_MSC_VER) || defined (__MINGW32__)
				boost::format fmter("\"\"%1%\" \"%2%\" \"%3%\"\"");
	#else
				boost::format fmter;
				const char* use_xvfb = getenv("VCOLLAB_USE_XVFB");
				if(use_xvfb != NULL)
					fmter = boost::format("\"%1%\" -xvfb \"%2%\" \"%3%\"");
				else
					fmter = boost::format("\"%1%\" \"%2%\" \"%3%\"");
	#endif
				std::string sBat = "-b";
				//input_path is xml path
				fmter % vcimg_exe_path % sBat % input_path;
				std::string ret_val = fmter.str();
				return ret_val;
			}
			else
			{
	#if defined (_MSC_VER) || defined (__MINGW32__)
				boost::format fmter("\"\"%1%\" \"%2%\" \"%3%\" %4% %5% %6%\"");
	#else
				boost::format fmter;
				const char* use_xvfb = getenv("VCOLLAB_USE_XVFB");
				if(use_xvfb != NULL)
                    fmter = boost::format("\"%1%\" -xvfb \"%2%\" \"%3%\" %4% %5% %6%");
				else
                    fmter = boost::format("\"%1%\" \"%2%\" \"%3%\" %4% %5% %6%");
	#endif
				std::string sImgDimension = "600 500";
				std::string sDefView      = "Iso";
				std::string sBkColor      = "0.5 0.5 0.5";
				//input_path is cax path
				fmter % vcimg_exe_path % input_path % png_path % sImgDimension % sDefView % sBkColor;
				std::string ret_val = fmter.str();
				return ret_val;
			}
        }

        //std::string AddModelImage(const std::pair< std::string,  XmlNodeVec>& sec_info, const std::vector<std::string>& cae_files, std::string& file_type)
		std::string AddModelImage( const std::vector<std::string>& cae_files)//, std::string& file_type)
        {
            namespace bfs = boost::filesystem;
            
            std::string ret_val;
          
            std::pair<std::string, std::string> exe_paths = getExePaths();
            if(exe_paths.first.empty() || exe_paths.second.empty())
                return ret_val;

            //bfs::path img_out_dir("ekm-report-files"); 
			bfs::path img_out_dir("reportFiles");
			if (g_bImgGenProcess)
			{
				if (!bfs::exists(img_out_dir))
					bfs::create_directory(img_out_dir);
			}

            std::string img_src_dir(g_sMinervaReportDir);
            bfs::path cax_path("model.cax");
            std::string cax_gen_command = getCaxGeneratorCommand(exe_paths.first, cae_files, cax_path.string());
            //TRACE_LOG << "Generating Cax ...";
            //TRACE_LOG << cax_gen_command;
            //TRACE_LOG << "Generating Cax ... done";
			vcUtils::LogMsg("Generating Cax ...");
			vcUtils::LogMsg(cax_gen_command);
            system(cax_gen_command.c_str());
			vcUtils::LogMsg("Generating Cax ... done");

            bfs::path img_path(img_out_dir / "cad_model.png");
            std::string img_src_str(img_src_dir + "/cad_model.png");
			bfs::path input_path = cax_path;
            
			//std::string img_gen_command = getImgGeneratorCommand(exe_paths.second, cax_path.string(), img_path.string());
			bfs::path vcdir(vcdir_env);
			bfs::path xml_path(vcdir / "Samples/Batchmode Inputs/VCollabEkm.xml");
			if(bfs::exists(xml_path))
			{
				g_bXml = true;
				input_path = xml_path;
			}

			if (g_bImgGenProcess)
			{
				std::string img_gen_command = getImgGeneratorCommand(exe_paths.second, input_path.string(), img_path.string());
				//TRACE_LOG << "Generating Image ...";
				//TRACE_LOG << img_gen_command;
				//TRACE_LOG << "Generating Image ... done";
				vcUtils::LogMsg("Generating Image ...");
				vcUtils::LogMsg(img_gen_command);
				system(img_gen_command.c_str());
				vcUtils::LogMsg("Generating Image ... done");
			}
			else
				vcUtils::LogMsg("Image Generation is ignored");



            /*bfs::path avz_path("model.avz");
            std::string avz_gen_command = getImgGeneratorCommand(exe_paths.second, cax_path.string(), avz_path.string());
            //TRACE_LOG << "Generating Image ...";
            //TRACE_LOG << img_gen_command;
            //TRACE_LOG << "Generating Image ... done";
            system(avz_gen_command.c_str());*/

            if(bfs::exists(cax_path))
                bfs::remove(cax_path);
            
            if(bfs::exists(img_path))
            {
                boost::format fmter("    <image name=\"%1%\" src=\"%2%\"/>\n");
                fmter % "Image" % img_src_str;
                ret_val = fmter.str();
            }

            return ret_val;
        }

#include <boost/locale/encoding_utf.hpp>
#include <string>
using boost::locale::conv::utf_to_utf;

inline std::wstring encodeString(const std::string& str, int encoding=GetACP())
{   
	/*int sz = (int) str.size();
	int wsz = MultiByteToWideChar(encoding, 0, str.c_str(), sz, 0, 0);
	std::wstring wstr;
	if(wsz > 0)
	{
		wstr.resize(wsz);
		MultiByteToWideChar(encoding, 0, str.c_str(), sz, &wstr[0], wsz);
	}
	return wstr;*/

	std::wstring wstr;
	wstr = utf_to_utf<wchar_t>(str.c_str(), str.c_str() + str.size());

	return wstr;
}

inline std::string decodeString(const std::wstring& wstr, int encoding=65001)
{
	/*int wsz = (int) wstr.size();   
	int sz = WideCharToMultiByte(encoding, 0, wstr.c_str(), wsz, 
	0, 0,  NULL, NULL);
	std::string str;
	if(sz > 0)
	{
		str.resize(sz);
		WideCharToMultiByte(encoding, 0, wstr.c_str(), wsz, &str[0], sz,  NULL, NULL);
	}
	return str;*/

	std::string str; 
	str = utf_to_utf<char>(wstr.c_str(), wstr.c_str() + wstr.size());
	return str;
}

inline std::string EncodeString(std::string str)
{
	std::wstring wstr = encodeString(str);
	std::string encodedStr = decodeString(wstr);
	return encodedStr;
}

std::string ToLower(std::string str)
{
	std::string lower = str;
	for(int i=0;i<lower.size();i++)
		lower[i] = tolower(lower[i]);

	return lower;
}

bool Convert_To_Xml_Key_Field(const  char* str,std::string &xml_string)
{
	try
	{
		for(;*str != '\0'; ++ str)
		{
			char buf[8];
			switch(*str)
			{
				case  '<': 
				case  '>': 
				case  '&': 
				case  '"': 
				case '\'': 
				case ':': 
				//case ' ': 
					break;
				default:
					if(isprint(*str))
						xml_string.push_back(*str);
					else
					{
	#if defined(_MSC_VER)
						_snprintf(buf, 8, "&#x%02X;", (unsigned)(*str & 0xff));
	#else
						snprintf(buf, 8, "&#x%02X;", (unsigned)(*str & 0xff));
	#endif
						xml_string.append(buf);
					}
			}
		}
	
		//removes if 1st char is between 0 and 9 or fist char is not alphabet
		if((xml_string[0]>47 && xml_string[0]<58) || !isalpha(xml_string[0]))
			xml_string.erase(0,1);

		//removes if first 3 chars are "xml"
		if(xml_string.size()>3)
		{
			std::string tmp = ToLower(xml_string);
			if(tmp.find("xml",0,3)!=std::string::npos)
				xml_string.erase(0,3);
		}
	
		if(ToLower(xml_string)=="xml")
			return false;
	}
	catch(...)
	{
		return false;
	}
    return true;
}

bool Convert_To_Xml_Value_Field(const  char* str,std::string &xml_string)
{
	try
	{
		for(;*str != '\0'; ++ str)
		{
			char buf[8];
			switch(*str)
			{
				case '<': xml_string.append("&lt;"); break;
				case '>': xml_string.append("&gt;"); break;
				case '&': xml_string.append("&amp;"); break;
				case '"': xml_string.append("&quot;"); break;
				case '\'': xml_string.append("&apos;"); break;
				default:
					if(isprint(*str))
						xml_string.push_back(*str);
					else
					{
	#if defined(_MSC_VER)
						_snprintf(buf, 8, "&#x%02X;", (unsigned)(*str & 0xff));
	#else
						snprintf(buf, 8, "&#x%02X;", (unsigned)(*str & 0xff));
	#endif
						xml_string.append(buf);
					}
			}
		}
	}
	catch(...)
	{
		return false;
	}
    return true;
}


/*
Names can contain letters, numbers, and other characters
Names cannot start with a number or punctuation character
Names cannot start with the letters xml (or XML, or Xml, etc)
//Names cannot contain spaces
*/
bool Convert_To_EKM_Xml_Key_Field(const  char* str,std::string &xml_string)
{
	try
	{
		//removes if char is other than alpha-numeric
		for(;*str != '\0'; ++ str)
		{
			if(isalnum(*str) || *str=='_' || *str==' ' ||  *str=='\t')
				  xml_string.push_back(*str);
		}

		//removes if 1st char is between 0 and 9 or fist char is not alphabet
		if((xml_string[0]>47 && xml_string[0]<58) || !isalpha(xml_string[0]))
			xml_string.erase(0,1);

		//removes if first 3 chars are "xml"
		if(xml_string.size()>3)
		{
			std::string tmp = ToLower(xml_string);
			if(tmp.find("xml",0,3)!=std::string::npos)
				xml_string.erase(0,3);
		}
	
		if(ToLower(xml_string)=="xml")
			return false;
	}
	catch(...)
	{
		return false;
	}
    return true;;
}

bool Convert_To_EKM_Xml_Value_Field(const  char* str,std::string &xml_string)
{
	try
	{
		for(;*str != '\0'; ++ str)
		{
			char buf[8];
			switch(*str)
			{
				case  '<': 
				case  '>': 
				case  '&': 
				case  '"': 
				case '\'': 
				case  '%':
					break;
				default:
					if(isprint(*str))
						xml_string.push_back(*str);
					else
					{
	#if defined(_MSC_VER)
						_snprintf(buf, 8, "&#x%02X;", (unsigned)(*str & 0xff));
	#else
						snprintf(buf, 8, "&#x%02X;", (unsigned)(*str & 0xff));
	#endif
						xml_string.append(buf);
					}
			}
		}
	}
	catch(...)
	{
		return false;
	}
    return true;
}

vcCad2Xml::vcCad2Xml(char *sInputFileName,char *sOutputFileName,char *sTmpWorkingDir,bool &bSuccess)
{
	/*std::string Key;
	bool ret;
	ret = Convert_To_EKM_Xml_Key_Field("$Xml ",Key);
	Key.erase();
	ret = Convert_To_EKM_Xml_Key_Field("$xml",Key);
	Key.erase();
	ret = Convert_To_EKM_Xml_Key_Field("ab",Key);
	Key.erase();
	ret = Convert_To_EKM_Xml_Key_Field("2xyz",Key);
	Key.erase();
	ret = Convert_To_EKM_Xml_Key_Field("xmlABC",Key);
	Key.erase();
	ret = Convert_To_EKM_Xml_Key_Field("XMLabc",Key);
	Key.erase();
	ret = Convert_To_EKM_Xml_Key_Field("Xmlabc",Key);
	Key.erase();
	ret = Convert_To_EKM_Xml_Key_Field("$Xmlabc",Key);
	Key.erase();
	ret = Convert_To_EKM_Xml_Key_Field("$Xml abc",Key);*/

	vcUtils::LogMsg("vcCad2Xml::Start");

	m_pDtkAPI = NULL;
	m_sInputFileName = sInputFileName;
	m_sOutputFileName = sOutputFileName;
	m_sTmpWorkingDir = sTmpWorkingDir;
	m_iIndent = 0;
	m_iXmlFormat = g_iXMLFormat;
	bSuccess = Convert();

	vcUtils::LogMsg("vcCad2Xml::End");
}

bool vcCad2Xml::Convert()
{
	vcUtils::LogMsg("vcCad2Xml::Convert()::Start");

	EnableCADReaders();

	DtkErrorStatus errorStatus = dtkNoError;
	try
	{
		errorStatus = StartDtkAPI();
	}
	catch(...)
	{
		m_iErrorCode = DATAKIT_ERROR;
		return false; 
	}
	if(errorStatus != dtkNoError)
	{
		m_dtkErrorStatus=errorStatus;
		m_iErrorCode = DATAKIT_ERROR;
		return false; 
	}

	igesr_SetConfigExcludeGroup(1);	

	Dtk_MainDocPtr CadDoc;
	try
	{
		errorStatus = OpenCADFile(CadDoc);
	}
	catch(...)
	{
		m_iErrorCode = DATAKIT_ERROR;
		return false; 
	}
	if(errorStatus != dtkNoError)
	{
		m_dtkErrorStatus=errorStatus;
		m_iErrorCode = DATAKIT_ERROR;
		vcUtils::LogMsg("vcCad2Xml::Convert()::Opening the CAD file failed");
		StopDtkAPI();
		return false;
	}
	
	//Open XML to write
	if (g_sOutputXMLFilesFormatMap.size())
	{
		std::map<std::string,int>::iterator itr;
		for (itr = g_sOutputXMLFilesFormatMap.begin(); itr != g_sOutputXMLFilesFormatMap.end(); itr++)
		{
			m_sOutputFileName = Dtk_string(itr->first.c_str());
			m_iXmlFormat = itr->second;
			vcUtils::LogMsg(std::string("vcCad2Xml::Convert()::Writing XML file :  ") + std::string(m_sOutputFileName.c_str()));
			if (g_bOutputXml)
			{
				if(OpenFile(m_sOutputFileName.c_str()) == false)
				{
					m_iErrorCode = FILE_ACCESS_ERROR;
					vcUtils::LogMsg("vcCad2Xml::Convert()::Opening the XML file to write failed");
					StopDtkAPI();
					return false; 
				}
			}
		
			try
			{
				errorStatus = WriteXml(CadDoc);
				outfile.close();
			}
			catch(...)
			{
				m_iErrorCode = DATAKIT_ERROR;
				return false; 
			}
			if (errorStatus != dtkNoError)
			{
				m_dtkErrorStatus = errorStatus;
				m_iErrorCode = DATAKIT_ERROR;
				vcUtils::LogMsg(std::string("vcCad2Xml::Convert()::Writing XML file failed for ") + std::string(m_sOutputFileName.c_str()));
				//StopDtkAPI();
				//return false;
			}
		}
	}
	else
	{
		try
		{
			errorStatus = WriteXml(CadDoc);
		}
		catch (...)
		{
			m_iErrorCode = DATAKIT_ERROR;
			return false;
		}
	}
	if(errorStatus != dtkNoError)
	{
		m_dtkErrorStatus=errorStatus;
		m_iErrorCode = DATAKIT_ERROR;
		vcUtils::LogMsg("vcCad2Xml::Convert()::Writing XML file failed");
		StopDtkAPI();
		return false;
	}
	
	CloseCADFile(CadDoc); 

	StopDtkAPI();

	vcUtils::LogMsg("vcCad2Xml::Convert()::End");
	return true;
}

vcCad2Xml::~vcCad2Xml()
{
}

void vcCad2Xml::EnableCADReaders()
{
	vcUtils::LogMsg("vcCad2Xml::EnableCADReaders():Start");

	if(vcLicense::m_bCatiaV6License)
		CatiaV6Reader::Enable();
	if(vcLicense::m_bCatiaV4License)
		CatiaV4Reader::Enable();
	if(vcLicense::m_bCatiaV5License)
		CatiaV5Reader::Enable();
	if(vcLicense::m_bIgesLicense)
		IgesReader::Enable();
	if(vcLicense::m_bProeLicense)
		ProeReader::Enable();
	if(vcLicense::m_bStepLicense)
		StepReader::Enable();
	if(vcLicense::m_bUgLicense)
		UgReader::Enable();
	if(vcLicense::m_bSolidWorksLicense)
		SwReader::Enable();
	if(vcLicense::m_bSolidEdgeLicense)
		SeReader::Enable();
	if(vcLicense::m_bInventorLicense)
		InvReader::Enable();
	if(vcLicense::m_bCgrLicense)
		CgrReader::Enable();
	if(vcLicense::m_bParasolidLicense)
		PsReader::Enable ();
	if(vcLicense::m_bAcisLicense)
		AcisReader::Enable();
	if (vcLicense::m_bRevitLicense)
		RevitReader::Enable();
	if (vcLicense::m_bIfcLicense)
		IfcReader::Enable();
	if (vcLicense::m_bStlLicense)
		StlReader::Enable();

	vcUtils::LogMsg("vcCad2Xml::EnableCADReaders():End");
}


Dtk_ErrorStatus vcCad2Xml::StartDtkAPI()
{
	vcUtils::LogMsg("vcCad2Xml::StartDtkAPI():Start");

	Dtk_ErrorStatus errorStatus = dtkNoError; 

#ifdef _LINUX64_
 	if(m_sTmpWorkingDir[m_sTmpWorkingDir.len()-1] != '/')
        	m_sTmpWorkingDir.Merge("/");
#else
	if(m_sTmpWorkingDir[m_sTmpWorkingDir.len()-1] != '\\')
		m_sTmpWorkingDir.Merge("\\");
#endif
	m_pDtkAPI = Dtk_API::StartAPI(m_sTmpWorkingDir, errorStatus, "VCOLLAB_SDK-170092217140010146187057140087227106200052053225206228253120022230154211033210227167181138169073103011117156088102093069252055244082189172018211172233079224079026239024130161046029174225164029254174199084181068197145000193037147121147071067175206137134191135246139094253034071232108167067156209204041162254022101034234144187006124072189031160233154208171163016244001047205185142117054206218112117081216204129175174117209165012129039149135093061082030195171114251117129028092211062011047119158081247159176232198054240139121174076028165212136137167122005230231178056230038012054186217056187025101060190068130097026035106034050151119110040148037156205072250084236203006095188028052192062211177002049174124208241255232035013086001056124255185056143057182218121164205065053252092092202225052042226117159250108164074003165148177001034046176056160147194225011059120138211228077120123215251086249158058108075120005140255-304V232");

	ss.str("");
	ss<<"vcCad2Xml::StartDtkAPI():Dtk_ErrorStatus: "<<dtkTypeError(errorStatus).c_str();
	vcUtils::LogMsg(ss.str());

	if(errorStatus == dtkErrorLicence)
	{
       printf("No license available\n");
	   vcUtils::LogMsg("vcCad2Xml::StartDtkAPI():No license available");
	   return dtkErrorLicence;
	}

	if(m_pDtkAPI == NULL)
	{
		printf("Can't Start DATAKIT API\n");
		vcUtils::LogMsg("vcCad2Xml::StartDtkAPI():Failed to start DATAKIT API");
		return dtkErrorAPINotStarted;
	}

	m_pDtkAPI->SetBodyModePreference(DTK_BODYMODE_COMPLETETOPOLOGY);
	m_pDtkAPI->ActivateSplitForPeriodicFaces();


	//If you want to use tesselation library start Tesselation Kernel
	#ifdef ACTIVATE_TESSELATION_LIB
		 int status = tess_InitTesselation("tess_tmp",0.05);
		 if(status == dtkErrorLicence)
		 {
			  printf("No tesselation license available\n");
			  vcUtils::LogMsg("vcCad2Xml::StartDtkAPI():No tesselation license available");
		 }
	#endif

	 vcUtils::LogMsg("vcCad2Xml::StartDtkAPI():End");
	 return dtkNoError;
}


void vcCad2Xml::StopDtkAPI()
{
	vcUtils::LogMsg("vcCad2Xml::StopAPI():Start");

	Dtk_API::StopAPI(m_pDtkAPI);

	vcUtils::LogMsg("vcCad2Xml::StopAPI():End");
}

Dtk_ErrorStatus vcCad2Xml::CloseCADFile(Dtk_MainDocPtr TmpDoc)
{
	vcUtils::LogMsg("vcCad2Xml::CloseCADFile():Start");

	// You Get the current API 
	DtkErrorStatus errorStatus = dtkNoError; 
	Dtk_API * MyAPI = Dtk_API::GetAPI();
	//We close the opened document
	if(MyAPI)
		errorStatus = MyAPI->EndDocument(TmpDoc);

	vcUtils::LogMsg("vcCad2Xml::CloseCADFile():End");
	return errorStatus;
}

Dtk_ErrorStatus vcCad2Xml::OpenCADFile(Dtk_MainDocPtr &TmpDoc)
{
	vcUtils::LogMsg("vcCad2Xml::OpenCADFile():Start");

	// You Get the current API 
	DtkErrorStatus stError = dtkNoError; 
	Dtk_API * MyAPI = Dtk_API::GetAPI();
	
#ifdef WIN32
	char szModuleFileName[MAX_PATH];
	GetModuleFileName(NULL, szModuleFileName, sizeof(szModuleFileName)); 
	std::string Path = vcUtils::GetPathFromFileName(szModuleFileName);
	Dtk_string inRepSchema = Path.c_str()+Dtk_string("\\schema");
	
	ss.str("");
	ss<<"vcCad2Xml::OpenCADFile():PsKernal Schema Path: "<<inRepSchema.c_str();
	vcUtils::LogMsg(ss.str());
	ss.str("");

	char TmpFullPathSchemaDir[_MAX_PATH];
	if( _fullpath( TmpFullPathSchemaDir, inRepSchema.c_str(), _MAX_PATH ) != NULL )
		MyAPI->SetSchemaDir (TmpFullPathSchemaDir);
	else
		MyAPI->SetSchemaDir (inRepSchema);
#else 
	char szModuleFileName[1024];
    //??GetModuleFileName(NULL, szModuleFileName, sizeof(szModuleFileName));
	std::string Path = vcUtils::GetPathFromFileName(szModuleFileName);
	Dtk_string inRepSchema = Path.c_str()+Dtk_string("\\schema");
	
	ss.str("");
	ss<<"vcCad2Xml::OpenCADFile():PsKernal Schema Path: "<<inRepSchema.c_str();
	vcUtils::LogMsg(ss.str());
	ss.str("");

	 MyAPI->SetSchemaDir (inRepSchema);
#endif 
	//If you want to get a log file for reader (inventory, missing files in assembly...) you have to set it
	 std::string dtklogpath;
#ifdef _LINUX64_
         dtklogpath = m_sTmpWorkingDir.c_str()+std::string("/DtkLogFile.txt");
#else
	 dtklogpath = m_sTmpWorkingDir.c_str()+std::string("\\DtkLogFile.txt");
#endif
	 Dtk_string DtkLogFilePath = dtklogpath.c_str();
	 MyAPI->SetLogFile(DtkLogFilePath);

	printf("\nReading file %s \n",m_sInputFileName.c_str());

	//You Open the file you want to read and get corresponding Document 
	Dtk_ErrorStatus errorStatus = MyAPI->OpenDocument(m_sInputFileName, TmpDoc);

	std::stringstream ss;
	ss<<"vcCad2Xml::OpenCADFile():DtkAPI->OpenDocument():Error Status: "<<dtkTypeError(errorStatus).c_str();
	vcUtils::LogMsg(ss.str());
	ss.str("");

	if(errorStatus == dtkErrorUnavailableReader)
	{
		std::string err;
		ss.str("");
		ss<<"License is not available for this format. Contact support@vcollab.com";
		vcUtils::LogMsg(ss.str(),true);
		return errorStatus;
	}

	//If no Error we write the Document
	if(errorStatus == dtkNoError && TmpDoc.IsNotNULL() )
	{
		vcUtils::LogMsg("vcCad2Xml::OpenCADFile():OpenDocument succeeded");
		Dtk_Config *config = MyAPI->GetConfig();
	}
	else
	{
		vcUtils::LogMsg("vcCad2Xml::OpenCADFile():Failed to load assembly tree");
		printf("Error Loading Assembly Tree : %s\n",dtkTypeError(errorStatus).c_str());
	}

	vcUtils::LogMsg("vcCad2Xml::ReadCADFile():End");
	return errorStatus;
}

Dtk_ErrorStatus vcCad2Xml::WriteXml(Dtk_MainDocPtr inDocument) 
{
	vcUtils::LogMsg("vcCad2Xml::WriteXml():Start");

	g_bImgGenProcess = true;
	char* sMinervaLeanReport = getenv("VCOLLAB_MINERVA_LEAN_REPORT");
	if (sMinervaLeanReport != NULL)
	{
		try
		{
			int iEnvVal = boost::lexical_cast<int>(sMinervaLeanReport);
			if (iEnvVal)
			{
				g_bImgGenProcess = false;
				vcUtils::LogMsg("VCOLLAB_MINERVA_LEAN_REPORT is set");
			}
		}
		catch(...)
		{}		
	}

	//First we get the root component in the document
	Dtk_ComponentPtr RootComponent = inDocument->RootComponent();
	m_RootComponent = RootComponent;

	Dtk_NodePtr RootNode;
	Dtk_API* inAPI = Dtk_API::GetAPI();
	inAPI->ReadComponent(RootComponent, RootNode);
	Dtk_MaterialPtr mat = inDocument->RootComponent()->GetMaterial();

	//if no Error we write the Component
	if(RootComponent.IsNotNULL())
	{ 
		//m_FilePathStrArray.push_back(RootComponent->FullPathName().c_str());

		m_MetaData.m_sNativeCadFileName = RootComponent->FullPathName().c_str();

		m_ReaderType = RootComponent->GetAssociatedModuleType(); 
		m_MetaData.m_sNativeCadPackageName = GetCadPackageName(m_ReaderType);

		if(RootComponent->GetFileVersion().is_not_NULL() && strlen(RootComponent->GetFileVersion().c_str()))
			this->m_MetaData.m_sNativeCadPackageVersion = RootComponent->GetFileVersion().c_str();
		else
			this->m_MetaData.m_sNativeCadPackageVersion = "Unknown";

		std::string units(GetUnitsName(RootComponent->GetConceptionUnitScale()));
		m_MetaData.m_sUnitsName  = units;

		Dtk_string ComponentName;
		if(RootComponent->Name().is_not_NULL())
			ComponentName = RootComponent->Name();
		if(ComponentName.is_not_NULL() && ComponentName.len())
			m_MetaData.m_sAssemblyName = ComponentName.c_str();
		else
			m_MetaData.m_sAssemblyName = "Default";
		
		m_MetaData.m_sAuthorName;// = "Unknown";
		m_MetaData.m_sDate;// = "Unknown";
		int cnt = RootComponent->GetNumMetaData();
	
		ss.str("");
		ss<<"NumMetaData : "<<cnt;
		vcUtils::LogMsg(ss.str());
		ss.str("");

		for(int i=0;i<cnt;i++)
		{
			Dtk_MetaDataPtr MetaData = RootComponent->GetMetaData(i);
			if(MetaData.IsNULL())
				continue;
			Dtk_string Title=MetaData->GetTitle();
			Dtk_string Value=MetaData->GetValue();
			if(Title.is_not_NULL() && Value.is_not_NULL())
			{
				/*ss.str("");
				ss<<"Title : "<<Title.c_str();
				vcUtils::LogMsg(ss.str());
				ss.str("");

				ss.str("");
				ss<<"Value : "<<Value.c_str();
				vcUtils::LogMsg(ss.str());
				ss.str("");*/

				if (!RootComponent->IsAssembly())
				{
					if (Title == "Density" || Title == "Volume" || Title == "Mass" || Title == "Surface" || Title == "xCenterOfGravity" ||
						Title == "yCenterOfGravity" || Title == "zCenterOfGravity" || Title == "Ixx" || Title == "Iyx" || Title == "Izx" ||
						Title == "Ixy" || Title == "Iyy" || Title == "Izy" || Title == "Ixz" || Title == "Iyz" || Title == "Izz"
						)
						continue;
				}

				this->m_MetaDataTitleStrArray.push_back(Title);
				this->m_MetaDataValueStrArray.push_back(Value);
			}
		}

		if(m_iXmlFormat==XML_FORMAT::GENERIC)
		{
			WriteXmlNode("VMoveCAD");
				WriteXmlNode(m_MetaData.m_sAssemblyName);
					WriteXmlAttribute("modelFile",m_MetaData.m_sNativeCadFileName);
					WriteXmlNode("modelParameters");

						WriteXmlAttribute("cadPackageName",m_MetaData.m_sNativeCadPackageName);
						WriteXmlAttribute("cadPackageVersion",m_MetaData.m_sNativeCadPackageVersion);
						/*if(!m_MetaData.m_sAuthorName.empty())
							WriteXmlAttribute("authorName",m_MetaData.m_sAuthorName);
						if(!m_MetaData.m_sDate.empty())
							WriteXmlAttribute("creationDate",m_MetaData.m_sDate);*/
						for(int i=0;i<m_MetaDataTitleStrArray.size();i++)
						{
							/*Dtk_string Key = m_MetaDataTitleStrArray[i];
							Dtk_string Value = m_MetaDataValueStrArray[i];
							if(Title.is_not_NULL() && Value.is_not_NULL())
								WriteXmlAttribute(Title.c_str(),Value.c_str());*/
							Dtk_string Key = m_MetaDataTitleStrArray[i];
							Dtk_string Value = m_MetaDataValueStrArray[i];
						
							if(Key.is_NULL() || Value.is_NULL())
								continue;

							/*std::string sKey,sValue;
							if(!Convert_To_Xml_Key_Field(Key.c_str(),sKey))
								continue;

							if(!Convert_To_Xml_Value_Field(Value.c_str(),sValue))
								continue;
								
							WriteXmlAttribute(sKey,sValue);*/
							//XML Compatible conversion is done in the following method. So above to conversion calls are commented out
							WriteXmlAttribute(Key.c_str(),Value.c_str());
						}
						WriteXmlAttribute("units",m_MetaData.m_sUnitsName);

					WriteXmlNode();//End tag for WriteXmlNode("Model Parameters");
	
					//WriteXmlNode(m_MetaData.m_sAssemblyName);
						WriteComponent( RootComponent );
					//WriteXmlNode();//End tag for WriteXmlNode("Assembly Tree");

				WriteXmlNode();//End tag for WriteXmlNode(m_MetaData.m_sAssemblyName);
			WriteXmlNode();//End tag for WriteXmlNode("VMoveCAD");
		}
		else if( m_iXmlFormat==XML_FORMAT::KEY_VALUE_PAIRS)
		{
			std::string standard = "<vct:metadata xmlns:vct=\"http://www.vcollab.com/xml/namespace\">\n";
			outfile.write(standard.c_str(),standard.length());
			WriteKeyValueXmlAttribute("modelFile",m_MetaData.m_sNativeCadFileName);
			WriteKeyValueXmlAttribute("cadPackageName",m_MetaData.m_sNativeCadPackageName);
			WriteKeyValueXmlAttribute("cadPackageVersion",m_MetaData.m_sNativeCadPackageVersion);
			WriteKeyValueXmlAttribute("authorName",m_MetaData.m_sAuthorName);
			WriteKeyValueXmlAttribute("units",m_MetaData.m_sUnitsName);

			WriteComponent( RootComponent );
			if(m_MetaData.m_PartNamesStrArray.size())
			{
				std::string sPartNames;
				for(int i=0;i<m_MetaData.m_PartNamesStrArray.size();i++)
				{
					sPartNames += m_MetaData.m_PartNamesStrArray[i];
					if(i!=m_MetaData.m_PartNamesStrArray.size()-1)
						sPartNames += std::string(", ");
				}
				WriteKeyValueXmlAttribute("parts",sPartNames);
			}

			if(m_MetaData.m_MaterialNamesStrArray.size()) 
			{
				std::string sMaterialNames;
				for(int i=0;i<m_MetaData.m_MaterialNamesStrArray.size();i++)
				{
					sMaterialNames += m_MetaData.m_MaterialNamesStrArray[i];
					if(i!=m_MetaData.m_MaterialNamesStrArray.size()-1)
						sMaterialNames += std::string(", ");
				}
				WriteKeyValueXmlAttribute("materials",sMaterialNames);
			}

			std::string end = "</vct:metadata>\n";
			outfile.write(end.c_str(),end.length()); 
		}	
		else if(m_iXmlFormat==XML_FORMAT::EKM_META_DATA)
		{
			std::string standard = "<ekmMetaData:meta-data \n \t     xmlns:ekmMetaData=\"http://www.ansys.com/ekm/metaData\">\n";
			outfile.write(standard.c_str(),standard.length());
			WriteEkmXmlAttribute("modelFile",m_MetaData.m_sNativeCadFileName);
			WriteEkmXmlAttribute("cadPackageName",m_MetaData.m_sNativeCadPackageName);
			WriteEkmXmlAttribute("cadPackageVersion",m_MetaData.m_sNativeCadPackageVersion);
			WriteEkmXmlAttribute("authorName",m_MetaData.m_sAuthorName);
			WriteEkmXmlAttribute("units",m_MetaData.m_sUnitsName);

			for(int i=0;i<m_MetaDataTitleStrArray.size();i++) 
			{
				Dtk_string Key = m_MetaDataTitleStrArray[i];
				Dtk_string Value = m_MetaDataValueStrArray[i];
			
				if(Key.is_NULL() || Value.is_NULL())
					continue;

				/*std::string sKey,sValue;
				if(!Convert_To_EKM_Xml_Key_Field(Key.c_str(),sKey))
					continue;

				if(!Convert_To_EKM_Xml_Value_Field(Value.c_str(),sValue))
					continue;
					
				WriteEkmXmlAttribute(sKey,sValue);*/
				//XML Compatible conversion is done in the following method. So above to conversion calls are commented out
				WriteEkmXmlAttribute(Key.c_str(),Value.c_str());
			}
			//Commented on 161226
			//The following information is not required for --ekm-meta-data
			WriteComponent( RootComponent );
			std::string end = "</ekmMetaData:meta-data>";
				outfile.write(end.c_str(),end.length());
		}
		else if(m_iXmlFormat==XML_FORMAT::EKM_REPORT)
		{
			std::vector<std::string> cad_files;
			cad_files.push_back(m_sInputFileName.c_str()); 
			

			std::string str;
			std::stringstream ss;

			str = "<ekmReport:section \n \t\txmlns:ekmReport=\"http://www.ansys.com/ekm/report\" \n \t\tname=\"CAD Model Report\" >\n";
			WriteXmlNode_Start(str);


			//To Write the image entries

			//Image Generation invoked by CadInfo
			std::string xml_img_str;
			xml_img_str = AddModelImage(cad_files);

			//Image Generated by Minerva Process (from WCAX)
			std::string xml_img_str_from_wcax;
			std::string sFileName = vcUtils::GetFileNameFromFullPath(m_sInputFileName.c_str());
			namespace bfs = boost::filesystem;
			bfs::path img_out_dir(g_sMinervaReportDir);
			std::string sImg = sFileName + ".png";
			bfs::path img_path(img_out_dir / sImg);
			std::string img_src_str(g_sMinervaReportDir + "/" + sImg);
			if (bfs::exists(img_src_str))
			{
				boost::format fmter("    <image name=\"%1%\" src=\"%2%\"/>\n");
				fmter % "Image" % img_src_str;
				xml_img_str_from_wcax = fmter.str();
			}
			if (xml_img_str.length())
			{
				//To keep both the image entries in the same indent level
				if (xml_img_str_from_wcax.length())
					WriteXmlNode_Start(xml_img_str, false);
				else
					WriteXmlNode_Start(xml_img_str);
			}
			if (xml_img_str_from_wcax.length())
				WriteXmlNode_Start(xml_img_str_from_wcax);


			
			//str = "<section name=\"CAD Model Report\">\n";	
			//WriteXmlNode_Start(str);

				/*str = "<text name=\"Description:\" showFormattedContent=\"false\">CAD Model Report\n";
				WriteXmlNode_Start(str);
				str = "</text>\n"; 
				WriteXmlNode_End(str);*/
				
				str = "<table caption=\"\" name=\"Model Information\" showBorder=\"true\" showHeaders=\"true\">\n";
				WriteXmlNode_Start(str);

					str = "<column id=\"Item\" name=\"Item\"/>\n";
					WriteXmlNode_Start(str,false);
					str = "<column id=\"Value\" name=\"Value\"/>\n";
					WriteXmlNode_Start(str,false);

					WriteEkmReport_Row("Model File",m_MetaData.m_sNativeCadFileName);
					WriteEkmReport_Row("CAD Package Name",m_MetaData.m_sNativeCadPackageName);
					WriteEkmReport_Row("CAD Package Version",m_MetaData.m_sNativeCadPackageVersion);
					WriteEkmReport_Row("Author Name",m_MetaData.m_sAuthorName);
					WriteEkmReport_Row("Units",m_MetaData.m_sUnitsName);
				
					for(int i=0;i<m_MetaDataTitleStrArray.size();i++) 
					{
						Dtk_string Key = m_MetaDataTitleStrArray[i];
						Dtk_string Value = m_MetaDataValueStrArray[i];
			
						if(Key.is_NULL() || Value.is_NULL())
							continue;

						/*std::string sKey,sValue;
						if(!Convert_To_EKM_Xml_Key_Field(Key.c_str(),sKey))
							continue;

						if(!Convert_To_EKM_Xml_Value_Field(Value.c_str(),sValue))
							continue;
					
						WriteEkmReport_Row(sKey,sValue);*/
						//XML Compatible conversion is done in the following method. So above to conversion calls are commented out
						WriteEkmReport_Row(Key.c_str(),Value.c_str());
					}

				str = "</table>\n"; 
				WriteXmlNode_End(str);

				//Commented on 161226
				//The following information is not required for --ekm-meta-data
				WriteComponent( RootComponent );

				
				//str = "</section>\n"; 
				//WriteXmlNode_End(str);

			str = "</ekmReport:section>"; 
			WriteXmlNode_End(str);
		}
		else if (m_iXmlFormat == XML_FORMAT::EKM_ASM_DEP_EXTRACT)
		{
			std::string standard = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<ansysMinerva:dependencies xmlns:ansysMinerva=\"https://www.ansys.com/minerva/dependency\">\n";
			outfile.write(standard.c_str(), standard.length());

			WriteComponent(RootComponent);

			std::string end = "</ansysMinerva:dependencies>";
			outfile.write(end.c_str(), end.length());
		}

		vcUtils::LogMsg("vcCad2Xml::WriteXml():End");
		return dtkNoError;
	}

	vcUtils::LogMsg("vcCad2Xml::WriteXml():RootComponent = NULL");
	return dtkErrorNullPointer;
}
Dtk_ErrorStatus vcCad2Xml::WriteMassProperties(Dtk_ComponentPtr inComponent)
{
	if (m_iXmlFormat == XML_FORMAT::EKM_ASM_DEP_EXTRACT)
	{
		return dtkNoError;
	}

	//Mass Properties is ignored for following formats EKM MetaData and Report
	if(m_iXmlFormat==XML_FORMAT::EKM_META_DATA || m_iXmlFormat==XML_FORMAT::EKM_REPORT )
	{
		if(m_ReaderType == DtkReaderType::V6ReaderModule ||
		   m_ReaderType == DtkReaderType::CgrReaderModule ||
		   m_ReaderType == DtkReaderType::StepReaderModule ||
		   m_ReaderType == DtkReaderType::IgesReaderModule ||
		   m_ReaderType == DtkReaderType::V4ReaderModule
		  )
			return dtkNoError;
	}


	if(inComponent.IsNULL())
		return dtkNoError;

	Dtk_Size_t NumMetaData;
	NumMetaData = inComponent->GetNumMetaData();
	std::vector<Dtk_string> MetaDataTitleStrArray;
	if(!NumMetaData)							
		return dtkNoError;

	int iMassPropertiesCount=0;
	for (int i = 0; i < NumMetaData; i++)
	{
		const Dtk_MetaDataPtr  MetaDataObject = inComponent->GetMetaData(i);
		if(MetaDataObject.IsNULL())
			continue;
		Dtk_MetaData::MetaDataTypeEnum MetaDataType = MetaDataObject->MetaDataType();
		switch( MetaDataType )
		{
		case Dtk_MetaData::TypeMassProperty:
			{ 
				++iMassPropertiesCount;
			}
		}
	}

	if(!iMassPropertiesCount)
		return dtkNoError;

	if(m_iXmlFormat==XML_FORMAT::EKM_REPORT)
	{
		std::string str = "<table name=\"Mass Properties\" showHeaders=\"true\" showBorder=\"true\" caption=\"\">\n";
		WriteXmlNode_Start(str);
		str = "<column name=\"Item\" id=\"Property\"/>\n";
		WriteXmlNode_Start(str,false);
		str = "<column name=\"Value\" id=\"Value\"/>\n";
		WriteXmlNode_Start(str,false);
	}
	else if(m_iXmlFormat==XML_FORMAT::EKM_META_DATA)
	{
	}
	else if (m_iXmlFormat == XML_FORMAT::EKM_ASM_DEP_EXTRACT)
	{
	}
	else
	{
		WriteXmlNode("massProperties");
	}
	double fDensity = 1000.f;
	char sVal[512];
	std::stringstream ss;
	ss.str("");
	for (int i = 0; i < NumMetaData; i++)
	{
		const Dtk_MetaDataPtr  MetaDataObject = inComponent->GetMetaData(i);
		if(MetaDataObject.IsNULL())
			continue;
		Dtk_MetaData::MetaDataTypeEnum MetaDataType = MetaDataObject->MetaDataType();
		switch( MetaDataType )
		{
		case Dtk_MetaData::TypeProperty:
			{   
				//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_PropertiesPtr( F, MetaDataObject->ToProperty() );
				break;
			}
		case Dtk_MetaData::TypeParameter:
			{   
				//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_PropertiesPtr( F, MetaDataObject->ToParameter() );
				break;
			}
		case Dtk_MetaData::TypeMassProperty:
			{ 
				//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_PropertiesPtr( F, MetaDataObject->ToMassProperty() );
				Dtk_string title = MetaDataObject->GetTitle();
				Dtk_string value = MetaDataObject->GetValue();

				if(title.is_NULL() || value.is_NULL())
					break;

				bool found=false;
				for(int j=0;j<MetaDataTitleStrArray.size();j++)
				{
					if(title == MetaDataTitleStrArray[j])
					{
						found = true;
						break;
					}
				}
				if(found)
					break;

				MetaDataTitleStrArray.push_back(title);

				
				//WriteXmlAttribute(MetaDataObject->GetTitle().c_str() ,MetaDataObject->GetValue().c_str());
				Dtk_string Key = MetaDataObject->GetTitle();
				Dtk_string Value = MetaDataObject->GetValue();
				
				if(Key.is_NULL() || Value.is_NULL())
					break;

				std::string sKey,sValue;
				if(!Convert_To_Xml_Key_Field(Key.c_str(),sKey))
					break;

				if(!Convert_To_Xml_Value_Field(Value.c_str(),sValue))
					break;
						
				if(vcUtils::StrCompare(sKey,std::string("Density")))
				{
					fDensity = atof(sValue.c_str());
				}
				else if(vcUtils::StrCompare(sKey,std::string("Ixx")) ||
				   vcUtils::StrCompare(sKey,std::string("Iyx")) ||
				   vcUtils::StrCompare(sKey,std::string("Izx")) ||
				   vcUtils::StrCompare(sKey,std::string("Ixy")) ||
				   vcUtils::StrCompare(sKey,std::string("Iyy")) ||
				   vcUtils::StrCompare(sKey,std::string("Izy")) ||
				   vcUtils::StrCompare(sKey,std::string("Ixz")) ||
				   vcUtils::StrCompare(sKey,std::string("Iyz")) ||
				   vcUtils::StrCompare(sKey,std::string("Izz")) 
				  )
				{
					double fInertiaVal = atof(sValue.c_str());
					fInertiaVal *= fDensity;
					
					//sprintf(sVal,"%l",fInertiaVal);
					ss.str("");
					ss<<fInertiaVal;
					sValue = ss.str().c_str();;
				}
				else if(vcUtils::StrCompare(sKey,std::string("xCenterOfGravity")) ||
					    vcUtils::StrCompare(sKey,std::string("yCenterOfGravity")) ||
						vcUtils::StrCompare(sKey,std::string("zCenterOfGravity"))
					   )
				{
					double fCenterOfGravityVal = atof(sValue.c_str());
					fCenterOfGravityVal *= 1000.f;
					
					//sprintf(sVal,"%l",fCenterOfGravityVal);
					//sValue = sVal;
					ss.str("");
					ss<<fCenterOfGravityVal;
					sValue = ss.str().c_str();;
				}

				if(m_iXmlFormat==XML_FORMAT::EKM_REPORT)
				{
					this->WriteEkmReport_Row(sKey,sValue);
				}
				else if(m_iXmlFormat==XML_FORMAT::EKM_META_DATA)
				{
					this->WriteEkmXmlAttribute(sKey,sValue);
				}
				else if(m_iXmlFormat!=XML_FORMAT::KEY_VALUE_PAIRS)
				{
					WriteXmlAttribute(sKey,sValue);
					//As Dr.Shastry advised, the weight field is suppressed (7-Dec-2015)
#if 0
					if(vcUtils::StrCompare(sKey,std::string("Mass")))
					{
						float mass = atof(sValue.c_str());
						if(mass>0)
						{
							float weight = mass * GRAVITY;
							char sWeight[512];
							sprintf(sWeight,"%f kg",weight);
							if(m_iXmlFormat!=XML_FORMAT::KEY_VALUE_PAIRS)
								WriteXmlAttribute("Weight",sWeight);
						}
					}
#endif
				}

				break;
			}
		}
	}
	if(m_iXmlFormat==XML_FORMAT::EKM_REPORT)
	{
		std::string str="</table>\n";
		this->WriteXmlNode_End(str);
	}
	else if(m_iXmlFormat==XML_FORMAT::EKM_META_DATA)
	{
	}
	else
	{
		WriteXmlNode();
	}
	return dtkNoError;
}

Dtk_ErrorStatus vcCad2Xml::WriteComponent(Dtk_ComponentPtr inComponent)
{
	if(inComponent.IsNULL())
		return dtkNoError;

	//this->WriteMassProperties(inComponent);

	//vcUtils::LogMsg("vcCad2Xml:WriteComponent():Start");
	Dtk_Size_t i;
	Dtk_ErrorStatus err;
	
	//Datakit component are given in MM, if original model had other unit you can get it with GetConceptionUnitScale()
	// return 25.4 for inch 
	double UnitFactor = inComponent->GetConceptionUnitScale(); 

	//GetName
	Dtk_string ComponentName;
	if(inComponent->Name().is_not_NULL())
		ComponentName = inComponent->Name();

	//You have 4 types for Component
	Dtk_Component::ComponentTypeEnum type = inComponent->ComponentType();
	switch(type)
	{
		//Instance represent a prototype with a matrix placement
		case Dtk_Component::InstanceComponentType :
		{
			Dtk_ComponentPtr prototype = inComponent->GetChild(0) ;
			WriteComponent(prototype);
			break;
		}
		//Prototype (you have to check if you ever read and write it to don't waste time)
		//You can use methods SetProcessed() and HasBeenProcessed() to do this
		case Dtk_Component::PrototypeComponentType :
		{
			//if(inComponent->HasBeenProcessed() == DTK_FALSE)
			{
				Dtk_NodePtr RootNode;
				Dtk_API *inAPI = Dtk_API::GetAPI();

				/*if(inComponent->FullPathName().is_not_NULL())
				{
					Dtk_string sPartPath = inComponent->FullPathName();
					if(sPartPath.len())
					{
						if (std::find(m_FilePathStrArray.begin(), m_FilePathStrArray.end(), vcUtils::GetFileNameWithExtn(sPartPath.c_str()).c_str()) != m_FilePathStrArray.end())
						{
							//break;
						}
					}
				}*/

				//To GET THE ABSOLUTE NAME OF ASSEMBLY
				/*Dtk_ComponentPtr inObject;
				Dtk_Val myVal;
				inComponent->GetInfos()->FindAttribute(L"V5AbsoluteName", myVal);
				Dtk_string str_main_assembly = myVal.GetString();

				Dtk_InfoPtr info = inComponent->GetInfos();
				Dtk_tab<Dtk_string> lst;
				info->ListAllAttributes(lst);
				for (int i = 0; i < lst.size(); i++)
				{
					Dtk_string st = lst[i];
					printf("\n%s\n", st.c_str());
				}*/


				if(m_RootComponent.IsNotNULL() && m_RootComponent->IsAssembly())
				if(m_iXmlFormat!=XML_FORMAT::KEY_VALUE_PAIRS && m_iXmlFormat!=XML_FORMAT::EKM_META_DATA && m_iXmlFormat!=XML_FORMAT::EKM_REPORT && m_iXmlFormat != XML_FORMAT::EKM_ASM_DEP_EXTRACT)
					WriteXmlNode(ComponentName.c_str());

				//it can also contain some instances
				Dtk_Size_t NumChildren = inComponent->GetNumChildren();
				for( i = 0; i < NumChildren; i++)
				{
					 Dtk_ComponentPtr child = inComponent->GetChild(i) ;
					 WriteComponent( child);
				}


				//Get the Construction tree for this prototype
				try
				{
					if(inComponent->FullPathName().is_NULL())
						return dtkNoError;
					std::string ppath = inComponent->FullPathName().c_str();
					err = inAPI->ReadComponent( inComponent, RootNode );
					/*if (err == dtkErrorComponentAlreadyLoaded)
						return dtkNoError;*/

					bool bContinue = true;
					if (m_iXmlFormat != XML_FORMAT::EKM_ASM_DEP_EXTRACT)
					{
						if (err == dtkErrorComponentAlreadyLoaded || (err == dtkNoError && RootNode.IsNotNULL()))
						{
						}
						else
							bContinue = false;
					}

					if(bContinue)
					{
						if(inComponent->FullPathName().is_not_NULL())// && !inComponent->IsAssembly())
						{
							if (m_iXmlFormat == XML_FORMAT::EKM_ASM_DEP_EXTRACT)// && !inComponent->IsAssembly())
							{
								m_iIndent = 1;
								

								std::string input_file = m_sInputFileName.c_str();
								std::replace(input_file.begin(), input_file.end(), '\\', '/');
								std::string asm_path = vcUtils::GetPathFromFileName(input_file);

								boost::filesystem::path ap(input_file);
								boost::filesystem::path dir = ap.parent_path();
								std::string asm_file_name = ap.filename().string();

								Dtk_string comp_name = inComponent->FullPathName().c_str();
								std::string part_path = inComponent->FullPathName().c_str();
								std::replace(part_path.begin(), part_path.end(), '\\', '/');
								
								boost::filesystem::path cp(part_path);
								/*if (m_iXmlFormat != XML_FORMAT::EKM_ASM_DEP_EXTRACT)
								{
									if (!boost::filesystem::exists(part_path))
										break;
								}*/

								std::string cmp_path = cp.parent_path().string();
								std::string comp_file_name = cp.filename().string();

								//if (asm_file_name == comp_file_name)
								{
									/*std::string dep_str = "<dependency>\n";
									WriteXmlNode_Start(dep_str);

									dep_str = "</dependency>\n";
									WriteXmlNode_End(dep_str);*/
								}
								//else
								{
									std::vector<Dtk_string>::iterator name_it;
									name_it = std::find(m_ComponentNameStrArray.begin(), m_ComponentNameStrArray.end(), comp_name);
									if (name_it == m_ComponentNameStrArray.end())
									{
										m_ComponentNameStrArray.push_back(comp_name);

										std::string current_dir = "./";
										//if (!dir.empty() && dir.string().length() > 2)
										{
#if 0
											part_path = part_path.substr(dir.string().length(), part_path.length());
											current_dir = ".";
#else
											/*std::string input_path = asm_path;
											std::string comp_path = cmp_path;
											RemoveCommonStrFromPath(asm_path, comp_path);
											int cnt = std::count(asm_path.begin(), asm_path.end(), '/');
											//std::string tmp;
											if (asm_path.empty())
												current_dir = ".";
											else
											{
												for (int i = 0; i < cnt + 1; i++)
													current_dir += "../";
											}

											part_path =  comp_path + "/"+comp_file_name;*/


#endif
										}



										if (asm_file_name != comp_file_name)
										{
											std::string tag = "relativePath";

											std::string dep_str = "<dependency>\n";
											WriteXmlNode_Start(dep_str);

											//part_path = std::string("\t{ ")+ current_dir + comp_file_name + std::string(" }\t");
											//WriteXmlNode_StartEnd(tag, part_path);
											WriteXmlNode_StartEnd(tag, comp_file_name);

											dep_str = "</dependency>\n";
											WriteXmlNode_End(dep_str);
										}
									}
								}
								
							}
#if 0
							if(m_RootComponent.IsNotNULL() && m_RootComponent->IsAssembly())
							{
								if(m_iXmlFormat!=XML_FORMAT::EKM_META_DATA && m_iXmlFormat!=XML_FORMAT::EKM_REPORT && m_iXmlFormat != XML_FORMAT::EKM_ASM_DEP_EXTRACT)
									WriteXmlNode("partInfo");
								if(inComponent->FullPathName().is_not_NULL())
								{
									Dtk_string sPartPath = ComponentName;//inComponent->FullPathName();
									if(sPartPath.len())
									{
										if (std::find(m_FilePathStrArray.begin(), m_FilePathStrArray.end(), sPartPath.c_str()) != m_FilePathStrArray.end())
										{
											//break;
										}
									}
									std::string sName = vcUtils::GetFileNameWithExtn(sPartPath.c_str());

									//m_FilePathStrArray.push_back(vcUtils::GetFileNameWithExtn(sPartPath.c_str()).c_str());
									m_FilePathStrArray.push_back(ComponentName.c_str());
									if(m_iXmlFormat!=XML_FORMAT::EKM_META_DATA && m_iXmlFormat!=XML_FORMAT::EKM_REPORT && m_iXmlFormat != XML_FORMAT::EKM_ASM_DEP_EXTRACT)
										WriteXmlAttribute("part",vcUtils::GetFileNameWithExtn(sPartPath.c_str()));
								}
								if(inComponent->GetFileVersion().is_not_NULL())
								{
									if(m_iXmlFormat!=XML_FORMAT::EKM_META_DATA && m_iXmlFormat!=XML_FORMAT::EKM_REPORT && m_iXmlFormat != XML_FORMAT::EKM_ASM_DEP_EXTRACT)
									{
										WriteXmlAttribute("cadPackageName",m_MetaData.m_sNativeCadPackageName.c_str());
										Dtk_string sVer = inComponent->GetFileVersion();
										WriteXmlAttribute("cadPackageVersion",sVer.c_str());
									}
								}
								else
								{
									if(m_iXmlFormat!=XML_FORMAT::EKM_META_DATA && m_iXmlFormat!=XML_FORMAT::EKM_REPORT && m_iXmlFormat != XML_FORMAT::EKM_ASM_DEP_EXTRACT)
									{
										Dtk_MainDocPtr Doc;
										m_pDtkAPI->OpenDocument(inComponent->FullPathName().c_str(), Doc);
										Dtk_string sVer = Doc->RootComponent()->GetFileVersion();
										if(sVer.is_not_NULL())
											WriteXmlAttribute("PartVersion",sVer.c_str());
										Doc=NULL;
									}
								}
								if(m_iXmlFormat!=XML_FORMAT::EKM_META_DATA && m_iXmlFormat!=XML_FORMAT::EKM_REPORT && m_iXmlFormat != XML_FORMAT::EKM_ASM_DEP_EXTRACT)
									WriteXmlNode();
							}//End - if(m_RootComponent.IsNotNULL() && m_RootComponent->IsAssembly())
#endif
						}
						else 
						{
							if(inComponent->Name().is_not_NULL())
							{
								ss.str("");
								ss<<"vcCad2Xml::WriteComponent():PrototypeComponentType():ReadComponent->ComponentName: "<<inComponent->Name().c_str();
 								vcUtils::LogMsg(ss.str());
							}
						}

						Dtk_string name; 
						if(inComponent->Name().is_not_NULL())
							name = inComponent->Name();//Component name

						bool found = false;
						for(int i=0;i<m_MetaData.m_PartNamesStrArray.size();i++)
						{
							if(strcmp(name.c_str(),m_MetaData.m_PartNamesStrArray[i].c_str())==0)
							{
								found=true;
								break;
							}
						}
						if(!found)
							m_MetaData.m_PartNamesStrArray.push_back(name.c_str());

						m_bBodyTypeGeometry = false;
						WriteNode(RootNode); 

						if(m_bBodyTypeGeometry && m_RootComponent.IsNotNULL() && m_RootComponent->IsAssembly())
						{
							if(m_iXmlFormat!=XML_FORMAT::EKM_META_DATA && m_iXmlFormat!=XML_FORMAT::EKM_REPORT && m_iXmlFormat != XML_FORMAT::EKM_ASM_DEP_EXTRACT)
								WriteXmlNode("partInfo");
							if(inComponent->FullPathName().is_not_NULL())
							{
								Dtk_string sPartPath = ComponentName;//inComponent->FullPathName();
								/*if(sPartPath.len())
								{
									if (std::find(m_FilePathStrArray.begin(), m_FilePathStrArray.end(), sPartPath.c_str()) != m_FilePathStrArray.end())
									{
										//break;
									}
								}*/
								std::string sName = vcUtils::GetFileNameWithExtn(sPartPath.c_str());

								//m_FilePathStrArray.push_back(vcUtils::GetFileNameWithExtn(sPartPath.c_str()).c_str());
								m_FilePathStrArray.push_back(ComponentName.c_str());
								if(m_iXmlFormat!=XML_FORMAT::EKM_META_DATA && m_iXmlFormat!=XML_FORMAT::EKM_REPORT && m_iXmlFormat != XML_FORMAT::EKM_ASM_DEP_EXTRACT)
									WriteXmlAttribute("part",vcUtils::GetFileNameWithExtn(sPartPath.c_str()));
							}
							if(inComponent->GetFileVersion().is_not_NULL())
							{
								if(m_iXmlFormat!=XML_FORMAT::EKM_META_DATA && m_iXmlFormat!=XML_FORMAT::EKM_REPORT && m_iXmlFormat != XML_FORMAT::EKM_ASM_DEP_EXTRACT)
								{
									WriteXmlAttribute("cadPackageName",m_MetaData.m_sNativeCadPackageName.c_str());
									Dtk_string sVer = inComponent->GetFileVersion();
									WriteXmlAttribute("cadPackageVersion",sVer.c_str());
								}
							}
							else
							{
								if(m_iXmlFormat!=XML_FORMAT::EKM_META_DATA && m_iXmlFormat!=XML_FORMAT::EKM_REPORT && m_iXmlFormat != XML_FORMAT::EKM_ASM_DEP_EXTRACT)
								{
									Dtk_MainDocPtr Doc;
									m_pDtkAPI->OpenDocument(inComponent->FullPathName().c_str(), Doc);
									if(Doc.IsNotNULL())
									{
										Dtk_string sVer = Doc->RootComponent()->GetFileVersion();
										if(sVer.is_not_NULL())
											WriteXmlAttribute("cadPackageVersion",sVer.c_str());
									}
									Doc=NULL;
								}
							}
							if(m_iXmlFormat!=XML_FORMAT::EKM_META_DATA && m_iXmlFormat!=XML_FORMAT::EKM_REPORT && m_iXmlFormat != XML_FORMAT::EKM_ASM_DEP_EXTRACT)
								WriteXmlNode();
						}//End - if(m_RootComponent.IsNotNULL() && m_RootComponent->IsAssembly())


						if(inComponent.IsNotNULL() && !inComponent->IsAssembly() && !m_RootComponent->IsAssembly())
						{
							WriteMassProperties(inComponent);
							WriteMaterial(m_BodyLevelMaterial);
						}//End - if(inComponent.IsNotNULL() && !inComponent->IsAssembly())
						m_bBodyTypeGeometry = false;
					}//End - if(bContinue)
					if(err != dtkNoError && err != dtkErrorComponentAlreadyLoaded)
					{
						ss.str("");
						ss<<"vcCad2Xml::WriteComponent():PrototypeComponentType():ReadComponent->Path: "<<inComponent->FullPathName().c_str();
 						vcUtils::LogMsg(ss.str());

						ss.str("");
						ss<<"vcCad2Xml::WriteComponent():PrototypeComponentType():ReadComponent->Error Status: "<<dtkTypeError(err);
	 					vcUtils::LogMsg(ss.str());
					}
				}
				catch(...)
				{
					if(inComponent->Name().is_not_NULL())
					{
						ss.str("");
						ss<<"Exception : vcCad2Xml::WriteComponent():PrototypeComponentType():ReadComponent->ComponentName: "<<inComponent->Name().c_str() <<" :Not able to read";
						vcUtils::LogMsg(ss.str());
					}
					else
					{
						vcUtils::LogMsg("Exception: vcCad2Xml::WriteComponent():PrototypeComponentType():ReadComponent");
					}
				}

				//it can also contain some instances
				/*Dtk_Size_t NumChildren = inComponent->GetNumChildren();
				for( i = 0; i < NumChildren; i++)
				{
					 Dtk_ComponentPtr child = inComponent->GetChild(i) ;
					 WriteComponent( child);
				}*/
				//??inComponent->SetProcessed();
				//We close the opened Component and free his construction tree
				err = inAPI->EndComponent(inComponent);
				
				if(m_RootComponent.IsNotNULL() && m_RootComponent->IsAssembly())
				if(m_iXmlFormat!=KEY_VALUE_PAIRS && m_iXmlFormat!=XML_FORMAT::EKM_META_DATA && m_iXmlFormat!=XML_FORMAT::EKM_REPORT && m_iXmlFormat != XML_FORMAT::EKM_ASM_DEP_EXTRACT)
					WriteXmlNode();


			}
			//else
			{
				//Get the prototype you ever write
				//The Component has a unique ID given by GetID to help to map it with your Write ID
			}
			break;
		}
		//Catalog Component represent a choice of several possible configuration 
		//(like scene in catiav5, workspace in catiav4, configuration in solidworks)
		//Default is the first child 
		case Dtk_Component::CatalogComponentType :
		{
			Dtk_string name;
			//name = inComponent->Name();//Component name
			
			//ss.str("");
			//ss<<"vcCad2Xml::WriteComponent():CatalogComponentType NodeName: "<<ComponentName;
			//vcUtils::LogMsg(ss.str());

			Dtk_ComponentPtr defaultchoice = inComponent->GetChild(0) ;
			if (defaultchoice.IsNotNULL())
			{
				name = defaultchoice->Name();
				//if(m_iXmlFormat!=XML_FORMAT::KEY_VALUE_PAIRS)
					//WriteXmlNode(name.c_str());

				WriteComponent(defaultchoice);

				//if(m_iXmlFormat!=XML_FORMAT::KEY_VALUE_PAIRS)
					//WriteXmlNode();
			}
			//if you don't want to use default you have to scan all children and choose the one you want to convert (see their name)
			break;
		}
		//Component containing only children 
		case Dtk_Component::VirtualComponentType :
		{
			Dtk_string name;
			Dtk_Size_t NumChildren;

			//name = inComponent->Name();//Component name
			NumChildren = inComponent->GetNumChildren();
			if(NumChildren)
			{
				//ss.str("");
				//ss<<"vcCad2Xml::WriteComponent():VirtualComponentType:NodeName: "<<ComponentName;
				//vcUtils::LogMsg(ss.str());
				//if(m_iXmlFormat!=XML_FORMAT::KEY_VALUE_PAIRS)
					//WriteXmlNode(name.c_str());

				for( i = 0; i < NumChildren; i++)
				{
					Dtk_ComponentPtr child = inComponent->GetChild(i) ;
					WriteComponent( child);
				}
				//if(m_iXmlFormat!=XML_FORMAT::KEY_VALUE_PAIRS)
					//WriteXmlNode();//End tag for WriteXmlNode(name.c_str());
			}
			break;
		}
	}
	//vcUtils::LogMsg("vcCad2Xml:WriteComponent():End");
	return dtkNoError;
}

Dtk_ErrorStatus vcCad2Xml::WriteNode(Dtk_NodePtr inNode)
{
	if(inNode.IsNULL())
		return dtkNoError;
	//vcUtils::LogMsg("vcCad2Xml:WriteNode():Start");
	Dtk_API *MyAPI = Dtk_API::GetAPI();

	//Get The node Blanked Status
	// -1 = undefined, 0 = Visible, 1=Invisible, 2=Construction Geometry
	int NodeBlankedStatus = inNode->GetInfos()->GetBlankedStatus();

	//GetName
	Dtk_string NodeName;
	if(inNode->Name().is_not_NULL())
	{
		NodeName = inNode->Name();
	 
		if(NodeName.find_substring("Geometrical Set")!= -1 || NodeName.find_substring("WireFrame")!= -1)
			return dtkNoError;
	}



	//In this sample we read and treat only visible node
	if( NodeBlankedStatus == 0)
	{
	//vcUtils::LogMsg("NodeBlankedStatus == 0");
		//You have 9 types for Node Data
		//BodyType  for 3D geometry (solid and wireframe)
		//MeshType  for 3D Tesselated geometry
		//AnnotationSetType  for FDT
		//DrawingType  for 2D
		//KinematicsType for Kinematics
		//AxisSystemType for AxisPlacement
		//LayerInfosSetType for LayerInfos
		//MetaDataType for Additional informations
		//VirtualType just for containing children
		Dtk_Node::NodeDataTypeEnum NodeType = inNode->GetNodeType();

		if(inNode->GetMaterial().IsNotNULL())
			m_BodyLevelMaterial = inNode->GetMaterial();

		switch(NodeType)
		{
			case Dtk_Node::BodyType:  
			{
				break; //Tessellation is not required in XML generation. Its ignored now.28-Nov-2018
				// Get if the node represent infinite geometry
				// 1 = Infinite, 0 = Finite
				int NodeInfiniteGeometry = inNode->GetInfos()->GetInfiniteGeometryFlag();
				if (NodeInfiniteGeometry == 0)
				{
					//Calling both methods GetDtk_MeshPtr() and GetDtk_BodyPtr() on BodyNode will give you the same result
					//Choose the one you need in order to avoid duplicated data
					const Dtk_BodyPtr TmpBody = inNode->GetDtk_BodyPtr();
					if (TmpBody.IsNotNULL())
					{
					}
					// Some CAD formats store also faceted data besides B-Rep. 
					// So you can get faceted data corresponding to the body using the following method
					const Dtk_MeshPtr TmpFacetedBody = inNode->GetDtk_MeshPtr();
					if (TmpFacetedBody.IsNotNULL())
					{
						WriteDtk_Mesh(TmpFacetedBody,NodeName);
						m_bBodyTypeGeometry = true;
					}
					#ifdef ACTIVATE_TESSELATION_LIB
					// If there is no mesh data associated to the current body, you can tessellate 
					if(TmpFacetedBody.IsNULL() && TmpBody.IsNotNULL())
					{
						Dtk_tab<Dtk_MeshPtr> meshes;
						Dtk_tab<Dtk_Int32> isclosed;
						int err_tess = tess_BodyToMeshes(TmpBody, meshes,isclosed);
						if(err_tess == 0)
						{
							ss.str("");
							ss<<"vcCad2Xml::WriteNode():BodyType NodeName: "<<NodeName;
							vcUtils::LogMsg(ss.str());

							Dtk_Size_t i,nbmeshes = meshes.size();

							for (i=0;i<nbmeshes;i++)
							{
								WriteDtk_Mesh(meshes[i],NodeName);
							}    
							if(nbmeshes>0)
								m_bBodyTypeGeometry = true;
						}
					}
					#endif
				}
				break;
			}
			case Dtk_Node::AnnotationSetType:
			{
				Dtk_FdtAnnotationSetPtr TmpFdtAnnotSet = inNode->GetDtk_FdtAnnotationSetPtr();
				if (TmpFdtAnnotSet.IsNotNULL())
				{
					/*if (xmlDumpFile)
					{
						Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_FdtAnnotationSetPtr(xmlDumpFile,TmpFdtAnnotSet);
					}*/                        
				}
				break;
			}
			case Dtk_Node::DrawingType:
			{
				Dtk_DrawingPtr TmpDrawing = inNode->GetDtk_DrawingPtr();
				if (TmpDrawing.IsNotNULL())
				{
					std::stringstream ss;

					/*type_detk  type = TmpDrawing->get_type_detk() ; 
					ss.str("");
					ss<<"vcDataKit::WriteNode() : Drawing->type : "<<type;
					vcUtils::LogMsg(ss.str());

					ss.str("");
					ss<<"vcDataKit::WriteNode() : Drawing->Num2dEntities : "<<TmpDrawing->GetNum2dEntities();
					vcUtils::LogMsg(ss.str());

					ss.str("");
					ss<<"vcDataKit::WriteNode() : Drawing-> NumOrigins : "<<TmpDrawing->GetNumOrigins();
					vcUtils::LogMsg(ss.str());

					ss.str("");
					ss<<"vcDataKit::WriteNode() : Drawing-> NumViews  : "<<TmpDrawing->GetNumViews ();
					vcUtils::LogMsg(ss.str());*/

					/*if (xmlDumpFile)
					{
						Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_DrawingPtr(xmlDumpFile,TmpDrawing,0);
					}*/                        
				}
				break;
			}
			case Dtk_Node::MeshType:
			{
				Dtk_MeshPtr TmpMesh = inNode->GetDtk_MeshPtr();
				if (TmpMesh.IsNotNULL())
				{
					ss.str("");
					ss<<"vcCad2Xml::WriteNode():MeshType NodeName: "<<NodeName;
					vcUtils::LogMsg(ss.str());
					WriteDtk_Mesh(TmpMesh,NodeName);
					m_bBodyTypeGeometry = true;
				}
				break;
			}
			case Dtk_Node::AxisSystemType:
			{
				Dtk_AxisSystemPtr TmpAxis = inNode->GetDtk_AxisSystemPtr();
				//if (TmpAxis.IsNotNULL() && xmlDumpFile)
				{
					//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_transfo(xmlDumpFile,TmpAxis->GetMatrix());
				}
				break;
			}
			case Dtk_Node::KinematicsType:
			{              
				Dtk_KinematicPtr TmpKinematics = inNode->GetDtk_KinematicPtr();  
				if (TmpKinematics.IsNotNULL())
				{    
					/*if (xmlDumpFile)
					{
						TmpKinematics->Dump(xmlDumpFile);
					}*/
				}                   
				break;
			}
			case Dtk_Node::LayerInfosSetType:
			{
				Dtk_LayerInfosSetPtr TmpLayerInfosSet = inNode->GetDtk_LayerInfosSetPtr();
				if (TmpLayerInfosSet.IsNotNULL())
				{
					/*Dtk_Size_t NumLayers = TmpLayerInfosSet->GetNumLayers();
					Dtk_Size_t NumLayerFilters = TmpLayerInfosSet->GetNumLayerFilters();
					Dtk_Size_t DefaultLayer, DefaultLayerFilter;
					TmpLayerInfosSet->GetDefaultLayer(DefaultLayer);
					TmpLayerInfosSet->GetDefaultLayerFilter(DefaultLayerFilter);*/


					/*std::stringstream ss;

					ss.str("");
					ss<<"vcDataKit::WriteNode() : NumLayers : "<<NumLayers;
					vcUtils::LogMsg(ss.str());

					ss.str("");
					ss<<"vcDataKit::WriteNode() : NumLayerFilters : "<<NumLayerFilters;
					vcUtils::LogMsg(ss.str());

					ss.str("");
					ss<<"vcDataKit::WriteNode() : DefaultLayer : "<<DefaultLayer;
					vcUtils::LogMsg(ss.str());

					ss.str("");
					ss<<"vcDataKit::WriteNode() : DefaultLayerFilter : "<<DefaultLayerFilter;
					vcUtils::LogMsg(ss.str());


					Dtk_Size_t i;
					for (i = 0; i < NumLayers; i++)
					{
						Dtk_string LayerName;
						TmpLayerInfosSet->GetLayerName(i, LayerName);

						ss.str("");
						ss<<"vcDataKit::WriteNode() : Layer "<<i+1<<" "<<LayerName;
						vcUtils::LogMsg(ss.str());
					}
					for (i = 0; i < NumLayerFilters; i++)
					{
						Dtk_LayerFilterInfosPtr TmpLayerFilter = TmpLayerInfosSet->GetLayerFilterByPos(i);
						Dtk_tab< Dtk_Size_t > SelectedLayers;
						TmpLayerFilter->GetSelectedLayers( SelectedLayers );
						Dtk_string LayerFilterName;
						TmpLayerFilter->GetName(LayerFilterName);

						ss.str("");
						ss<<"vcDataKit::WriteNode() : LayerFilterName "<<i+1<<" "<<LayerFilterName;
						vcUtils::LogMsg(ss.str());
					}*/
				}
				break;
			}
			case Dtk_Node::MetaDataType:
			{
				Dtk_MetaDataPtr TmpMetaData = inNode->GetDtk_MetaDataPtr();
				if (TmpMetaData.IsNotNULL())
				{
					/*if (xmlDumpFile)
					{
						Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_MetadataPtr(xmlDumpFile, TmpMetaData);
					} */                                      
					//Dtk_string sTitle = TmpMetaData->GetTitle(); 
				
					/*std::stringstream ss;

					if(sTitle.len())
					{
					ss.str("");
					ss<<"vcDataKit::WriteNode() : MetaData Title : "<<sTitle;
					vcUtils::LogMsg(ss.str());
					}

					Dtk_string sType = TmpMetaData->GetType();
					if(sType.len())
					{
					ss.str("");
					ss<<"vcDataKit::WriteNode() : MetaData sType : "<<sType;
					vcUtils::LogMsg(ss.str());
					}

					Dtk_string sValue = TmpMetaData->GetValue();
					if(sValue.len())
					{
					ss.str("");
					ss<<"vcDataKit::WriteNode() : MetaData sValue : "<<sValue;
					vcUtils::LogMsg(ss.str());
					}*/

				}
				break;
			}
			case Dtk_Node::VirtualType:
			{
				if(inNode->GetNumChildren())
				{
					//ss.str("");
					//ss<<"vcCad2Xml::WriteNode():VirtualType NodeName: "<<NodeName;
					//vcUtils::LogMsg(ss.str());

					//WriteXmlNode(NodeName.c_str());

					Dtk_Size_t i, NumChildren;
					NumChildren = inNode->GetNumChildren();
					for (i = 0; i < NumChildren; i++)
					{
						WriteNode(inNode->GetChild(i));
					}    

					//WriteXmlNode();//End tag for WriteXmlNode(NodeName.c_str());
				}
				break;
			}
			default:
			{
				break;
			}
		} 
	}

	//Treat recursive TreeNode for Visible and Construction node
	//if(m_ReaderType == DtkReaderType::UgReaderModule || m_ReaderType == DtkReaderType::SwReaderModule || m_ReaderType == DtkReaderType::SeReaderModule)
	if (NodeBlankedStatus != 1)
	{
		Dtk_Size_t i, NumChildren;
		NumChildren = inNode->GetNumChildren();
		for (i = 0; i < NumChildren; i++)
		{
			WriteNode(inNode->GetChild(i));
		}    
	}

	// Get the Feature associated to the current node, if exists,
	// otherwise a NULL pointer is returned
	Dtk_FeaturePtr TmpFeature = inNode->GetDtk_FeaturePtr ();
	if (TmpFeature.IsNotNULL())
	{    
		/*FILE* FeaturesDumpFile = Dtk_DumpXMLNamespace::Dtk_DumpXml_GetFeaturesDumpFile();
		if (FeaturesDumpFile)
		{          
			Dtk_Dump_Dtk_Feat (FeaturesDumpFile, TmpFeature);
		}*/
	}

	return dtkNoError;
}

void vcCad2Xml::WriteDtk_Mesh(const Dtk_MeshPtr& inMeshToWrite,Dtk_string NodeName)
{
	//if(NodeName.is_not_NULL() && NodeName.len())
	{
		//commented following 2 lines on 18-jul-2011 as they seem to be not required
		//WriteXmlNode(NodeName.c_str());
		//WriteXmlNode();//End tag for WriteXmlNode(NodeName.c_str());

		/*Dtk_pnt min,max;
		inMeshToWrite->compute_bb(&min,&max);
		ss.str("");
		ss<<min[0]<<" "<<min[1]<<" "<<min[2];
		WriteXmlAttribute("BoundingBox_Min" ,ss.str());
		ss.str("");
		ss<<max[0]<<" "<<max[1]<<" "<<max[2];
		WriteXmlAttribute("BoundingBox_Max" ,ss.str());*/

		/*bool found = false;
		for(int i=0;i<m_MetaData.m_PartNamesStrArray.size();i++)
		{
			if(strcmp(NodeName.c_str(),m_MetaData.m_PartNamesStrArray[i].c_str())==0)
			{
				found=true;
				break;
			}
		}
		if(!found)
			m_MetaData.m_PartNamesStrArray.push_back(NodeName.c_str());*/
	}
}

std::string vcCad2Xml::GetCadPackageName(DtkReaderType readerType)
{
	std::string sCadPackageName;
	switch(readerType)
	{
	case DtkReaderType::V6ReaderModule:// = 0,
		sCadPackageName = "Catia V6";
			break;
	case DtkReaderType::V5ReaderModule:// = 0,
		sCadPackageName = "Catia V5";
			break;
	case DtkReaderType::VdaReaderModule:// = 1,
		sCadPackageName = "VDA";
			break;
	case DtkReaderType::InvReaderModule: //= 2,
		sCadPackageName = "Inventor";
			break;
	case DtkReaderType::V4ReaderModule: //= 3,
		sCadPackageName = "Catia V4";
			break;
	case DtkReaderType::UgReaderModule: //= 4,
		sCadPackageName = "Unigraphics";
			break;
	case DtkReaderType::XmtReaderModule: //= 5,
		sCadPackageName = "Xmt";
			break;
	case DtkReaderType::SwReaderModule: //= 6,
		sCadPackageName = "Solidworks";
			break;
	case DtkReaderType::SeReaderModule: //= 7,
		sCadPackageName = "Solid Edge";
			break;
	case DtkReaderType::IgesReaderModule: //= 8,
		sCadPackageName = "Iges";
			break;
	case DtkReaderType::StepReaderModule: //= 9,
		sCadPackageName = "Step";
			break;
	case DtkReaderType::PsReaderModule: //= 10,
		sCadPackageName = "Parasolid";
			break;
	case DtkReaderType::ProeReaderModule: //= 11,
		sCadPackageName = "ProE";
			break;
	case DtkReaderType::SatReaderModule: //= 12,
		sCadPackageName = "Sat";
			break;
	case DtkReaderType::JtReaderModule: //= 13,
		sCadPackageName = "JT";
			break;
	case DtkReaderType::CgrReaderModule: //= 14,
		sCadPackageName = "CGR";
			break;
	case DtkReaderType::CaddsReaderModule: //= 15,
		sCadPackageName = "Cadds";
			break;
	case DtkReaderType::AcisReaderModule: //= 16,
		sCadPackageName = "Acis";
			break;
	case DtkReaderType::ProCeraReaderModule: //= 17,
		sCadPackageName = "ProCera";
			break;
	case DtkReaderType::RevitReaderModule: //= 17,
		sCadPackageName = "Revit";
		break;
	case DtkReaderType::IfcReaderModule: //= 17,
		sCadPackageName = "Ifc";
		break;
	case DtkReaderType::StlReaderModule: //= 17,
		sCadPackageName = "Stl";
		break;
	default://MaxReaderModules,UnknownModule
		sCadPackageName = "Unknown";
			break;
	}
return sCadPackageName;
}

std::string vcCad2Xml::GetUnitsName(Dtk_Double64 unit)
{
	std::string sUnitsName="unknown";
	if(unit == 1.0f)
	{
		sUnitsName = "millimeter";
	}
	else if(unit>25.f && unit<26.f)
	{
		sUnitsName = "inch";
	} 
	else if(unit>304.f && unit<305.f)
	{
		sUnitsName = "feet";
	}
	else if(unit == 1000.f)
	{
		sUnitsName = "meter";
	}
	else if(unit == 10.f)
	{
		sUnitsName = "centimeter";
	}
	else
	{
		sUnitsName = "unknown";
	}
	return sUnitsName;
}

bool vcCad2Xml::OpenFile(const char *sOutputFileName)
{
   std::ios_base::openmode mode = std::ios_base::out;
	bool is_open = outfile.is_open();
    if(true == is_open)
         return false;

	outfile.open(sOutputFileName, mode);

    is_open = outfile.is_open();
    if(false == is_open)
		return false;

	return true;
}

void vcCad2Xml::WriteXmlNode(std::string sName)
{
	std::stringstream ss;
	std::string tag;
	for(int i=0;i<m_iIndent;i++)
		ss<<"\t";
	ss<<"<node name=\""<<EncodeString(sName)<<"\">\n";
	tag = ss.str();

	WriteTag(tag);

	m_iIndent++;
}
void vcCad2Xml::WriteXmlNode()
{
	m_iIndent--;
	std::stringstream ss;
	std::string tag;
	for(int i=0;i<m_iIndent;i++)
		ss<<"\t";
	ss<<"</node>\n";
	tag = ss.str();

	WriteTag(tag);
}
void vcCad2Xml::WriteXmlAttribute(std::string _sKey,std::string _sValue)
{
	std::string sKey,sValue;
	if(!Convert_To_Xml_Key_Field(_sKey.c_str(),sKey))
		return;

	if(!Convert_To_Xml_Value_Field(_sValue.c_str(),sValue))
		return;

	std::stringstream ss;
	std::string tag;
	for(int i=0;i<m_iIndent;i++)
		ss<<"\t";
	ss<<"<attribute key=\""<<sKey<<"\" value=\""<<EncodeString(sValue)<<"\"/>\n";
	tag =ss.str();

	WriteTag(tag);
}
void vcCad2Xml::WriteTag(std::string tag)
{
	if(!g_bOutputXml)
	{
		std::cout<<tag.c_str();
	}
	else
	{
		outfile.write(tag.c_str(),tag.length());
	}
}

void vcCad2Xml::WriteEkmXmlAttribute(std::string _sKey,std::string _sValue)
{
	std::string sKey,sValue;
	if(!Convert_To_EKM_Xml_Key_Field(_sKey.c_str(),sKey))
		return;

	if(!Convert_To_EKM_Xml_Value_Field(_sValue.c_str(),sValue))
		return;

	std::stringstream ss;
	std::string tag;
	for(int i=0;i<m_iIndent;i++)
		ss<<"\t";
	ss<<"\t<data name=\""<<sKey<<"\" value=\""<<EncodeString(sValue)<<"\"/>\n";
	tag =ss.str();

	WriteTag(tag);
}

void vcCad2Xml::WriteKeyValueXmlAttribute(std::string _sKey,std::string _sValue)
{
	std::string sKey,sValue;
	if(!Convert_To_Xml_Key_Field(_sKey.c_str(),sKey))
		return;

	if(!Convert_To_Xml_Value_Field(_sValue.c_str(),sValue))
		return;

	std::stringstream ss;
	std::string tag;
	for(int i=0;i<m_iIndent;i++)
		ss<<"\t";
	ss<<"    <attribute name=\""<<sKey<<"\" value=\""<<EncodeString(sValue)<<"\"/>\n";
	tag =ss.str();

	WriteTag(tag);
}

bool vcCad2Xml::WriteMaterial(Dtk_MaterialPtr Material)
{
	//Material Properties is ignored for following formats EKM MetaData and Report
	if(m_iXmlFormat==XML_FORMAT::EKM_META_DATA || m_iXmlFormat==XML_FORMAT::EKM_REPORT || m_iXmlFormat == XML_FORMAT::EKM_ASM_DEP_EXTRACT)
	{
		if(m_ReaderType == DtkReaderType::V6ReaderModule ||
		   m_ReaderType == DtkReaderType::CgrReaderModule ||
		   m_ReaderType == DtkReaderType::StepReaderModule ||
		   m_ReaderType == DtkReaderType::IgesReaderModule ||
		   m_ReaderType == DtkReaderType::V4ReaderModule
		  )
			return dtkNoError;
	}

	Dtk_ErrorStatus err;
	err = dtkNoError;
	if(m_RootComponent.IsNotNULL() && !m_RootComponent->IsAssembly())
	{
		Dtk_NodePtr RootNode;
		err = m_pDtkAPI->ReadComponent( m_RootComponent, RootNode );  
		if(m_RootComponent->GetMaterial().IsNotNULL())
			Material = m_RootComponent->GetMaterial();
	}

	if(Material.IsNULL())
		return false;

	std::stringstream ss;

	if(Material->composites && Material->composites->value.size() || 
       Material->analysis && Material->analysis->value.size() ||
	   Material->label.len()
	   )
	{
		if(m_iXmlFormat==XML_FORMAT::EKM_REPORT)
		{
			std::string str = "<table name=\"Material\" showHeaders=\"true\" showBorder=\"true\" caption=\"\">\n";
			WriteXmlNode_Start(str);
			str = "<column name=\"Item\" id=\"Item\"/>\n";
			WriteXmlNode_Start(str,false);
			str = "<column name=\"Value\" id=\"Value\"/>\n";
			WriteXmlNode_Start(str,false);
		}
		else if(m_iXmlFormat==XML_FORMAT::EKM_META_DATA)
		{
		}
		else if (m_iXmlFormat == XML_FORMAT::EKM_ASM_DEP_EXTRACT)
		{
		}
		else
		{
			WriteXmlNode("materialProperties");
		}
	}

	if(Material->label.len())
	{
		if(m_iXmlFormat==XML_FORMAT::EKM_REPORT)
		{
			this->WriteEkmReport_Row("Type Of Material",Material->label.c_str());
			WriteXmlNode_End("</table>\n");
		}
		else if(m_iXmlFormat==XML_FORMAT::EKM_META_DATA)
		{
			this->WriteEkmXmlAttribute("Type Of Material",Material->label.c_str());
		}
		else if (m_iXmlFormat == XML_FORMAT::EKM_ASM_DEP_EXTRACT)
		{
		}
		else if(m_iXmlFormat!=XML_FORMAT::KEY_VALUE_PAIRS)
		{
			WriteXmlAttribute("TypeOfMaterial" ,Material->label.c_str());
		}

	}

	if(Material->composites && Material->composites->value.size())
	{
		if(Material->composites->value.size())
		{
			if(m_iXmlFormat==XML_FORMAT::EKM_REPORT)
			{
				std::stringstream ss;
				ss<<"<table name=\"Material Properties - ";
				ss<<Material->composites->name.c_str();
				ss<<"\" showHeaders=\"true\" showBorder=\"true\" caption=\"\">\n";
				std::string str = ss.str();
				
				WriteXmlNode_Start(str);
				str = "<column name=\"Item\" id=\"Property\"/>\n";
				WriteXmlNode_Start(str,false);
				str = "<column name=\"Value\" id=\"Value\"/>\n";
				WriteXmlNode_Start(str,false);
			}
			else if(m_iXmlFormat==XML_FORMAT::EKM_META_DATA)
			{
			}
			else if (m_iXmlFormat == XML_FORMAT::EKM_ASM_DEP_EXTRACT)
			{
			}
			else
			{
				WriteXmlNode(Material->composites->name.c_str());
			}
		}

		for(int i=0;i<Material->composites->value.size();i++)
		{
			/*ss.str("");
			ss<<"vcDataKit::WriteNode() : NameProperties : "<<Material->composites->value[i].NameProperties;
			vcUtils::LogMsg(ss.str());*/
			
			ss.str("");
			ss<<Material->composites->value[i].PropertiesValue[0].GetDouble();
			if(m_iXmlFormat==XML_FORMAT::EKM_REPORT)
			{
				this->WriteEkmReport_Row(Material->composites->value[i].NameProperties.c_str() ,ss.str().c_str());
			}
			else if(m_iXmlFormat==XML_FORMAT::EKM_META_DATA)
			{
			}
			else if (m_iXmlFormat == XML_FORMAT::EKM_ASM_DEP_EXTRACT)
			{
			}
			else
			{
				WriteXmlAttribute(Material->composites->value[i].NameProperties.c_str() ,ss.str().c_str());
			}

			/*for(int j=0;j<Material->composites->value[i].PropertiesValue.size();j++)
			{
				ss.str("");
				ss<<"vcDataKit::WriteNode() : PropertiesValue :"<<j<<":"<<Material->composites->value[i].PropertiesValue[j].GetDouble();
				vcUtils::LogMsg(ss.str());
			}*/
		}
		if(Material->composites->value.size())
		{
			if(m_iXmlFormat==XML_FORMAT::EKM_REPORT)
			{
				WriteXmlNode_End("</table>\n");
			}
			else if(m_iXmlFormat==XML_FORMAT::EKM_META_DATA)
			{
			}
			else if (m_iXmlFormat == XML_FORMAT::EKM_ASM_DEP_EXTRACT)
			{
			}
			else
			{
				WriteXmlNode();
			}
		}
	}//End - if(Material->composites && Material->composites->value.size())

	if(Material->analysis && Material->analysis->value.size())
	{
		/*if(Material->analysis->name.len())
		{
			ss.str("");
			ss<<"vcDataKit::WriteNode() : Analysis Name : "<<Material->analysis->name;
			vcUtils::LogMsg(ss.str());
		}*/	
		if(Material->analysis->value.size() && Material->analysis->name.len())
		{
			if(m_iXmlFormat==XML_FORMAT::EKM_REPORT)
			{
				std::stringstream ss;
				ss<<"<table name=\"Material Properties - ";
				ss<<Material->analysis->name.c_str();
				ss<<"\" showHeaders=\"true\" showBorder=\"true\" caption=\"\">\n";
				std::string str = ss.str();
				
				WriteXmlNode_Start(str);
				str = "<column name=\"Item\" id=\"Property\"/>\n";
				WriteXmlNode_Start(str,false);
				str = "<column name=\"Value\" id=\"Value\"/>\n";
				WriteXmlNode_Start(str,false);
			}
			else if(m_iXmlFormat==XML_FORMAT::EKM_META_DATA)
			{
			}
			else if (m_iXmlFormat == XML_FORMAT::EKM_ASM_DEP_EXTRACT)
			{
			}
			else
			{
				WriteXmlNode(Material->analysis->name.c_str());
			}
		}

		for(int i=0;i<Material->analysis->value.size();i++)
		{
			/*ss.str("");
			ss<<"vcDataKit::WriteNode() : NameProperties : "<<Material->analysis->value[i].NameProperties;
			vcUtils::LogMsg(ss.str());*/
			
			ss.str("");
			ss<<Material->analysis->value[i].PropertiesValue[0].GetDouble();
			if(m_iXmlFormat==XML_FORMAT::EKM_REPORT)
			{
				this->WriteEkmReport_Row(Material->analysis->value[i].NameProperties.c_str() ,ss.str().c_str());
			}
			else if(m_iXmlFormat==XML_FORMAT::EKM_META_DATA)
			{
			}
			else if (m_iXmlFormat == XML_FORMAT::EKM_ASM_DEP_EXTRACT)
			{
			}
			else
			{
				WriteXmlAttribute(Material->analysis->value[i].NameProperties.c_str() ,ss.str().c_str());
			}

			/*for(int j=0;j<Material->analysis->value[i].PropertiesValue.size();j++)
			{
				ss.str("");
				ss<<"vcDataKit::WriteNode() : PropertiesValue :"<<j<<":"<<Material->analysis->value[i].PropertiesValue[j].GetDouble();
				vcUtils::LogMsg(ss.str());
			}*/
		}
		if(Material->analysis->value.size())
		{
			if(m_iXmlFormat==XML_FORMAT::EKM_REPORT)
			{
				WriteXmlNode_End("</table>\n");
			}
			else if(m_iXmlFormat==XML_FORMAT::EKM_META_DATA)
			{
			}
			else if (m_iXmlFormat == XML_FORMAT::EKM_ASM_DEP_EXTRACT)
			{
			}
			else
			{
				WriteXmlNode();
			}
		}
	}//End - if(Material->analysis && Material->analysis->value.size())

	if(Material->composites && Material->composites->value.size() || 
       Material->analysis && Material->analysis->value.size()  ||
	   Material->label.len()
	   )
	{
		if(m_iXmlFormat==XML_FORMAT::EKM_REPORT)
		{
		}
		else if(m_iXmlFormat==XML_FORMAT::EKM_META_DATA)
		{
		}
		else if (m_iXmlFormat == XML_FORMAT::EKM_ASM_DEP_EXTRACT)
		{
		}
		else
		{
			WriteXmlNode();
		}
	}

	if(err == dtkNoError && m_RootComponent.IsNotNULL() && !m_RootComponent->IsAssembly())
		m_pDtkAPI->EndComponent(m_RootComponent);

	return true;
}

void vcCad2Xml::WriteEkmReport_Row(std::string _sKey,std::string _sValue)
{

	std::string sKey,sValue;
	if(!Convert_To_EKM_Xml_Key_Field(_sKey.c_str(),sKey))
		return;

	if(!Convert_To_EKM_Xml_Value_Field(_sValue.c_str(),sValue))
		return;

	std::stringstream ss;
	std::string tag;

	ss.str("");
	for(int i=0;i<m_iIndent;i++)
		ss<<"\t";
	ss<<"<row>\n";
	tag =ss.str();
	WriteTag(tag);

	ss.str("");
	for(int i=0;i<m_iIndent+1;i++)
		ss<<"\t";
	ss<<"<value>"<<EncodeString(sKey)<<"</value>\n";
	tag =ss.str();
	WriteTag(tag);

	ss.str("");
	for(int i=0;i<m_iIndent+1;i++)
		ss<<"\t";
	ss<<"<value>"<<EncodeString(sValue)<<"</value>\n";
	tag =ss.str();
	WriteTag(tag);

	ss.str("");
	for(int i=0;i<m_iIndent;i++)
		ss<<"\t";
	ss<<"</row>\n";
	tag =ss.str();
	WriteTag(tag);
}


void vcCad2Xml::WriteXmlNode_Start(std::string sStr,bool bIndent)
{
	std::stringstream ss;
	std::string tag;

	for(int i=0;i<m_iIndent;i++)
		ss<<"\t";
	ss<<sStr;
	tag = ss.str();

	WriteTag(tag);

	if(bIndent)
		m_iIndent++;

}
void vcCad2Xml::WriteXmlNode_End(std::string sStr)
{
	m_iIndent--;
	std::stringstream ss;
	std::string tag;
	for(int i=0;i<m_iIndent;i++)
		ss<<"\t";
	ss<<sStr;
	tag = ss.str();

	WriteTag(tag);
}

void vcCad2Xml::WriteXmlNode_StartEnd(std::string _tag,std::string _sVal, bool bIndent)
{
	std::string tag, sVal;
	if (!Convert_To_Xml_Key_Field(_tag.c_str(), tag))
		return;

	if (!Convert_To_Xml_Value_Field(_sVal.c_str(), sVal))
		return;

	std::stringstream ss;

	for (int i = 0; i<m_iIndent; i++)
		ss << "\t";
	ss << "<" << EncodeString(tag) << ">";
	outfile.write(ss.str().c_str(), ss.str().length());

	
	ss.str("");
	ss <<EncodeString(sVal);

	//outfile.write(sVal.c_str(), sVal.length());
	outfile.write(ss.str().c_str(), ss.str().length());
	
	
	ss.str("");
	ss << "</" << EncodeString(tag) << ">\n";
	outfile.write(ss.str().c_str(), ss.str().length());

	//m_iIndent--;
}

void vcCad2Xml::RemoveCommonStrFromPath(std::string &input_path, std::string &comp_path) 
{
	int min_len = input_path.size();
	if (min_len > comp_path.size())
		min_len = comp_path.size();
	int common_str_len = 0, last_folder_index=0;
	std::string str1, str2;	
	int i = 0;
	for ( i = 0; i < min_len; i++)
	{
		if (input_path[i] == '/')
		{
			if (str1 == str2)
			{
				str1.clear();
				str2.clear();
				last_folder_index = i+1;
			}
			else
			{
				//common_str_len = last_folder_index;
				break;
			}
			continue;
		}
		str1.push_back(input_path[i]);
		str2.push_back(comp_path[i]);
	}
	if (str1 == str2)
	{
		str1.clear();
		str2.clear();
		last_folder_index = i;
		input_path = "";
		comp_path = "";
		return;
	}
	input_path = input_path.substr(last_folder_index, input_path.length());
	comp_path = comp_path.substr(last_folder_index, comp_path.length());
}