#pragma once

#ifndef _VC_CAD2METADATA_
#define _VC_CAD2METADATA_

#define _BIND_TO_CURRENT_CRT_VERSION 1
#define _BIND_TO_CURRENT_MFC_VERSION 1


#include <direct.h>

//DATAKIT headers needed
#include "datakit.h"
#include "g5w/catiav5w.hpp"
#include "util/utilwriter.h"
#include "util/util_geom_dtk.hpp"
#include "util/util_topology_dtk.hpp"
#include "tess/tess.h"
#include "util/dtk_maindoc.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include "vcCadMetaData.h"

extern bool g_bOutputXml;
extern int g_iXMLFormat;

class vcCad2Xml
{
	Dtk_string m_sInputFileName;
	Dtk_string m_sOutputFileName;
	Dtk_string m_sTmpWorkingDir;
	Dtk_API *m_pDtkAPI;
	vcCadMetaData m_MetaData;
	DtkReaderType m_ReaderType;
	bool m_bBodyTypeGeometry;

	std::stringstream ss;
	std::fstream outfile;
	int m_iIndent;

	std::vector<Dtk_string> m_MetaDataTitleStrArray;
	std::vector<Dtk_string> m_MetaDataValueStrArray;

	std::vector<Dtk_string> m_FilePathStrArray;
	std::vector<Dtk_string> m_ComponentNameStrArray;

	Dtk_ComponentPtr m_RootComponent;
	Dtk_MaterialPtr m_BodyLevelMaterial;
public:
	int m_iErrorCode;
	DtkErrorStatus	m_dtkErrorStatus;
	enum 
	{
		NOT_TRANSLATED=100,SUCCESS=101,DATAKIT_ERROR=102,XML_WRITER_ERROR=103,FILE_ACCESS_ERROR=104
	};

	enum XML_FORMAT
	{
		GENERIC=0,EKM_META_DATA,EKM_REPORT,KEY_VALUE_PAIRS,EKM_ASM_DEP_EXTRACT
	};
	int m_iXmlFormat;

	vcCad2Xml(char *sInputFileName,char *sOutputFileName,char *sTmpWorkingDir,bool &bSuccess);
	bool Convert();
	~vcCad2Xml();
	void EnableCADReaders();
	Dtk_ErrorStatus StartDtkAPI();
	void StopDtkAPI();

	Dtk_ErrorStatus CloseCADFile(Dtk_MainDocPtr TmpDoc);
	Dtk_ErrorStatus OpenCADFile(Dtk_MainDocPtr &TmpDoc);

	// Dtk_Document represent Assembly tree of model
	Dtk_ErrorStatus WriteXml(Dtk_MainDocPtr inDocument);
	
	Dtk_ErrorStatus WriteMassProperties(Dtk_ComponentPtr inComponent);

	// Dtk_Component can be an instance, a prototype and have children
	Dtk_ErrorStatus WriteComponent(Dtk_ComponentPtr inComponent);

	//Dtk_node can be an Body, Mesh , Drawing, Fdt, AxisPlacement and/or have children (recursive function)
	//It represent construction tree for Model
	Dtk_ErrorStatus WriteNode(Dtk_NodePtr inNode);

	//Writing Each face as a shape to retain Material color properly.
	void WriteDtk_Mesh(const Dtk_MeshPtr& inMeshToWrite,Dtk_string NodeName);

	std::string GetCadPackageName(DtkReaderType readerType);
	std::string GetUnitsName(Dtk_Double64 unit);
	bool OpenFile(const char *sOutputFileName);
	void WriteXmlNode(std::string sName);
	void WriteXmlNode();
	void WriteXmlAttribute(std::string sKey,std::string sValue);
	void WriteTag(std::string tag);
	void WriteEkmXmlAttribute(std::string sKey,std::string sValue);
	void WriteKeyValueXmlAttribute(std::string sKey,std::string sValue);

	bool WriteMaterial(Dtk_MaterialPtr Material);

	void WriteEkmReport_Row(std::string sKey,std::string sValue);
	void WriteXmlNode_Start(std::string sName,bool bIndent=true);
	void WriteXmlNode_End(std::string sName);
	void WriteXmlNode_StartEnd(std::string tag, std::string sVal, bool bIndent=true);

	void RemoveCommonStrFromPath(std::string &input_path, std::string &comp_path);

};

#endif