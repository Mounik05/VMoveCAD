#include <Windows.h>
#include "vcExtractMetadata.h"
#include <float.h>
#include "vcUtils.h"
#include "StopWatch.h"
#include <sstream>

vcExtractMetadata::vcExtractMetadata()
{
	m_fMinDistance = FLT_MAX;
}

vcExtractMetadata::~vcExtractMetadata()
{

}

float vcExtractMetadata::GetMinimumDistance()
{
	return m_fMinDistance;
}

bool vcExtractMetadata::ExtractMetadata(std::string _sInputFile, std::string _sTmpWorkingDir)
{
	m_sInputFileName = _sInputFile.c_str();
	m_sTmpWorkingDir = _sTmpWorkingDir.c_str();
	
	DtkErrorStatus errorStatus = dtkNoError;
	EnableCADReaders();
	try
	{
		errorStatus = StartDtkAPI();
	}
	catch (...)
	{
		m_iErrorCode = DATAKIT_ERROR;
		return false;
	}
	if (errorStatus != dtkNoError)
	{
		m_dtkErrorStatus = errorStatus;
		m_iErrorCode = DATAKIT_ERROR;
		return false;
	}

	Dtk_MainDocPtr CadDoc;
	try
	{
		errorStatus = OpenCADFile(CadDoc);
	}
	catch (...)
	{
		m_iErrorCode = DATAKIT_ERROR;
		return false;
	}
	if (errorStatus != dtkNoError && CadDoc.IsNULL())
	{
		m_dtkErrorStatus = errorStatus;
		m_iErrorCode = DATAKIT_ERROR;
		StopDtkAPI();
		return false;
	}

	try
	{
		errorStatus = ParseCadFile(CadDoc);
	}
	catch (...)
	{
		m_iErrorCode = DATAKIT_ERROR;
		return false;
	}
	if (errorStatus != dtkNoError)
	{
		m_dtkErrorStatus = errorStatus;
		m_iErrorCode = DATAKIT_ERROR;
		StopDtkAPI();
		return false;
	}

	CloseCADFile(CadDoc);

	StopDtkAPI();

	return true;
}

void vcExtractMetadata::EnableCADReaders()
{
	CatiaV6Reader::Enable();
	CatiaV4Reader::Enable();
	CatiaV5Reader::Enable();
	IgesReader::Enable();
	ProeReader::Enable();
	StepReader::Enable();
	UgReader::Enable();
	SwReader::Enable();
	SeReader::Enable();
	InvReader::Enable();
	CgrReader::Enable();
	PsReader::Enable();
	AcisReader::Enable();
	RevitReader::Enable();
	IfcReader::Enable();
	StlReader::Enable();
}

Dtk_ErrorStatus vcExtractMetadata::StartDtkAPI()
{
	Dtk_ErrorStatus errorStatus = dtkNoError;
	std::stringstream ss;
#ifdef _LINUX64_
	if (m_sTmpWorkingDir[m_sTmpWorkingDir.len() - 1] != '/')
		m_sTmpWorkingDir.Merge("/");
#else
	if (m_sTmpWorkingDir[m_sTmpWorkingDir.len() - 1] != '\\')
		m_sTmpWorkingDir.Merge("\\");
#endif

	try
	{
		m_pDtkAPI = Dtk_API::StartAPI(m_sTmpWorkingDir, errorStatus, "VCOLLAB_SDK-170092217140010146187057140087227106200052053225206228253120022230154211033210227167181138169073103011117156088102093069252055244082189172018211172233079224079026239024130161046029174225164029254174199084181068197145000193037147121147071067175206137134191135246139094253034071232108167067156209204041162254022101034234144187006124072189031160233154208171163016244001047205185142117054206218112117081216204129175174117209165012129039149135093061082030195171114251117129028092211062011047119158081247159176232198054240139121174076028165212136137167122005230231178056230038012054186217056187025101060190068130097026035106034050151119110040148037156205072250084236203006095188028052192062211177002049174124208241255232035013086001056124255185056143057182218121164205065053252092092202225052042226117159250108164074003165148177001034046176056160147194225011059120138211228077120123215251086249158058108075120005140255-304V232");
	}
	catch (...)
	{
		vcUtils::LogMsg("vcCad2Cax::StartDtkAPI(): Dtk_API::StartAPI() ***Exception*** caught");
		return errorStatus;
	}

	ss.str("");
	ss << "vcCad2Cax::StartDtkAPI(): StartAPI()::Error Status: " << dtkTypeError(errorStatus);
	vcUtils::LogMsg(ss.str());
	ss.str("");

	if (errorStatus == dtkErrorLicence)
	{
		vcUtils::LogMsg("vcCad2Cax::StartDtkAPI(): No license available");
		return errorStatus;

#ifdef VMOVECAD_BATCH
		printf("\nError : No license available.  Error Code : %s\n", dtkTypeError(errorStatus).c_str());
		printf("Please refer troubleshooting page of VMoveCAD help\n");
#endif
		ss.str("");
		ss << "vcCad2Cax::StartDtkAPI():No license available :Error Status: " << dtkTypeError(errorStatus);
		vcUtils::LogMsg(ss.str());
		ss.str("");
		vcUtils::LogMsg(ss.str());
		return errorStatus;
	}

	if (errorStatus == dtkErrorOpenFiles)
	{
#ifdef VMOVECAD_BATCH
		printf("\nError : Not able to read CAD file.  Error Code : %s\n", dtkTypeError(errorStatus).c_str());
		printf("Please refer troubleshooting page of VMoveCAD help\n");
#endif
		ss.str("");
		ss << "vcCad2Cax::StartDtkAPI():Failed to read CAD file:Error Status: " << dtkTypeError(errorStatus);
		vcUtils::LogMsg(ss.str());
		ss.str("");
		vcUtils::LogMsg(ss.str());
		return errorStatus;
	}

	if (errorStatus != dtkNoError)
	{
#ifdef VMOVECAD_BATCH
		printf("\nError:Not able to read the CAD file.  Error Code : %s\n", dtkTypeError(errorStatus).c_str());
		printf("Please refer troubleshooting page of VMoveCAD help\n");
#endif
		ss.str("");
		ss << "vcCad2Cax::StartDtkAPI():Failed to read CAD file:Error Status : " << dtkTypeError(errorStatus);
		vcUtils::LogMsg(ss.str());
		ss.str("");
		return errorStatus;
	}

	if (m_pDtkAPI == NULL)
	{
		printf("Can't Start DATAKIT API\n");
		vcUtils::LogMsg("vcCad2Cax::StartDtkAPI():Failed to start DATAKIT API");
		return dtkErrorAPINotStarted;
	}
	vcUtils::LogMsg("vcCad2Cax::StartDtkAPI():End");
	return errorStatus;
}

void vcExtractMetadata::StopDtkAPI()
{
	vcUtils::LogMsg("vcCad2Cax::StopAPI():Start");
	Dtk_API::StopAPI(m_pDtkAPI);
	vcUtils::LogMsg("vcCad2Cax::StopAPI():End");
}

Dtk_ErrorStatus vcExtractMetadata::OpenCADFile(Dtk_MainDocPtr& TmpDoc)
{
	vcUtils::LogMsg("vcCad2Cax::OpenCADFile():Start");
	std::stringstream ss;

	// You Get the current API 
	DtkErrorStatus stError = dtkNoError;
	Dtk_API* MyAPI = Dtk_API::GetAPI();

	//Set the Schema directory needed for readers based on Pskernel (UG, Solidworks, Solidedge), or CADDS
#ifdef WIN32
	try
	{
		char szModuleFileName[MAX_PATH];
		GetModuleFileName(NULL, szModuleFileName, sizeof(szModuleFileName));
		std::string Path = vcUtils::GetPathFromFileName(szModuleFileName);
		Dtk_string inRepSchema = Path.c_str() + Dtk_string("\\schema");

		ss.str("");
		ss << "vcCad2Cax::OpenCADFile():PsKernal Schema Path: " << inRepSchema.c_str();
		vcUtils::LogMsg(ss.str());
		ss.str("");

		char TmpFullPathSchemaDir[_MAX_PATH];
		if (_fullpath(TmpFullPathSchemaDir, inRepSchema.c_str(), _MAX_PATH) != NULL)
			stError = MyAPI->SetSchemaDir(TmpFullPathSchemaDir);
		else
			stError = MyAPI->SetSchemaDir(inRepSchema);
	}
	catch (...)
	{
		vcUtils::LogMsg("vcCad2Cax::OpenCADFile(): SetSchemaDir ***Exception*** caught");
	}
#else 
	Dtk_string inRepSchema = "./Schema";
	MyAPI->SetSchemaDir(inRepSchema);
#endif 


	//If you want to get a log file for reader (inventory, missing files in assembly...) you have to set it
	std::string dtklogpath;
#ifdef _LINUX64_
	dtklogpath = m_sTmpWorkingDir.c_str() + std::string("/DtkLogFile.txt");
#else
	dtklogpath = m_sTmpWorkingDir.c_str() + std::string("\\DtkLogFile.txt");
#endif
	Dtk_string DtkLogFilePath = dtklogpath.c_str();
	MyAPI->SetLogFile(DtkLogFilePath);

	std::string memusage = vcUtils::GetMemoryUsage();
	ss.str("");
	ss << "vcCad2Cax::OpenCADFile:b4 OpenDocument():Memory Usage: " << memusage;
	vcUtils::LogMsg(ss.str());

	Dtk_ErrorStatus errorStatus;
	try
	{
		//You Open the file you want to read and get corresponding Document 
		errorStatus = MyAPI->OpenDocument(m_sInputFileName, TmpDoc);
	}
	catch (...)
	{
#ifdef VMOVECAD_BATCH
		printf("\nError : Not able to open CAD file. \n");
		printf("Please contact support@vcollab.com for further support\n");
#endif
		ss.str("");
		ss << "vcCad2Cax::OpenCADFile():MyAPI->OpenDocument() ***Exception*** caught ";
		vcUtils::LogMsg(ss.str());
		ss.str("");
		vcUtils::LogMsg(ss.str());
		return errorStatus;
	}

	ss.str("");
	ss << "vcCad2Cax::OpenCADFile():OpenDocument():Error Status: " << dtkTypeError(errorStatus);
	vcUtils::LogMsg(ss.str());
	ss.str("");

	if (errorStatus == dtkErrorUnavailableReader)
	{
		std::string err;
		ss.str("");
		ss << "License is not available for this format. Contact support@vcollab.com";
		vcUtils::LogMsg(ss.str(), true);
		return errorStatus;
	}
	memusage = vcUtils::GetMemoryUsage();
	ss.str("");
	ss << "vcCad2Cax::OpenCADFile: after OpenDocument():Memory Usage: " << memusage;
	vcUtils::LogMsg(ss.str());

	//If no Error we write the Document
	if (errorStatus == dtkNoError && TmpDoc.IsNotNULL())
	{
		vcUtils::LogMsg("vcCad2Cax::OpenCADFile():OpenDocument succeeded");
	}
	else
	{
		if (errorStatus != dtkNoError)
		{
#ifdef VMOVECAD_BATCH
			printf("\nError : Not able to open CAD file.  Error Code : %s\n", dtkTypeError(errorStatus).c_str());
			printf("Please refer troubleshooting page of VMoveCAD help\n");
#endif
			ss.str("");
			ss << "vcCad2Cax::OpenCADFile():Failed to open CAD file:Error Status: " << dtkTypeError(errorStatus);
			vcUtils::LogMsg(ss.str());
			ss.str("");
			vcUtils::LogMsg(ss.str());
			return errorStatus;
		}
	}

	vcUtils::LogMsg("vcCad2Cax::OpenCADFile():End");
	return errorStatus;
}
Dtk_ErrorStatus vcExtractMetadata::CloseCADFile(Dtk_MainDocPtr TmpDoc)
{
	vcUtils::LogMsg("vcCad2Cax::CloseCADFile():Start");
	std::stringstream ss;

	// You Get the current API 
	DtkErrorStatus errorStatus = dtkNoError;
	Dtk_API* MyAPI = Dtk_API::GetAPI();
	//We close the opened document
	try
	{
		if (MyAPI)
			errorStatus = MyAPI->EndDocument(TmpDoc);
	}
	catch (...)
	{
		vcUtils::LogMsg("vcCad2Cax::CloseCADFile():MyAPI->EndDocument(TmpDoc) ***Exception*** caught");
	}

	ss.str("");
	ss << "vcCad2Cax::CloseCADFile():MyAPI->EndDocument():Error Status: " << dtkTypeError(errorStatus);
	vcUtils::LogMsg(ss.str());
	ss.str("");

	vcUtils::LogMsg("vcCad2Cax::CloseCADFile():End");
	return errorStatus;
}

Dtk_ErrorStatus vcExtractMetadata::ParseCadFile(Dtk_MainDocPtr inDocument)
{
	vcUtils::LogMsg("vcCad2Cax::ParseCadFile():Start");
	Dtk_ComponentPtr RootComponent = inDocument->RootComponent();

	if (RootComponent.IsNotNULL())
	{
		ReadComponent(RootComponent);
	}
	vcUtils::LogMsg("vcCad2Cax::ParseCadFile():RootComponent=NULL");
	return dtkNoError;
}

Dtk_ErrorStatus vcExtractMetadata::ReadComponent(Dtk_ComponentPtr inComponent, const Dtk_transfo& inMatrix)
{
	Dtk_Size_t i;

	//Datakit component are given in MM, if original model had other unit you can get it with GetConceptionUnitScale()
	// return 25.4 for inch 
	double UnitFactor = inComponent->GetConceptionUnitScale();

	//GetName
	Dtk_string ComponentName = inComponent->Name();

	//GetAttributes
	Dtk_InfoPtr attributes = inComponent->GetInfos();
	if (attributes.IsNotNULL())
	{
		int CompoActivationStatus = attributes->GetActivationFlag(); // if CompoActivationStatus == 0 Component and his children aren't visible
		int CompoBlankedStatus = attributes->GetBlankedStatus(); // if CompoBlankedStatus == 1 Component and his children aren't visible
		Dtk_RGB CompoColor = attributes->GetColor(); // if CompoBlankedStatus == 1 Component and his children are using this color 
	}

	//You can GetPreview if you want to handle it
	/*Dtk_PreviewPtr TmpPreview = inComponent->GetPreview();
	if (TmpPreview.IsNotNULL())
	{
		Dtk_Int32 size = TmpPreview->GetStreamSize();
		char* jpgimage = TmpPreview->GetStream();
		Dtk_string Preview_name = "ComponentPreview.jpg";
		FILE* jpg = Preview_name.OpenFile("wb");
		if (jpg)
		{
			fwrite(jpgimage, sizeof(char), size, jpg);
			fclose(jpg);
		}
	}*/

	//You have 4 types for Component
	Dtk_Component::ComponentTypeEnum type = inComponent->ComponentType();
	switch (type)
	{
		//Instance represent a prototype with a matrix placement
		case Dtk_Component::InstanceComponentType:
		{
			ReadInstance(inComponent, inMatrix);
			break;
		}
		//Prototype (you have to check if you ever read and write it to don't waste time)
		//you can use the method inComponent->GetID() to get Unique ID for Component
		case Dtk_Component::PrototypeComponentType:
		{
			ReadPrototype(inComponent, inMatrix);
			break;
		}
		//Catalog Component represent a choice of several possible configuration 
		//(like scene in catiav5, workspace in catiav4, configuration in solidworks)
		//Default is the first child 
		case Dtk_Component::CatalogComponentType:
		{
			Dtk_string name = inComponent->Name();//Component name

			Dtk_Size_t numComp = inComponent->GetNumChildren();
			if (numComp > 0)
			{
				Dtk_Int32 defaultindex = inComponent->GetDefaultChildInCatalog(); //Get Default child to use
				Dtk_ComponentPtr defaultchoice = inComponent->GetChild(defaultindex);
				if (defaultchoice.IsNotNULL())
				{
					ReadComponent(defaultchoice, inMatrix);
				}
			}
			//if you don't want to use default you have to scan all children and choose the one you want to convert (see their name)
			break;
		}
		//Component containing only children 
		case Dtk_Component::VirtualComponentType:
		{
			Dtk_string name;
			Dtk_Size_t NumChildren;

			name = inComponent->Name();//Component name

			NumChildren = inComponent->GetNumChildren();
			for (i = 0; i < NumChildren; i++)
			{
				Dtk_ComponentPtr child = inComponent->GetChild(i);
				ReadComponent(child, inMatrix);
			}
			break;

		}
	}
	return dtkNoError;
}

//This function is a sample on how to write component :it is recursive function
// Dtk_Component can be an instance, a prototype and have children
Dtk_ErrorStatus vcExtractMetadata::ReadInstance(Dtk_ComponentPtr inComponent, const Dtk_transfo& inMatrix)
{
	//Instance represent a prototype with a matrix placement
	Dtk_string ComponentName;
	ComponentName = inComponent->Name();//Component name
	Dtk_ComponentPtr prototype = inComponent->GetChild(0);
	Dtk_transfo matrix = inComponent->TransformationMatrix();
	Dtk_ID pdfInstID;
	Dtk_ID childID = inComponent->GetChild(0)->GetID();

	//you have to write matrix and instance the prototype
	ReadComponent(prototype, Dtk_transfo());
	return dtkNoError;
}


void vcExtractMetadata::ReadPrototype(Dtk_ComponentPtr inComponent, const Dtk_transfo& inMatrix)
{
	Dtk_Size_t i;

	Dtk_NodePtr RootNode;
	Dtk_API* inAPI = Dtk_API::GetAPI();

	//it can also contain some instances
	Dtk_Size_t NumChildren = inComponent->GetNumChildren();
	for (i = 0; i < NumChildren; i++)
	{
		Dtk_ComponentPtr child = inComponent->GetChild(i);

		/*StopWatch sw;
		sw.Restart();

		Dtk_pnt min, max;
		child->GetBoundingBox(min, max);
		float fDistance = sqrt(((max[0] - min[0]) * (max[0] - min[0])) + ((max[1] - min[1]) * (max[1] - min[1])) + ((max[2] - min[2]) * (max[2] - min[2])));



		std::stringstream ss;
		ss.str("");
		ss << "Root Tess Tolerance: " << fDistance * 0.005;
		vcUtils::LogMsg(ss.str());
		ss.str("");
		ss << "Time taken for BB (µs) ******************* :" << sw.ElapsedUs();
		vcUtils::LogMsg(ss.str());*/

		ReadComponent(child, inMatrix);
	}

	//Get the Construction tree for this prototype
	Dtk_ErrorStatus err;
	if (inComponent->ComponentAvailability() == Dtk_Component::ComponentMissing)
		err = dtkErrorFileNotExist;
	else
		err = inAPI->ReadComponent(inComponent, RootNode);

	//a RootNode=  NULL with err == dtkNoError means that component is empty
	if (err == dtkNoError && RootNode.IsNotNULL())
	{
		ReadNode(RootNode);
	}

	//We close the opened Component and free his construction tree
	err = inAPI->EndComponent(inComponent);
}

//Dtk_node can be an Body, Mesh , Drawing, Fdt, AxisPlacement and/or have children (recursive function)
//It represent construction tree for Model
Dtk_ErrorStatus vcExtractMetadata::ReadNode(Dtk_NodePtr inNode)
{
	//GetName
	Dtk_string NodeName = inNode->Name();

	// You can Get Preview if you want to handle it
	/*Dtk_PreviewPtr TmpPreview = inNode->GetPreview();
	if (TmpPreview.IsNotNULL())
	{
		Dtk_Int32 size = TmpPreview->GetStreamSize();
		char* jpgimage = TmpPreview->GetStream();
		Dtk_string Preview_name = "NodePreview.jpg";
		FILE* jpg = Preview_name.OpenFile("wb");
		if (jpg)
		{
			fwrite(jpgimage, sizeof(char), size, jpg);
			fclose(jpg);
		}
	}*/


	//Get The node Blanked Status
	// -1 = undefined, 0 = Visible, 1=Invisible, 2=Construction Geometry
	int NodeBlankedStatus = inNode->GetInfos()->GetBlankedStatus();

	Dtk_Node::NodeDataTypeEnum NodeType = inNode->GetNodeType();

	//In this sample we read and treat only visible node - change this test if you want to read invisible and/or construction node
	if (NodeBlankedStatus == 0)
	{
		//You have many types for Node Data : 
		// BodyType for 3D geometry (solid and wireframe)
		// MeshType for 3D tessellated geometry
		// AnnotationSetType  
		// FdtType for FDT
		// DrawingType for 2D
		// KinematicsType for Kinematics
		// AxisSystemType for AxisPlacement
		// LayerInfosSetType for LayerInfos
		// MetaDataType for Additional informations
		// CameraType for Camera 
		// ModelDisplayType for ModelDisplay
		// VirtualType just for containing children
		switch (NodeType)
		{
		case Dtk_Node::BodyType:
		{
			ReadBody(inNode);
			break;
		}
		case Dtk_Node::AnnotationSetType:
		{
			//WriteAnnotation(inNode);
			break;
		}
		case Dtk_Node::FdtType:
		{
			//WriteFdt(inNode);
			break;
		}
		case Dtk_Node::DrawingType:
		{
			//WriteDrawing(inNode);
			break;
		}
		case Dtk_Node::MeshType:
		{
			//WriteMesh(inNode);
			break;
		}
		case Dtk_Node::AxisSystemType:
		{
			//WriteAxisSystem(inNode);
			break;
		}
		case Dtk_Node::KinematicsType:
		{
			//WriteKinematics(inNode);
			break;
		}
		case Dtk_Node::LayerInfosSetType:
		{
			//WriteLayerFilter(inNode);
			break;
		}
		case Dtk_Node::MetaDataType:
		{
			//WriteMetaData(inNode);
			break;
		}
		case Dtk_Node::CameraType:
		{
			//WriteCamera(inNode);
			break;
		}
		case Dtk_Node::ModelDisplayType:
		{
			//WriteModelDisplay(inNode);
			break;
		}
		case Dtk_Node::ConstraintType:
		{
			//WriteConstraints(inNode);
			break;
		}
		/*case Dtk_Node::VirtualType:
		{
			Dtk_Size_t i, NumChildren;
			NumChildren = inNode->GetNumChildren();
			if (NumChildren > 100)
			{
				int ab = 0;
				ab = 1;
			}
			for (i = 0; i < NumChildren; i++)
			{
				WriteNode(inNode->GetChild(i));
			}
			break;
		}*/
		default:
		{
			break;
		}
		}
	}

	//Treat recursive TreeNode for Visible and Construction node
	// NodeBlankedStatus : -1 = undefined, 0 = Visible, 1=Invisible, 2=Construction Geometry
	//remove this test if you want to treat invisible entities
	if (NodeBlankedStatus != 1 || NodeType == Dtk_Node::BodyType)
		//invisible body can have visible children
	{

		Dtk_Size_t i, NumChildren;
		NumChildren = inNode->GetNumChildren();
		for (i = 0; i < NumChildren; i++)
		{
			ReadNode(inNode->GetChild(i));
		}
	}


	// Write Feature associated if you want to handle it
	//WriteFeature(inNode);

	//Write NodeConnector if you want to handle it
	//WriteNodeConnector(inNode);


	return dtkNoError;
}

void vcExtractMetadata::ReadBody(Dtk_NodePtr inNode)
{
	// Get if the node represent infinite geometry
	// 1 = Infinite, 0 = Finite
	int NodeInfiniteGeometry = inNode->GetInfos()->GetInfiniteGeometryFlag();
	if (NodeInfiniteGeometry == 0)
	{
		//Calling both methods GetDtk_MeshPtr() and GetDtk_BodyPtr() on BodyNode will give you the same result
		//Choose the one you need in order to avoid duplicated data
		const Dtk_BodyPtr TmpBody = inNode->GetDtk_BodyPtr();
		if (TmpBody.IsNULL())
			return;
		Dtk_pnt min, max;
		Dtk_pnt min1, max1;
		StopWatch sw;
		sw.Restart();

		TmpBody->ComputeBoundingBox(min, max);
		float fDistance = 0;
		fDistance = sqrt(((max[0] - min[0]) * (max[0] - min[0])) + ((max[1] - min[1]) * (max[1] - min[1])) + ((max[2] - min[2]) * (max[2] - min[2])));
		if (fDistance == 0)
		{
			TmpBody->GetVertexBound(min, max);
			fDistance = sqrt(((max[0] - min[0]) * (max[0] - min[0])) + ((max[1] - min[1]) * (max[1] - min[1])) + ((max[2] - min[2]) * (max[2] - min[2])));
		}

		//float tol=TmpBody->GetTolerance();

		std::stringstream ss;
		/*ss.str("");
		ss << "Body tol :" << tol;
		vcUtils::LogMsg(ss.str());*/
		 

		//float fDistance = sqrt(((max[0] - min[0]) * (max[0] - min[0])) + ((max[1] - min[1]) * (max[1] - min[1])) + ((max[2] - min[2]) * (max[2] - min[2])));

		ss.str("");
		ss << "BB Distance ******************* :" << fDistance;
		vcUtils::LogMsg(ss.str());

		if (fDistance>0 && fDistance < m_fMinDistance)
			m_fMinDistance = fDistance;

		ss.str("");
		ss << "Time taken for BB (µs) ******************* :" << sw.ElapsedUs();
		vcUtils::LogMsg(ss.str());
	}
}