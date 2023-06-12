#pragma once

#include "datakit.h"
#include <string>

class vcExtractMetadata
{
public:
	vcExtractMetadata();
	~vcExtractMetadata();

	bool ExtractMetadata(std::string _sInputFile, std::string _sTmpWorkingDir);
	float GetMinimumDistance();
	float GetUnits();

private:
	Dtk_string  m_sInputFileName;
	Dtk_string m_sTmpWorkingDir;
	Dtk_API* m_pDtkAPI;
	float m_fMinDistance;
	int m_iErrorCode;
	DtkErrorStatus m_dtkErrorStatus;
	enum
	{
		NOT_TRANSLATED = 100, SUCCESS = 101, DATAKIT_ERROR = 102, CAX_WRITER_ERROR = 103
	};

	void			EnableCADReaders();
	Dtk_ErrorStatus StartDtkAPI();
	void			StopDtkAPI();
	Dtk_ErrorStatus OpenCADFile(Dtk_MainDocPtr& TmpDoc);
	Dtk_ErrorStatus CloseCADFile(Dtk_MainDocPtr TmpDoc);
	Dtk_ErrorStatus ParseCadFile(Dtk_MainDocPtr inDocument);

	Dtk_ErrorStatus ReadComponent(Dtk_ComponentPtr inComponent, const Dtk_transfo& inMatrix = Dtk_transfo());
	Dtk_ErrorStatus ReadInstance(Dtk_ComponentPtr inComponent, const Dtk_transfo& inMatrix);
	void            ReadPrototype(Dtk_ComponentPtr inComponent, const Dtk_transfo& inMatrix);
	Dtk_ErrorStatus ReadNode(Dtk_NodePtr inNode);
	void            ReadBody(Dtk_NodePtr inNode);
};

