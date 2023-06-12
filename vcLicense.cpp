#include "vcLicense.h"
#include "vcUtils.h"
//#include "VCollabSuiteLicense.h"
#include "Vct/License/Client_V4.h"
using namespace Vct;

bool vcLicense::m_bCatiaV6License;
bool vcLicense::m_bCatiaV5License;
bool vcLicense::m_bCatiaV4License;
bool vcLicense::m_bUgLicense;
bool vcLicense::m_bProeLicense;
bool vcLicense::m_bSolidWorksLicense;
bool vcLicense::m_bSolidEdgeLicense;
bool vcLicense::m_bInventorLicense;
bool vcLicense::m_bCgrLicense;
bool vcLicense::m_bStepLicense;
bool vcLicense::m_bIgesLicense;
bool vcLicense::m_bAcisLicense;
bool vcLicense::m_bParasolidLicense;
bool vcLicense::m_bRevitLicense;
bool vcLicense::m_bIfcLicense;
bool vcLicense::m_bStlLicense;
bool vcLicense::m_bVMoveCADLicense;
//CVCollabSuiteLicense License;

extern bool g_bDetailedLog;
char* g_sMSC_CEID = NULL;

vcLicense::vcLicense(void)
{
}

vcLicense::~vcLicense(void)
{
}

void vcLicense::Initialize()
{
	m_bCatiaV6License = false;
	m_bCatiaV5License = false;
	m_bCatiaV4License = false;
	m_bUgLicense = false;
	m_bProeLicense = false;
	m_bSolidWorksLicense = false;
	m_bSolidEdgeLicense = false;
	m_bInventorLicense = false;
	m_bCgrLicense = false;
	m_bStepLicense = false;
	m_bIgesLicense = false;
	m_bAcisLicense = false;
	m_bParasolidLicense = false;
	m_bRevitLicense = false;
	m_bIfcLicense = false;
	m_bStlLicense = false;
	m_bVMoveCADLicense = false;
}

bool vcLicense::CheckOutLicense()
{
	vcUtils::LogMsg("vcLicense::CheckOutLicense : Start");
	bool bLicenseFound = false;
	Initialize();
	std::string sLicVersion = "2021";
	std::string sLicFeature = "VMoveCAD";
	bool res = Vct::License::Client::get().acquireLicense(sLicFeature.c_str(), sLicVersion.c_str(), Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
	if(res == true)
	{
		m_bVMoveCADLicense = true;
		m_bCatiaV6License = true;
		m_bCatiaV5License = true;
		m_bCatiaV4License = true;
		m_bUgLicense = true;
		m_bProeLicense = true;
		m_bSolidWorksLicense = true;
		m_bSolidEdgeLicense = true;
		m_bInventorLicense = true;
		m_bCgrLicense = true;
		m_bStepLicense = true;
		m_bIgesLicense = true;
		m_bAcisLicense = true;
		m_bParasolidLicense = true;
		m_bRevitLicense = true;
		m_bIfcLicense = true;
		m_bStlLicense = true;
		vcUtils::LogMsg("vcLicense::CheckOutLicense : Acquired VMoveCAD license");

		char* sCEID = Vct::License::Client::get().GetCEID();
		if (sCEID && strlen(sCEID))
		{
			g_sMSC_CEID = sCEID;
			vcUtils::LogMsg(std::string("MSC Customer Entitlement ID :") + std::string(g_sMSC_CEID));
		}
		return true;
	}
	else
	{
		bLicenseFound = false;
		vcUtils::LogMsg("vcLicense::CheckOutLicense : VMoveCAD license failed");
		vcUtils::LogMsg(Vct::License::Client::get().getErrorString());
	}

#if 1
	//res=License.CheckOut("VMoveCAD_CatiaV6","2010",0);
	sLicFeature = "VMoveCAD_CatiaV6";
	res = Vct::License::Client::get().acquireLicense(sLicFeature.c_str(), sLicVersion.c_str(), Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
	if(res==true)
	{
		m_bCatiaV6License = true;
		bLicenseFound = true;
		//if(g_bDetailedLog)
			vcUtils::LogMsg("vcLicense::CheckOutLicense : Acquired VMoveCAD_CatiaV6 license");
	}
	else
	{
		//if(g_bDetailedLog)
			//vcUtils::LogMsg(License.GetErrorString());
		vcUtils::LogMsg("vcLicense::CheckOutLicense : VMoveCAD_CatiaV6 license failed");
	}

	//res=License.CheckOut("VMoveCAD_CatiaV5","2010",0);
	sLicFeature = "VMoveCAD_CatiaV5";
	res = Vct::License::Client::get().acquireLicense(sLicFeature.c_str(), sLicVersion.c_str(), Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
	if(res==true)
	{
		m_bCatiaV5License = true;
		bLicenseFound = true;
		//if(g_bDetailedLog)
			vcUtils::LogMsg("vcLicense::CheckOutLicense : Acquired VMoveCAD_CatiaV5 license");
	}
	else
	{
		//if(g_bDetailedLog)
			//vcUtils::LogMsg(License.GetErrorString());
		vcUtils::LogMsg("vcLicense::CheckOutLicense : VMoveCAD_CatiaV5 license failed");
	}

	//res=License.CheckOut("VMoveCAD_CatiaV4","2010",0);
	sLicFeature = "VMoveCAD_CatiaV4";
	res = Vct::License::Client::get().acquireLicense(sLicFeature.c_str(), sLicVersion.c_str(), Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
	if(res==true)
	{
		m_bCatiaV4License = true;
		bLicenseFound = true;
		//if(g_bDetailedLog)
			vcUtils::LogMsg("vcLicense::CheckOutLicense : Acquired VMoveCAD_CatiaV4 license");
	}
	else
	{
		//if(g_bDetailedLog)
			//vcUtils::LogMsg(License.GetErrorString());
		vcUtils::LogMsg("vcLicense::CheckOutLicense : VMoveCAD_CatiaV4 license failed");
	}

	//res=License.CheckOut("VMoveCAD_UG","2010",0);
	sLicFeature = "VMoveCAD_UG";
	res = Vct::License::Client::get().acquireLicense(sLicFeature.c_str(), sLicVersion.c_str(), Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
	if(res==true)
	{
		m_bUgLicense = true;
		bLicenseFound = true;
		//if(g_bDetailedLog)
			vcUtils::LogMsg("vcLicense::CheckOutLicense : Acquired VMoveCAD_UG license");
	}
	else
	{
		//if(g_bDetailedLog)
			//vcUtils::LogMsg(License.GetErrorString());
		vcUtils::LogMsg("vcLicense::CheckOutLicense : VMoveCAD_UG license failed");
	}

	//res=License.CheckOut("VMoveCAD_ProE","2010",0);
	sLicFeature = "VMoveCAD_ProE";
	res = Vct::License::Client::get().acquireLicense(sLicFeature.c_str(), sLicVersion.c_str(), Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
	if(res==true)
	{
		m_bProeLicense = true;
		bLicenseFound = true;
		//if(g_bDetailedLog)
			vcUtils::LogMsg("vcLicense::CheckOutLicense : Acquired VMoveCAD_ProE license");
	}
	else
	{
		//if(g_bDetailedLog)
			//vcUtils::LogMsg(License.GetErrorString());
		vcUtils::LogMsg("vcLicense::CheckOutLicense : VMoveCAD_ProE license failed");
	}

	//res=License.CheckOut("VMoveCAD_SolidWorks","2010",0);
	sLicFeature = "VMoveCAD_SolidWorks";
	res = Vct::License::Client::get().acquireLicense(sLicFeature.c_str(), sLicVersion.c_str(), Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
	if(res==true)
	{
		m_bSolidWorksLicense = true;
		bLicenseFound = true;
		//if(g_bDetailedLog)
			vcUtils::LogMsg("vcLicense::CheckOutLicense : Acquired VMoveCAD_SolidWorks license");
	}
	else
	{
		//if(g_bDetailedLog)
			//vcUtils::LogMsg(License.GetErrorString());
		vcUtils::LogMsg("vcLicense::CheckOutLicense : VMoveCAD_SolidWorks license failed");
	}

	//res=License.CheckOut("VMoveCAD_SolidEdge","2010",0);
	sLicFeature = "VMoveCAD_SolidEdge";
	res = Vct::License::Client::get().acquireLicense(sLicFeature.c_str(), sLicVersion.c_str(), Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
	if(res==true)
	{
		m_bSolidEdgeLicense = true;
		bLicenseFound = true;
		//if(g_bDetailedLog)
			vcUtils::LogMsg("vcLicense::CheckOutLicense : Acquired VMoveCAD_SolidEdge license");
	}
	else
	{
		//if(g_bDetailedLog)
			//vcUtils::LogMsg(License.GetErrorString());
		vcUtils::LogMsg("vcLicense::CheckOutLicense : VMoveCAD_SolidEdge license failed");
	}

	//res=License.CheckOut("VMoveCAD_Inventor","2010",0);
	sLicFeature = "VMoveCAD_Inventor";
	res = Vct::License::Client::get().acquireLicense(sLicFeature.c_str(), sLicVersion.c_str(), Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
	if(res==true)
	{
		m_bInventorLicense = true;
		bLicenseFound = true;
		//if(g_bDetailedLog)
			vcUtils::LogMsg("vcLicense::CheckOutLicense : Acquired VMoveCAD_Inventor license");
	}
	else
	{
		//if(g_bDetailedLog)
			//vcUtils::LogMsg(License.GetErrorString());
		vcUtils::LogMsg("vcLicense::CheckOutLicense : VMoveCAD_Inventor license failed");
	}

	//res=License.CheckOut("VMoveCAD_Step","2010",0);
	sLicFeature = "VMoveCAD_Step";
	res = Vct::License::Client::get().acquireLicense(sLicFeature.c_str(), sLicVersion.c_str(), Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
	if(res==true)
	{
		m_bStepLicense = true;
		bLicenseFound = true;
		//if(g_bDetailedLog)
			vcUtils::LogMsg("vcLicense::CheckOutLicense : Acquired VMoveCAD_Step license");
	}
	else
	{
		//if(g_bDetailedLog)
			//vcUtils::LogMsg(License.GetErrorString());
		vcUtils::LogMsg("vcLicense::CheckOutLicense : VMoveCAD_Step license failed");
	}

	//res=License.CheckOut("VMoveCAD_Iges","2010",0);
	sLicFeature = "VMoveCAD_Iges";
	res = Vct::License::Client::get().acquireLicense(sLicFeature.c_str(), sLicVersion.c_str(), Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
	if(res==true)
	{
		m_bIgesLicense = true;
		bLicenseFound = true;
		//if(g_bDetailedLog)
			vcUtils::LogMsg("vcLicense::CheckOutLicense : Acquired VMoveCAD_Iges license");
	}
	else
	{
		//if(g_bDetailedLog)
			//vcUtils::LogMsg(License.GetErrorString());
		vcUtils::LogMsg("vcLicense::CheckOutLicense : VMoveCAD_Iges license failed");
	}

	//res=License.CheckOut("VMoveCAD_Cgr","2010",0);
	sLicFeature = "VMoveCAD_Cgr";
	res = Vct::License::Client::get().acquireLicense(sLicFeature.c_str(), sLicVersion.c_str(), Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
	if(res==true)
	{
		m_bCgrLicense = true;
		bLicenseFound = true;
		//if(g_bDetailedLog)
			vcUtils::LogMsg("vcLicense::CheckOutLicense : Acquired VMoveCAD_Cgr license");
	}
	else
	{
		//if(g_bDetailedLog)
			//vcUtils::LogMsg(License.GetErrorString());
		vcUtils::LogMsg("vcLicense::CheckOutLicense : VMoveCAD_Cgr license failed");
	}

	//res=License.CheckOut("VMoveCAD_Acis","2010",0);
	sLicFeature = "VMoveCAD_Acis";
	res = Vct::License::Client::get().acquireLicense(sLicFeature.c_str(), sLicVersion.c_str(), Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
	if(res==true)
	{
		m_bAcisLicense = true;
		bLicenseFound = true;
		//if(g_bDetailedLog)
			vcUtils::LogMsg("vcLicense::CheckOutLicense : Acquired VMoveCAD_Acis license");
	}
	else
	{
		//if(g_bDetailedLog)
			//vcUtils::LogMsg(License.GetErrorString());
			vcUtils::LogMsg("vcLicense::CheckOutLicense : VMoveCAD_Acis license failed");
	}

	//res=License.CheckOut("VMoveCAD_Parasolid","2010",0);
	sLicFeature = "VMoveCAD_Parasolid";
	res = Vct::License::Client::get().acquireLicense(sLicFeature.c_str(), sLicVersion.c_str(), Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
	if(res==true)
	{
		m_bParasolidLicense = true;
		bLicenseFound = true;
		//if(g_bDetailedLog)
			vcUtils::LogMsg("vcLicense::CheckOutLicense : Acquired VMoveCAD_Parasolid license");
	}
	else
	{
		//if(g_bDetailedLog)
			//vcUtils::LogMsg(License.GetErrorString());
		vcUtils::LogMsg("vcLicense::CheckOutLicense : VMoveCAD_Parasolid license failed");
	}
#endif	

	sLicFeature = "VMoveCAD_Revit";
	res = Vct::License::Client::get().acquireLicense(sLicFeature.c_str(), sLicVersion.c_str(), Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
	if (res == true)
	{
		m_bRevitLicense = true;
		bLicenseFound = true;
		vcUtils::LogMsg("vcLicense::CheckOutLicense : Acquired VMoveCAD_Revit license");
	}
	else
	{
		vcUtils::LogMsg("vcLicense::CheckOutLicense : VMoveCAD_Revit license failed");
	}

	sLicFeature = "VMoveCAD_Ifc";
	res = Vct::License::Client::get().acquireLicense(sLicFeature.c_str(), sLicVersion.c_str(), Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
	if (res == true)
	{
		m_bIfcLicense = true;
		bLicenseFound = true;
		vcUtils::LogMsg("vcLicense::CheckOutLicense : Acquired VMoveCAD_Ifc license");
	}
	else
	{
		vcUtils::LogMsg("vcLicense::CheckOutLicense : VMoveCAD_Ifc license failed");
	}

	sLicFeature = "VMoveCAD_Stl";
	res = Vct::License::Client::get().acquireLicense(sLicFeature.c_str(), sLicVersion.c_str(), Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
	if (res == true)
	{
		m_bStlLicense = true;
		bLicenseFound = true;
		vcUtils::LogMsg("vcLicense::CheckOutLicense : Acquired VMoveCAD_Stl license");
	}
	else
	{
		vcUtils::LogMsg("vcLicense::CheckOutLicense : VMoveCAD_Stl license failed");
	}

	if (bLicenseFound == false)
	{
		vcUtils::LogMsg("vcLicense::CheckOutLicense : Failed to acquire License");
		vcUtils::LogMsg("License is not available.Exiting the application!", true);
		return false;
	}
	vcUtils::LogMsg("vcLicense::CheckOutLicense : End"); 
	return true;
}
void vcLicense::CheckInLicense()
{
	vcUtils::LogMsg("vcLicense::CheckInLicense : Start");
	if(m_bVMoveCADLicense)
		//License.CheckIn("VMoveCAD");
		Vct::License::Client::get().releaseLicense("VMoveCAD", Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
#if 1
	else
	{
		if(m_bCatiaV6License)
			//License.CheckIn("VMoveCAD_CatiaV6");
			Vct::License::Client::get().releaseLicense("VMoveCAD_CatiaV6", Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
		if(m_bCatiaV5License)
			//License.CheckIn("VMoveCAD_CatiaV5");
			Vct::License::Client::get().releaseLicense("VMoveCAD_CatiaV5", Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
		if(m_bCatiaV4License)
			//License.CheckIn("VMoveCAD_CatiaV4");
			Vct::License::Client::get().releaseLicense("VMoveCAD_CatiaV4", Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
		if(m_bUgLicense)
			//License.CheckIn("VMoveCAD_UG");
			Vct::License::Client::get().releaseLicense("VMoveCAD_UG", Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
		if(m_bProeLicense)
			//License.CheckIn("VMoveCAD_ProE");
			Vct::License::Client::get().releaseLicense("VMoveCAD_ProE", Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
		if(m_bSolidWorksLicense)
			//License.CheckIn("VMoveCAD_SolidWorks");
			Vct::License::Client::get().releaseLicense("VMoveCAD_SolidWorks", Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
		if(m_bSolidEdgeLicense)
			//License.CheckIn("VMoveCAD_SolidEdge");
			Vct::License::Client::get().releaseLicense("VMoveCAD_SolidEdge", Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
		if(m_bInventorLicense)
			//License.CheckIn("VMoveCAD_Inventor");
			Vct::License::Client::get().releaseLicense("VMoveCAD_Inventor", Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
		if(m_bStepLicense)
			//License.CheckIn("VMoveCAD_Step");
			Vct::License::Client::get().releaseLicense("VMoveCAD_Step", Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
		if(m_bIgesLicense)
			//License.CheckIn("VMoveCAD_Iges");
			Vct::License::Client::get().releaseLicense("VMoveCAD_Iges", Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
		if(m_bCgrLicense)
			//License.CheckIn("VMoveCAD_Cgr");
			Vct::License::Client::get().releaseLicense("VMoveCAD_Cgr", Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
		if(m_bAcisLicense)
			//License.CheckIn("VMoveCAD_Acis");
			Vct::License::Client::get().releaseLicense("VMoveCAD_Acis", Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
		if (m_bRevitLicense)
			Vct::License::Client::get().releaseLicense("VMoveCAD_Revit", Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
		if (m_bIfcLicense)
			Vct::License::Client::get().releaseLicense("VMoveCAD_Ifc", Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
		if (m_bStlLicense)
			Vct::License::Client::get().releaseLicense("VMoveCAD_Stl", Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
		if(m_bParasolidLicense)
			//License.CheckIn("VMoveCAD_Parasolid");
			Vct::License::Client::get().releaseLicense("VMoveCAD_Parasolid", Vct::License::Client::SCOPE_IF_ENV_ELSE_GENERIC);
	}
#endif
	vcUtils::LogMsg("vcLicense::CheckInLicense : End");
}