
#include <Windows.h>
#include "vcCad2Cax.h"
#include "version.h"
#include <direct.h>
#if defined(__WXMSW__)
#include <wx/string.h>
#include <wx/msgdlg.h>
#endif
#include "tess/tess.h"
#include "vcUtils.h"
#include "vcLicense.h"

#include "vcRemoveDuplicateVertices.h"
#include "vcVec.h"
#include "StopWatch.h"

//#define VCT_DEF_AND_USE 1
//#define VCT_LINE_ELEMENTS 1
//#define VCT_POINT_SETS 1

//#define PART_WISE_TESSELLATION 1

DtkErrorStatus g_dtkErrorStatus = dtkNoError;
extern std::vector<float *> FloatArrays;
extern std::vector<uint32_t *> IntArrays;
extern bool g_bIgnoreTransparency;
#ifdef PART_WISE_TESSELLATION
	double g_fTessTolerance = 0.0005f;
#else
	double g_fTessTolerance = 0.05f;
#endif
extern char g_sVersion[20];
int g_iUnit = 0;
bool g_bInfoAboutCAD = false;
bool g_bUserUnits = false;
bool g_bDetailedLog = false;
bool g_bDataLoss = false;
bool g_bTestResults = false;
std::string g_sTestResultsFileName;
extern bool g_bMesh,g_b2DElements,g_bPointSet, g_bCombinePartsInGroup;;
bool g_bValidInputFormat = false;

bool g_fMinDistancce = FLT_MAX;
int g_iParseMode=1;
vcCad2Cax::vcCad2Cax(char *sInputFileName,char *sOutputFileName,char *sTmpWorkingDir,bool &bSuccess):m_CaxScene("Model")
{
	vcUtils::LogMsg("vcCad2Cax::Start");
	
    m_iErrorCode = NOT_TRANSLATED;
	if(!g_bInfoAboutCAD)
	{
		m_CaxScene.setApplicationInfo("VMoveCAD", 1.0);
		m_CaxScene.saveTo(sOutputFileName,"",true,true);
	}

	m_DtkColorNFaceArrayMap.clear();
	m_DtkColorNLinesArrayMap.clear();
	m_DtkColorNPointsArrayMap.clear();

	m_bSinglePart = false;

	m_iShapeIndexOfComponent = 0;
	m_dtkErrorStatus = dtkNoError;
	m_iIndex = 0;
	m_iFaceIndex = 0;
	m_iShapeIndex = 0;
	m_pDtkAPI = NULL;
	m_sInputFileName = sInputFileName;
	m_sOutputFileName = sOutputFileName;
	m_sTmpWorkingDir = sTmpWorkingDir;

	m_bNegativeScale = false;
	m_iNegativeScaleNodeIndex=-1;
	m_NegativeScaleVec[0]=1;
	m_NegativeScaleVec[1]=1;
	m_NegativeScaleVec[2]=1;

	std::string memusage = vcUtils::GetMemoryUsage();
    ss.str("");
	ss<<"vcCad2Cax::Memory Usage b4 conversion: "<<memusage;
	vcUtils::LogMsg(ss.str());
    ss.str("");

	m_iUnit = g_iUnit;

	if(!g_bInfoAboutCAD)
		InitializeTestResults();

	bSuccess = Convert();

	m_pDtkAPI = NULL;

	memusage = vcUtils::GetMemoryUsage(); 
    ss.str("");
	ss<<"vcCad2Cax::Memory Usage after conversion: "<<memusage;
	vcUtils::LogMsg(ss.str());
    ss.str("");

	g_dtkErrorStatus = m_dtkErrorStatus;

	if(g_bTestResults)
	{
		if(m_TestResultsFile.is_open())
			m_TestResultsFile.close();
	}

	vcUtils::LogMsg("vcCad2Cax::End");
}

vcCad2Cax::~vcCad2Cax()
{ 
	if(m_NodesArray.size())
		m_NodesArray.empty();

	std::string memusage = vcUtils::GetMemoryUsage();
    ss.str("");
	ss<<"vcCad2Cax::~vcCad2Cax():Memory Usage(b4 cleaing matrix array): "<<memusage;
	vcUtils::LogMsg(ss.str());

	for(int i=0;i<m_DtkMatrixPtrArray.size();i++)
	{
		Dtk_matrix *mat = m_DtkMatrixPtrArray[i];
		mat = NULL;
	}
	if(m_DtkMatrixPtrArray.size())
		m_DtkMatrixPtrArray.empty();

	memusage = vcUtils::GetMemoryUsage();
    ss.str("");
	ss<<"vcCad2Cax::~vcCad2Cax():Memory Usage(after cleaing matrix array): "<<memusage;
	vcUtils::LogMsg(ss.str());

	if(m_DtkComponentColorArray.size())
		m_DtkComponentColorArray.empty();

	if(m_NodeNameAndIndexMap.size())
		m_NodeNameAndIndexMap.empty();
} 

bool vcCad2Cax::Convert()
{
	vcUtils::LogMsg("vcCad2Cax::Convert():Start");

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
	if(errorStatus != dtkNoError && CadDoc.IsNULL())
	{
		m_dtkErrorStatus=errorStatus;
		m_iErrorCode = DATAKIT_ERROR;
		StopDtkAPI();
		return false;
	}


	try
	{
		StopWatch sw;
		sw.Restart();

		errorStatus = ParseCadFile(CadDoc);

		std::stringstream ss;
		if(!g_bInfoAboutCAD)
		{
			ss.str("");
			ss<<"Time taken for Translation (µs) ******************* :"<<sw.ElapsedUs();
			vcUtils::LogMsg(ss.str());
		}
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
		StopDtkAPI();
		return false;
	}

	CloseCADFile(CadDoc);

	StopDtkAPI();

	if(g_bInfoAboutCAD)
		return true;

	if(!WriteCax())
	{
		m_iErrorCode = CAX_WRITER_ERROR;
		return false; 
	}

	if(g_bTestResults)
	{
		m_TestResultsFile<<"Total Components: "<<m_iTotalComponents<<std::endl;
		m_TestResultsFile<<"Total Surfaces: "<<m_iTotalSurfaces<<std::endl;
		m_TestResultsFile<<"Total Lines: "<<m_iTotalLines<<std::endl;
		m_TestResultsFile<<"Total PointSets: "<<m_iTotalPointSets<<std::endl;
	}

	vcUtils::LogMsg("vcCad2Cax::Convert():End");
	return true;
}

bool vcCad2Cax::WriteCax()
{
	vcUtils::LogMsg("vcCad2Cax::WriteCax():Start"); 

	std::string memusage = vcUtils::GetMemoryUsage();
    ss.str("");
	ss<<"vcCad2Cax::WriteCax():b4 Writing CAX:Memory Usage: "<<memusage;
	vcUtils::LogMsg(ss.str());

	try
	{
		//m_CaxScene.Write(m_sOutputFileName.c_str(), true);
		m_CaxScene.createDefaultViews(m_CaxRoot);
		m_CaxScene.closeFile(m_sOutputFileName.c_str());
	}
	catch(...)
	{
#ifdef VMOVECAD_BATCH
		printf("\nError : Not able to write the CAX file.\n");
		printf("Please make sure you have write permission in the output folder\n");
		printf("Please refer troubleshooting page of VMoveCAD help\n");
#else
		std::string err;
		ss.str("");
		ss<<"Not able to write the CAX file. "<<"\n"<<"Please make sure you have write permission in the output folder."<<"\n"<<"Please refer troubleshooting page of VMoveCAD help.";
#if defined(__WXMSW__)
		wxMessageBox(ss.str(),wxT("Error"),wxICON_ERROR, NULL);
#else
		//%%%::MessageBox(NULL,ss.str().c_str(),"VMoveCAD",MB_ICONERROR);
#endif
		ss.str("");
#endif
		ss.str("");
		ss<<"vcCad2Cax::WriteCax():Not able to write the CAX file.";
		vcUtils::LogMsg(ss.str());
		ss.str("");

		m_iErrorCode = CAX_WRITER_ERROR;
		return false;
	}

	memusage = vcUtils::GetMemoryUsage();
    ss.str("");
	ss<<"vcCad2Cax::WriteCax():after Writing CAX:Memory Usage: "<<memusage;
	vcUtils::LogMsg(ss.str());
    ss.str("");

	vcUtils::LogMsg("vcCad2Cax::WriteCax():End");
	return true;
}

Dtk_ErrorStatus vcCad2Cax::CloseCADFile(Dtk_MainDocPtr TmpDoc)
{
	vcUtils::LogMsg("vcCad2Cax::CloseCADFile():Start");

	// You Get the current API 
	DtkErrorStatus errorStatus = dtkNoError; 
	Dtk_API * MyAPI = Dtk_API::GetAPI();
	//We close the opened document
	try
	{
		if(MyAPI)
			errorStatus = MyAPI->EndDocument(TmpDoc);
	}
	catch(...)
	{
		vcUtils::LogMsg("vcCad2Cax::CloseCADFile():MyAPI->EndDocument(TmpDoc) ***Exception*** caught");
	}

	ss.str("");
	ss<<"vcCad2Cax::CloseCADFile():MyAPI->EndDocument():Error Status: "<<dtkTypeError(errorStatus);
	vcUtils::LogMsg(ss.str());
	ss.str("");

	vcUtils::LogMsg("vcCad2Cax::CloseCADFile():End");
	return errorStatus;
}

void vcCad2Cax::InitializeCaxScene(const char *rootName)
{
	vcUtils::LogMsg("vcCad2Cax::InitializeCaxScene():Start");

	m_CaxRoot = m_CaxScene.createObject(Cax::ASSEMBLY); 
	m_CaxScene.setName(m_CaxRoot,this->GetValidNodeName(rootName).c_str());
	m_NodesArray.push_back(m_CaxRoot);

	vcUtils::LogMsg("vcCad2Cax::InitializeCaxScene():End");
}

void vcCad2Cax::EnableCADReaders()
{
	vcUtils::LogMsg("vcCad2Cax::EnableCADReaders():Start");

	std::string memusage = vcUtils::GetMemoryUsage();
    ss.str("");
	ss<<"vcCad2Cax::EnableCADReaders():b4 enabling CAD readers:Memory Usage: "<<memusage;
	vcUtils::LogMsg(ss.str());

	//CaddsReader::Enable(); 
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

	if (g_iParseMode == 2)
	{
		vcUtils::LogMsg("Full Computations in turned off for STEP Reader - stepr_SetParseMode(2)");
		stepr_SetParseMode(2);
	}
	//stepr_SetParseMode(2);
	//JtReader::Enable();
	//XmtReader::Enable();
	//VdaReader::Enable();

	memusage = vcUtils::GetMemoryUsage();
    ss.str("");
	ss<<"vcCad2Cax::EnableCADReaders():after enabling readers:Memory Usage: "<<memusage;
	vcUtils::LogMsg(ss.str());

	vcUtils::LogMsg("vcCad2Cax::EnableCADReaders():End");
}


Dtk_ErrorStatus vcCad2Cax::StartDtkAPI()
{
	vcUtils::LogMsg("vcCad2Cax::StartDtkAPI():Start");

	Dtk_ErrorStatus errorStatus = dtkNoError; 

#ifdef _LINUX64_
    if(m_sTmpWorkingDir[m_sTmpWorkingDir.len()-1] != '/')
        m_sTmpWorkingDir.Merge("/");
#else
	if(m_sTmpWorkingDir[m_sTmpWorkingDir.len()-1] != '\\')
		m_sTmpWorkingDir.Merge("\\");
#endif
	 
	try
	{
		m_pDtkAPI = Dtk_API::StartAPI(m_sTmpWorkingDir, errorStatus,"VCOLLAB_SDK-170092217140010146187057140087227106200052053225206228253120022230154211033210227167181138169073103011117156088102093069252055244082189172018211172233079224079026239024130161046029174225164029254174199084181068197145000193037147121147071067175206137134191135246139094253034071232108167067156209204041162254022101034234144187006124072189031160233154208171163016244001047205185142117054206218112117081216204129175174117209165012129039149135093061082030195171114251117129028092211062011047119158081247159176232198054240139121174076028165212136137167122005230231178056230038012054186217056187025101060190068130097026035106034050151119110040148037156205072250084236203006095188028052192062211177002049174124208241255232035013086001056124255185056143057182218121164205065053252092092202225052042226117159250108164074003165148177001034046176056160147194225011059120138211228077120123215251086249158058108075120005140255-304V232");
	}
	catch(...)
	{
		vcUtils::LogMsg("vcCad2Cax::StartDtkAPI(): Dtk_API::StartAPI() ***Exception*** caught");
		return errorStatus;
	}

	ss.str("");
	ss<<"vcCad2Cax::StartDtkAPI(): StartAPI()::Error Status: "<<dtkTypeError(errorStatus);
	vcUtils::LogMsg(ss.str());
	ss.str("");

	if(errorStatus == dtkErrorLicence)
	{
	   vcUtils::LogMsg("vcCad2Cax::StartDtkAPI(): No license available");
	   return errorStatus;
	
#ifdef VMOVECAD_BATCH
	   printf("\nError : No license available.  Error Code : %s\n",dtkTypeError(errorStatus).c_str());
	   printf("Please refer troubleshooting page of VMoveCAD help\n");
#else
		std::string err;
		ss.str("");
		ss<<"No license available.  Error Code : "<<dtkTypeError(errorStatus)<<"\n"<<"Please refer troubleshooting page of VMoveCAD help.";
#if defined(__WXMSW__)
		wxMessageBox(ss.str(),wxT("Error"),wxICON_ERROR, NULL);
#else
		//%%%::MessageBox(NULL,ss.str().c_str(),"VMoveCAD",MB_ICONERROR);
#endif
		ss.str("");
#endif
		ss.str("");
		ss<<"vcCad2Cax::StartDtkAPI():No license available :Error Status: "<<dtkTypeError(errorStatus);
		vcUtils::LogMsg(ss.str());
		ss.str("");
		vcUtils::LogMsg(ss.str());
	    return errorStatus;
	}

	if(errorStatus == dtkErrorOpenFiles)
	{
#ifdef VMOVECAD_BATCH
		printf("\nError : Not able to read CAD file.  Error Code : %s\n",dtkTypeError(errorStatus).c_str());
		printf("Please refer troubleshooting page of VMoveCAD help\n");
#else
		std::string err;
		ss.str("");
		ss<<"Not able to read CAD file.  Error Code : "<<dtkTypeError(errorStatus)<<"\n"<<"Please refer troubleshooting page of VMoveCAD help.";
#if defined(__WXMSW__)
		wxMessageBox(ss.str(),wxT("Error"),wxICON_ERROR, NULL);
#else
		//%%%::MessageBox(NULL,ss.str().c_str(),"VMoveCAD",MB_ICONERROR);
#endif
		ss.str("");
#endif
		ss.str("");
		ss<<"vcCad2Cax::StartDtkAPI():Failed to read CAD file:Error Status: "<<dtkTypeError(errorStatus);
		vcUtils::LogMsg(ss.str());
		ss.str("");
		vcUtils::LogMsg(ss.str());
	    return errorStatus;
	}

	if(errorStatus != dtkNoError)
	{
#ifdef VMOVECAD_BATCH
		printf("\nError:Not able to read the CAD file.  Error Code : %s\n",dtkTypeError(errorStatus).c_str());
		printf("Please refer troubleshooting page of VMoveCAD help\n");
#else
		std::string err;
		ss.str("");
		ss<<"Not able to read the CAD file. Error Code : "<<dtkTypeError(errorStatus)<<"\n"<<"Please refer troubleshooting page of VMoveCAD help.";;
#if defined(__WXMSW__)
		wxMessageBox(ss.str(),wxT("Error"),wxICON_ERROR, NULL);
#else
		//%%%::MessageBox(NULL,ss.str().c_str(),"VMoveCAD",MB_ICONERROR);
#endif
		ss.str("");
#endif
		ss.str("");
		ss<<"vcCad2Cax::StartDtkAPI():Failed to read CAD file:Error Status : "<<dtkTypeError(errorStatus);
		vcUtils::LogMsg(ss.str());
		ss.str("");
	    return errorStatus;
	}

	if(m_pDtkAPI == NULL)
	{
		printf("Can't Start DATAKIT API\n");
		vcUtils::LogMsg("vcCad2Cax::StartDtkAPI():Failed to start DATAKIT API");
		return dtkErrorAPINotStarted;
	}

	m_pDtkAPI->SetBodyModePreference(DTK_BODYMODE_COMPLETETOPOLOGY);
	m_pDtkAPI->ActivateSplitForPeriodicFaces();

#ifndef PART_WISE_TESSELLATION
	//If you want to use tesselation library start Tesselation Kernel
#ifdef ACTIVATE_TESSELATION_LIB
	if(g_fTessTolerance==0.0f)
		g_fTessTolerance = 0.05f;

	int status; 
	try
	{
		status = tess_InitTesselation("tess_tmp",g_fTessTolerance);
		/*double LinearTol = 0.1; 
		tess_set_linear(LinearTol); 
		double AngularTol = 0.175; // angle in radian
		tess_set_angular(AngularTol); */
		if(status == dtkErrorLicence)
		{
			printf("No tesselation license available\n");
			vcUtils::LogMsg("vcCad2Cax::StartDtkAPI():No tesselation license available");
		}
	}
	catch(...)
	{
		vcUtils::LogMsg("vcCad2Cax::StartDtkAPI(): Tesselation failed");
	}
#endif
#endif

	vcUtils::LogMsg("vcCad2Cax::StartDtkAPI():End");
	return errorStatus;
}

void vcCad2Cax::StopDtkAPI()
{
	vcUtils::LogMsg("vcCad2Cax::StopAPI():Start");
	Dtk_API::StopAPI(m_pDtkAPI);
	vcUtils::LogMsg("vcCad2Cax::StopAPI():End");
}

Dtk_ErrorStatus vcCad2Cax::OpenCADFile(Dtk_MainDocPtr &TmpDoc)
{
	vcUtils::LogMsg("vcCad2Cax::OpenCADFile():Start");

	// You Get the current API 
	DtkErrorStatus stError = dtkNoError; 
	Dtk_API * MyAPI = Dtk_API::GetAPI();

	//Set the Schema directory needed for readers based on Pskernel (UG, Solidworks, Solidedge), or CADDS
#ifdef WIN32
	try
	{
		char szModuleFileName[MAX_PATH];
		GetModuleFileName(NULL, szModuleFileName, sizeof(szModuleFileName)); 
		std::string Path = vcUtils::GetPathFromFileName(szModuleFileName);
		Dtk_string inRepSchema = Path.c_str()+Dtk_string("\\schema");
		
		ss.str("");
		ss<<"vcCad2Cax::OpenCADFile():PsKernal Schema Path: "<<inRepSchema.c_str();
		vcUtils::LogMsg(ss.str());
		ss.str("");

		char TmpFullPathSchemaDir[_MAX_PATH];
		if( _fullpath( TmpFullPathSchemaDir, inRepSchema.c_str(), _MAX_PATH ) != NULL )
			stError = MyAPI->SetSchemaDir (TmpFullPathSchemaDir);
		else
			stError = MyAPI->SetSchemaDir (inRepSchema);
	}
	catch(...)
	{
		vcUtils::LogMsg("vcCad2Cax::OpenCADFile(): SetSchemaDir ***Exception*** caught");
	}
#else 
		Dtk_string inRepSchema = "./Schema";
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

    if(!g_bInfoAboutCAD)
		printf("\nTranslating \'%s\' \n",m_sInputFileName.c_str());

	std::string memusage = vcUtils::GetMemoryUsage();
    ss.str("");
	ss<<"vcCad2Cax::OpenCADFile:b4 OpenDocument():Memory Usage: "<<memusage;
	vcUtils::LogMsg(ss.str());

	Dtk_ErrorStatus errorStatus;
	try
	{
		//You Open the file you want to read and get corresponding Document 
		errorStatus = MyAPI->OpenDocument(m_sInputFileName, TmpDoc);
	}
	catch(...)
	{
#ifdef VMOVECAD_BATCH
			printf("\nError : Not able to open CAD file. \n");
			printf("Please contact support@vcollab.com for further support\n");
#else
			std::string err;
			ss.str("");
			ss<<"Not able to open CAD file. Please contact support@vcollab.com for further support.";
#if defined(__WXMSW__)
			wxMessageBox(ss.str(),wxT("Error"),wxICON_ERROR, NULL);
#else
			//%%%::MessageBox(NULL,ss.str().c_str(),"VMoveCAD",MB_ICONERROR);
#endif
			ss.str("");
#endif
			ss.str("");
			ss<<"vcCad2Cax::OpenCADFile():MyAPI->OpenDocument() ***Exception*** caught ";
			vcUtils::LogMsg(ss.str());
			ss.str("");
			vcUtils::LogMsg(ss.str());
			return errorStatus;
	}

	std::stringstream ss;
	ss<<"vcCad2Cax::OpenCADFile():OpenDocument():Error Status: "<<dtkTypeError(errorStatus);
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
	memusage = vcUtils::GetMemoryUsage();
    ss.str("");
	ss<<"vcCad2Cax::OpenCADFile: after OpenDocument():Memory Usage: "<<memusage;
	vcUtils::LogMsg(ss.str());

	//If no Error we write the Document
	if(errorStatus == dtkNoError && TmpDoc.IsNotNULL() )
	{
		vcUtils::LogMsg("vcCad2Cax::OpenCADFile():OpenDocument succeeded");
	}
	else
	{
		if(errorStatus != dtkNoError)
		{
#ifdef VMOVECAD_BATCH
			printf("\nError : Not able to open CAD file.  Error Code : %s\n",dtkTypeError(errorStatus).c_str());
			printf("Please refer troubleshooting page of VMoveCAD help\n");
#else
			std::string err;
			ss.str("");
			ss<<"Not able to open CAD file.  Error Code : "<<dtkTypeError(errorStatus)<<"\n"<<"Please refer troubleshooting page of VMoveCAD help.";
#if defined(__WXMSW__)
			wxMessageBox(ss.str(),wxT("Error"),wxICON_ERROR, NULL);
#else
			::MessageBox(NULL,ss.str().c_str(),"VMoveCAD",MB_ICONERROR);
#endif
			ss.str("");
#endif
			ss.str("");
			ss<<"vcCad2Cax::OpenCADFile():Failed to open CAD file:Error Status: "<<dtkTypeError(errorStatus);
			vcUtils::LogMsg(ss.str());
			ss.str("");
			vcUtils::LogMsg(ss.str());
			return errorStatus;
		}
	}

	vcUtils::LogMsg("vcCad2Cax::OpenCADFile():End");
	return errorStatus;
}

std::string vcCad2Cax::GetMeshType(type_detk readerType)
{
	std::string sMeshTypeName;

	return sMeshTypeName;
}

std::string vcCad2Cax::GetCadPackageName(DtkReaderType readerType)
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
	case DtkReaderType::RevitReaderModule  : //= 17,
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

std::string vcCad2Cax::GetUnitsName(Dtk_Double64 unit)
{
	this->m_fUnit = unit;
	std::string sUnitsName="unknown";
	if(unit == 1.0f)
	{
		sUnitsName = "millimeter";
		m_iUnit = vcCad2Cax::MILLIMETER;
	}
	else if(unit>25.f && unit<26.f)
	{
		sUnitsName = "inch";
		m_iUnit = vcCad2Cax::INCH;
	} 
	else if(unit>304.f && unit<305.f)
	{
		sUnitsName = "feet";
		m_iUnit = vcCad2Cax::FEET;
	}
	else if(unit == 1000.f)
	{
		sUnitsName = "meter";
		m_iUnit = vcCad2Cax::METER;
	}
	else if(unit == 10.f)
	{
		sUnitsName = "centimeter";
		m_iUnit = vcCad2Cax::CENTIMETER;
	}
	else
	{
		sUnitsName = "unknown";
	}
	return sUnitsName;
}

Dtk_ErrorStatus vcCad2Cax::ParseCadFile(Dtk_MainDocPtr inDocument) 
{
	vcUtils::LogMsg("vcCad2Cax::ParseCadFile():Start");
	//First we get the root component in the document
	Dtk_ComponentPtr RootComponent = inDocument->RootComponent();
	//if no Error we write the Component
	if(RootComponent.IsNotNULL())
	{     
		//GetName
		Dtk_string ComponentName;
		if(RootComponent->Name().is_not_NULL())
			ComponentName = RootComponent->Name();

		if(!g_bInfoAboutCAD)
		{
			if(ComponentName.is_not_NULL())
				InitializeCaxScene(ComponentName.c_str()); 
			else
				InitializeCaxScene("Default"); 
		}


		std::stringstream ss;
		if(RootComponent->FullPathName().is_not_NULL())
		{
			ss<<"vcCad2Cax::ParseCadFile():CAD FileName: "<<vcUtils::GetFileNameWithExtn(RootComponent->FullPathName().c_str()).c_str();
			vcUtils::LogMsg(ss.str());
			ss.str("");
		}
		//m_MetaData.m_sNativeCadFileName = RootComponent->FullPathName().c_str();

		m_ReaderType = RootComponent->GetAssociatedModuleType(); 
		ss<<"vcCad2Cax::ParseCadFile():Source CAD Package: "<<GetCadPackageName(m_ReaderType).c_str();
		vcUtils::LogMsg(ss.str());
		ss.str("");

		if(RootComponent->GetFileVersion().is_NULL())
			ss<<"vcCad2Cax::ParseCadFile():Source CAD Package Version: Unknown";
		else
			ss<<"vcCad2Cax::ParseCadFile():Source CAD Package Version: "<<RootComponent->GetFileVersion().c_str();
		vcUtils::LogMsg(ss.str());
		ss.str(""); 

		this->GetUnitsName(RootComponent->GetConceptionUnitScale());
		m_iOriginalUnit = m_iUnit;
		ss<<"vcCad2Cax::ParseCadFile():Units In: "<<GetUnitsName(RootComponent->GetConceptionUnitScale()).c_str();
		vcUtils::LogMsg(ss.str());
		ss.str("");
		
		if(g_bTestResults)
		{
			m_TestResultsFile<<"CAD Package Name: "<<GetCadPackageName(m_ReaderType).c_str()<<std::endl;
			if(RootComponent->GetFileVersion().is_NULL())
				m_TestResultsFile<<"CAD Package Version: Unknown"<<std::endl;
			else
				m_TestResultsFile<<"CAD Package Version: "<<RootComponent->GetFileVersion().c_str()<<std::endl;
			m_TestResultsFile<<"Units: "<<GetUnitsName(RootComponent->GetConceptionUnitScale()).c_str()<<std::endl;
		}

		if(RootComponent->GetNumMetaData())
		{
			Dtk_MetaDataPtr MetaData = RootComponent->GetMetaData(0);
			int MetaDatasize = MetaData->GetSize();

			if(MetaData->GetTitle().is_not_NULL() && MetaData->GetTitle().len())
			{
				ss.str("");
				ss<<"vcCad2Cax::ParseCadFile():MetaData Title: "<<MetaData->GetTitle().c_str();
				vcUtils::LogMsg(ss.str());
				ss.str("");
			}

			if(MetaData->GetValue().is_not_NULL())
			{
				Dtk_string Value=MetaData->GetValue();
				if(Value.len()) 
				{
					ss<<"vcCad2Cax::ParseCadFile():MetaData Value: "<<Value.c_str();
					vcUtils::LogMsg(ss.str());
					ss.str("");
				}
				else
				{
					ss<<"vcCad2Cax::ParseCadFile():MetaData Value: No value is available for above title";
					vcUtils::LogMsg(ss.str());
					ss.str("");
				}
			}

		}

		if(RootComponent->Name().is_not_NULL())
		{
			ss<<"vcCad2Cax::ParseCadFile():Assembly Name: "<<RootComponent->Name();
			vcUtils::LogMsg(ss.str());
			ss.str("");
		}
		
		//if(g_bInfoAboutCAD)
				//return dtkNoError;

		m_fUnit = 1.0f;

		if(g_bUserUnits)
		{
			m_iUnit = g_iUnit;

			if(m_iOriginalUnit!=g_iUnit)
				m_fUnit = ConvertUnits(m_iOriginalUnit, g_iUnit);
			else
				m_fUnit = 1.0f;
		}
	
		WriteComponent( RootComponent );

		vcUtils::LogMsg("vcCad2Cax::ParseCadFile():End");
		return dtkNoError;
	}
	vcUtils::LogMsg("vcCad2Cax::ParseCadFile():RootComponent=NULL");
	return dtkErrorNullPointer;
}

Dtk_ErrorStatus vcCad2Cax::WriteComponent(Dtk_ComponentPtr inComponent)
{
	Dtk_Size_t i=0;
	Dtk_ErrorStatus err;
	
	//GetName
	Dtk_string ComponentName; 
	if(inComponent->Name().is_not_NULL())
		ComponentName = inComponent->Name();

	m_sCurrentComponentName = ComponentName;
	if(g_bDetailedLog)
	{
		if(ComponentName.is_not_NULL()) 
		{ 
			ss.str("");
			ss<<"vcCad2Cax::WriteComponent() : ComponentName : "<<ComponentName.c_str();
			vcUtils::LogMsg(ss.str());
			ss.str("");
		}
	}

	Dtk_RGB Color;
	if(inComponent->GetInfos().IsNotNULL())
	{
		Color = inComponent->GetInfos()->GetColor();
		m_DtkComponentColorArray.push_back(Color);
		if(g_bDetailedLog)
		{
			ss.str("");
			ss<<"vcCad2Cax::WriteComponent():inComponent->GetInfos()->GetColor(): "<<Color.R()<<" "<<Color.G()<<" "<<Color.B();
			vcUtils::LogMsg(ss.str());
			ss.str("");
		}
	}
	/*if(Material.IsNotNULL()) 
	{
		double ambient = Material->ambient;
		double diffuse = Material->diffuse; 
		double specular = Material->specular;
		double transparency = Material->transparency;
		double emissive = Material->reflectivity;
	}*/ 

	//You have 4 types for Component
	Dtk_Component::ComponentTypeEnum type = inComponent->ComponentType();
	switch(type)
	{
		//Instance represent a prototype with a matrix placement
		case Dtk_Component::InstanceComponentType :
		{
			try
			{
				Dtk_transfo transfo;
				Dtk_matrix *matrix = NULL;
				
				if(g_bDetailedLog)
				{
					ss.str("");
					ss<<"vcCad2Cax::WriteComponent():InstanceComponentType(Transform) NodeName: "<<ComponentName.c_str();
					vcUtils::LogMsg(ss.str());
					ss.str("");
				}

				transfo = inComponent->TransformationMatrix();
				transfo.setScale(1.0f);
				matrix = transfo.GetDtkMatrix();
				if(matrix) 
					m_DtkMatrixPtrArray.push_back(matrix);

				Dtk_ComponentPtr prototype = inComponent->GetChild(0) ;
				WriteComponent(prototype); 
			}
			catch(...)
			{
				g_bDataLoss = true;
				ss.str("");
				ss<<"vcCad2Cax::WriteComponent():InstanceComponentType(Transform) ***Exception*** caught: ";
				vcUtils::LogMsg(ss.str());
			}
			break;  
		}
		//Prototype (you have to check if you ever read and write it to don't waste time)
		//You can use methods SetProcessed() and HasBeenProcessed() to do this
		case Dtk_Component::PrototypeComponentType :
		{  
			try
			{
			//if(inComponent->HasBeenProcessed() == DTK_FALSE)
			{ 
				Dtk_NodePtr RootNode;  
				Dtk_API *inAPI = Dtk_API::GetAPI();
 
				if(g_bDetailedLog)
				{
					ss.str("");
					ss<<"vcCad2Cax::WriteComponent():PrototypeComponentType() NodeName: "<<ComponentName.c_str();
					vcUtils::LogMsg(ss.str());
				}

				//if(!ComponentName.len())
				//	ComponentName = "hello";
				//CAX-Group
				CaxId CaxGroup = m_CaxScene.createObject(Cax::ASSEMBLY);
				m_CaxScene.setName(CaxGroup,GetValidNodeName(ComponentName.c_str()).c_str());
				CaxId CurrentCaxParent = m_NodesArray.at(m_NodesArray.size()-1);
				//m_CaxScene.addComponent(CurrentCaxParent,CaxGroup);
				m_NodesArray.push_back(CaxGroup);

				//m_DataKitAndCaxNodeMap[inComponent->GetID()] = CaxGroup;

				if(g_bDetailedLog)
				{
					ss.str("");
					ss<<"vcCad2Cax::WriteComponent():Cax Group is created and appended to CAX Scene";
					vcUtils::LogMsg(ss.str());
				}

				if(m_DtkMatrixPtrArray.size())
				{
					Dtk_matrix* matrix = (Dtk_matrix*)m_DtkMatrixPtrArray.at(m_DtkMatrixPtrArray.size()-1);
					m_DtkMatrixPtrArray.pop_back();
					if(matrix)
					{
						CaxId CaxTrans = m_CaxScene.createObject(Cax::TRANSFORMATION);
						//std::string tname = std::string(name.c_str());// + std::string("#trans");
						m_CaxScene.setName(CaxTrans,GetValidNodeName(ComponentName.c_str()).c_str());
						m_NodesArray.push_back(CaxTrans);
						this->SetCAXTransformation(matrix); 
						m_NodesArray.pop_back();
						m_CaxScene.addComponent(CurrentCaxParent,CaxGroup,CaxTrans);
					}
					else
					{
						m_CaxScene.addComponent(CurrentCaxParent,CaxGroup);
					}	
					matrix = NULL;
				}
				else
				{
					m_CaxScene.addComponent(CurrentCaxParent,CaxGroup);
				}
 
				//it can also contain some instances 
				Dtk_Size_t NumChildren = inComponent->GetNumChildren();

				//std::stringstream ss;
				if(g_bDetailedLog)
				{
					ss.str("");
					ss<<"vcCad2Cax::WriteComponent():PrototypeComponentType():ReadComponent->NumChildren: "<<NumChildren;
	 				vcUtils::LogMsg(ss.str());
				}
				
				for( i = 0; i < NumChildren; i++)
				{ 
					 Dtk_ComponentPtr child = inComponent->GetChild(i) ;
 
					 if(child.IsNotNULL())
						WriteComponent( child);
				}

				if(g_bDetailedLog)
				{
					if(inComponent->FullPathName().is_not_NULL())
					{
						ss.str("");
						ss<<"vcCad2Cax::WriteComponent():PrototypeComponentType():ReadComponent->Path: "<<inComponent->FullPathName().c_str();
	 					vcUtils::LogMsg(ss.str());
					}
				}

				try
				{
					//Get the Construction tree for this prototype
					err = inAPI->ReadComponent( inComponent, RootNode );
				}
				catch(...)
				{
					g_bDataLoss = true;
					ss.str("");
					ss<<"vcCad2Cax::WriteComponent():PrototypeComponentType():inAPI->ReadComponent() ***Exception*** caught ";
	 				vcUtils::LogMsg(ss.str());
					break;
				}

				if(err != dtkNoError)
				{
					if(inComponent->FullPathName().is_not_NULL())
					{
						ss.str("");
						ss<<"vcCad2Cax::WriteComponent():PrototypeComponentType():ReadComponent->Path: "<<inComponent->FullPathName().c_str();
	 					vcUtils::LogMsg(ss.str());
					}
					ss.str("");
					ss<<"vcCad2Cax::WriteComponent():PrototypeComponentType():ReadComponent->Error Status: "<<dtkTypeError(err);
	 				vcUtils::LogMsg(ss.str());
				}

				m_dtkErrorStatus = err;
				if (err == dtkNoError && RootNode.IsNotNULL())
				{
					if(g_bTestResults)
					{
						if(inComponent->FullPathName().is_not_NULL())
							m_TestResultsFile<<"Component Name: "<<inComponent->FullPathName().c_str()<<std::endl;
						else
							m_TestResultsFile<<"Component Name: "<<ComponentName.c_str()<<std::endl;

						m_iTotalComponents++;
					}
					if(g_bDetailedLog)
					{
						ss.str("");
						ss<<"vcCad2Cax::WriteComponent():ProtoTypeComponentType: Writing Node...";
						vcUtils::LogMsg(ss.str());
					}

					m_bSinglePart = false;

					WriteNode(RootNode);

					//if(m_bSinglePart)
					if(g_bCombinePartsInGroup)
					{
						m_bSinglePart = false;

						m_iShapeIndexOfComponent = 0;
						if(g_bMesh)
						{
							StopWatch sw;
							if(g_bDetailedLog)
							{
								sw.Restart();
							}

							this->ConstructCaxShape(m_DtkColorNFaceArrayMap,ComponentName.c_str());

							if(g_bDetailedLog)
							{
								std::stringstream ss;
								ss.str("");
								ss<<"Time taken for ConstructCaxShape******************* :"<<sw.ElapsedUs();
								vcUtils::LogMsg(ss.str());
							}
						}
						if(g_b2DElements)
							ConstructShapeFor2DElements(ComponentName.c_str());
						if(g_bPointSet)
							ConstructShapeForPointElements(ComponentName.c_str());
					}
				}

				//??inComponent->SetProcessed(); 
				//We close the opened Component and free his construction tree
				err = inAPI->EndComponent(inComponent);

				if(m_NodesArray.size()==m_iNegativeScaleNodeIndex && m_iNegativeScaleNodeIndex != -1)
				{
					m_iNegativeScaleNodeIndex = -1;
					m_bNegativeScale = false;
				}

				//CAX
				if(m_NodesArray.size())
					m_NodesArray.pop_back();
				//
			}
			//else
			//{
			//	CaxId CurrentCaxParent = m_NodesArray.at(m_NodesArray.size()-1);
			//	CaxId CaxGroup = m_DataKitAndCaxNodeMap[inComponent->GetID()];
			//	//m_CaxScene.SetName(CaxGroup,GetValidNodeName(ComponentName.c_str()));
			//	m_CaxScene.AppendChild(CurrentCaxParent,CaxGroup);
			//}
			//else
			{
				//Get the prototype you ever write
				//The Component has a unique ID given by GetID to help to map it with your Write ID
			}
			}
			catch(...)
			{
				g_bDataLoss = true;
				ss.str("");
				ss<<"vcCad2Cax::WriteComponent():PrototypeComponentType(): ***Exception*** caught ";
 				vcUtils::LogMsg(ss.str());
				//AfxMessageBox("WriteComponent Exception block");
			}
			break;
		}
		//Catalog Component represent a choice of several possible configuration 
		//(like scene in catiav5, workspace in catiav4, configuration in solidworks)
		//Default is the first child 
		case Dtk_Component::CatalogComponentType :
		{
			try
			{
				Dtk_ComponentPtr defaultchoice = inComponent->GetChild(0) ;
				if (defaultchoice.IsNotNULL())
				{
					if(g_bDetailedLog)
					{
						ss.str("");
						ss<<"vcCad2Cax::WriteComponent():CatalogComponentType:ComponentName: "<<ComponentName.c_str();
						vcUtils::LogMsg(ss.str());
						ss.str("");
					}

					WriteComponent(defaultchoice);
				}
			}
			catch(...)
			{
				g_bDataLoss = true;
				ss.str("");
				ss<<"vcCad2Cax::WriteComponent():CatalogComponentType(): ***Exception*** caught ";
 				vcUtils::LogMsg(ss.str());
			}
			//if you don't want to use default you have to scan all children and choose the one you want to convert (see their name)
			break;
		}
		//Component containing only children 
		case Dtk_Component::VirtualComponentType :
		{
			try
			{
				Dtk_Size_t NumChildren;
				NumChildren = inComponent->GetNumChildren();

				if(NumChildren)
				{
					if(g_bDetailedLog)
					{
						ss.str("");
						ss<<"vcCad2Cax::WriteComponent():VirtualComponentType:NodeName: "<<ComponentName.c_str();
						vcUtils::LogMsg(ss.str());
						ss.str("");
					}

					for( i = 0; i < NumChildren; i++)
					{
						Dtk_ComponentPtr child = inComponent->GetChild(i) ;
						if(child.IsNotNULL())
							WriteComponent( child);
					}
				}
			}
			catch(...)
			{
				g_bDataLoss = true;
				ss.str("");
				ss<<"vcCad2Cax::WriteComponent():VirtualComponentType(): ***Exception*** caught ";
 				vcUtils::LogMsg(ss.str());
			}
			break;
		}
	}
	m_DtkComponentColorArray.pop_back();
	return dtkNoError;
}

Dtk_ErrorStatus vcCad2Cax::WriteNode(Dtk_NodePtr inNode)
{
	if(inNode.IsNULL())
		return dtkNoError;


	Dtk_API *MyAPI = Dtk_API::GetAPI();

	//GetName
	Dtk_string NodeName;
	if(inNode->Name().is_not_NULL())
	{
		NodeName= inNode->Name(); 
		if(g_bDetailedLog)
		{
			ss.str("");
			ss<<"vcCad2Cax::WriteNode() : NodeName : "<<NodeName.c_str();
			vcUtils::LogMsg(ss.str());
			ss.str("");
		}
	}
	else
	{
		if(g_bDetailedLog)
		{
			ss.str("");
			ss<<"vcCad2Cax::WriteNode() : NodeName : "<<NULL;
			vcUtils::LogMsg(ss.str());
			ss.str("");
		}
	}

#if 0
	if(inNode->GetPreview().IsNotNULL())
	{
		Dtk_PreviewPtr preview = inNode->GetPreview();
		char * stream = preview->GetStream();
		if(stream)
		{
			preview_type_detk type = preview->GetType();
/*PREVIEW_TYPE_DETK_UNKNOWN   
PREVIEW_TYPE_DETK_JPG   
PREVIEW_TYPE_DETK_BMP   
PREVIEW_TYPE_DETK_PNG   
PREVIEW_TYPE_DETK_CGM   
PREVIEW_TYPE_DETK_GIF   
PREVIEW_TYPE_DETK_TIFF   
PREVIEW_TYPE_DETK_ICO  */
			std::string ext;
			switch(type)
			{
			case PREVIEW_TYPE_DETK_JPG:
				ext = ".jpg";
				break;
			case PREVIEW_TYPE_DETK_BMP:
				ext = ".bmp";
				break;
			case PREVIEW_TYPE_DETK_PNG:
				ext = ".png";
				break;
			case PREVIEW_TYPE_DETK_CGM:
				ext = ".cgm";
				break;
			case PREVIEW_TYPE_DETK_GIF:
				ext = ".gif";
				break;
			case PREVIEW_TYPE_DETK_TIFF:
				ext = ".tiff";
				break;
			case PREVIEW_TYPE_DETK_ICO:
				ext = ".ico";
				break;
			case PREVIEW_TYPE_DETK_UNKNOWN:
				ext = ".unknown";
				break;
			}
			std::string filename = std::string("d:\\test")+ext;
			FILE *fp=fopen(filename.c_str(),"w");
			fprintf(fp,"%s",stream);
			fclose(fp);
		}
	}
#endif

	//Get The node Blanked Status
	// -1 = undefined, 0 = Visible, 1=Invisible, 2=Construction Geometry
	int NodeBlankedStatus=-1;
	int NodeInfiniteGeometry=-1;
	if(inNode->GetInfos().IsNotNULL())
	{
		NodeBlankedStatus = inNode->GetInfos()->GetBlankedStatus();
	    NodeInfiniteGeometry = inNode->GetInfos()->GetInfiniteGeometryFlag();
	}
	if(g_bDetailedLog)
	{
		ss.str("");
		ss<<"vcCad2Cax::WriteNode():NodeBlankedStatus: "<<NodeBlankedStatus;
		vcUtils::LogMsg(ss.str());

		ss.str("");
		ss<<"vcCad2Cax::WriteNode():NodeInfiniteGeometry: "<<NodeInfiniteGeometry;
		vcUtils::LogMsg(ss.str());
	}

	
	//if(NodeName.find_substring("Geometrical Set")!= -1 || NodeName.find_substring("WireFrame")!= -1)
		//return dtkNoError;
	if(NodeName.find_substring("Axis System")!= -1 )
		return dtkNoError;

	// Get Preview
	/*Dtk_PreviewPtr TmpPreview = inNode->GetPreview();
	if (TmpPreview.IsNotNULL())
	{
		Dtk_Int32 size = TmpPreview->GetStreamSize();
		char *jpgimage = (char *)malloc(size * sizeof(char));
		jpgimage = TmpPreview->GetStream();
		Dtk_string Preview_name = "NodePreview.jpg";
		FILE *jpg = Preview_name.OpenFile("w");
		if (jpg) 
		{
			fprintf(jpg,jpgimage,size);
			fclose(jpg);
		} 
	}*/ 


	//In this sample we read and treat only visible node
	if( NodeBlankedStatus == 0)
	//if( NodeBlankedStatus == 0 || NodeBlankedStatus == 1)
	//if( NodeBlankedStatus == 0 || NodeBlankedStatus == 2)
	//if(NodeBlankedStatus == 2)
	//if(NodeBlankedStatus != -1)
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

		Dtk_RGB Color;
		if(inNode->GetInfos().IsNotNULL())
		{
			Color = inNode->GetInfos()->GetColor();
			m_DtkComponentColorArray.push_back(Color);
			if(g_bDetailedLog)
			{
				ss.str("");
				ss<<"vcCad2Cax::WriteNode():inNode->GetInfos()->GetColor(): "<<Color.R()<<" "<<Color.G()<<" "<<Color.B();
				vcUtils::LogMsg(ss.str());
			}
		}
		/*if(Material.IsNotNULL())
		{
			double ambient = Material->ambient;
			double diffuse = Material->diffuse;
			double specular = Material->specular;
			double transparency = Material->transparency;
			double emissive = Material->reflectivity;
		}*/
		switch(NodeType) 
		{
			case Dtk_Node::BodyType:
			{
				//try
				{
					// Get if the node represent infinite geometry
					// 1 = Infinite, 0 = Finite
					int NodeInfiniteGeometry = inNode->GetInfos()->GetInfiniteGeometryFlag();
					if(g_bDetailedLog)
					{
						ss.str("");
						ss<<"vcCad2Cax::WriteNode():Dtk_Node::BodyType - NodeInfiniteGeometry:"<<NodeInfiniteGeometry;
						vcUtils::LogMsg(ss.str());
					}

					if (NodeInfiniteGeometry == 0)
					{
						if(g_bDetailedLog)
						{
							ss.str("");
							ss<<"vcCad2Cax::WriteNode():Dtk_Node::BodyType - Start";
							vcUtils::LogMsg(ss.str());
						}

						//Calling both methods GetDtk_MeshPtr() and GetDtk_BodyPtr() on BodyNode will give you the same result
						//Choose the one you need in order to avoid duplicated data
						const Dtk_BodyPtr TmpBody = inNode->GetDtk_BodyPtr();

						if (TmpBody.IsNotNULL())
						{
#ifdef PART_WISE_TESSELLATION
#ifdef ACTIVATE_TESSELATION_LIB
							StopWatch sw;
							sw.Restart();

							Dtk_pnt min, max;float fDistance = 0;

							Dtk_pnt min1, max1;
							//TmpBody->ComputeBoundingBox(min, max); 
							//fDistance = sqrt(((max[0] - min[0]) * (max[0] - min[0])) + ((max[1] - min[1]) * (max[1] - min[1])) + ((max[2] - min[2]) * (max[2] - min[2])));
							TmpBody->GetVertexBound(min, max);
							
							fDistance = sqrt(((max[0] - min[0]) * (max[0] - min[0])) + ((max[1] - min[1]) * (max[1] - min[1])) + ((max[2] - min[2]) * (max[2] - min[2])));

							/*Dtk_pnt min1, max1;
							TmpBody->GetVertexBound(min1, max1);
							if (fDistance == 0)*/
							{
								//TmpBody->GetVertexBound(min, max);
								//fDistance = sqrt(((max[0] - min[0]) * (max[0] - min[0])) + ((max[1] - min[1]) * (max[1] - min[1])) + ((max[2] - min[2]) * (max[2] - min[2])));
							}

							float fTessTol = 0.05;
							if (fDistance > 0)
								fTessTol = fDistance * g_fTessTolerance;

							std::stringstream ss;
							if (g_bDetailedLog)
							{
								ss.str("");
								ss << "BB Distance ********************************* :" << fDistance;
								vcUtils::LogMsg(ss.str());
													
								ss.str("");
								ss << "Tess Tolerance ****************************** :" << fTessTol;
							}	
							if (fTessTol > 5.0f)
							{
								fTessTol = 5.0f;
								if (g_bDetailedLog)
									ss << " ========> " << fTessTol;
							}
							if (fTessTol < 0.05f)
							{
								if (fDistance > 0.5)
								{
									fTessTol = 0.05f;
									if (g_bDetailedLog)
										ss << " ========> " << fTessTol;
								}
							}
							if (g_bDetailedLog)
								vcUtils::LogMsg(ss.str());

							if (g_bDetailedLog)
							{
								ss.str("");
								ss << "Time taken for BB (µs) ******************* :" << sw.ElapsedUs();
								vcUtils::LogMsg(ss.str());
							}

							int status;
							status = tess_InitTesselation("tess_tmp", fTessTol);
#endif
#endif
						}

						if(g_bDetailedLog)
						{
							ss.str("");
							ss<<"vcCad2Cax::WriteNode():Dtk_Node::BodyType - extracted Dtk_BodyPtr";
							vcUtils::LogMsg(ss.str());
						}

						if(TmpBody.IsNotNULL())
						{
							type_detk body_type = TmpBody->get_type_detk();
							if(g_bDetailedLog)
							{
								ss.str("");
								ss<<"vcCad2Cax::WriteNode():Dtk_Node::BodyType - "<< body_type;
								vcUtils::LogMsg(ss.str());
							}	
							
							if(g_b2DElements || g_bPointSet)
							{
								if(g_bDetailedLog)
								{
									ss.str("");
									ss<<"vcCad2Cax::WriteNode(): Branching to 2D Elements and Pointset methods";
									vcUtils::LogMsg(ss.str());
								}
								WriteBody(TmpBody);
								if(g_bDetailedLog)
								{
									ss.str("");
									ss<<"vcCad2Cax::WriteNode():2D Elements and PointSet handled";
									vcUtils::LogMsg(ss.str());
								}
							}
							else
							{
								if(g_bDetailedLog)
								{
									ss.str("");
									ss<<"vcCad2Cax::WriteNode():Export 2D Elements and PointSet are unchecked ";
									vcUtils::LogMsg(ss.str());
								}
							}
						}
						
						if(!g_bMesh)
						{
							if(g_bDetailedLog)
							{
								ss.str("");
								ss<<"vcCad2Cax::WriteNode():Export Mesh is unchecked ";
								vcUtils::LogMsg(ss.str());
							}
							break;
						}

						// Some CAD formats store also faceted data besides B-Rep. 
						// So you can get faceted data corresponding to the body using the following method
						const Dtk_MeshPtr TmpFacetedBody = inNode->GetDtk_MeshPtr();

						if(g_bDetailedLog)
						{
							ss.str("");
							ss<<"vcCad2Cax::WriteNode():Dtk_Node::BodyType - extracted Dtk_MeshPtr";
							vcUtils::LogMsg(ss.str());
						}

						if (TmpFacetedBody.IsNotNULL())
						{
							if(g_bDetailedLog)
							{
								ss.str("");
								ss<<"vcCad2Cax::WriteNode():Dtk_Node::BodyType - Mesh Data available...No tessellation is required";
								vcUtils::LogMsg(ss.str());
							}
							WriteDtk_Mesh(TmpFacetedBody,NodeName);
						}
						#ifdef ACTIVATE_TESSELATION_LIB
							// If there is no mesh data associated to the current body, you can tessellate 
							if(TmpFacetedBody.IsNULL() && TmpBody.IsNotNULL())
							{
								if(g_bDetailedLog)
								{
									ss.str("");
									ss<<"vcCad2Cax::WriteNode():Dtk_Node::BodyType - Manipulating Dtk_BodyPtr...";
									vcUtils::LogMsg(ss.str());
								}

								Dtk_tab<Dtk_MeshPtr> meshes;
								Dtk_tab<Dtk_Int32> isclosed;
								int err_tess = 0;
								try
								{
									err_tess = tess_BodyToMeshes(TmpBody, meshes,isclosed);
								}
								catch(...)
								{
									g_bDataLoss = true;
									ss.str("");
									ss<<"vcCad2Cax::WriteNode():Dtk_Node::BodyType : tess_BodyToMeshes() ***Exception*** caught";
									vcUtils::LogMsg(ss.str());
									break;
								}
								
								if(err_tess == 0)
								{
									if(g_bDetailedLog)
									{
										ss.str("");
										ss<<"vcCad2Cax::WriteNode():Dtk_Node::BodyType - Tessellation is done";
										vcUtils::LogMsg(ss.str());
									}

									Dtk_Size_t i,nbmeshes = meshes.size(); 
									
									if(g_bDetailedLog)
									{
										ss.str("");
										ss<<"vcCad2Cax::WriteNode():Dtk_Node::BodyType: No of Meshes: "<<nbmeshes;
										vcUtils::LogMsg(ss.str());
									}

									//for (i=0;i<nbmeshes;i++)
									{
										if(nbmeshes==1)
											WriteDtk_Mesh(meshes[0],NodeName);
										else if(nbmeshes>=1)
											WriteDtk_Meshes(meshes,NodeName);
									}    
								}
								else
								{
									if(g_bDetailedLog)
									{
										ss.str("");
										ss<<"vcCad2Cax::WriteNode():Dtk_Node::BodyType - Tessellation failed";
										vcUtils::LogMsg(ss.str());
									}
								}
							}
						#endif
						if(g_bDetailedLog)
						{
							ss.str("");
							ss<<"vcCad2Cax::WriteNode():Dtk_Node::BodyType - End";
							vcUtils::LogMsg(ss.str());
						}
					}
				}
				/*catch(...)
				{
					g_bDataLoss = true;
					ss.str("");
					ss<<"vcCad2Cax::WriteNode():Dtk_Node::BodyType ***Exception*** caught";
					vcUtils::LogMsg(ss.str());
					break;
				}*/
#ifdef PART_WISE_TESSELLATION
				tess_EndTesselation();
#endif
				break;
			}
			case Dtk_Node::AnnotationSetType:
			{
				if(g_bDetailedLog)
				{
					ss.str("");
					ss<<"vcCad2Cax::WriteNode():Dtk_Node::AnnotationSetType - Ignored";
					vcUtils::LogMsg(ss.str());
				}
				break;
			}
			case Dtk_Node::DrawingType:
			{
				if(g_bDetailedLog)
				{
					ss.str("");
					ss<<"vcCad2Cax::WriteNode():Dtk_Node::DrawingType - Ignored";
					vcUtils::LogMsg(ss.str());
				}
				break;
			}
			case Dtk_Node::MeshType:
			{
				if(g_bDetailedLog)
				{
					ss.str("");
					ss<<"vcCad2Cax::WriteNode():Dtk_Node::MeshType - Start";
					vcUtils::LogMsg(ss.str());
				}

				Dtk_MeshPtr TmpMesh;
				try
				{
					TmpMesh = inNode->GetDtk_MeshPtr();
				}
				catch(...)
				{
					g_bDataLoss = true;
					ss.str("");
					ss<<"vcCad2Cax::WriteNode():Dtk_Node::MeshType:inNode->GetDtk_MeshPtr() ***Exception*** caught";
					vcUtils::LogMsg(ss.str());
					break;
				}
				if (TmpMesh.IsNotNULL())
				{
					WriteDtk_Mesh(TmpMesh,NodeName);
				}

				if(g_bDetailedLog)
				{
					ss.str("");
					ss<<"vcCad2Cax::WriteNode():Dtk_Node::MeshType - End";
					vcUtils::LogMsg(ss.str());
				}
				break;
			}
			case Dtk_Node::AxisSystemType:
			{
				if(g_bDetailedLog)
				{
					ss.str("");
					ss<<"vcCad2Cax::WriteNode():Dtk_Node::AxisSystemType - Ignored";
					vcUtils::LogMsg(ss.str());
				}
				break;
			}
			case Dtk_Node::KinematicsType:
			{
				if(g_bDetailedLog)
				{
					ss.str("");
					ss<<"vcCad2Cax::WriteNode():Dtk_Node::KinematicsType - Ignored";
					vcUtils::LogMsg(ss.str());
				}
				break;
			}
			case Dtk_Node::LayerInfosSetType:
			{
				if(g_bDetailedLog)
				{
					ss.str("");
					ss<<"vcCad2Cax::WriteNode():Dtk_Node::LayerInfosSetType - Ignored";
					vcUtils::LogMsg(ss.str());
				}
				break;
			}
			case Dtk_Node::MetaDataType:
			{
				if(g_bDetailedLog)
				{
					ss.str("");
					ss<<"vcCad2Cax::WriteNode():Dtk_Node::MetaDataType - Ignored";
					vcUtils::LogMsg(ss.str());
				}
				break;
			}
			case Dtk_Node::VirtualType:
			{
				try
				{
					if(inNode->GetNumChildren())
					{
						if(g_bDetailedLog)
						{
							ss.str("");
							ss<<"vcCad2Cax::WriteNode():Dtk_Node::VirtualType - Start: ";
							vcUtils::LogMsg(ss.str());

							ss.str("");
							ss << "vcCad2Cax::WriteNode():Dtk_Node::VirtualType - NumChildren: "<< inNode->GetNumChildren();;
							vcUtils::LogMsg(ss.str());

						}

						/*if(m_ReaderType == DtkReaderType::V5ReaderModule)
						{
							//CAX-Group
							CaxId CaxGroup = m_CaxScene.CreateObject(GetValidNodeName(NodeName.c_str()),Cax::GROUP);
							CaxId CurrentCaxParent = m_NodesArray.at(m_NodesArray.size()-1);
							m_CaxScene.AppendChild(CurrentCaxParent,CaxGroup);
							m_NodesArray.push_back(CaxGroup);
						}*/
						
						Dtk_Size_t i, NumChildren;
						NumChildren = inNode->GetNumChildren();

						/*if(NumChildren && !m_bSinglePart)
						{
							m_DtkColorNLinesArrayMap.clear();
							m_bSinglePart = true;
						}*/
						 
						for (i = 0; i < NumChildren; i++)
						{
							WriteNode(inNode->GetChild(i));
						}    

						/*if(m_ReaderType == DtkReaderType::V5ReaderModule)
						{
							//CAX
							if(m_NodesArray.size())
								m_NodesArray.pop_back();
							//
						}*/

						if(g_bDetailedLog)
						{
							ss.str("");
							ss<<"vcCad2Cax::WriteNode():Dtk_Node::VirtualType - End: ";
							vcUtils::LogMsg(ss.str());
						}
					}
				}
				catch(...)
				{
					g_bDataLoss = true;
					ss.str("");
					ss<<"vcCad2Cax::WriteNode():Dtk_Node::VirtualType ***Exception*** caught ";
					vcUtils::LogMsg(ss.str());
					break;
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
	if(m_ReaderType == DtkReaderType::UgReaderModule || m_ReaderType == DtkReaderType::SwReaderModule || m_ReaderType == DtkReaderType::SeReaderModule)
	if (NodeBlankedStatus != 1)
	{
		if(g_bDetailedLog)
		{
			ss.str("");
			ss<<"vcCad2Cax::WriteNode():recursive TreeNode for Visible and Construction node - Start: ";
			vcUtils::LogMsg(ss.str());
		}

		Dtk_Size_t i, NumChildren;
		NumChildren = inNode->GetNumChildren();
 
		//m_bSinglePart = false;
		if(NumChildren && !m_bSinglePart)
		{
			m_bSinglePart = true;
		}

		for (i = 0; i < NumChildren; i++)
		{
			//WriteNode(inNode->GetChild(i));
		}   

		if(g_bDetailedLog)
		{
			ss.str("");
			ss<<"vcCad2Cax::WriteNode():recursive TreeNode for Visible and Construction node - End: ";
			vcUtils::LogMsg(ss.str());
		}
	}

	// Get the Feature associated to the current node, if exists,
	// otherwise a NULL pointer is returned
	//long * TabOriginalGeometriesIDS;
	//int NbOriginalGeometriesIDS = 0;
	//Dtk_FeaturePtr CurrentFeature = inNode->GetDtk_FeaturePtr();
	//if (CurrentFeature.IsNotNULL())
	//{    
	//	DTK_FEATURE_TYPE FeatureType;
	//	CurrentFeature->get_type (&FeatureType);

	//	if (FeatureType == DTK_FEAT_SYMMETRY) 
	//	{
	//		  Dtk_feat_symmetry * CurrentSymmetry;
	//		  CurrentFeature->get_symmetry (&CurrentSymmetry);
	//		  // get original geometry IDs that are being applied the symmetry
	//		 /* long * TabOriginalGeometriesIDS;
	//		  int NbOriginalGeometriesIDS = 0;*/
	//		  Dtk_feat_geometry * OriginalGeometries;
	//		  CurrentSymmetry->get_reference (&OriginalGeometries);
	//		  OriginalGeometries->get_ids (&TabOriginalGeometriesIDS, &NbOriginalGeometriesIDS);
	//	}		
	//}

	if (!g_bCombinePartsInGroup)
	{
		m_bSinglePart = false;

		m_iShapeIndexOfComponent = 0;
		if (g_bMesh)
		{
			StopWatch sw;
			if (g_bDetailedLog)
			{
				sw.Restart();
			}

			this->ConstructCaxShape(m_DtkColorNFaceArrayMap, NodeName.c_str());

			if (g_bDetailedLog)
			{
				std::stringstream ss;
				ss.str("");
				ss << "Time taken for ConstructCaxShape******************* :" << sw.ElapsedUs();
				vcUtils::LogMsg(ss.str());
			}
		}
		if (g_b2DElements)
			ConstructShapeFor2DElements(NodeName.c_str());
		if (g_bPointSet)
			ConstructShapeForPointElements(NodeName.c_str());
	}
	return dtkNoError;
} 

void vcCad2Cax::WriteDtk_Mesh(const Dtk_MeshPtr& inMeshToWrite,Dtk_string NodeName)
{
	if(inMeshToWrite.IsNULL())
		return;

	if(g_bDetailedLog)
	{
		vcUtils::LogMsg("vcCad2Cax::WriteDtk_Mesh():Start...");

		ss.str("");
		int len = NodeName.len();
		if(NodeName.is_not_NULL())
		{
			ss<<"vcCad2Cax::WriteDtk_Mesh():Shape Name: "<<NodeName.c_str();
		}
		else
		{
			ss<<"vcCad2Cax::WriteDtk_Mesh():Shape Name: NoName is present for this mesh";
		}
		vcUtils::LogMsg(ss.str());
	}

	/*int mesh_type = inMeshToWrite->get_type_detk();
	if(mesh_type!=1013)
	{
		//AfxMessageBox("Its not a mesh");
	}
	std::string sMeshTypeName = GetMeshType(inMeshToWrite->get_type_detk());
	if(g_bDetailedLog)
	{
		ss.str("");
		ss<<"vcCad2Cax::WriteDtk_Mesh():Mesh Type: "<<sMeshTypeName;
		vcUtils::LogMsg(ss.str());
	}*/

	bool bVertexColor = 1; 
	m_iFaceIndex = 0;

	try
	{
		// convert triangles-strip, fans, polygons into simple triangles
		inMeshToWrite->explode();  
	}
	catch(...)
	{
		g_bDataLoss = true;
		ss.str("");
		ss<<"vcCad2Cax::WriteDtk_Mesh():Mesh Type: inMeshToWrite->explode() ***Exception*** caught";
		vcUtils::LogMsg(ss.str());
		return;
	}

	Dtk_Size_t k=0,m=0,nbmeshfaces = inMeshToWrite->get_nb_mesh_face();
	if(g_bDetailedLog)
	{
		ss.str("");
		ss<<"vcCad2Cax::WriteDtk_Mesh():No. of DATAKIT Faces: "<<nbmeshfaces;
		vcUtils::LogMsg(ss.str());
	}

	double tranparency=0.0f;
	Dtk_RGB *ComponentColor= new Dtk_RGB();
	ComponentColor->SetRGBA(255,255,255);

	//To check whether component color or face color should be applied
	bool bComponentColor = false;
#if 1
	//if(m_ReaderType == DtkReaderType::V5ReaderModule || m_ReaderType == DtkReaderType::V4ReaderModule)
	bComponentColor = IsComponentColor(ComponentColor);
#endif

	//bComponentColor = false;

	//if(!bComponentColor)
		//ComponentColor = NULL;

	//Extract the mesh faces with corresponding color
	//std::map< Dtk_RGB*, std::pair< std::vector<Dtk_mesh_face*> *,std::vector<Dtk_MeshPtr> > > DtkColorNFaceArrayMap;
	//std::map< Dtk_RGB*, int> DtkColorNVerticesCountMap;
#if 0
	if(bComponentColor)
		GetMeshFacesWithComponentColor(inMeshToWrite,m_DtkColorNFaceArrayMap,ComponentColor,m_DtkColorNVerticesCountMap);
	else
#endif
	StopWatch sw;
	if(g_bDetailedLog)
	{
		sw.Restart();
	}

	GetMeshFacesWithFaceColor(inMeshToWrite,m_DtkColorNFaceArrayMap,m_DtkColorNVerticesCountMap,ComponentColor);

	if(g_bDetailedLog)
	{
		std::stringstream ss;
		ss.str("");
		ss<<"Time taken for GetMeshFacesWithFaceColor******************* :"<<sw.ElapsedUs();
		vcUtils::LogMsg(ss.str());
	}

	//if(!m_bSinglePart)
		//ConstructCaxShape(DtkColorNFaceArrayMap,NodeName);

	if(g_bDetailedLog)
	{
		vcUtils::LogMsg("vcCad2Cax::WriteDtk_Mesh():End");
	}
}



void vcCad2Cax::WriteDtk_Meshes(Dtk_tab<Dtk_MeshPtr> &meshes,Dtk_string NodeName)
{
	if(meshes.size() == 0)
		return;

	if(g_bDetailedLog)
	{
		vcUtils::LogMsg("vcCad2Cax::WriteDtk_Meshes():Start...");

		ss.str("");
		int len = NodeName.len();
		if(NodeName.is_not_NULL())
		{
			ss<<"vcCad2Cax::WriteDtk_Mesh():Shape Name: "<<NodeName.c_str();
		}
		else
		{
			ss<<"vcCad2Cax::WriteDtk_Mesh():Shape Name: NoName is present for this mesh";
		}
		vcUtils::LogMsg(ss.str());
	}

	bool bVertexColor = 1; 
	m_iFaceIndex = 0;
	double tranparency=0.0f;
	Dtk_RGB *ComponentColor = new Dtk_RGB(255,255,255);

	//To check whether component color or face color should be applied
	bool bComponentColor = false;
#if 1
	//if(m_ReaderType == DtkReaderType::V5ReaderModule || m_ReaderType == DtkReaderType::V4ReaderModule)
		bComponentColor = IsComponentColor(ComponentColor);
#endif

	//bComponentColor = false;

	if(!bComponentColor)
		ComponentColor = NULL;

	//Extract the mesh faces with corresponding color
	//std::map< Dtk_RGB*, std::pair<std::vector<Dtk_mesh_face*> *,std::vector<Dtk_MeshPtr>>> DtkColorNFaceArrayMap;
	//std::map< Dtk_RGB*, int> DtkColorNVerticesCountMap;
	for(int i=0;i<meshes.size();i++)
	{
		// convert triangles-strip, fans, polygons into simple triangles
		meshes[i]->explode();
		Dtk_Size_t nbmeshfaces = meshes[i]->get_nb_mesh_face();
		if(g_bDetailedLog)
		{
			ss.str("");
			ss<<"vcCad2Cax::WriteDtk_Meshes():No. of DATAKIT Faces in Mesh "<<i+1<<" : "<<nbmeshfaces;
			vcUtils::LogMsg(ss.str());
		}
#if 0
		if(bComponentColor)
			GetMeshFacesWithComponentColor(inMeshToWrite,DtkColorNFaceArrayMap,ComponentColor,DtkColorNVerticesCountMap);
		else
#endif
			GetMeshFacesWithFaceColor(meshes[i],m_DtkColorNFaceArrayMap,m_DtkColorNVerticesCountMap,ComponentColor);
	}

	//if(!m_bSinglePart)
		//ConstructCaxShape(DtkColorNFaceArrayMap,NodeName);

	if(g_bDetailedLog)
	{
		vcUtils::LogMsg("vcCad2Cax::WriteDtk_Meshes():End");	
	}
}

void vcCad2Cax::ConstructCaxShape(std::map< Dtk_RGB*, std::pair<std::vector<Dtk_mesh_face*> *,std::vector<Dtk_MeshPtr>>> &DtkColorNFaceArrayMap,Dtk_string NodeName)
{
	try
	{
		if(g_bDetailedLog)
		{
			vcUtils::LogMsg("vcCad2Cax::ConstructCaxShape():Start...");
		}

		std::string memusage;
		if(g_bDetailedLog)
		{
			memusage = vcUtils::GetMemoryUsage();
			ss.str("");
			ss<<"vcCad2Cax::ConstructCaxShape():Memory Usage b4 creating CAX geometry: "<<memusage;
			vcUtils::LogMsg(ss.str());
		}

		int iTotalVerticesInShape = 0,iTotalIndicesInShape = 0, iTotalPrimsInShape = 0;
		std::vector<std::string> MaterialStrArray;

		std::map< Dtk_RGB*, std::pair<std::vector<Dtk_mesh_face*> *,std::vector<Dtk_MeshPtr>>>::iterator MI;
		std::map< Dtk_RGB*, int>::iterator VerticesCountIterator;
		Dtk_RGB diffuseColor;
		diffuseColor.SetRGBA(0,0,0,0);
		Dtk_mesh_face* mf =  NULL;
		Dtk_Size_t nbtriangles = 0;
		int iVerticesCount = 0;
		int iPrimCount=0;

		if(g_bDetailedLog)
		{
			ss.str("");
			ss<<"vcCad2Cax::ConstructCaxShape():No. of Cax shapes: "<<DtkColorNFaceArrayMap.size();
			vcUtils::LogMsg(ss.str());
		}

		int loop=0;
		int iShapeIndex=0;

		if(DtkColorNFaceArrayMap.size()>1)
		{
			//CAX-Group
			CaxId CaxGroup = m_CaxScene.createObject(Cax::ASSEMBLY);
			m_CaxScene.setName(CaxGroup,GetValidNodeName(NodeName.c_str()).c_str());
			CaxId CurrentCaxParent = m_NodesArray.at(m_NodesArray.size()-1);
			m_CaxScene.addComponent(CurrentCaxParent,CaxGroup);
			m_NodesArray.push_back(CaxGroup);
		}
		std::string sShapeName;
		std::map<std::string,CaxId>::iterator NameNCaxIdMapIterator;
		for(MI=DtkColorNFaceArrayMap.begin();MI!=DtkColorNFaceArrayMap.end();MI++)
		{
			if(g_bDetailedLog)
			{
				ss.str("");
				ss<<"vcCad2Cax::ConstructCaxShape():Shape : "<<iShapeIndex;
				vcUtils::LogMsg(ss.str());
			}

			if(!MI->first)
				continue;

			iVerticesCount = 0;
			iPrimCount=0;
			diffuseColor.SetRGBA(MI->first->R(),MI->first->G(),MI->first->B(),MI->first->A());

			if(g_bDetailedLog)
			{
				ss.str("");
				ss<<"vcCad2Cax::ConstructCaxShape(): Shape Color: "<<diffuseColor.R()<<" "<<diffuseColor.G()<<" "<<diffuseColor.B()<<" "<<diffuseColor.A();
				vcUtils::LogMsg(ss.str());
			}

			if(!MI->second.first)
				continue;

			std::pair<std::vector<Dtk_mesh_face*> *,std::vector<Dtk_MeshPtr>> pair;
			pair = MI->second;
			std::vector<Dtk_mesh_face*> *FaceArray = pair.first;
			std::vector<Dtk_MeshPtr> MeshPtrArr = pair.second;
			//std::vector<Dtk_mesh_face*> *FaceArray = MI->second;
			for(int i=0;i<FaceArray->size();i++)
			{
				Dtk_mesh_face* mf = (Dtk_mesh_face*)FaceArray->at(i);
				if(!mf)
					continue;
				
				DTK_MESH_TYPE_FACE mtf=mf->get_face_type();

				Dtk_Size_t nbtriangles = mf->get_nbtriangles();
				iVerticesCount+=nbtriangles*3;
				iPrimCount+=nbtriangles;
			}

			if(iVerticesCount==0) 
				continue;

			iShapeIndex++;
			m_iShapeIndexOfComponent++;
			ss.str("");
			ss<<NodeName;//<<"#"<<m_iShapeIndexOfComponent;
			sShapeName = ss.str();

#ifdef VCT_DEF_AND_USE
			//if(!m_bNegativeScale)
			{
				NameNCaxIdMapIterator = m_NameNCaxIdMap.find(sShapeName);
				if(NameNCaxIdMapIterator!=m_NameNCaxIdMap.end())
				{
					CaxId caxId = NameNCaxIdMapIterator->second;
					CaxId CurrentCaxParent = m_NodesArray.at(m_NodesArray.size()-1);
					m_CaxScene.addComponent(CurrentCaxParent,caxId);
					if(g_bDetailedLog)
					{
						ss.str("");
						ss<<"vcCad2Cax::ConstructCaxShape(): Shape : "<<sShapeName<<" is reused";
						vcUtils::LogMsg(ss.str());
					}
					continue;
				}
			}
#endif

			if(g_bDetailedLog)
			{
				ss.str("");
				ss<<"vcCad2Cax::ConstructCaxShape(): Coordinates size : "<<iVerticesCount;
				vcUtils::LogMsg(ss.str());

				ss.str("");
				ss<<"vcCad2Cax::ConstructCaxShape(): Primitive size : "<<iPrimCount;
				vcUtils::LogMsg(ss.str());
			}

			float *pCaxCoordFloatArray = NULL; 
			float *pCaxColorFloatArray = NULL;
	#if 0
			float *pCaxNormalFloatArray = NULL;
	#endif

			std::vector<uint32_t> pCaxCoordIndexIntArray;
			pCaxCoordIndexIntArray.resize(iVerticesCount);

			std::vector<uint32_t> pCaxPolylenghtIntArray;
			pCaxPolylenghtIntArray.resize(iPrimCount);

			uint32_t iIndex=0;

			int iCoordIndexCount=0;
			int iCoordIndex=0;
			int iPolylengthIndex = 0;
			int iCoordSetIndex = 0;
			int iNormalSetIndex = 0;
			int iColorSetIndex = 0;
			Dtk_pnt pnt;
			Dtk_dir dir;
			Dtk_RGB color;
			float u,v;
			u=v=0.0f;
			
			StopWatch sw;
			sw.Restart();
			std::vector<vcVec> OriCoordSet;
			OriCoordSet.resize(iVerticesCount);
			std::vector<vcVec> OriColorSet;

			if(!m_bNegativeScale)
			{
				if(g_bDetailedLog)
				{
					ss.str("");
					ss<<"vcCad2Cax::ConstructCaxShape(): Regular Tx case";
					vcUtils::LogMsg(ss.str());
				}

				Dtk_MeshPtr inMeshToWrite;
				uint32_t k=0;

				uint32_t cur_tri_count=0;
				for(uint32_t i=0;i<FaceArray->size();i++)
				{
					mf = (Dtk_mesh_face*)FaceArray->at(i);
					inMeshToWrite = MeshPtrArr[i];
					if(!mf)
						continue;
					nbtriangles = mf->get_nbtriangles();
					int tri_index = (cur_tri_count*3);
					#pragma omp parallel for
					for(uint32_t j=0;j<nbtriangles;j++)
					{
						const Dtk_UInt32* tri = mf->get_triangle_indices(j);
									
						uint32_t index = tri_index + (j*3);

						//Coordinates
						vcVec vec;
						inMeshToWrite->get_vertex(tri[0],&pnt);  
						vec.x = pnt[0];
						vec.y = pnt[1];
						vec.z = pnt[2];
						OriCoordSet[index]=vec;
						
						inMeshToWrite->get_vertex(tri[1],&pnt);  
						vec.x = pnt[0];
						vec.y = pnt[1]; 
						vec.z = pnt[2];
						OriCoordSet[index+1]=vec;
						
						inMeshToWrite->get_vertex(tri[2],&pnt);  
						vec.x = pnt[0];
						vec.y = pnt[1];
						vec.z = pnt[2];
						OriCoordSet[index+2]=vec;

						//Setting Normals
						/*if(inMeshToWrite->has_normals())
						{
							inMeshToWrite->get_normal(tri[0],&dir);  // vertex normal (if enabled)
								pCaxNormalFloatArray[iNormalSetIndex++] = dir[0];
								pCaxNormalFloatArray[iNormalSetIndex++] = dir[1];
								pCaxNormalFloatArray[iNormalSetIndex++] = dir[2];
							inMeshToWrite->get_normal(tri[1],&dir);  // vertex normal (if enabled)
								pCaxNormalFloatArray[iNormalSetIndex++] = dir[0];
								pCaxNormalFloatArray[iNormalSetIndex++] = dir[1];
								pCaxNormalFloatArray[iNormalSetIndex++] = dir[2];
							inMeshToWrite->get_normal(tri[2],&dir);  // vertex normal (if enabled)
								pCaxNormalFloatArray[iNormalSetIndex++] = dir[0];
								pCaxNormalFloatArray[iNormalSetIndex++] = dir[1];
								pCaxNormalFloatArray[iNormalSetIndex++] = dir[2];
						}*/

						//Setting Colors
						//Vertex Color is commented..Assuming that it may not be required...181206
						/*if (inMeshToWrite->has_colors())
						{
							inMeshToWrite->get_color(tri[0],&color);  // vertex color (if enabled)
							vec.x = color[0];
							vec.y = color[1];
							vec.z = color[2];
							OriColorSet.push_back(vec);

							inMeshToWrite->get_color(tri[1],&color);  // vertex color (if enabled)
							vec.x = color[0];
							vec.y = color[1];
							vec.z = color[2];
							OriColorSet.push_back(vec);

							inMeshToWrite->get_color(tri[2],&color);  // vertex color (if enabled)
							vec.x = color[0];
							vec.y = color[1];
							vec.z = color[2];
							OriColorSet.push_back(vec);
						}*/

						//Setting Texture Coordinates
						//if (inMeshToWrite->has_texcoords())
						//{
						//   u = inMeshToWrite->GetU(i);  // vertex u,v texture coordinate (if enabled)
						//   v = inMeshToWrite->GetV(i);
						//}
						
						//Connectivity
						pCaxCoordIndexIntArray[index] = index;
						pCaxCoordIndexIntArray[index+1] = index+1;
						pCaxCoordIndexIntArray[index+2] = index+2;

						//Polylength
						pCaxPolylenghtIntArray[index/3] = 3;
					}
					cur_tri_count += nbtriangles;
				}
				std::stringstream ss;
				if(g_bDetailedLog)
				{				
					ss.str("");
					ss<<"Time taken for CSet******************* :"<<sw.ElapsedUs();
					vcUtils::LogMsg(ss.str());

					ss.str("");
					ss<<"vcCad2Cax::ConstructCaxShape(): Regular Tx case completed";
					vcUtils::LogMsg(ss.str());
				}
			}
			else//NegativeScale(mirror)
			{
				if(g_bDetailedLog)
				{
					ss.str("");
					ss<<"vcCad2Cax::ConstructCaxShape(): Tx with Negative Scale case";
					vcUtils::LogMsg(ss.str());
				}

				Dtk_MeshPtr inMeshToWrite;
				uint32_t k=0;
				uint32_t cur_tri_count=0;

				for(uint32_t i=0;i<FaceArray->size();i++)
				{
					mf = (Dtk_mesh_face*)FaceArray->at(i);
					inMeshToWrite = MeshPtrArr[i]; 
					if(!mf)
						continue;
					nbtriangles = mf->get_nbtriangles();
					int tri_index = (cur_tri_count*3);
					
					#pragma omp parallel for
					for(int j=0;j<nbtriangles;j++)
					{
						//pCaxPolylenghtIntArray[iPolylengthIndex++] = 3;
						const Dtk_UInt32* tri = mf->get_triangle_indices(j);
						
						uint32_t index = tri_index + (j*3);
						
						//Setting Coordinates
						vcVec vec;
						inMeshToWrite->get_vertex(tri[0],&pnt);  
						vec.x = pnt[0];// *m_NegativeScaleVec[0];
						vec.y = pnt[1];//* m_NegativeScaleVec[1];
						vec.z = pnt[2];//* m_NegativeScaleVec[2];
						//OriCoordSet.push_back(vec);
						OriCoordSet[index] = vec;

						inMeshToWrite->get_vertex(tri[1],&pnt);  
						vec.x = pnt[0];//* m_NegativeScaleVec[0];
						vec.y = pnt[1];//* m_NegativeScaleVec[1];
						vec.z = pnt[2];//* m_NegativeScaleVec[2];
						//OriCoordSet.push_back(vec);
						OriCoordSet[index + 1] = vec;

						inMeshToWrite->get_vertex(tri[2],&pnt);  
						vec.x = pnt[0];//* m_NegativeScaleVec[0];
						vec.y = pnt[1];//* m_NegativeScaleVec[1];
						vec.z = pnt[2];//* m_NegativeScaleVec[2];
						//OriCoordSet.push_back(vec);
						OriCoordSet[index + 2] = vec;
					
						//Setting Normals
						/*if(inMeshToWrite->has_normals())
						{
							inMeshToWrite->get_normal(tri[0],&dir);  // vertex normal (if enabled)
								pCaxNormalFloatArray[iNormalSetIndex++] = dir[0];
								pCaxNormalFloatArray[iNormalSetIndex++] = dir[1];
								pCaxNormalFloatArray[iNormalSetIndex++] = dir[2];
							inMeshToWrite->get_normal(tri[1],&dir);  // vertex normal (if enabled)
								pCaxNormalFloatArray[iNormalSetIndex++] = dir[0];
								pCaxNormalFloatArray[iNormalSetIndex++] = dir[1];
								pCaxNormalFloatArray[iNormalSetIndex++] = dir[2];
							inMeshToWrite->get_normal(tri[2],&dir);  // vertex normal (if enabled)
								pCaxNormalFloatArray[iNormalSetIndex++] = dir[0];
								pCaxNormalFloatArray[iNormalSetIndex++] = dir[1];
								pCaxNormalFloatArray[iNormalSetIndex++] = dir[2];
						}*/

						//Setting Colors
						//Vertex Color is commented..Assuming that it may not be required...181206
						/*if (inMeshToWrite->has_colors())
						{
							inMeshToWrite->get_color(tri[0],&color);  // vertex color (if enabled)
							vec.x = color[0];
							vec.y = color[1];
							vec.z = color[2];
							OriColorSet.push_back(vec);

							inMeshToWrite->get_color(tri[1],&color);  // vertex color (if enabled)
							vec.x = color[0];
							vec.y = color[1];
							vec.z = color[2];
							OriColorSet.push_back(vec);

							inMeshToWrite->get_color(tri[2],&color);  // vertex color (if enabled)
							vec.x = color[0];
							vec.y = color[1];
							vec.z = color[2];
							OriColorSet.push_back(vec);
						}*/

						//Setting Texture Coordinates
						//if (inMeshToWrite->has_texcoords())
						//{
						//   u = inMeshToWrite->GetU(i);  // vertex u,v texture coordinate (if enabled)
						//   v = inMeshToWrite->GetV(i);
						//} 

						/*pCaxCoordIndexIntArray[iCoordIndexCount++] = iCoordIndex;
						pCaxCoordIndexIntArray[iCoordIndexCount++] = iCoordIndex+1;
						pCaxCoordIndexIntArray[iCoordIndexCount++] = iCoordIndex+2;
						iCoordIndex+=3;*/
						//Connectivity
						pCaxCoordIndexIntArray[index] = index;
						pCaxCoordIndexIntArray[index + 1] = index + 1;
						pCaxCoordIndexIntArray[index + 2] = index + 2;

						//Polylength
						pCaxPolylenghtIntArray[index / 3] = 3;
					}
					cur_tri_count += nbtriangles;
				} 

				if(g_bDetailedLog)
				{
					ss.str("");
					ss<<"vcCad2Cax::ConstructCaxShape(): Tx with Negative Scale case completed";
					vcUtils::LogMsg(ss.str());
				}
			}
		#if 0
					//face boundaries
					Dtk_Size_t nbpolyline = mf->get_nbpolylines();
					for (int j=0;j<nbpolyline;j++)
					{ 
					   const Dtk_tab<Dtk_UInt32> *pnt;
					   pnt = mf->get_polyline_indices(j);
					   Dtk_Size_t nbvertex = pnt->size();
					   for (k=0;k<nbvertex;k++)
					   {
							m = (*pnt)[k];  // id of vertex in vertices table
							Dtk_pnt pt = inMeshToWrite->get_vertex(m);
					   }
					}
		#endif

			if(g_bDetailedLog)
			{
				ss.str(""); 
				ss<<"vcCad2Cax::ConstructCaxShape(): coordinates and connectivity are done";
				vcUtils::LogMsg(ss.str());
			}

			if(g_bDetailedLog)
			{
				sw.Restart();
			}

			//Remove the Duplicate Vertices
			std::vector<float> newCoordSet; 
			std::vector<float> newColorSet; 
			vcRemoveDuplicateVertices::RemoveDuplicateVertices(OriCoordSet,newCoordSet,&pCaxCoordIndexIntArray[0],OriCoordSet.size(),OriColorSet,newColorSet,m_fUnit); 
			
			std::stringstream ss;
			if(g_bDetailedLog)
			{
				ss.str("");
				ss<<"Time taken for RDV******************* :"<<sw.ElapsedUs();
				vcUtils::LogMsg(ss.str());
			}

			
			if(g_bDetailedLog)
			{
				ss.str("");
				ss<<"vcCad2Cax::ConstructCaxShape(): RemoveDuplicateVertices completed";
				vcUtils::LogMsg(ss.str());

				ss.str("");
				ss<<"vcCad2Cax::ConstructCaxShape(): No of vertices           :  "<<OriCoordSet.size();
				vcUtils::LogMsg(ss.str());

				ss.str("");
				ss<<"vcCad2Cax::ConstructCaxShape(): No of vertices after RDV :  "<<newCoordSet.size();
				vcUtils::LogMsg(ss.str());
			}
#if 0
			if(newCoordSet.size() && newColorSet.size())
			{
				if(g_bDetailedLog)
				{
					ss.str("");
					ss<<"vcCad2Cax::ConstructCaxShape(): Assigning new coordset and colorset...";
					vcUtils::LogMsg(ss.str());
				}

				pCaxCoordFloatArray = new float[newCoordSet.size()*3];
				pCaxColorFloatArray = new float[newCoordSet.size()*3];
				int ii=0;
				/*if(this->m_iUnit == vcCad2Cax::MILLIMETER)
				{
					for(int i=0;i<newCoordSet.size();i++)
					{
						vcVec v = newCoordSet[i];
						pCaxCoordFloatArray[ii]   = v.x;
						pCaxCoordFloatArray[ii+1] = v.y;
						pCaxCoordFloatArray[ii+2] = v.z;
						  
						v = newColorSet[i];
						pCaxColorFloatArray[ii]   = v.x;
						pCaxColorFloatArray[ii+1] = v.y;
						pCaxColorFloatArray[ii+2] = v.z;

						ii+=3;
					}
				}
				else*/
				{
					for(int i=0;i<newCoordSet.size();i++)
					{
						vcVec v = newCoordSet[i];
						pCaxCoordFloatArray[ii]   = v.x*m_fUnit;
						pCaxCoordFloatArray[ii+1] = v.y*m_fUnit;
						pCaxCoordFloatArray[ii+2] = v.z*m_fUnit;
						  
						v = newColorSet[i];
						pCaxColorFloatArray[ii]   = v.x;
						pCaxColorFloatArray[ii+1] = v.y;
						pCaxColorFloatArray[ii+2] = v.z;

						ii+=3;
					}
				}
				iVerticesCount = newCoordSet.size();
				FloatArrays.push_back(pCaxCoordFloatArray);
				FloatArrays.push_back(pCaxColorFloatArray);
				
				if(g_bDetailedLog)
				{
					ss.str("");
					ss<<"vcCad2Cax::ConstructCaxShape(): new coordset and colorset are assigned";
					vcUtils::LogMsg(ss.str());
				}
			}
			else if(newCoordSet.size())
			{
				if(g_bDetailedLog)
				{
					ss.str("");
					ss<<"vcCad2Cax::ConstructCaxShape(): Assigning new coordset...";
					vcUtils::LogMsg(ss.str());
				}

				pCaxCoordFloatArray = new float[newCoordSet.size()*3];
				int ii=0;
				/*if(this->m_iUnit == vcCad2Cax::MILLIMETER)
				{
					for(int i=0;i<newCoordSet.size();i++)
					{
						vcVec v = newCoordSet[i];
						pCaxCoordFloatArray[ii]   = v.x;
						pCaxCoordFloatArray[ii+1] = v.y;
						pCaxCoordFloatArray[ii+2] = v.z;
						ii+=3;
					}		
				}
				else*/
				{
					for(int i=0;i<newCoordSet.size();i++)
					{
						vcVec v = newCoordSet[i];
						pCaxCoordFloatArray[ii]   = v.x*m_fUnit;
						pCaxCoordFloatArray[ii+1] = v.y*m_fUnit;
						pCaxCoordFloatArray[ii+2] = v.z*m_fUnit;
						ii+=3;
					}
				}
				iVerticesCount = newCoordSet.size();
				FloatArrays.push_back(pCaxCoordFloatArray);

				if(g_bDetailedLog)
				{
					ss.str("");
					ss<<"vcCad2Cax::ConstructCaxShape(): new coordset is assigned";
					vcUtils::LogMsg(ss.str());
				}
			}
#endif
			//End


			sw.Restart();
			pCaxCoordFloatArray = &newCoordSet[0];
			iVerticesCount = newCoordSet.size()/3;


			CaxId CaxShape;
			CaxShape = m_CaxScene.createObject(Cax::GEOMETRY);
			m_CaxScene.setName(CaxShape,GetValidNodeName(sShapeName.c_str()).c_str());

#ifdef VCT_DEF_AND_USE
			//if(!m_bNegativeScale)
				m_NameNCaxIdMap[sShapeName] = CaxShape;
#endif
			
			if(g_bDetailedLog)
			{
				ss.str("");
				ss<<"vcCad2Cax::ConstructCaxShape():Cax Shape object is created ";
				vcUtils::LogMsg(ss.str());
			}

			CaxId CurrentCaxParent = m_NodesArray.at(m_NodesArray.size()-1);
			m_CaxScene.addComponent(CurrentCaxParent,CaxShape);
			
			CaxId CaxMesh;
			CaxMesh = m_CaxScene.createObject(Cax::MESH);
			m_CaxScene.setMeshType(CaxMesh,Cax::SURFACE_MESH);
			m_CaxScene.setName(CaxMesh,GetValidNodeName("mesh").c_str());
			m_CaxScene.addComponent(CaxShape,CaxMesh);

			if(g_bDetailedLog)
			{
				ss.str("");
				ss<<"vcCad2Cax::ConstructCaxShape():Cax Shape is added to Cax Scene ";
				vcUtils::LogMsg(ss.str());
			}

			CaxId CaxCoordSet = m_CaxScene.createObject(Cax::FLOAT_SET);
			m_CaxScene.setDimension(CaxCoordSet, 3);
			m_CaxScene.setValues(CaxCoordSet, pCaxCoordFloatArray, iVerticesCount);
			m_CaxScene.setCoordinates(CaxMesh,CaxCoordSet);

			CaxId CaxCoordIndexSet = m_CaxScene.createObject(Cax::UINT32_SET);
			m_CaxScene.setValues(CaxCoordIndexSet,&pCaxCoordIndexIntArray[0],OriCoordSet.size());

			CaxId CaxPolylengthSet = m_CaxScene.createObject(Cax::UINT32_SET);
			m_CaxScene.setValues(CaxPolylengthSet,&pCaxPolylenghtIntArray[0],iPrimCount);

			m_CaxScene.setConnectivity(CaxMesh, CaxCoordIndexSet, CaxPolylengthSet);

			if(g_bDetailedLog)
			{
				ss.str("");
				ss<<"vcCad2Cax::ConstructCaxShape(): Cax shape is completed";
				vcUtils::LogMsg(ss.str());
			}

			CaxId CaxMaterial = m_CaxScene.createObject(Cax::MATERIAL);
			m_CaxScene.setName(CaxMaterial,GetValidNodeName("Material").c_str());
			m_CaxScene.setMaterial(CaxMesh,CaxMaterial);

			float trans=0.0f;
			if(diffuseColor[3]>=0 && diffuseColor[3]<=255)  
			{
				if(g_bDetailedLog)
				{
					ss.str("");
					ss<<"vcCad2Cax::ConstructCaxShape(): "<<"Transparency from DATAKIT: "<<diffuseColor[3];
					vcUtils::LogMsg(ss.str());
				}
				ss.str("");
				trans = 1.0f - (diffuseColor[3]/255.0f); 
			}
			
			if(g_bDetailedLog)
			{
				ss.str("");
				ss<<"vcCad2Cax::ConstructCaxShape():Cax transparency: "<<trans;
				vcUtils::LogMsg(ss.str());
			}

			float r=diffuseColor[0];
			float g=diffuseColor[1];
			float b=diffuseColor[2];
			if(g_bIgnoreTransparency)
			{
				if(g_bDetailedLog)
				{
					ss.str("");
					ss<<"vcCad2Cax::ConstructCaxShape():  transparecny ignored ";
					vcUtils::LogMsg(ss.str());
				}
				m_CaxScene.setColor(CaxMaterial,Cax::DIFFUSE,diffuseColor[0]/255.0f,diffuseColor[1]/255.0f,diffuseColor[2]/255.0f,0.0f);
			}
			else
			{ 
				m_CaxScene.setColor(CaxMaterial,Cax::DIFFUSE,diffuseColor[0]/255.0f,diffuseColor[1]/255.0f,diffuseColor[2]/255.0f,trans);
			}

			if(g_bTestResults)
			{
				/*m_TestResultsFile<<"Shape Name: "<<sShapeName.c_str()<<std::endl;
				m_TestResultsFile<<"Shape Type: Cax::SURFACE_MESH"<<std::endl;
				m_TestResultsFile<<"Vertices Count: "<<iVerticesCount<<std::endl;
				m_TestResultsFile<<"Indices Count: "<<iCoordIndexCount<<std::endl;
				m_TestResultsFile<<"Prim Count: "<<iPrimCount<<std::endl;
				if(g_bIgnoreTransparency)
					m_TestResultsFile<<"Material: "<<diffuseColor[0]<<" "<<diffuseColor[1]<<" "<<diffuseColor[2]<<" "<<0.0f<<std::endl;
				else
					m_TestResultsFile<<"Material: "<<diffuseColor[0]<<" "<<diffuseColor[1]<<" "<<diffuseColor[2]<<" "<<trans<<std::endl;
				*/
				ss.str("");
				if(g_bIgnoreTransparency)
					ss<<"Material: "<<diffuseColor[0]<<" "<<diffuseColor[1]<<" "<<diffuseColor[2]<<" "<<0.0f<<std::endl;
				else
					ss<<"Material: "<<diffuseColor[0]<<" "<<diffuseColor[1]<<" "<<diffuseColor[2]<<" "<<trans<<std::endl;
				
				iTotalVerticesInShape += iVerticesCount;
				iTotalIndicesInShape += iCoordIndexCount;
				iTotalPrimsInShape += iPrimCount;
				MaterialStrArray.push_back(ss.str());
				m_iTotalSurfaces++;
			}
			if(g_bDetailedLog)
			{
				ss.str("");
				ss<<"Time taken for CAX******************* :"<<sw.ElapsedUs();
				vcUtils::LogMsg(ss.str());
			}
		}
		if(g_bTestResults && iTotalVerticesInShape)
		{
			m_TestResultsFile<<"Shape Name: "<<NodeName.c_str()<<std::endl;
			m_TestResultsFile<<"Shape Type: Cax::SURFACE_MESH"<<std::endl;
			m_TestResultsFile<<"Vertices Count: "<<iTotalVerticesInShape<<std::endl;
			m_TestResultsFile<<"Indices Count: "<<iTotalIndicesInShape<<std::endl;
			m_TestResultsFile<<"Prim Count: "<<iTotalPrimsInShape<<std::endl;	

			for(int i=0;i<MaterialStrArray.size();i++)
			{
				for(int j=i+1;j<MaterialStrArray.size();j++)
				{
					if((std::string)MaterialStrArray[i]>=(std::string)MaterialStrArray[j])
					{
						std::string tmpStr = (std::string)MaterialStrArray[i];
						MaterialStrArray[i] = (std::string)MaterialStrArray[j];
						MaterialStrArray[j] = tmpStr;
					}
				}
			}
			for(int i=0;i<MaterialStrArray.size();i++)
			{
				m_TestResultsFile<<(std::string)MaterialStrArray[i];
			}
		}
		if(DtkColorNFaceArrayMap.size()>1)
		{
			//CAX
			if(m_NodesArray.size())
				m_NodesArray.pop_back();
			//
		}
		if(g_bDetailedLog)
		{
			ss.str("");
			ss<<"vcCad2Cax::ConstructCaxShape(): Cax material is completed";
			vcUtils::LogMsg(ss.str());
		}

		if(g_bDetailedLog)
		{
			memusage = vcUtils::GetMemoryUsage();
			ss.str("");
			ss<<"vcCad2Cax::ConstructCaxShape():Memory Usage after creating CAX geometry: "<<memusage;
			vcUtils::LogMsg(ss.str());
		}

		StopWatch sw;
		sw.Restart();
		//Cleaning the Map
		std::vector<Dtk_mesh_face*> *FaceArray = NULL;	
		for(MI=DtkColorNFaceArrayMap.begin();MI!=DtkColorNFaceArrayMap.end();MI++)
		{
			FaceArray = MI->second.first;
			for(int i=0;i<FaceArray->size();i++)
			{
				Dtk_mesh_face* mf = (Dtk_mesh_face*)FaceArray->at(i);
				mf = NULL;
			}
			delete FaceArray;

			Dtk_RGB* Color = MI->first; 
			Color = NULL;
		}
		DtkColorNFaceArrayMap.clear();

		std::stringstream ss;
		if(g_bDetailedLog)
		{
			ss.str("");
			ss<<"Time taken for CLR******************* :"<<sw.ElapsedUs();
			vcUtils::LogMsg(ss.str());
		}

		if(g_bDetailedLog)
		{
			memusage = vcUtils::GetMemoryUsage();
			ss.str("");
			ss<<"vcCad2Cax::ConstructCaxShape():Memory Usage(end) : "<<memusage;
			vcUtils::LogMsg(ss.str());

			vcUtils::LogMsg("vcCad2Cax::ConstructCaxShape():End");
		}
	}
	catch(...)
	{
		g_bDataLoss = true;
		ss.str("");
		ss<<"vcCad2Cax::ConstructCaxShape(): ***Exception*** caught";
		vcUtils::LogMsg(ss.str());
	}
}



void vcCad2Cax::GetAxisAndAngle(const float rotmat[9], float axis[3], float& angle)
{
	try
	{
		const float epsilon = 1.0e-6f;
		const float pi = 3.1415926535897932384626433832795f;

		float trace = rotmat[0] + rotmat[4] + rotmat[8];
		float cos_theta = 0.5f * (trace - 1.0f);
		angle = acosf( cos_theta );

		// angle is zero, axis can be anything
		if ( fabsf(angle) < epsilon /* is zero */ )
		{
			axis[0] = 1;
		}
		// standard case
		else if ( angle < pi-epsilon )
		{        
			axis[0] = rotmat[5]-rotmat[7];
			axis[1] = rotmat[6]-rotmat[2];
			axis[2] = rotmat[1]-rotmat[3];

			// normalize axis
			float lengthsq = axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2];
			if ( fabsf(lengthsq) < epsilon /* is zero */ )
			{
				axis[0] = axis[1] = axis[2] = 0.0f;
			}
			else
			{
				float factor = 1.0f/sqrtf(lengthsq);
				axis[0] *= factor;
				axis[1] *= factor;
				axis[2] *= factor;
			}
		}
		// angle is 180 degrees
		else
		{
			unsigned int i = 0;
			if ( rotmat[4] > rotmat[0] )
				i = 1;
			if ( rotmat[8] > rotmat[i + 3*i] )
				i = 2;
			unsigned int j = (i+1)%3;
			unsigned int k = (j+1)%3;
			float s = sqrtf( rotmat[i + 3*i] - rotmat[j + 3*j] - rotmat[k + 3*k] + 1.0f );
			axis[i] = 0.5f*s;

			float recip = 1.0f/s;
			axis[j] = (rotmat[i + 3*j])*recip;
			axis[k] = (rotmat[k + 3*i])*recip;
		}
	}
	catch(...)
	{
		vcUtils::LogMsg("vcCad2Cax::GetAxisAndAngle():Exception caught");
	}
}
#include "vcTransformation.h"
void vcCad2Cax::SetCAXTransformation(Dtk_matrix *mat)
{
	try
	{
		double *a0 = mat[0][0]; 
		double *a1 = mat[0][1];
		double *a2 = mat[0][2];
		double *a3 = mat[0][3];

		float a[4][4]; 
		a[0][0] = a0[0]; a[0][1]= a0[1]; a[0][2]=a0[2]; a[0][3]=a0[3]; 
		a[1][0] = a1[0]; a[1][1]= a1[1]; a[1][2]=a1[2]; a[1][3]=a1[3];
		a[2][0] = a2[0]; a[2][1]= a2[1]; a[2][2]=a2[2]; a[2][3]=a2[3];
		a[3][0] = a3[0]; a[3][1]= a3[1]; a[3][2]=a3[2]; a[3][3]=a3[3];

		if(g_bDetailedLog)
		{
			ss.str("");
			ss<<"vcCad2Cax::vcCad2Cax::SetCAXTransformation(): Transformation Matrix";
			vcUtils::LogMsg(ss.str());
			ss.str("");

			ss<<"vcCad2Cax::vcCad2Cax::SetCAXTransformation(): "<<a[0][0]<<" "<<a[0][1]<<" "<<a[0][2]<<" "<<a[0][3];
			vcUtils::LogMsg(ss.str());
			ss.str("");

			ss<<"vcCad2Cax::vcCad2Cax::SetCAXTransformation(): "<<a[1][0]<<" "<<a[1][1]<<" "<<a[1][2]<<" "<<a[1][3];
			vcUtils::LogMsg(ss.str());
			ss.str("");

			ss<<"vcCad2Cax::vcCad2Cax::SetCAXTransformation(): "<<a[2][0]<<" "<<a[2][1]<<" "<<a[2][2]<<" "<<a[2][3];
			vcUtils::LogMsg(ss.str());
			ss.str("");

			ss<<"vcCad2Cax::vcCad2Cax::SetCAXTransformation(): "<<a[3][0]<<" "<<a[3][1]<<" "<<a[3][2]<<" "<<a[3][3];
			vcUtils::LogMsg(ss.str());
			ss.str("");
		}

		float rotation[4],translation[3],scale[3];
		vcTransformation::DecomposeMatrix(a,rotation,translation,scale);

		CaxId CurrentCaxTransform = m_NodesArray.at(m_NodesArray.size()-1);
		if(scale[0]<0 || scale[1]<0 || scale[2]<0)//This case is for the Transform nodes with Negative Scale
		{
			if(g_bDetailedLog)
			{
				vcUtils::LogMsg("vcCad2Cax::SetCaxTransformation(): Transformation with Negative Scale case");
				vcUtils::LogMsg("vcCad2Cax::SetCaxTransformation(): Scale is not with Transform Node,instead multiplied with the coordinates");
				
				ss.str("");
				ss<<"vcCad2Cax::SetCaxTransformation():Translation: "<<translation[0]<<" "<<translation[1]<<" "<<translation[2];
				vcUtils::LogMsg(ss.str());

				ss.str("");
				ss<<"vcCad2Cax::SetCaxTransformation():Rotation: "<<rotation[0]<<" "<<rotation[1]<<" "<<rotation[2]<<" "<<rotation[3];
				vcUtils::LogMsg(ss.str());
		 
				ss.str("");
				ss<<"vcCad2Cax::SetCaxTransformation():Scale: "<<scale[0]<<" "<<scale[1]<<" "<<scale[2];
				vcUtils::LogMsg(ss.str());
			}
			m_bNegativeScale = true; 
			m_iNegativeScaleNodeIndex = m_NodesArray.size();
			m_NegativeScaleVec[0]=scale[0];
			m_NegativeScaleVec[1]=scale[1];
			m_NegativeScaleVec[2]=scale[2]; 

			m_CaxScene.setTranslation(CurrentCaxTransform,translation[0]*m_fUnit,translation[1]*m_fUnit,translation[2]*m_fUnit); 
			m_CaxScene.setRotation(CurrentCaxTransform,rotation[0],rotation[1],rotation[2],rotation[3]);
			m_CaxScene.setScale(CurrentCaxTransform, scale[0], scale[1], scale[2]);
		}
		else if(m_bNegativeScale)//This case is for the Transform nodes,which are under the Transform node with Negative Scale
		{
			if(g_bDetailedLog)
			{
				vcUtils::LogMsg("vcCad2Cax::SetCaxTransformation(): Transformation, which are under Transformation with Negative Scale case");
				vcUtils::LogMsg("vcCad2Cax::SetCaxTransformation(): Translation is reversed");
				
				ss.str("");
				ss<<"vcCad2Cax::SetCaxTransformation():Translation: "<<translation[0]<<" "<<translation[1]<<" "<<translation[2];
				vcUtils::LogMsg(ss.str());

				ss.str("");
				ss<<"vcCad2Cax::SetCaxTransformation():Rotation: "<<rotation[0]<<" "<<rotation[1]<<" "<<rotation[2]<<" "<<rotation[3];
				vcUtils::LogMsg(ss.str());

				ss.str("");
				ss<<"vcCad2Cax::SetCaxTransformation():Scale: "<<scale[0]<<" "<<scale[1]<<" "<<scale[2];
				vcUtils::LogMsg(ss.str());
			}

			//m_CaxScene.setTranslation(CurrentCaxTransform,-translation[0]*m_fUnit,-translation[1]*m_fUnit,-translation[2]*m_fUnit); 
			m_CaxScene.setTranslation(CurrentCaxTransform, translation[0] * m_fUnit, translation[1] * m_fUnit, translation[2] * m_fUnit);
			m_CaxScene.setRotation(CurrentCaxTransform,rotation[0],rotation[1],rotation[2],rotation[3]);
			m_CaxScene.setScale(CurrentCaxTransform,scale[0],scale[1],scale[2]);
		}
		else
		{
			if(g_bDetailedLog)
			{
				vcUtils::LogMsg("vcCad2Cax::SetCaxTransformation():General Transformation case");
				
				ss.str("");
				ss<<"vcCad2Cax::SetCaxTransformation():Translation: "<<translation[0]<<" "<<translation[1]<<" "<<translation[2];
				vcUtils::LogMsg(ss.str());

				ss.str("");
				ss<<"vcCad2Cax::SetCaxTransformation():Rotation: "<<rotation[0]<<" "<<rotation[1]<<" "<<rotation[2]<<" "<<rotation[3];
				vcUtils::LogMsg(ss.str());

				ss.str("");
				ss<<"vcCad2Cax::SetCaxTransformation():Scale: "<<scale[0]<<" "<<scale[1]<<" "<<scale[2];
				vcUtils::LogMsg(ss.str());
			}
			m_CaxScene.setTranslation(CurrentCaxTransform,translation[0]*m_fUnit,translation[1]*m_fUnit,translation[2]*m_fUnit); 
			m_CaxScene.setRotation(CurrentCaxTransform,rotation[0],rotation[1],rotation[2],rotation[3]);
			m_CaxScene.setScale(CurrentCaxTransform,scale[0],scale[1],scale[2]);
		}
	}
	catch(...)
	{
		vcUtils::LogMsg("vcCad2Cax::SetCaxTransformation():Exception caught");
	}
}

bool vcCad2Cax::AreAllSpaces(const char *sNodeName)
{
	if(sNodeName && strlen(sNodeName))
	{
		for(int i=0;i<strlen(sNodeName);i++) 
		{
			if(sNodeName[i] != ' ')
				return false;
		}
	}
	return true;
}

std::string vcCad2Cax::GetValidNodeName(const char *sNodeName,int index)
{
	if(g_bDetailedLog)
	{
		vcUtils::LogMsg("vcCad2Cax::GetValidNodeName():Start...");
	}
	
	if(!sNodeName || strlen(sNodeName)==0 || AreAllSpaces(sNodeName))
	{
		sNodeName = "Default";
	}	

	if(g_bDetailedLog)
	{
		ss.str("");
		ss<<"vcCad2Cax::GetValidNodeName(): Name: "<<sNodeName;
		vcUtils::LogMsg(ss.str());
	}

	std::stringstream sName;
	if(m_NodeNameAndIndexMap.size() && m_NodeNameAndIndexMap.find(sNodeName)!=m_NodeNameAndIndexMap.end())
	{
		int index = m_NodeNameAndIndexMap[sNodeName];
		sName<<sNodeName<<"_"<<++index;
		m_NodeNameAndIndexMap[sNodeName] = index;
	
		if(g_bDetailedLog)
		{
			ss.str("");
			ss<<"vcCad2Cax::GetValidNodeName():Cax Name: "<<sName.str().c_str();
			vcUtils::LogMsg(ss.str());
		}
		
		return GetValidNodeName(sName.str().c_str());
	}
	else
	{
		m_NodeNameAndIndexMap[sNodeName] = 1; 
	}
	if(g_bDetailedLog)
	{
		ss.str("");
		ss<<"vcCad2Cax::GetValidNodeName():Cax Name: "<<sNodeName;
		vcUtils::LogMsg(ss.str());

		vcUtils::LogMsg("vcCad2Cax::GetValidNodeName():End");
	}
	return std::string(sNodeName);
}

bool vcCad2Cax::IsComponentColor(Dtk_RGB *ComponentColor)
{
	bool bComponentColor = false;
	try
	{
		if(m_DtkComponentColorArray.size())
		{
			Dtk_RGB Color;
			for(int i=m_DtkComponentColorArray.size()-1;i>0;i--)
			{
				Color = m_DtkComponentColorArray.at(i);
				if(Color.R()!=-1 && Color.G()!=-1 && Color.B()!=-1)
				{
					bComponentColor = true;
					ComponentColor->SetRGBA(m_DtkComponentColorArray.at(i).R(),m_DtkComponentColorArray.at(i).G(),m_DtkComponentColorArray.at(i).B(),m_DtkComponentColorArray.at(i).A());

					if(g_bDetailedLog)
					{
						ss.str("");
						ss<<"vcCad2Cax::IsComponentColor():Component Color applied";
						vcUtils::LogMsg(ss.str());
						ss.str("");
					}

					break;
				}
			}
			if(!bComponentColor)
			{
				if(g_bDetailedLog)
				{
					ss.str("");
					ss<<"vcCad2Cax::WriteDtk_Mesh():Mesh Color applied";
					vcUtils::LogMsg(ss.str());
					ss.str("");
				}
			}
		}
	}
	catch(...)
	{
		vcUtils::LogMsg("vcCad2Cax::IsComponentColor():exception caught");
	}
	return bComponentColor;
}
void vcCad2Cax::GetMeshFacesWithComponentColor(const Dtk_MeshPtr& inMeshToWrite,
											   std::map< Dtk_RGB*, std::pair<std::vector<Dtk_mesh_face*> *,std::vector<Dtk_MeshPtr> >> &DtkColorNFaceArrayMap,
											   Dtk_RGB *ComponentColor,
											   std::map< Dtk_RGB*, int> &DtkColorNVerticesCountMap
											   )
{
	if(g_bDetailedLog)
	{
		vcUtils::LogMsg("vcCad2Cax::GetMeshFacesWithComponentColor():Start");
	}

	Dtk_Size_t nbmeshfaces = inMeshToWrite->get_nb_mesh_face();
	std::vector<Dtk_mesh_face*> *FaceArray = new std::vector<Dtk_mesh_face*>;
	if(ComponentColor->R()==-1 && ComponentColor->G()==-1 && ComponentColor->B()==-1)
		ComponentColor->SetRGBA(255,255,255);

	int iVerticesCount = 0;
	Dtk_mesh_face* mf = NULL;
	Dtk_Size_t nbtriangles=0;
	for(int i = 0;i<nbmeshfaces;i++)
	{
		mf = inMeshToWrite->get_mesh_face(i);
		FaceArray->push_back(mf);
		DtkColorNFaceArrayMap[ComponentColor].second.push_back(inMeshToWrite);
	}
	//DtkColorNFaceArrayMap[ComponentColor] = FaceArray;
	DtkColorNFaceArrayMap[ComponentColor].first = FaceArray;
			//DtkColorNFaceArrayMap[ComponentColor].second.push_back(inMeshToWrite);

	if(g_bDetailedLog)
	{
		vcUtils::LogMsg("vcCad2Cax::GetMeshFacesWithComponentColor():End");
	}
}

void vcCad2Cax::GetMeshFacesWithFaceColor(const Dtk_MeshPtr& inMeshToWrite,
										  std::map< Dtk_RGB*, std::pair<std::vector<Dtk_mesh_face*> *,std::vector<Dtk_MeshPtr>>> &DtkColorNFaceArrayMap,
										  std::map< Dtk_RGB*, int> &DtkColorNVerticesCountMap,
										  Dtk_RGB *ComponentColor
										  )
{
	try
	{
		if(g_bDetailedLog)
		{
			vcUtils::LogMsg("vcCad2Cax::GetMeshFacesWithFaceColor():Start");
		}
		Dtk_Size_t nbmeshfaces = inMeshToWrite->get_nb_mesh_face();
	#if 0
		std::map< Dtk_RGB*, int>::iterator VerticesCountIterator;
	#endif
		Dtk_Size_t nbtriangles = 0;
		Dtk_mesh_face* mf = NULL;
		Dtk_RGB *FaceColor = NULL;
		std::map< Dtk_RGB*, std::pair<std::vector<Dtk_mesh_face*> *,std::vector<Dtk_MeshPtr>>>::iterator MI;
		bool bFound = false;
		Dtk_RGB *FColor=NULL;

#if 0
		//This loop gets the Faces Count based on FaceColor
		std::map< Dtk_RGB*, int> ColorNFaceCountMap;
		std::map< Dtk_RGB*, int>::iterator ColorCountMI;
		for(int i = 0;i<nbmeshfaces;i++)
		{
			mf = inMeshToWrite->get_mesh_face(i);
			if(!mf)
				continue;
			FaceColor = NULL;
			FaceColor = new Dtk_RGB(mf->get_face_color()); 
			if(FaceColor->R()==-1 && FaceColor->G()==-1 && FaceColor->B()==-1)
			{
				if(ComponentColor)
					FaceColor->SetRGBA(ComponentColor->R(),ComponentColor->G(),ComponentColor->B());
				else
					FaceColor->SetRGBA(255,255,255);
			}

			bFound = false;
			FColor=NULL;
			for(ColorCountMI=ColorNFaceCountMap.begin();ColorCountMI!=ColorNFaceCountMap.end();ColorCountMI++)
			{
				FColor = ColorCountMI->first;
				if(FColor->R() == FaceColor->R() && FColor->G() == FaceColor->G() && FColor->B() == FaceColor->B())
				{
					bFound = true;
					break;
				}
			}
			if(!bFound)
			{
				ColorNFaceCountMap[FaceColor]=1;
			}
			else
			{
				ColorNFaceCountMap[FColor] = ColorNFaceCountMap[FColor]+1;
			}

		}

		for(int i = 0;i<nbmeshfaces;i++)
		{
			mf = inMeshToWrite->get_mesh_face(i);
			if(!mf)
				continue;
			/*FaceColor = NULL;
			FaceColor = new Dtk_RGB(mf->get_face_color()); 
			if(FaceColor->R()==-1 && FaceColor->G()==-1 && FaceColor->B()==-1)
			{
				if(ComponentColor)
					FaceColor->SetRGBA(ComponentColor->R(),ComponentColor->G(),ComponentColor->B());
				else
					FaceColor->SetRGBA(255,255,255);
			}
			FaceColor->SetRGBA(255,255,255,-1);*/
			Dtk_RGB mesh_color(mf->get_face_color());

			bFound = false;
			FColor=NULL;
			int mesh_count=0;
			for(ColorCountMI=ColorNFaceCountMap.begin();ColorCountMI!=ColorNFaceCountMap.end();ColorCountMI++)
			{
				FColor = ColorCountMI->first;
				if(FColor->R() == mesh_color.R() && FColor->G() == mesh_color.G() && FColor->B() == mesh_color.B())
				{
					bFound = true;
					mesh_count = ColorCountMI->second;
					break;
				}
			}

			MI = DtkColorNFaceArrayMap.find(FColor);
			if(MI == DtkColorNFaceArrayMap.end())
			{
				std::vector<Dtk_mesh_face*> *FaceArray = new std::vector<Dtk_mesh_face*>;

				//FaceArray->reserve(mesh_count);
				//DtkColorNFaceArrayMap[FColor].second.reserve(mesh_count);

				FaceArray->push_back(mf);
				DtkColorNFaceArrayMap[FColor].first = FaceArray;
				DtkColorNFaceArrayMap[FColor].second.push_back(inMeshToWrite);
			}
			else
			{
				DtkColorNFaceArrayMap[FColor].first->push_back(mf);
				DtkColorNFaceArrayMap[FColor].second.push_back(inMeshToWrite);
			}
			/*if(!bFound)
			{
				std::vector<Dtk_mesh_face*> *FaceArray = new std::vector<Dtk_mesh_face*>;
				FaceArray->push_back(mf);
				DtkColorNFaceArrayMap[FaceColor].first = FaceArray;
				DtkColorNFaceArrayMap[FaceColor].second.push_back(inMeshToWrite);

			}
			else
			{
				DtkColorNFaceArrayMap[FColor].first->push_back(mf);
				DtkColorNFaceArrayMap[FColor].second.push_back(inMeshToWrite);
			}*/
			mesh_count++;
		}
#else
		for(int i = 0;i<nbmeshfaces;i++)
		{
			mf = inMeshToWrite->get_mesh_face(i);
			if(!mf)
				continue;
			FaceColor = NULL;
			FaceColor = new Dtk_RGB(mf->get_face_color()); 
			if(FaceColor->R()==-1 && FaceColor->G()==-1 && FaceColor->B()==-1)
			{
				if(ComponentColor)
					FaceColor->SetRGBA(ComponentColor->R(),ComponentColor->G(),ComponentColor->B());
				else
					FaceColor->SetRGBA(255,255,255);
			}
			//FaceColor->SetRGBA(255,255,255,-1);

			bFound = false;
			FColor=NULL;
			for(MI=DtkColorNFaceArrayMap.begin();MI!=DtkColorNFaceArrayMap.end();MI++)
			{
				FColor = MI->first;
				if(FColor->R() == FaceColor->R() && FColor->G() == FaceColor->G() && FColor->B() == FaceColor->B())
				{
					bFound = true;
					break;
				}
			}

			if(!bFound)
			{
				std::vector<Dtk_mesh_face*> *FaceArray = new std::vector<Dtk_mesh_face*>;
				FaceArray->push_back(mf);
				DtkColorNFaceArrayMap[FaceColor].first = FaceArray;
				DtkColorNFaceArrayMap[FaceColor].second.push_back(inMeshToWrite);

			}
			else
			{
				DtkColorNFaceArrayMap[FColor].first->push_back(mf);
				DtkColorNFaceArrayMap[FColor].second.push_back(inMeshToWrite);
			}
		}
#endif
		if(g_bDetailedLog)
		{
			vcUtils::LogMsg("vcCad2Cax::GetMeshFacesWithFaceColor():End");
		}
	}
	catch(...)
	{
		vcUtils::LogMsg("vcCad2Cax::GetMeshFacesWithFaceColor():Exception caught");
	}
}

Dtk_ErrorStatus vcCad2Cax::WriteBody(const Dtk_BodyPtr& inBody)
{
	if(inBody.IsNULL())
		return dtkNoError;

	Dtk_Size_t numLump, i, j;

	//Dtk_string name = inBody->get_info()->GetName();
	//fprintf(F,"<Dtk_BodyPtr>");
	//fprintf(F,"<Status>%d</Status>",inBody->GetBodyStatus());
	//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_info(F, inBody->get_info());
	//fprintf(F,"<Id>%d</Id>",inBody->GetID());
	numLump = inBody->GetNumLumps();
	for(i = 0; i < numLump; i++)
	{
		Dtk_LumpPtr lump;
		inBody->GetLump(i, lump);
		//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_LumpPtr(F, lump);
		//WriteLump(lump);
	}
	Dtk_ShellPtr myshell;
	Dtk_Size_t m,NumOpenshell = inBody->GetNumOpenShells();

	for(m = 0 ; m<NumOpenshell ; m++ )
	{
		inBody->GetOpenShell(m,myshell);
		if(myshell.IsNotNULL())
		{
            Dtk_Size_t NumFaces = myshell->GetNumFaces();
 			//if(g_b2DElements)
			{
				for (i=0;i<NumFaces;i++)
				{
					Dtk_FacePtr FacePtr;
					Dtk_bool Orientation;
					myshell->GetFace(i,FacePtr, Orientation);
					//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_FacePtr(F, FacePtr);
						WriteFace(FacePtr);
				}
			}
			Dtk_Size_t nbWires=myshell->GetNumWireSet();
			if(nbWires != 0)
			{
				//fprintf(F,"<Wireframe>");
				for(i=0;i<nbWires;i++)
				{
					Dtk_tab<Dtk_EntityPtr> wireSet;
					myshell->GetWireSetTopo(i,wireSet);
					for (j=0;j<wireSet.size();j++)
					{
						if(wireSet[j]->get_type_detk() != DTK_TYPE_VERTEX)
						{
							//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_EdgePtr( F, Dtk_EdgePtr::DtkDynamicCast( wireSet[j] ) );
							if(g_b2DElements)
								WriteEdge(Dtk_EdgePtr::DtkDynamicCast(wireSet[j]));
						}
						else
						{
							//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_VertexPtr( F, Dtk_VertexPtr::DtkDynamicCast( wireSet[j] ) );
							if(g_bPointSet)
								WriteVertex(Dtk_VertexPtr::DtkDynamicCast( wireSet[j]));
						}
					}
				}
			}
		}
	}
	//fprintf(F,"</Dtk_BodyPtr>");
	return dtkNoError;
}

bool vcCad2Cax::ConstructShapeFor2DElements(Dtk_string sName)
{
	try
	{
		if(g_bDetailedLog)
		{
			vcUtils::LogMsg("vcCad2Cax::ConstructShapeFor2DElements():Start...");
		}

		std::string memusage;
		if(g_bDetailedLog)
		{
			memusage = vcUtils::GetMemoryUsage();
			ss.str("");
			ss<<"vcCad2Cax::ConstructShapeFor2DElements():Memory Usage b4 creating CAX geometry: "<<memusage;
			vcUtils::LogMsg(ss.str());
		}

		if(g_bDetailedLog)
		{
			ss.str("");
			ss<<"vcCad2Cax::ConstructShapeFor2DElements():No. of Cax shapes: "<<m_DtkColorNLinesArrayMap.size();
			vcUtils::LogMsg(ss.str());
		}

		std::map<Dtk_RGB*, std::vector<Dtk_PolylinePtr>* >::iterator MI;
		bool bFound = false;
		int r,g,b,a;

		if(m_DtkColorNLinesArrayMap.size()>1)
		{
			//CAX-Group
			CaxId CaxGroup = m_CaxScene.createObject(Cax::ASSEMBLY);
			m_CaxScene.setName(CaxGroup,GetValidNodeName(sName.c_str()).c_str());;
			CaxId CurrentCaxParent = m_NodesArray.at(m_NodesArray.size()-1);
			m_CaxScene.addComponent(CurrentCaxParent,CaxGroup);
			m_NodesArray.push_back(CaxGroup);
		}

		int iShapeIndex = 0;
		std::map<std::string,CaxId>::iterator NameNCaxIdMapIterator;
		for(MI=m_DtkColorNLinesArrayMap.begin();MI!=m_DtkColorNLinesArrayMap.end();MI++)
		{
			if(g_bDetailedLog)
			{
				ss.str("");
				ss<<"vcCad2Cax::ConstructShapeFor2DElements():Shape : "<<iShapeIndex;
				vcUtils::LogMsg(ss.str());
			}

			Dtk_RGB *pColor = (Dtk_RGB *)MI->first;
			r = pColor->R();
			g = pColor->G();
			b = pColor->B();
			a = pColor->A();

			std::vector<Dtk_PolylinePtr> *pPolylinesArray = (std::vector<Dtk_PolylinePtr>*)MI->second;
			int iTotalVerticesCount=0;
			for(int k=0;k<pPolylinesArray->size();k++)
			{
				Dtk_PolylinePtr Polyline = pPolylinesArray->at(k);
				iTotalVerticesCount += Polyline->GetNumPoints();
			}

			if(!iTotalVerticesCount|| !pPolylinesArray->size())
				continue;

			iShapeIndex++;
			ss.str("");
			m_iShapeIndexOfComponent++;
			//ss<<sName<<"#"<<m_iShapeIndexOfComponent<<"#LineElement";
			ss<<sName<<"#Lines";
			std::string sShapeName = ss.str();
			ss.str("");

	#ifdef VCT_DEF_AND_USE
			NameNCaxIdMapIterator = m_NameNCaxIdMap.find(sShapeName);
			if(NameNCaxIdMapIterator!=m_NameNCaxIdMap.end())
			{
				CaxId caxId = NameNCaxIdMapIterator->second;
				CaxId CurrentCaxParent = m_NodesArray.at(m_NodesArray.size()-1);
				m_CaxScene.addComponent(CurrentCaxParent,caxId);
				if(g_bDetailedLog)
				{
					ss.str("");
					ss<<"vcCad2Cax::ConstructShapeFor2DElements(): Shape : "<<sShapeName<<" is reused";
					vcUtils::LogMsg(ss.str());
				}
				continue;
			}
	#endif

			if(g_bDetailedLog)
			{
				ss.str("");
				ss<<"vcCad2Cax::ConstructShapeFor2DElements(): Coordinates size : "<<iTotalVerticesCount;
				vcUtils::LogMsg(ss.str());

				ss.str("");
				ss<<"vcCad2Cax::ConstructShapeFor2DElements(): Primitive size : "<<pPolylinesArray->size();
				vcUtils::LogMsg(ss.str());
			}
			float *pCaxCoordFloatArray = new float[iTotalVerticesCount*3]; 
			uint32_t *pCaxCoordIndexIntArray = new uint32_t[iTotalVerticesCount];
			uint32_t *pCaxPolylenghtIntArray = new uint32_t[pPolylinesArray->size()];

			FloatArrays.push_back(pCaxCoordFloatArray);
			IntArrays.push_back(pCaxCoordIndexIntArray);
			IntArrays.push_back(pCaxPolylenghtIntArray);

			int j=0;
			int CoordIndex=0;

			for(int k=0;k<pPolylinesArray->size();k++)
			{
				Dtk_PolylinePtr Polyline = pPolylinesArray->at(k);
		
				Dtk_Size_t num, i;
				num = Polyline->GetNumPoints();
				for(i = 0; i < num; i++)
				{
					Dtk_pnt point = Polyline->Point(i)	;
					pCaxCoordFloatArray[j++]=point[0]*m_fUnit;
					pCaxCoordFloatArray[j++]=point[1]*m_fUnit;
					pCaxCoordFloatArray[j++]=point[2]*m_fUnit;
					
					pCaxCoordIndexIntArray[CoordIndex]=CoordIndex;
					CoordIndex++;
				}
				pCaxPolylenghtIntArray[k]=num;
			}

			if(g_bDetailedLog)
			{
				ss.str(""); 
				ss<<"vcCad2Cax::ConstructShapeFor2DElements(): coordinates and connectivity are done";
				vcUtils::LogMsg(ss.str());
			}	

			int iVerticesCount = iTotalVerticesCount;
			int iCoordIndexCount = iTotalVerticesCount;
			int iPrimCount = pPolylinesArray->size();

			CaxId CaxShape;
			CaxShape = m_CaxScene.createObject(Cax::GEOMETRY);
			//std::string nam = std::string(sName.c_str()) + std::string("#2DElements");
			m_CaxScene.setName(CaxShape,GetValidNodeName(sShapeName.c_str()).c_str());
			//m_CaxScene.setName(CaxShape,nam.c_str());
			CaxId CurrentCaxParent = m_NodesArray.at(m_NodesArray.size()-1);
			m_CaxScene.addComponent(CurrentCaxParent,CaxShape);

			if(g_bDetailedLog)
			{
				ss.str("");
				ss<<"vcCad2Cax::ConstructShapeFor2DElements():Cax Shape object is created ";
				vcUtils::LogMsg(ss.str());
			}

	#ifdef VCT_DEF_AND_USE
			m_NameNCaxIdMap[sShapeName] = CaxShape;
	#endif

			CaxId CaxMesh;
			CaxMesh = m_CaxScene.createObject(Cax::MESH);
			m_CaxScene.setMeshType(CaxMesh,Cax::LINE_MESH);
			m_CaxScene.setName(CaxMesh,GetValidNodeName("mesh").c_str());
			m_CaxScene.addComponent(CaxShape,CaxMesh);
			
			if(g_bDetailedLog)
			{
				ss.str("");
				ss<<"vcCad2Cax::ConstructShapeFor2DElements():Cax Shape is added to Cax Scene ";
				vcUtils::LogMsg(ss.str());
			}

			CaxId CaxCoordSet = m_CaxScene.createObject(Cax::FLOAT_SET);
			m_CaxScene.setDimension(CaxCoordSet, 3);
			m_CaxScene.setValues(CaxCoordSet, pCaxCoordFloatArray, iVerticesCount);
			m_CaxScene.setCoordinates(CaxMesh,CaxCoordSet);

			CaxId CaxCoordIndexSet = m_CaxScene.createObject(Cax::UINT32_SET);
			m_CaxScene.setValues(CaxCoordIndexSet,pCaxCoordIndexIntArray,iCoordIndexCount);

			CaxId CaxPolylengthSet = m_CaxScene.createObject(Cax::UINT32_SET);
			m_CaxScene.setValues(CaxPolylengthSet,pCaxPolylenghtIntArray,iPrimCount);

			m_CaxScene.setConnectivity(CaxMesh, CaxCoordIndexSet, CaxPolylengthSet);

			if(g_bDetailedLog)
			{ 
				ss.str("");
				ss<<"vcCad2Cax::ConstructShapeFor2DElements(): Cax shape is completed";
				vcUtils::LogMsg(ss.str());
			}
			
			CaxId CaxMaterial = m_CaxScene.createObject(Cax::MATERIAL);
			m_CaxScene.setName(CaxMaterial,GetValidNodeName("Material").c_str());
			m_CaxScene.setMaterial(CaxMesh,CaxMaterial);
			m_CaxScene.setColor(CaxMaterial,Cax::DIFFUSE,r/255.0f,g/255.0f,b/255.0f,1.0f);

			if(g_bDetailedLog)
			{
				ss.str("");
				ss<<"vcCad2Cax::ConstructShapeFor2DElements(): Material is set";
				vcUtils::LogMsg(ss.str());
			}
			
			if(g_bTestResults)
			{
				m_TestResultsFile<<"Shape Name: "<<sShapeName.c_str()<<std::endl;
				m_TestResultsFile<<"Shape Type: Cax::LINE_MESH"<<std::endl;
				m_TestResultsFile<<"Vertices Count: "<<iVerticesCount<<std::endl;
				m_TestResultsFile<<"Indices Count: "<<iCoordIndexCount<<std::endl;
				m_TestResultsFile<<"Prim Count: "<<iPrimCount<<std::endl;
				m_TestResultsFile<<"Material: "<<r<<" "<<g<<" "<<b<<" "<<a<<std::endl;
				m_iTotalLines++;
			}
		}

		if(m_DtkColorNLinesArrayMap.size()>1)
		{
			if(m_NodesArray.size())
				m_NodesArray.pop_back();
		}
			
		if(g_bDetailedLog)
		{
			memusage = vcUtils::GetMemoryUsage();
			ss.str("");
			ss<<"vcCad2Cax::ConstructShapeFor2DElements():Memory Usage after creating CAX geometry: "<<memusage;
			vcUtils::LogMsg(ss.str());
		}

		//Cleaning the Map
		std::vector<Dtk_PolylinePtr> *LinesArray = NULL;	
		for(MI=m_DtkColorNLinesArrayMap.begin();MI!=m_DtkColorNLinesArrayMap.end();MI++)
		{
			LinesArray = MI->second;
			for(int i=0;i<LinesArray->size();i++)
			{
				Dtk_PolylinePtr polyline = (Dtk_PolylinePtr)LinesArray->at(i);
				polyline = NULL;
			}
			delete LinesArray;

			Dtk_RGB* Color = MI->first; 
			Color = NULL;
		}
		m_DtkColorNLinesArrayMap.clear();

		if(g_bDetailedLog)
		{
			memusage = vcUtils::GetMemoryUsage();
			ss.str("");
			ss<<"vcCad2Cax::ConstructShapeFor2DElements():Memory Usage(end) : "<<memusage;
			vcUtils::LogMsg(ss.str());

			vcUtils::LogMsg("vcCad2Cax::ConstructShapeFor2DElements():End");
		}
	}
	catch(...)
	{
		g_bDataLoss = true;
		ss.str("");
		ss<<"vcCad2Cax::ConstructShapeFor2DElements(): ***Exception*** caught";
		vcUtils::LogMsg(ss.str());
	}

	return true;
}

Dtk_ErrorStatus vcCad2Cax::WriteLump(const Dtk_LumpPtr& inLump)
{
	if(inLump.IsNULL())
		return dtkNoError;

	Dtk_Size_t numVolume, i;

	//fprintf(F,"<Dtk_LumpPtr>");
	//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_info(F, inLump->get_info());
	//fprintf(F,"<Id>%d</Id>",inLump->GetID());
	numVolume = inLump->GetNumVolumes();
	for(i = 0; i < numVolume; i++)
	{
		Dtk_VolumePtr volume;
		inLump->GetVolume(i, volume);
		//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_VolumePtr(F, volume);
		WriteVolume(volume);
	}
	//fprintf(F,"</Dtk_LumpPtr>");
	return dtkNoError;
}

Dtk_ErrorStatus vcCad2Cax::WriteVolume(const Dtk_VolumePtr& inVol)
{
	if(inVol.IsNULL())
		return dtkNoError;

	Dtk_Size_t numShell, i;

	//fprintf(F,"<Dtk_VolumePtr>");
	//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_info(F, inVol->get_info());
	//fprintf(F,"<Id>%d</Id>",inVol->GetID());
	numShell = inVol->GetNumShells();
	for(i = 0; i < numShell; i++)
	{
		Dtk_ShellPtr shell;
		inVol->GetShell(i, shell);
		//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_ShellPtr(F, shell);
		WriteShell(shell);
	}
	//fprintf(F,"</Dtk_VolumePtr>");
	return dtkNoError;
}

Dtk_ErrorStatus vcCad2Cax::WriteShell(const Dtk_ShellPtr& inShell)
{
	if(inShell.IsNULL())
		return dtkNoError;

	Dtk_Size_t numFace, i;
	Dtk_bool orientation;

	//fprintf(F,"<Dtk_ShellPtr>");
	//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_info(F, inShell->get_info());
	//fprintf(F,"<Id>%d</Id>",inShell->GetID());
	numFace = inShell->GetNumFaces();
	if(numFace == 0)
	{
        //fprintf(F,"</Dtk_ShellPtr>");
		return dtkTopologyShellHasNoFaces;
	}
	for(i = 0; i < numFace; i++)
	{
		Dtk_FacePtr face;
		inShell->GetFace(i, face, orientation);
		//fprintf(F, "<orientation>%d</orientation>", orientation);
		//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_FacePtr(F, face);
		WriteFace(face);
	}
	//fprintf(F,"</Dtk_ShellPtr>");
	return dtkNoError;
}

Dtk_ErrorStatus vcCad2Cax::WriteFace(const Dtk_FacePtr& inFace)
{
	if(inFace.IsNULL())
		return dtkNoError;

	Dtk_Size_t numLoop, i;
	Dtk_SurfacePtr surf;

	//fprintf(F,"<Dtk_FacePtr>");
	//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_info(F, inFace->get_info());
	//fprintf(F,"<Id>%d</Id>",inFace->GetID());
	surf = inFace->GetGeom();
	if (surf.IsNotNULL())
	{
		//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_SurfacePtr(F, surf);
		//WriteSurface(surf);
	}
	numLoop = inFace->GetNumLoops(); 
	for(i = 0; i < numLoop; i++)
	{
		Dtk_LoopPtr loop;
		inFace->GetLoop(i, loop);
		//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_LoopPtr(F, loop);
		WriteLoop(loop);
	}

	if(numLoop)
	{
		//if(!this->m_bSinglePart)
			//this->ConstructShapeFor2DElements("Default");
	}


	//fprintf(F,"</Dtk_FacePtr>");
	return dtkNoError;
}

Dtk_ErrorStatus vcCad2Cax::WriteLoop(const Dtk_LoopPtr& inLoop)
{
	if(inLoop.IsNULL())
		return dtkNoError;

	Dtk_Size_t numCoedge, i;
	Dtk_bool orientation;

	//fprintf(F,"<Dtk_LoopPtr>");
	//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_info(F, inLoop->get_info());
	//fprintf(F,"<Id>%d</Id>",inLoop->GetID());
	//fprintf(F,"<loop_sens>%d</loop_sens>", inLoop->GetOrientation());
	numCoedge = inLoop->GetNumCoedges();
	for(i = 0; i < numCoedge; i++)
	{
		Dtk_CoedgePtr coedge;
		inLoop->GetCoedge(i, coedge, orientation);
		//fprintf(F, "<coedge_orientation_in_loop>%d</coedge_orientation_in_loop>", orientation);
		//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_CoedgePtr(F, coedge);
		WriteCoedge(coedge);
	}

	//fprintf(F,"</Dtk_LoopPtr>");
	return dtkNoError;
}

Dtk_ErrorStatus vcCad2Cax::WriteCoedge(const Dtk_CoedgePtr& inCoedge)
{
	if(inCoedge.IsNULL())
		return dtkNoError;

	Dtk_EdgePtr edge;
	Dtk_CurvePtr curveUV;

	//fprintf(F,"<Dtk_CoedgePtr>");
    //fprintf(F, "<orientation>%d</orientation>", inCoedge->GetOrientation());
	//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_info(F, inCoedge->get_info());
	//fprintf(F,"<Id>%d</Id>",inCoedge->GetID());
	curveUV = inCoedge->GetGeom();
	if(curveUV.IsNotNULL())
	{
		//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_CurvePtr(F, curveUV);
		//WriteCurve(curveUV);
	}
	inCoedge->GetEdge(edge);
	if(edge.IsNotNULL())
	{
		//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_EdgePtr(F, edge);
		WriteEdge(edge);
	}

	//fprintf(F,"</Dtk_CoedgePtr>");
	return dtkNoError;
}

Dtk_ErrorStatus vcCad2Cax::WriteEdge(const Dtk_EdgePtr& inEdge)
{
	if(inEdge.IsNULL())
		return dtkNoError;

	Dtk_CurvePtr curve3d;
	Dtk_VertexPtr startVertex, endVertex;

	//Dtk_string n = inEdge->get_info()->GetName();
	//fprintf(F,"<Dtk_EdgePtr>");
	//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_info(F, inEdge->get_info());
	//fprintf(F,"<Id>%d</Id>",inEdge->GetID());
	curve3d = inEdge->GetGeom();
	//Dtk_string name=curve3d->get_info()->GetName();
	if(curve3d.IsNotNULL())
	{
		//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_CurvePtr(F, curve3d);
		WriteCurve(curve3d);
	}
	inEdge->GetStartVertex(startVertex);
	if( startVertex.IsNotNULL() )
    {
        //Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_VertexPtr(F, startVertex);
		WriteVertex(startVertex);
    }
	inEdge->GetEndVertex(endVertex);
	if( endVertex.IsNotNULL() ) 
    {
        //Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_VertexPtr(F, endVertex);
		WriteVertex(endVertex);
    }

	//fprintf(F,"</Dtk_EdgePtr>");
	return dtkNoError;
}

Dtk_ErrorStatus vcCad2Cax::WriteVertex(const Dtk_VertexPtr& inVertex)
{
	if(inVertex.IsNULL())
		return dtkNoError;

	Dtk_PointPtr point;
	//fprintf(F,"<Dtk_VertexPtr>");
	//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_info(F, inVertex->get_info());
	//fprintf(F,"<Id>%d</Id>",inVertex->GetID());
	point = inVertex->GetGeom();
	if(point.IsNotNULL())
	{
		//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_PointPtr(F, point);
		WritePoint(point);
	}

	//fprintf(F,"</Dtk_VertexPtr>");
	return dtkNoError;
}

Dtk_ErrorStatus vcCad2Cax::WritePoint(const Dtk_PointPtr& inPoint)
{
	try
	{
		if(!g_bPointSet)
			return dtkNoError;

		int r=255,g=255,b=255,a=0;
		if(inPoint->get_info().IsNotNULL())
			GetColorInfo(inPoint->get_info(),r,g,b,a);

		std::map<Dtk_RGB*, std::vector<Dtk_PointPtr>* >::iterator MI;
		bool bFound = false;
		Dtk_RGB *pColor=NULL;
		for(MI=m_DtkColorNPointsArrayMap.begin();MI!=m_DtkColorNPointsArrayMap.end();MI++)
		{
			pColor = (Dtk_RGB *)MI->first;
			if(pColor->R() == r && pColor->G() == g && pColor->B() == b)
			{
				bFound = true;
				break;
			}
		}
		if(!bFound)
		{
			Dtk_RGB *pPointColor = new Dtk_RGB(); 
			pPointColor->SetRGBA(r,g,b,a);

			std::vector<Dtk_PointPtr> *pPointsArray = new std::vector<Dtk_PointPtr>;
			pPointsArray->push_back(inPoint);

			m_DtkColorNPointsArrayMap[pPointColor] = pPointsArray;
		}
		else
		{
			m_DtkColorNPointsArrayMap[pColor]->push_back(inPoint);
		}
	}
	catch(...)
	{
		g_bDataLoss = true;
		ss.str("");
		ss<<"vcCad2Cax::WritePoint(): ***Exception*** caught";
		vcUtils::LogMsg(ss.str());
	}	
	return dtkNoError;


	Dtk_pnt point;

	//fprintf(F,"<Dtk_PointPtr>");
	//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_info(F, inPoint->get_info());
	//fprintf(F,"<Id>%d</Id>",inPoint->GetID());
	inPoint->GetCoordinates(point);
	//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_pnt(F, point);

	//return dtkNoError;
	/*float x=point[0], y=point[1], z=point[2];

	float *pCaxCoordFloatArray = new float[12]; 
	
	float delimiter = 0.5;
	//point1
	pCaxCoordFloatArray[0] = x-delimiter;
	pCaxCoordFloatArray[1] = y+delimiter;
	pCaxCoordFloatArray[2] = z;

	//point2
	pCaxCoordFloatArray[3] = x+delimiter;
	pCaxCoordFloatArray[4] = y-delimiter;
	pCaxCoordFloatArray[5] = z;

	//point3
	pCaxCoordFloatArray[6] = x-delimiter;
	pCaxCoordFloatArray[7] = y-delimiter;
	pCaxCoordFloatArray[8] = z;

	//point4
	pCaxCoordFloatArray[9] = x+delimiter;
	pCaxCoordFloatArray[10] = y+delimiter;
	pCaxCoordFloatArray[11] = z;

	int *pCaxCoordIndexIntArray = new int[4];
	pCaxCoordIndexIntArray[0] = 0;
	pCaxCoordIndexIntArray[1] = 1;
	pCaxCoordIndexIntArray[2] = 2;
	pCaxCoordIndexIntArray[3] = 3;


	int *pCaxPolylenghtIntArray = new int[2];
	pCaxPolylenghtIntArray[0] = 2;
	pCaxPolylenghtIntArray[1] = 2;

	int iVerticesCount = 4;
	int iCoordIndexCount = 4;
	int iPrimCount = 2;*/

	/*float *pCaxCoordFloatArray = new float[3];
	pCaxCoordFloatArray[0] = point[0];
	pCaxCoordFloatArray[1] = point[1];
	pCaxCoordFloatArray[2] = point[2];

	int iVerticesCount  = 1;

	CaxId CaxShape;
	CaxShape = m_CaxScene.createObject(Cax::GEOMETRY);
	m_CaxScene.setName(CaxShape,"default");
	CaxId CurrentCaxParent = m_NodesArray.at(m_NodesArray.size()-1);
	m_CaxScene.addComponent(CurrentCaxParent,CaxShape);

	CaxId CaxMesh;
	CaxMesh = m_CaxScene.createObject(Cax::MESH);
	m_CaxScene.setMeshType(CaxMesh,Cax::POINT_SET);
	m_CaxScene.setName(CaxMesh,"mesh");
	m_CaxScene.addComponent(CaxShape,CaxMesh);

	CaxId CaxCoordSet = m_CaxScene.createObject(Cax::FLOAT_SET);
	m_CaxScene.setDimension(CaxCoordSet, 3);
	m_CaxScene.setValues(CaxCoordSet, pCaxCoordFloatArray, iVerticesCount);
	m_CaxScene.setCoordinates(CaxMesh,CaxCoordSet);

	int r=255,g=255,b=255,a=0;
	if(inPoint->get_info().IsNotNULL())
		GetColorInfo(inPoint->get_info(),r,g,b,a);

	CaxId CaxMaterial = m_CaxScene.createObject(Cax::MATERIAL);
	m_CaxScene.setName(CaxMaterial,"Material");
	m_CaxScene.setMaterial(CaxMesh,CaxMaterial);
	m_CaxScene.setColor(CaxMaterial,Cax::DIFFUSE,r/255.0f,g/255.0f,b/255.0f,1.0f);*/

	//fprintf(F,"</Dtk_PointPtr>");
	return dtkNoError;
}

Dtk_ErrorStatus vcCad2Cax::WriteCurve(const Dtk_CurvePtr &inCurve)
{
	type_detk type;
	if (inCurve.IsNULL())
	{
		return dtkErrorNullPointer; 
	}
	//fprintf(F,"<Dtk_CurvePtr>");
	//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_info(F, inCurve->get_info());
	//fprintf(F,"<Id>%d</Id>",inCurve->GetID());

	if(inCurve->IsTrimmed())
	{
		//fprintf(F,"<Domain>");

		//fprintf(F,"<UMin>%f</UMin>", inCurve->GetTrimUMin());
		//fprintf(F,"<UMax>%f</UMax>", inCurve->GetTrimUMax());

		//fprintf(F,"</Domain>");
	}
	type = inCurve->get_type_detk();
	switch(type)
	{
	case DTK_TYPE_LINE:
		{
			//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_LinePtr(F, Dtk_LinePtr::DtkDynamicCast(inCurve));
			WriteLine(Dtk_LinePtr::DtkDynamicCast(inCurve),inCurve->get_info());
			break;
		}
	case DTK_TYPE_POLYLINE:
		{
			//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_PolylinePtr(F, Dtk_PolylinePtr::DtkDynamicCast(inCurve));
			WritePolyline(Dtk_PolylinePtr::DtkDynamicCast(inCurve),inCurve->get_info());
			break;
		}
	case DTK_TYPE_NURBS_CURVE:
		{
			//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_NurbsCurvePtr(F, Dtk_NurbsCurvePtr::DtkDynamicCast(inCurve));
			WriteNurbsCurve(Dtk_NurbsCurvePtr::DtkDynamicCast(inCurve),inCurve->get_info());
			break;
		}
	case DTK_TYPE_HYPERBOLA:
		{
			//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_HyperbolaPtr(F, Dtk_HyperbolaPtr::DtkDynamicCast(inCurve));
			WriteHyperbola(Dtk_HyperbolaPtr::DtkDynamicCast(inCurve),inCurve->get_info());
			break;
		}
	case DTK_TYPE_PARABOLA:
		{
			//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_ParabolaPtr(F, Dtk_ParabolaPtr::DtkDynamicCast(inCurve));
			WriteParabola(Dtk_ParabolaPtr::DtkDynamicCast(inCurve),inCurve->get_info());
			break;
		}
	case DTK_TYPE_CIRCLE:
	case DTK_TYPE_ELLIPSE:
		{
			//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_EllipsePtr(F, Dtk_EllipsePtr::DtkDynamicCast(inCurve));
			WriteEllipse(Dtk_EllipsePtr::DtkDynamicCast(inCurve),inCurve->get_info());
			break;
		}
	default:
		{
			WriteNurbsCurve(Dtk_NurbsCurvePtr::DtkDynamicCast(inCurve->ToNurbs()),inCurve->get_info());
			//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_NurbsCurvePtr(F, Dtk_NurbsCurvePtr::DtkDynamicCast(inCurve->ToNurbs()));
			break;
		}
	}
	//fprintf(F,"</Dtk_CurvePtr>");
	return dtkNoError;
}

Dtk_ErrorStatus vcCad2Cax::WriteLine(const Dtk_LinePtr &inCurve,const Dtk_InfoPtr &Info)
{
	Dtk_PolylinePtr polyline = inCurve->ToPolyline(0,0.01);
	this->WritePolyline(polyline,Info);

	return dtkNoError;
}

Dtk_ErrorStatus vcCad2Cax::WritePolyline(const Dtk_PolylinePtr &inCurve,const Dtk_InfoPtr &Info)
{
	try
	{
		if(!g_b2DElements)
			return dtkNoError;

		int r=255,g=255,b=255,a=0;

		Dtk_RGB ComponentColor;// = new Dtk_RGB();
		bool bComponentColor = false;
		bComponentColor = IsComponentColor(&ComponentColor);
		if(bComponentColor)
		{
			r=ComponentColor.R();
			g=ComponentColor.G();
			b=ComponentColor.B();
		}
		else
		{
			if(Info.IsNotNULL())
				GetColorInfo(Info,r,g,b,a);
		}

		std::map<Dtk_RGB*, std::vector<Dtk_PolylinePtr>* >::iterator MI;
		bool bFound = false;
		Dtk_RGB *pColor=NULL;
		for(MI=m_DtkColorNLinesArrayMap.begin();MI!=m_DtkColorNLinesArrayMap.end();MI++)
		{
			pColor = (Dtk_RGB *)MI->first;
			if(pColor->R() == r && pColor->G() == g && pColor->B() == b)
			{
				bFound = true;
				break;
			}
		}
		if(!bFound)
		{
			Dtk_RGB *pCurveColor = new Dtk_RGB(); 
			pCurveColor->SetRGBA(r,g,b,a);

			std::vector<Dtk_PolylinePtr> *pPolylinesArray = new std::vector<Dtk_PolylinePtr>;
			pPolylinesArray->push_back(inCurve);

			m_DtkColorNLinesArrayMap[pCurveColor] = pPolylinesArray;
		}
		else
		{
			m_DtkColorNLinesArrayMap[pColor]->push_back(inCurve);
		}
	}
	catch(...)
	{
		g_bDataLoss = true;
		ss.str("");
		ss<<"vcCad2Cax::WritePolyline(): ***Exception*** caught";
		vcUtils::LogMsg(ss.str());
	}	
	return dtkNoError;
}


Dtk_ErrorStatus vcCad2Cax::WriteNurbsCurve(const Dtk_NurbsCurvePtr &inCurve,const Dtk_InfoPtr &Info)
{
	Dtk_Size_t num, i;
	//fprintf(F,"<Dtk_NurbsCurvePtr>");
	//fprintf(F,"<IsBezier>%d</IsBezier>",inCurve->IsBezier());
	//fprintf(F,"<IsRationnal>%d</IsRationnal>",inCurve->IsRationnal());
	//fprintf(F,"<IsUniform>%d</IsUniform>",inCurve->IsUniform());

	//fprintf(F,"<Degree>%d</Degree>",inCurve->GetDegree());

	//fprintf(F,"<Knots>");
	num = inCurve->GetNumKnots();
	//for(i = 0; i < num-1; i++)
	{
		//fprintf(F, "%f,",inCurve->GetKnotValue(i));
	}
	//fprintf(F, "%f",inCurve->GetKnotValue(i));
	//fprintf(F,"<Mult>");
	//for(i = 0; i < num-1; i++)
	{
		//fprintf(F, "%d,",inCurve->GetKnotMultiplicity(i));
	}
	//fprintf(F, "%d",inCurve->GetKnotMultiplicity(i));
	//fprintf(F,"</Mult>");
	//fprintf(F,"</Knots>");

	//fprintf(F,"<Weights>");
	num = inCurve->GetNumPoints();
	//fprintf(F,"<NumPoints>"__XmlWriteSize_tFormat"</NumPoints>", num);
	//for(i = 0; i < num-1; i++)
	{
		//fprintf(F,"%f,",inCurve->GetWeight(i));
	}

	//fprintf(F,"%f",inCurve->GetWeight(i));
	//fprintf(F,"</Weights>");

	//fprintf(F,"<Points>");
	Dtk_PolylinePtr polyline = inCurve->ToPolyline(0,0.01);
	this->WritePolyline(polyline,Info);
	//fprintf(F,"</Points>");
	//fprintf(F,"</Dtk_NurbsCurvePtr>");
	return dtkNoError;
}

Dtk_ErrorStatus vcCad2Cax::WriteEllipse(const Dtk_EllipsePtr &inCurve,const Dtk_InfoPtr &Info)
{
	/*fprintf(F,"<Dtk_EllipsePtr>");
	fprintf(F,"<Center>");
	Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_pnt(F, inCurve->GetCenterPoint());
	fprintf(F,"</Center>");
	fprintf(F,"<XDir>");
	Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_dir(F, inCurve->GetXDirection());
	fprintf(F,"</XDir>");
	fprintf(F,"<YDir>");
	Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_dir(F, inCurve->GetYDirection());
	fprintf(F,"</YDir>");
	fprintf(F,"<ZDir>");
	Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_dir(F, inCurve->GetZDirection());
	fprintf(F,"</ZDir>");
	fprintf(F,"<MajorRadius>%f</MajorRadius>", inCurve->GetMajorRadius());
	fprintf(F,"<MinorRadius>%f</MinorRadius>", inCurve->GetMinorRadius());
	fprintf(F,"</Dtk_EllipsePtr>");*/

	Dtk_PolylinePtr polyline = inCurve->ToPolyline(0,0.01);
	this->WritePolyline(polyline,Info);

	return dtkNoError;
}


Dtk_ErrorStatus vcCad2Cax::WriteHyperbola(const Dtk_HyperbolaPtr &inCurve,const Dtk_InfoPtr &Info)
{
	/*fprintf(F,"<Dtk_HyperbolaPtr>");
	fprintf(F,"<Center>");
	Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_pnt(F, inCurve->GetCenterPoint());
	fprintf(F,"</Center>");
	fprintf(F,"<XDir>");
	Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_dir(F, inCurve->GetXDirection());
	fprintf(F,"</XDir>");
	fprintf(F,"<YDir>");
	Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_dir(F, inCurve->GetYDirection());
	fprintf(F,"</YDir>");
	fprintf(F,"<ZDir>");
	Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_dir(F, inCurve->GetZDirection());
	fprintf(F,"</ZDir>");
	fprintf(F,"<SemiAxis>%f</SemiAxis>", inCurve->GetSemiAxis());
	fprintf(F,"<SemiImageAxis>%f</SemiImageAxis>", inCurve->GetSemiImageAxis());
	fprintf(F,"</Dtk_HyperbolaPtr>");*/

	Dtk_PolylinePtr polyline = inCurve->ToPolyline(0,0.01);
	this->WritePolyline(polyline,Info);

	return dtkNoError;
}

Dtk_ErrorStatus vcCad2Cax::WriteParabola(const Dtk_ParabolaPtr &inCurve,const Dtk_InfoPtr &Info)
{
	/*fprintf(F,"<Dtk_ParabolaPtr>");

	fprintf(F,"<Center>");
	Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_pnt(F, inCurve->GetCenterPoint());
	fprintf(F,"</Center>");
	fprintf(F,"<XDir>");
	Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_dir(F, inCurve->GetXDirection());
	fprintf(F,"</XDir>");
	fprintf(F,"<YDir>");
	Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_dir(F, inCurve->GetYDirection());
	fprintf(F,"</YDir>");
	fprintf(F,"<ZDir>");
	Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_dir(F, inCurve->GetZDirection());
	fprintf(F,"</ZDir>");
	fprintf(F,"<FocalDistance>%f</FocalDistance>", inCurve->GetFocalDistance());

	fprintf(F,"</Dtk_ParabolaPtr>");*/

	Dtk_PolylinePtr polyline = inCurve->ToPolyline(0,0.01);
	this->WritePolyline(polyline,Info);

	return dtkNoError;
}


Dtk_ErrorStatus vcCad2Cax::GetColorInfo(const Dtk_InfoPtr& I,int &r,int &g,int &b,int &a)
{
    Dtk_status st;
    Dtk_MaterialPtr material = NULL;

	if (I.IsNULL())
		return dtkNoError;

	Dtk_tab<Dtk_string> lst;
	I->ListAllAttributes(lst);
	Dtk_Size_t i;
	Dtk_Size_t size = lst.size();
	//if (size > 0)
		//fprintf(F,"<Dtk_Info>\n");

	//fprintf(F,"<Dtk_ID>%i</Dtk_ID>\n",I->GetId());


	for(i=0;i<size;i++)
	{
		Dtk_Val v;
		if (lst[i]=="Dtk_ColorId")
		{
			int index = I->GetColorId();
			//int r,g,b,a;
			//commented by sethil
			int res = 0;//dtk_GetRGBColor(index,&r,&g,&b,&a);
			if(res!=0)
			{	
				r=-1;g=-1;b=-1;a=-1;
			}
			/*if (res==0)
				fprintf(F,"<Dtk_Color><index>%d</index><r>%d</r><g>%d</g><b>%d</b><a>%d</a></Dtk_Color>\n",index,r,g,b,a);
			else
				fprintf(F,"<Dtk_Color><index>%d</index><r>%d</r><g>%d</g><b>%d</b><a>%d</a></Dtk_Color>\n",index,-1,-1,-1,-1);*/
		}
		/*else
		{
            if(lst[i]=="Dtk_Uuid")
            {
                Dtk_Size_t i;
                Dtk_status st;
                Dtk_UUID uuid;
                uuid = I->GetUuid();
                fprintf(F,"<Dtk_uuid>\n");
                for(i=0;i<4;i++)
                {
                    Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_val(F,uuid[i]);
                }
                fprintf(F,"</Dtk_uuid>\n");	
            }
            else
            {
                I->FindAttribute(lst[i],v);
//DTK_TOREMOVE_START
#ifdef AF51BDA7E49648f8AB95949F4CA52EF8
				char	namebuf[256];
				sanitiseXML(namebuf,256,"%s",lst[i].c_str());
				fprintf(F,"<%s>\n",namebuf);
                Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_val(F,v);
                fprintf(F,"</%s>\n",namebuf);
#else
//DTK_TOREMOVE_END
                fprintf(F,"<%s>\n",lst[i].c_str());
                Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_val(F,v);
                fprintf(F,"</%s>\n",lst[i].c_str());
#endif //DTK_TOREMOVE
            }
		}*/
	}

	//if (size > 0) 
		//fprintf(F,"</Dtk_Info>\n");
	return dtkNoError;
}

bool vcCad2Cax::ConstructShapeForPointElements(Dtk_string sName)
{
	try
	{
		if(g_bDetailedLog)
		{
			vcUtils::LogMsg("vcCad2Cax::ConstructShapeForPointElements():Start...");
		}

		std::string memusage;
		if(g_bDetailedLog)
		{
			memusage = vcUtils::GetMemoryUsage();
			ss.str("");
			ss<<"vcCad2Cax::ConstructShapeForPointElements():Memory Usage b4 creating CAX geometry: "<<memusage;
			vcUtils::LogMsg(ss.str());
		}

		if(g_bDetailedLog)
		{
			ss.str("");
			ss<<"vcCad2Cax::ConstructShapeForPointElements():No. of Cax shapes: "<<m_DtkColorNPointsArrayMap.size();
			vcUtils::LogMsg(ss.str());
		}
		std::map<Dtk_RGB*, std::vector<Dtk_PointPtr>* >::iterator MI;
		bool bFound = false;
		int r,g,b,a;

		if(m_DtkColorNPointsArrayMap.size()>1)
		{
			//CAX-Group
			CaxId CaxGroup = m_CaxScene.createObject(Cax::ASSEMBLY);
			m_CaxScene.setName(CaxGroup,GetValidNodeName(sName.c_str()).c_str());
			CaxId CurrentCaxParent = m_NodesArray.at(m_NodesArray.size()-1);
			m_CaxScene.addComponent(CurrentCaxParent,CaxGroup);
			m_NodesArray.push_back(CaxGroup);
		}

		int iShapeIndex = 0;
		std::map<std::string,CaxId>::iterator NameNCaxIdMapIterator;
		for(MI=m_DtkColorNPointsArrayMap.begin();MI!=m_DtkColorNPointsArrayMap.end();MI++)
		{
			if(g_bDetailedLog)
			{
				ss.str("");
				ss<<"vcCad2Cax::ConstructShapeForPointElements():Shape : "<<iShapeIndex;
				vcUtils::LogMsg(ss.str());
			}

			Dtk_RGB *pColor = (Dtk_RGB *)MI->first;
			r = pColor->R();
			g = pColor->G();
			b = pColor->B();
			a = pColor->A();

			std::vector<Dtk_PointPtr> *pPointsArray = (std::vector<Dtk_PointPtr>*)MI->second;
			int iVerticesCount=pPointsArray->size();

			if(!iVerticesCount)
				continue;

			iShapeIndex++;
			ss.str("");
			m_iShapeIndexOfComponent++;
			//ss<<sName<<"#"<<m_iShapeIndexOfComponent<<"#PointSet";
			ss<<sName<<"#Points";
			std::string sShapeName = ss.str();
			ss.str("");

	#ifdef VCT_DEF_AND_USE
			NameNCaxIdMapIterator = m_NameNCaxIdMap.find(sShapeName);
			if(NameNCaxIdMapIterator!=m_NameNCaxIdMap.end())
			{
				CaxId caxId = NameNCaxIdMapIterator->second;
				CaxId CurrentCaxParent = m_NodesArray.at(m_NodesArray.size()-1);
				m_CaxScene.addComponent(CurrentCaxParent,caxId);
				if(g_bDetailedLog)
				{
					ss.str("");
					ss<<"vcCad2Cax::ConstructShapeForPointElements(): Shape : "<<sShapeName<<" is reused";
					vcUtils::LogMsg(ss.str());
				}
				continue;
			}
	#endif

			if(g_bDetailedLog)
			{
				ss.str("");
				ss<<"vcCad2Cax::ConstructShapeForPointElements(): Coordinates size : "<<iVerticesCount;
				vcUtils::LogMsg(ss.str());
			}

			float *pCaxCoordFloatArray = new float[iVerticesCount*3]; 
			FloatArrays.push_back(pCaxCoordFloatArray);
			int j=0;

			for(int k=0;k<pPointsArray->size();k++)
			{
				Dtk_PointPtr DtkPoint = pPointsArray->at(k);
		
				Dtk_pnt point;
				DtkPoint->GetCoordinates(point);
				pCaxCoordFloatArray[j++]=point[0]*m_fUnit;
				pCaxCoordFloatArray[j++]=point[1]*m_fUnit;
				pCaxCoordFloatArray[j++]=point[2]*m_fUnit;
			}
			
			if(g_bDetailedLog)
			{
				ss.str(""); 
				ss<<"vcCad2Cax::ConstructShapeForPointElements(): coordinates are done";
				vcUtils::LogMsg(ss.str());
			}	

			CaxId CaxShape;
			CaxShape = m_CaxScene.createObject(Cax::GEOMETRY);
			m_CaxScene.setName(CaxShape,GetValidNodeName(sShapeName.c_str()).c_str());;
			CaxId CurrentCaxParent = m_NodesArray.at(m_NodesArray.size()-1);
			m_CaxScene.addComponent(CurrentCaxParent,CaxShape);

			if(g_bDetailedLog)
			{
				ss.str("");
				ss<<"vcCad2Cax::ConstructShapeForPointElements():Cax Shape object is created ";
				vcUtils::LogMsg(ss.str());
			}

	#ifdef VCT_DEF_AND_USE
			m_NameNCaxIdMap[sShapeName] = CaxShape;
	#endif

			CaxId CaxMesh;
			CaxMesh = m_CaxScene.createObject(Cax::MESH);
			m_CaxScene.setMeshType(CaxMesh,Cax::POINT_SET);
			m_CaxScene.setName(CaxMesh,GetValidNodeName("pointset").c_str());
			m_CaxScene.addComponent(CaxShape,CaxMesh);

			if(g_bDetailedLog)
			{
				ss.str("");
				ss<<"vcCad2Cax::ConstructShapeForPointElements():Cax Shape is added to Cax Scene ";
				vcUtils::LogMsg(ss.str());
			}

			CaxId CaxCoordSet = m_CaxScene.createObject(Cax::FLOAT_SET);
			m_CaxScene.setDimension(CaxCoordSet, 3);
			m_CaxScene.setValues(CaxCoordSet, pCaxCoordFloatArray, iVerticesCount);
			m_CaxScene.setCoordinates(CaxMesh,CaxCoordSet);

			if(g_bDetailedLog)
			{
				ss.str("");
				ss<<"vcCad2Cax::ConstructShapeForPointElements(): Cax shape is completed";
				vcUtils::LogMsg(ss.str());
			}

			CaxId CaxMaterial = m_CaxScene.createObject(Cax::MATERIAL);
			m_CaxScene.setName(CaxMaterial,GetValidNodeName("Material").c_str());
			m_CaxScene.setMaterial(CaxMesh,CaxMaterial);
			m_CaxScene.setColor(CaxMaterial,Cax::DIFFUSE,r/255.0f,g/255.0f,b/255.0f,1.0f);

			if(g_bDetailedLog)
			{
				ss.str("");
				ss<<"vcCad2Cax::ConstructShapeForPointElements(): Material is set";
				vcUtils::LogMsg(ss.str());
			}
			if(g_bTestResults)
			{
				m_TestResultsFile<<"Shape Name: "<<sShapeName.c_str()<<std::endl;
				m_TestResultsFile<<"Shape Type: Cax::POINT_SET"<<std::endl;
				m_TestResultsFile<<"Vertices Count: "<<iVerticesCount<<std::endl;
				m_TestResultsFile<<"Material: "<<r<<" "<<g<<" "<<b<<" "<<a<<std::endl;
				m_iTotalPointSets++;
			}	
		}

		if(m_DtkColorNPointsArrayMap.size()>1)
		{
			if(m_NodesArray.size())
				m_NodesArray.pop_back();
		}

		if(g_bDetailedLog)
		{
			memusage = vcUtils::GetMemoryUsage();
			ss.str("");
			ss<<"vcCad2Cax::ConstructShapeForPointElements():Memory Usage after creating CAX geometry: "<<memusage;
			vcUtils::LogMsg(ss.str());
		}

		//Cleaning the Map
		std::vector<Dtk_PointPtr> *PointsArray = NULL;	
		for(MI=m_DtkColorNPointsArrayMap.begin();MI!=m_DtkColorNPointsArrayMap.end();MI++)
		{
			PointsArray = MI->second;
			for(int i=0;i<PointsArray->size();i++)
			{
				Dtk_PointPtr Point = (Dtk_PointPtr)PointsArray->at(i);
				Point = NULL;
			}
			delete PointsArray;

			Dtk_RGB* Color = MI->first; 
			Color = NULL;
		}
		m_DtkColorNPointsArrayMap.clear();

		if(g_bDetailedLog)
		{
			memusage = vcUtils::GetMemoryUsage();
			ss.str("");
			ss<<"vcCad2Cax::ConstructShapeForPointElements():Memory Usage(end) : "<<memusage;
			vcUtils::LogMsg(ss.str());

			vcUtils::LogMsg("vcCad2Cax::ConstructShapeForPointElements():End");
		}

	}
	catch(...)
	{
		g_bDataLoss = true;
		ss.str("");
		ss<<"vcCad2Cax::ConstructShapeForPointElements(): ***Exception*** caught";
		vcUtils::LogMsg(ss.str());
	}
	return true;
}

float vcCad2Cax::ConvertUnits(int iInputUnit, int iOutputUnit)
{
	float fNewUnit = 1.0f;
	switch(iInputUnit)
	{
		case  vcCad2Cax::INCH :
		{
			switch(iOutputUnit)
			{
				case vcCad2Cax::METER :
				{
					fNewUnit = 0.0254f;
					break;
				}
				case vcCad2Cax::MILLIMETER :
				{
					fNewUnit = 25.4f;
					break;
				}
				case vcCad2Cax::FEET :
				{
					fNewUnit = 0.083333f;
					break;
				}
				case vcCad2Cax::CENTIMETER :
				{
					fNewUnit = 2.54f;
					break;
				}
			}
			break;
		}
		case  vcCad2Cax::METER :
		{
			switch(iOutputUnit)
			{
				case vcCad2Cax::INCH :
				{
					fNewUnit = 39.3701f;
					break;
				}
				case vcCad2Cax::MILLIMETER :
				{
					fNewUnit = 1000.f;
					break;
				}
				case vcCad2Cax::FEET :
				{
					fNewUnit = 3.28084;
					break;
				}
				case vcCad2Cax::CENTIMETER :
				{
					fNewUnit = 100.f;
					break;
				}
			}
			break;
		}	
		case  vcCad2Cax::MILLIMETER :
		{
			switch(iOutputUnit)
			{
				case vcCad2Cax::INCH :
				{
					fNewUnit = 0.0393701f;
					break;
				}
				case vcCad2Cax::METER :
				{
					fNewUnit = 0.001f;
					break;
				}
				case vcCad2Cax::FEET :
				{
					fNewUnit = 0.00328084f;
					break;
				}
				case vcCad2Cax::CENTIMETER :
				{
					fNewUnit = 0.1f;
					break;
				}
			}
			break;
		}
		case  vcCad2Cax::CENTIMETER :
		{
			switch(iOutputUnit)
			{
				case vcCad2Cax::INCH :
				{
					fNewUnit = 0.393701f;
					break;
				}
				case vcCad2Cax::METER :
				{
					fNewUnit = 0.01f;
					break;
				}
				case vcCad2Cax::FEET :
				{
					fNewUnit = 0.0328084f;
					break;
				}
				case vcCad2Cax::MILLIMETER :
				{
					fNewUnit = 10.f;
					break;
				}
			}
			break;
		}
		case  vcCad2Cax::FEET :
		{
			switch(iOutputUnit)
			{
				case vcCad2Cax::INCH :
				{
					fNewUnit = 12.f;
					break;
				}
				case vcCad2Cax::METER :
				{
					fNewUnit = 0.3048f;
					break;
				}
				case vcCad2Cax::MILLIMETER :
				{
					fNewUnit = 304.8f;
					break;
				}
				case vcCad2Cax::CENTIMETER :
				{
					fNewUnit = 30.48f;
					break;
				}
			}
			break;
		}
	}
	return fNewUnit;
}

void vcCad2Cax::InitializeTestResults()
{
	//vcUtils::LogMsg("vcCad2Cax::InitializeTestResults(): Start");
	if(g_bTestResults)
	{
		vcUtils::LogMsg("vcCad2Cax::InitializeTestResults(): Create Report enabled");
		if(m_sInputFileName.is_NULL() || m_sOutputFileName.is_NULL())
		{
			vcUtils::LogMsg("vcCad2Cax::InitializeTestResults(): Create Report failed 1");
			g_bTestResults = false;
			return;
		}
		if(m_sOutputFileName.is_not_NULL())
		{
			g_sTestResultsFileName = std::string(m_sOutputFileName.c_str());
			g_sTestResultsFileName.replace(g_sTestResultsFileName.end()-4,g_sTestResultsFileName.end(),".txt");
		}
		else
		{
			vcUtils::LogMsg("vcCad2Cax::InitializeTestResults(): Create Report failed 2");
			g_bTestResults = false;
			return;
		}
		m_TestResultsFile.open(g_sTestResultsFileName.c_str(),std::ios::out);
		if(!m_TestResultsFile.is_open())
		{
			vcUtils::LogMsg("vcCad2Cax::InitializeTestResults(): Create Report failed 3");
			g_bTestResults = false;
			return;
		}
	 	m_iTotalShapes = 0;
		m_iTotalComponents = 0;
		m_iTotalSurfaces = 0;
		m_iTotalLines = 0;
		m_iTotalPointSets = 0;

		m_TestResultsFile<<"InputFile: "<<vcUtils::GetFileNameWithExtn(m_sInputFileName.c_str()).c_str()<<std::endl;
		m_TestResultsFile<<"OutputFile: "<<vcUtils::GetFileNameWithExtn(m_sOutputFileName.c_str()).c_str()<<std::endl;
	}
}

void ClearMemory()
{
	for(int i=0;i<FloatArrays.size();i++)
	{
		float *pFloat = (float*)FloatArrays[i];
		delete []pFloat;
		pFloat = NULL;
	}
	for(int i=0;i<IntArrays.size();i++)
	{ 
		float *pInt = (float*)IntArrays[i];
		delete []pInt;
		pInt = NULL;
	}

	if(FloatArrays.size())
		FloatArrays.erase(FloatArrays.begin(),FloatArrays.end());
	if(IntArrays.size())
		IntArrays.erase(IntArrays.begin(),IntArrays.end());
}