#pragma once

#ifndef _VC_CAD2CGR_
#define _VC_CAD2CGR_

#define _BIND_TO_CURRENT_CRT_VERSION 1
#define _BIND_TO_CURRENT_MFC_VERSION 1

#include "vcUtils.h"

#include <direct.h>

//DATAKIT headers needed
#include "datakit.h"
#include "g5w/catiav5w.hpp"
#include "util/utilwriter.h"
#include "util/util_geom_dtk.hpp"
#include "util/util_topology_dtk.hpp"
#include "tess/tess.h"
#include "cgrw/cgrw.hpp"
#include <iostream>
#include <sstream>
#include "vcLicense.h"

extern double g_fTessTolerance;

class vcCad2Cgr
{
	Dtk_string m_sInputFileName;
	Dtk_string m_sOutputFileName;
	Dtk_string m_sTmpWorkingDir;
	Dtk_API *m_pDtkAPI;

	DtkReaderType m_ReaderType;
	std::vector<Dtk_mesh_asm_node*> m_Dtk_mesh_asm_node_PtrArray;


public:
	vcCad2Cgr(char *sInputFileName,char *sOutputFileName,char *sTmpWorkingDir,bool bSuccess=true)
	{
		vcUtils::LogMsg("vcCad2Cgr::Start");

		m_pDtkAPI = NULL;
		m_sInputFileName = sInputFileName;
		m_sOutputFileName = sOutputFileName;
		m_sTmpWorkingDir = sTmpWorkingDir;

		EnableCADReaders();

		if(dtkErrorAPINotStarted == StartDtkAPI())
		{
			bSuccess = false;
			return;
		}

		Dtk_mesh_asm_node *pMesh_asm = new Dtk_mesh_asm_node();
		m_Dtk_mesh_asm_node_PtrArray.push_back(pMesh_asm);

		ReadCADFile();

		//CGR write start here 
		// First we initialize writing with name of files and protection fonction if needed (3rd parameter)
		int status = cgrw_InitWrite(sOutputFileName, Dtk_string(""),NULL,Dtk_string());
		//int status = cgrw_InitWrite(sOutputFileName, Dtk_string("testcgrw.log"),NULL,Dtk_string());
		if(status != 0)
		{
			//printf("CgrWrite : %s\n",dtkTypeError(status).c_str());
			return ;   
		}
 
		// Then we write for the mesh assembly constructed
		cgrw_WriteMeshAsm(pMesh_asm);

		// And free memory allocated for writing
		cgrw_EndWrite();
 

		vcUtils::LogMsg("vcCad2Cgr:: End");
	}
	~vcCad2Cgr()
	{
		StopDtkAPI();
	}


	void EnableCADReaders()
	{
		vcUtils::LogMsg("vcCad2Cgr::EnableCADReaders(): Start");

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
		//XmtReader::Enable();
		//VdaReader::Enable();
		//AcisReader::Enable();
		if(vcLicense::m_bSolidWorksLicense)
			SwReader::Enable();
		if(vcLicense::m_bSolidEdgeLicense)
			SeReader::Enable();
		if(vcLicense::m_bInventorLicense)
			InvReader::Enable();
		if(vcLicense::m_bCgrLicense)
			CgrReader::Enable();
		//JtReader::Enable();
		PsReader::Enable ();

		vcUtils::LogMsg("vcCad2Cgr::EnableCADReaders(): End");
	}

	int StartDtkAPI()
	{
		vcUtils::LogMsg("vcCad2Cgr::StartDtkAPI(): Start");
		Dtk_ErrorStatus errorStatus = dtkNoError; 
        #ifdef _LINUX64_
            if(m_sTmpWorkingDir[m_sTmpWorkingDir.len()-1] != '/')
                m_sTmpWorkingDir.Merge("/");
        #else
		if(m_sTmpWorkingDir[m_sTmpWorkingDir.len()-1] != '\\')
			m_sTmpWorkingDir.Merge("\\");
        #endif
		m_pDtkAPI = Dtk_API::StartAPI(m_sTmpWorkingDir, errorStatus, "VCOLLAB_SDK-170092217140010146187057140087227106200052053225206228253120022230154211033210227167181138169073103011117156088102093069252055244082189172018211172233079224079026239024130161046029174225164029254174199084181068197145000193037147121147071067175206137134191135246139094253034071232108167067156209204041162254022101034234144187006124072189031160233154208171163016244001047205185142117054206218112117081216204129175174117209165012129039149135093061082030195171114251117129028092211062011047119158081247159176232198054240139121174076028165212136137167122005230231178056230038012054186217056187025101060190068130097026035106034050151119110040148037156205072250084236203006095188028052192062211177002049174124208241255232035013086001056124255185056143057182218121164205065053252092092202225052042226117159250108164074003165148177001034046176056160147194225011059120138211228077120123215251086249158058108075120005140255-304V232");

		if(errorStatus != dtkNoError)
		{
#ifdef VMOVECAD_BATCH
			printf("\nError:Not able to read the CAD file.  Error Code : %s\n",dtkTypeError(errorStatus));
			printf("\nPlease refer troubleshooting page of VMoveCAD help\n");
#else
			std::string err;
			ss.str("");
			ss<<"Not able to read the CAD file. Error Code : "<<dtkTypeError(errorStatus)<<"\n"<<"Please refer troubleshooting page of VMoveCAD help.";;
#if defined(__WXMSW__)
			wxMessageBox(ss.str(),wxT("Error"),wxICON_ERROR, NULL);
#else
            //??::MessageBox(NULL,ss.str().c_str(),"VMoveCAD",MB_ICONERROR);
#endif
			ss.str("");
#endif
			ss.str("");
			ss<<"vcCad2Cgr::StartDtkAPI():Failed to read CAD file:Error Status : "<<dtkTypeError(errorStatus);
			vcUtils::LogMsg(ss.str());
			ss.str("");
		    return errorStatus;
		}

		if(errorStatus == dtkErrorLicence)
		{
           printf("No license available\n");
		   return 0;
		}

		if(m_pDtkAPI == NULL)
		{
			printf("Can't Start DATAKIT API\n");
			vcUtils::LogMsg("vcCad2Cgr::StartDtkAPI(): Failed to start DATAKIT API");
			return dtkErrorAPINotStarted;
		}

		m_pDtkAPI->SetBodyModePreference(DTK_BODYMODE_COMPLETETOPOLOGY);
		m_pDtkAPI->ActivateSplitForPeriodicFaces();


		//If you want to use tesselation library start Tesselation Kernel
		#ifdef ACTIVATE_TESSELATION_LIB
			 //int status = tess_InitTesselation("tess_tmp",0.05);
			 int status = tess_InitTesselation("tess_tmp",g_fTessTolerance);
			 if(status == dtkErrorLicence)
			 {
				  printf("No tesselation license available\n");
			 }
		#endif

			 vcUtils::LogMsg("vcCad2Cgr::StartDtkAPI(): End");
	}

	void StopDtkAPI()
	{
		vcUtils::LogMsg("vcCad2Cgr::StopAPI(): Start");
		Dtk_API::StopAPI(m_pDtkAPI);
		vcUtils::LogMsg("vcCad2Cgr::StopAPI(): End");
	}

	Dtk_ErrorStatus ReadCADFile()
	{
		vcUtils::LogMsg("vcCad2Cgr::ReadCADFile(): Start");
		// You Get the current API 
		DtkErrorStatus stError = dtkNoError; 
		Dtk_API * MyAPI = Dtk_API::GetAPI();
		Dtk_MainDocPtr TmpDoc;


		//Set the Schema directory needed for readers based on Pskernel (UG, Solidworks, Solidedge), or CADDS
#ifdef WIN32
		char szModuleFileName[MAX_PATH];
		GetModuleFileName(NULL, szModuleFileName, sizeof(szModuleFileName)); 
		std::string Path = vcUtils::GetPathFromFileName(szModuleFileName);
		Dtk_string inRepSchema = Path.c_str()+Dtk_string("\\schema");
		
		ss<<"vcCad2Cgr::ReadCADFile():PsKernal Schema Path : "<<inRepSchema.c_str();
		vcUtils::LogMsg(ss.str());
		ss.str("");

		char TmpFullPathSchemaDir[_MAX_PATH];
		if( _fullpath( TmpFullPathSchemaDir, inRepSchema.c_str(), _MAX_PATH ) != NULL )
			MyAPI->SetSchemaDir (TmpFullPathSchemaDir);
		else
			MyAPI->SetSchemaDir (inRepSchema);
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

#if 0
		if (toDumpXML)
		{
			stError = Dtk_DumpXMLNamespace::Dtk_DumpXml_Init(inOutputFile);
			if(stError != dtkNoError)
			{
				return dtkErrorOpenOutputFile;
			}
		}
#endif
		printf("\nReading file %s \n",m_sInputFileName.c_str());

		//You Open the file you want to read and get corresponding Document 
		Dtk_ErrorStatus err = MyAPI->OpenDocument(m_sInputFileName, TmpDoc);

		//If no Error we write the Document
		if(err == dtkNoError && TmpDoc.IsNotNULL() )
		{
			vcUtils::LogMsg("vcCad2Cgr::ReadCADFile(): OpenDocument succeeded");
			WriteCGR( TmpDoc ); //see writeAPI.cpp
		}
		else
		{
			vcUtils::LogMsg("vcCad2Cgr::ReadCADFile(): Failed to load assembly tree");
			printf("Error Loading Assembly Tree : %d\n",err);
		}

		//We close the opened document
		err = MyAPI->EndDocument(TmpDoc);
#if 0
		if (toDumpXML)
		{
			stError = Dtk_DumpXMLNamespace::Dtk_DumpXml_End(); 
		}
#endif
		vcUtils::LogMsg("vcCad2Cgr::ReadCADFile(): End");
		return err;
	}

	// Dtk_Document represent Assembly tree of model
	Dtk_ErrorStatus WriteCGR(Dtk_MainDocPtr inDocument)
	{
		vcUtils::LogMsg("vcCad2Cgr::WriteCGR(): Start");
		//First we get the root component in the document
		Dtk_ComponentPtr RootComponent = inDocument->RootComponent();
		//if no Error we write the Component
		if(RootComponent.IsNotNULL())
		{
			m_ReaderType = RootComponent->GetAssociatedModuleType(); 

			WriteComponent( RootComponent );
			vcUtils::LogMsg("vcCad2Cgr::WriteCGR(): End");
			return dtkNoError;
		}

		vcUtils::LogMsg("vcCad2Cgr::WriteCGR(): RootComponent = NULL");
		return dtkErrorNullPointer;
	}
	
	std::stringstream ss;
	// Dtk_Component can be an instance, a prototype and have children
	Dtk_ErrorStatus WriteComponent(Dtk_ComponentPtr inComponent)
	{
		//vcUtils::LogMsg("vcDataKit:WriteComponent() : Start");
		Dtk_Size_t i;
		Dtk_ErrorStatus err;
		
		//COMMENTED BY SENTHIL
		//FILE * xmlDumpFile = Dtk_DumpXMLNamespace::Dtk_DumpXml_GetFile(); 

		//Datakit component are given in MM, if original model had other unit you can get it with GetConceptionUnitScale()
		// return 25.4 for inch 
		double UnitFactor = inComponent->GetConceptionUnitScale(); 

		//GetName
		Dtk_string ComponentName;
		if(inComponent->Name().is_not_NULL())
			ComponentName = inComponent->Name();

		//GetPreview 
		/*Dtk_PreviewPtr TmpPreview = inComponent->GetPreview();
		if (TmpPreview.IsNotNULL())
		{
			Dtk_Int32 size = TmpPreview->GetStreamSize();
			char *jpgimage = (char *)malloc(size * sizeof(char));
			jpgimage = TmpPreview->GetStream();
			Dtk_string Preview_name = "ComponentPreview.jpg";
			FILE *jpg = Preview_name.OpenFile("w");
			if (jpg)
			{
				fprintf(jpg,jpgimage,size);
				fclose(jpg);
			}
		}*/


		//You have 4 types for Component
		Dtk_Component::ComponentTypeEnum type = inComponent->ComponentType();
		switch(type)
		{
			//Instance represent a prototype with a matrix placement
			case Dtk_Component::InstanceComponentType :
			{
				Dtk_transfo transfo;
				transfo = inComponent->TransformationMatrix();
				transfo.setScale(1.0f);

				if(m_Dtk_mesh_asm_node_PtrArray.size()<1)
					break;

				Dtk_mesh_asm_node *pParent_Mesh_asm_node = m_Dtk_mesh_asm_node_PtrArray[m_Dtk_mesh_asm_node_PtrArray.size()-1];
				Dtk_mesh_asm_node *pCurrent_Mesh_asm_node = new Dtk_mesh_asm_node();
				pParent_Mesh_asm_node->add_asm_instance(pCurrent_Mesh_asm_node,&transfo);
				m_Dtk_mesh_asm_node_PtrArray.push_back(pCurrent_Mesh_asm_node);

				Dtk_ComponentPtr prototype = inComponent->GetChild(0) ;
				WriteComponent(prototype);
				
				m_Dtk_mesh_asm_node_PtrArray.pop_back();
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
					//Get the Construction tree for this prototype
					err = inAPI->ReadComponent( inComponent, RootNode );
					if (err == dtkNoError && RootNode.IsNotNULL())
					{
						WriteNode(RootNode);
					}

					//it can also contain some instances
					Dtk_Size_t NumChildren = inComponent->GetNumChildren();
					for( i = 0; i < NumChildren; i++)
					{
						 Dtk_ComponentPtr child = inComponent->GetChild(i) ;
						 WriteComponent( child);
					}
					//??inComponent->SetProcessed();
					//We close the opened Component and free his construction tree
					err = inAPI->EndComponent(inComponent);

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
				if(inComponent->Name().is_not_NULL())
					name = inComponent->Name();

				Dtk_ComponentPtr defaultchoice = inComponent->GetChild(0) ;
				if (defaultchoice.IsNotNULL())
				{
					WriteComponent(defaultchoice);
				}
				//if you don't want to use default you have to scan all children and choose the one you want to convert (see their name)
				break;
			}
			//Component containing only children 
			case Dtk_Component::VirtualComponentType :
			{
				Dtk_string name;
				Dtk_Size_t NumChildren;

				if(inComponent->Name().is_not_NULL())
					name = inComponent->Name();

				NumChildren = inComponent->GetNumChildren();
				for( i = 0; i < NumChildren; i++)
				{
					Dtk_ComponentPtr child = inComponent->GetChild(i) ;
					WriteComponent( child);
				}
				break;
			}
		}
		//vcUtils::LogMsg("vcDataKit:WriteComponent() : End");
		return dtkNoError;
	}


	//Dtk_node can be an Body, Mesh , Drawing, Fdt, AxisPlacement and/or have children (recursive function)
	//It represent construction tree for Model
	Dtk_ErrorStatus WriteNode(Dtk_NodePtr inNode)
	{
		//commented by senthil
		//FILE * xmlDumpFile = Dtk_DumpXMLNamespace::Dtk_DumpXml_GetFile(); 
		Dtk_API *MyAPI = Dtk_API::GetAPI();

		//Get The node Blanked Status
		// -1 = undefined, 0 = Visible, 1=Invisible, 2=Construction Geometry
		int NodeBlankedStatus = inNode->GetInfos()->GetBlankedStatus();

		//GetName
		Dtk_string NodeName;
		if(inNode->Name().is_not_NULL())
			NodeName = inNode->Name();
		
		if(NodeName.find_substring("Geometrical Set")!= -1)
			return dtkNoError;


		//See Dtk_InfoPtr methods for others attributes in infos (color, layer...)
		//commented by senthil
		/*if (xmlDumpFile)
		{
			Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_NodePtr_Init( xmlDumpFile, inNode);
		}*/

		//In this sample we read and treat only visible node
		if( NodeBlankedStatus == 0)
		{
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

			Dtk_RGB Color = inNode->GetInfos()->GetColor();
			Dtk_MaterialPtr Material = inNode->GetInfos()->GetMaterial();
			if(Material.IsNotNULL())
			{
				double ambient = Material->ambient;
				double diffuse = Material->diffuse;
				double specular = Material->specular;
				double transparency = Material->transparency;
				double emissive = Material->reflectivity;
			}

			switch(NodeType)
			{
				case Dtk_Node::BodyType:
				{
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
							//commented by senthil
							/*if (xmlDumpFile)
							{
								Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_BodyPtr(xmlDumpFile,TmpBody);
							}*/
						}
						// Some CAD formats store also faceted data besides B-Rep. 
						// So you can get faceted data corresponding to the body using the following method
						const Dtk_MeshPtr TmpFacetedBody = inNode->GetDtk_MeshPtr();
						if (TmpFacetedBody.IsNotNULL())
						{
							WriteDtk_Mesh(TmpFacetedBody,NodeName);
						}
						#ifdef ACTIVATE_TESSELATION_LIB
						// If there is no mesh data associated to the current body, you can tessellate 
						if(TmpFacetedBody.IsNULL())
						{
							Dtk_tab<Dtk_MeshPtr> meshes;
							Dtk_tab<Dtk_Int32> isclosed;
							int err_tess = tess_BodyToMeshes(TmpBody, meshes,isclosed);
							if(err_tess == 0)
							{
								Dtk_Size_t i,nbmeshes = meshes.size();

								for (i=0;i<nbmeshes;i++)
								{
									WriteDtk_Mesh(meshes[i],NodeName);
								}   
							}
						}
						#endif
					}
					break;
				}
				case Dtk_Node::AnnotationSetType:
				{
					//Dtk_FdtAnnotationSetPtr TmpFdtAnnotSet = inNode->GetDtk_FdtAnnotationSetPtr();
					//if (TmpFdtAnnotSet.IsNotNULL())
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
					//Dtk_DrawingPtr TmpDrawing = inNode->GetDtk_DrawingPtr();
					//if (TmpDrawing.IsNotNULL())
					{
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
						WriteDtk_Mesh(TmpMesh,NodeName);
					}
					break;
				}
				case Dtk_Node::AxisSystemType:
				{
					//Dtk_AxisSystemPtr TmpAxis = inNode->GetDtk_AxisSystemPtr();
					//if (TmpAxis.IsNotNULL() && xmlDumpFile)
					{
						//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_transfo(xmlDumpFile,TmpAxis->GetMatrix());
					}
					break;
				}
				case Dtk_Node::KinematicsType:
				{              
					//Dtk_KinematicPtr TmpKinematics = inNode->GetDtk_KinematicPtr();  
					//if (TmpKinematics.IsNotNULL())
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
					/*Dtk_LayerInfosSetPtr TmpLayerInfosSet = inNode->GetDtk_LayerInfosSetPtr();
					if (TmpLayerInfosSet.IsNotNULL())
					{
						Dtk_Size_t NumLayers = TmpLayerInfosSet->GetNumLayers();
						Dtk_Size_t NumLayerFilters = TmpLayerInfosSet->GetNumLayerFilters();
						Dtk_Size_t DefaultLayer, DefaultLayerFilter;
						TmpLayerInfosSet->GetDefaultLayer(DefaultLayer);
						TmpLayerInfosSet->GetDefaultLayerFilter(DefaultLayerFilter);

						Dtk_Size_t i;
						for (i = 0; i < NumLayers; i++)
						{
							Dtk_string LayerName;
							TmpLayerInfosSet->GetLayerName(i, LayerName);
						}
						for (i = 0; i < NumLayerFilters; i++)
						{
							Dtk_LayerFilterInfosPtr TmpLayerFilter = TmpLayerInfosSet->GetLayerFilterByPos(i);
							Dtk_tab< Dtk_Size_t > SelectedLayers;
							TmpLayerFilter->GetSelectedLayers( SelectedLayers );
							Dtk_string LayerFilterName;
							TmpLayerFilter->GetName(LayerFilterName);
						}
					}*/
					break;
				}
				case Dtk_Node::MetaDataType:
				{
					//Dtk_MetaDataPtr TmpMetaData = inNode->GetDtk_MetaDataPtr();
					//if (TmpMetaData.IsNotNULL())
					{
						/*if (xmlDumpFile)
						{
							Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_MetadataPtr(xmlDumpFile, TmpMetaData);
						} */                                      
						//Dtk_string sTitle = TmpMetaData->GetTitle(); 
					}
					break;
				}
				case Dtk_Node::VirtualType:
				{
					if(inNode->GetNumChildren())
					{
						Dtk_Size_t i, NumChildren;
						NumChildren = inNode->GetNumChildren();
						for (i = 0; i < NumChildren; i++)
						{
							WriteNode(inNode->GetChild(i));
						}    
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

		/*if (xmlDumpFile)
		{
			Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_NodePtr_End( xmlDumpFile);
		}*/

		return dtkNoError;
	}

	//Writing Each face as a shape to retain Material color properly.
	void WriteDtk_Mesh(const Dtk_MeshPtr& inMeshToWrite,Dtk_string NodeName)
	{
		Dtk_mesh_asm_node *pMesh_asm = this->m_Dtk_mesh_asm_node_PtrArray[m_Dtk_mesh_asm_node_PtrArray.size()-1];
		pMesh_asm->add_mesh(inMeshToWrite);
	}
};

#endif