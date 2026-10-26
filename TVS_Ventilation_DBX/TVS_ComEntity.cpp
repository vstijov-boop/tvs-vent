#include "StdAfx.h"
#include "TVS_ComEntity.h"

bool CTVS_ComEntity::tryToSetCommonDisplayName(const DISPID &dispId, BSTR *propName)
{
	switch (dispId) 
	{ 
		// стандартное 
	case (0x516): 
		*propName  = ::SysAllocString(_T("Boja"));
		return true;
		break; 

	case (0x501): 
		*propName  = ::SysAllocString(_T("Sloj"));
		return true;
		break; 

	case (0x502): 
		*propName  = ::SysAllocString(_T("Tip linije"));
		return true;
		break; 

	case (0x503): 
		*propName  = ::SysAllocString(_T("Razmjera tipa linije"));
		return true;
		break; 

	case (0x513): 
		*propName  = ::SysAllocString(_T("Stil štampe"));
		return true;
		break; 

	case (0x514): 
		*propName  = ::SysAllocString(_T("Debljina linije"));
		return true;
		break; 

	case (0x515): 
		*propName  = ::SysAllocString(_T("Hiperveza"));
		return true;
		break; 

	case (0x577): 
		*propName  = ::SysAllocString(_T("Materijal"));
		return true;
		break; 

	}
	return false;
}



