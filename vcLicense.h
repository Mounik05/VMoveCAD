#pragma once

class vcLicense
{
public:
	vcLicense(void);
	~vcLicense(void);

	static bool m_bVMoveCADLicense;
	static bool m_bCatiaV6License;
	static bool m_bCatiaV5License;
	static bool m_bCatiaV4License;
	static bool m_bUgLicense;
	static bool m_bProeLicense;
	static bool m_bSolidWorksLicense;
	static bool m_bSolidEdgeLicense;
	static bool m_bInventorLicense;
	static bool m_bCgrLicense;
	static bool m_bStepLicense;
	static bool m_bIgesLicense;
	static bool m_bAcisLicense;
	static bool m_bParasolidLicense;

	static bool m_bRevitLicense;
	static bool m_bIfcLicense;
	static bool m_bStlLicense;

	static void Initialize();
	static bool CheckOutLicense();
	static void CheckInLicense(); 

};
