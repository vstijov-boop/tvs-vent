#include "StdAfx.h"
#include "TVS_ComEntity.h"

bool CTVS_ComEntity::tryToSetCommonDisplayName(const DISPID &dispId, BSTR *propName)
{
	switch (dispId) 
	{ 
    case DISPID_SheetArea:
        *propName = ::SysAllocString(L"Površina lima, m²"); return true;
    case DISPID_SheetThickness:
        *propName = ::SysAllocString(L"Debljina lima, mm"); return true;
    case DISPID_SheetMass:
        *propName = ::SysAllocString(L"Masa elementa, kg"); return true;
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




HRESULT CTVS_ComEntity::sheetValue(const TVS_Entity *entity, int property, VARIANT *value)
{
    if (!value) return E_POINTER;
    ::VariantInit(value);
    double area, thickness, mass;
    if (!entity->getSheetMetal(area,thickness,mass)) return S_OK;
    V_VT(value) = VT_R8;
    V_R8(value) = property == 0 ? area : property == 1 ? thickness : mass;
    return S_OK;
}
