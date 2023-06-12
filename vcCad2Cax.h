#pragma once

#ifndef _VC_DATAKIT_
#define _VC_DATAKIT_ 1

#define _BIND_TO_CURRENT_CRT_VERSION 1
#define _BIND_TO_CURRENT_MFC_VERSION 1

#include <stdint.h>
#include "datakit.h"
#include <iostream>
#include <sstream>
//#include "test_options.hpp"

//CAX
#include "VctCaxScene.h"
#include "VctException.h"
using namespace Vct;
typedef Cax::Id CaxId;
typedef Cax::uint32_t uint32_t;


#ifdef ACTIVATE_TESSELATION_LIB            
#include "util/util_mesh_dtk.hpp" 
//#include "util/util_mesh_asm_dtk.hpp"
#include "tess/tess.h"
#endif

class vcCad2Cax
{
	std::map< Dtk_RGB*, int> m_DtkColorNVerticesCountMap;
	bool m_bSinglePart;

	std::map< Dtk_RGB*, std::pair< std::vector<Dtk_mesh_face*> *,std::vector<Dtk_MeshPtr> > > m_DtkColorNFaceArrayMap;
	std::map<Dtk_RGB*, std::vector<Dtk_PolylinePtr> *> m_DtkColorNLinesArrayMap;
	std::map<Dtk_RGB*, std::vector<Dtk_PointPtr> *> m_DtkColorNPointsArrayMap;
	std::map<std::string,CaxId> m_NameNCaxIdMap;

	bool ConstructShapeFor2DElements(Dtk_string sName);
	bool ConstructShapeForPointElements(Dtk_string sName);
	float ConvertUnits(int iInputUnit, int iOutputUnit); 

	int m_iOriginalUnit;
	int m_iShapeIndexOfComponent;
	
	std::ofstream m_TestResultsFile;
   	int m_iTotalShapes;
	int m_iTotalComponents;
	int m_iTotalSurfaces;
	int m_iTotalLines;
	int m_iTotalPointSets;
	void InitializeTestResults();


	Cax::Scene m_CaxScene;
	CaxId m_CaxRoot;
	std::vector<CaxId> m_NodesArray;
	std::map<std::string,int> m_NodeNameAndIndexMap;

	//std::map<uint32_t,CaxId> m_DataKitAndCaxNodeMap;

	Dtk_Double64 m_fUnit;
	int m_iIndex;
	int m_iFaceIndex;
	int m_iShapeIndex;
	DtkErrorStatus m_dtkErrorStatus;
	Dtk_string m_sInputFileName; 
	Dtk_string m_sOutputFileName;
	Dtk_string m_sTmpWorkingDir; 
	Dtk_API *m_pDtkAPI;
	DtkReaderType m_ReaderType;
	std::vector<Dtk_matrix*> m_DtkMatrixPtrArray;
	std::vector<Dtk_RGB> m_DtkComponentColorArray;
	Dtk_string m_sCurrentComponentName;
	

	bool m_bNegativeScale;
	int m_iNegativeScaleNodeIndex;
	float m_NegativeScaleVec[3];
public:
	vcCad2Cax(char *sInputFileName,char *sOutputFileName,char *sTmpWorkingDir,bool &bSuccess);
	~vcCad2Cax();

public:
	enum 
	{
		NOT_TRANSLATED=100,SUCCESS=101,DATAKIT_ERROR=102,CAX_WRITER_ERROR=103
	};
	int m_iErrorCode;
	enum 
	{
		MILLIMETER=0,METER,INCH,FEET,CENTIMETER
	};
	int m_iUnit;

private:
	std::stringstream ss;

	bool Convert();

	void InitializeCaxScene(const char *rootName);
	bool WriteCax();

	void EnableCADReaders();
	Dtk_ErrorStatus StartDtkAPI();
	void StopDtkAPI();
	Dtk_ErrorStatus OpenCADFile(Dtk_MainDocPtr &TmpDoc);
	Dtk_ErrorStatus ParseCadFile(Dtk_MainDocPtr inDocument); 
	Dtk_ErrorStatus CloseCADFile(Dtk_MainDocPtr TmpDoc);
	Dtk_ErrorStatus WriteComponent(Dtk_ComponentPtr inComponent);
	Dtk_ErrorStatus WriteNode(Dtk_NodePtr inNode);
	void WriteDtk_Mesh(const Dtk_MeshPtr& inMeshToWrite,Dtk_string NodeName);
	void WriteDtk_Meshes(Dtk_tab<Dtk_MeshPtr> &meshes,Dtk_string NodeName);
	void GetMeshFacesWithComponentColor(const Dtk_MeshPtr& inMeshToWrite,
		                                std::map< Dtk_RGB*, std::pair<std::vector<Dtk_mesh_face*> *,std::vector<Dtk_MeshPtr> >> &DtkColorNFaceArrayMap,
										Dtk_RGB *ComponentColor,
										std::map< Dtk_RGB*, int> &DtkColorNVerticesCountMap
										);
	void GetMeshFacesWithFaceColor(const Dtk_MeshPtr& inMeshToWrite,
		                           std::map< Dtk_RGB*, std::pair<std::vector<Dtk_mesh_face*> *,std::vector<Dtk_MeshPtr> >> &DtkColorNFaceArrayMap,
								   std::map< Dtk_RGB*, int> &DtkColorNVerticesCountMap,
								   Dtk_RGB *ComponentColor
								  );
	//void GetMeshFacesWithFaceColor(const Dtk_MeshPtr& inMeshToWrite,
	//	                           std::map< Dtk_RGB*, std::vector<Dtk_mesh_face*> *> &DtkColorNFaceArrayMap,
	//							   std::map< Dtk_RGB*, int> &DtkColorNVerticesCountMap
	//							  );
	bool IsComponentColor(Dtk_RGB *ComponentColor);
	std::string GetUnitsName(Dtk_Double64 unit);
	std::string GetCadPackageName(DtkReaderType readerType);
	std::string GetMeshType(type_detk readerType);
	bool AreAllSpaces(const char *sNodeName);
	std::string GetValidNodeName(const char *sNodeName,int index = -1);
	void GetAxisAndAngle(const float rotmat[9], float axis[3], float& angle);
	void SetCAXTransformation(Dtk_matrix *mat);

	void ConstructCaxShape(std::map< Dtk_RGB*, std::pair<std::vector<Dtk_mesh_face*> *,std::vector<Dtk_MeshPtr>>>& ,Dtk_string NodeName);

	Dtk_ErrorStatus WriteBody(const Dtk_BodyPtr& inBody);
	Dtk_ErrorStatus WriteLump(const Dtk_LumpPtr& inLump);
	Dtk_ErrorStatus WriteVolume(const Dtk_VolumePtr& inVol);
	Dtk_ErrorStatus WriteShell(const Dtk_ShellPtr& inShell);
	Dtk_ErrorStatus WriteFace(const Dtk_FacePtr& inFace);
	Dtk_ErrorStatus WriteLoop(const Dtk_LoopPtr& inLoop);
	Dtk_ErrorStatus WriteCoedge(const Dtk_CoedgePtr& inCoedge);
	Dtk_ErrorStatus WriteEdge(const Dtk_EdgePtr& inEdge);
	Dtk_ErrorStatus WriteVertex(const Dtk_VertexPtr& inVertex);
	Dtk_ErrorStatus WritePoint(const Dtk_PointPtr& inPoint);
	Dtk_ErrorStatus WriteCurve(const Dtk_CurvePtr &inCurve);
	Dtk_ErrorStatus WriteLine(const Dtk_LinePtr &inCurve,const Dtk_InfoPtr &Info);
	Dtk_ErrorStatus WritePolyline(const Dtk_PolylinePtr &inCurve,const Dtk_InfoPtr &Info);
	Dtk_ErrorStatus WriteNurbsCurve(const Dtk_NurbsCurvePtr &inCurve,const Dtk_InfoPtr &Info);
	Dtk_ErrorStatus WriteEllipse(const Dtk_EllipsePtr &inCurve,const Dtk_InfoPtr &Info);
	Dtk_ErrorStatus WriteParabola(const Dtk_ParabolaPtr &inCurve,const Dtk_InfoPtr &Info);
	Dtk_ErrorStatus WriteHyperbola(const Dtk_HyperbolaPtr &inCurve,const Dtk_InfoPtr &Info);
	Dtk_ErrorStatus GetColorInfo(const Dtk_InfoPtr& I,int &r,int &g,int &b,int &a);
};

#endif