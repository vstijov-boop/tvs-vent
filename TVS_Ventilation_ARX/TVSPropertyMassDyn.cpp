#include "StdAfx.h"
#include "TVSPropertyMassDyn.h"
#include "TVSController.h"


//----- CTVSPropertyMassDyn
STDMETHODIMP CTVSPropertyMassDyn::InterfaceSupportsErrorInfo(REFIID riid) {
	static const IID* arr[] = {
		&IID_ITVSPropertyMassDyn
	};

	for (int i = 0; i < sizeof(arr) / sizeof(arr[0]); i++) {
		if (InlineIsEqualGUID(*arr[i], riid))
			return (S_OK);
	}
	return (S_FALSE);
}

//----- IDynamicProperty
STDMETHODIMP CTVSPropertyMassDyn::GetGUID(GUID *pPropGUID) {
	if (pPropGUID == NULL)
		return (E_POINTER);
	// TODO: add your code here (and comment the line below)
	memcpy(pPropGUID, &CLSID_TVSPropertyMassDyn, sizeof(GUID));

	return (S_OK);
}

STDMETHODIMP CTVSPropertyMassDyn::GetDisplayName(BSTR *pBstrName) {
	if (pBstrName == NULL)
		return (E_POINTER);
	// TODO: add your code here (and comment the line below)
	*pBstrName = ::SysAllocString(L"Masa/jed., kg");

	return (S_OK);
}

STDMETHODIMP CTVSPropertyMassDyn::IsPropertyEnabled(IUnknown *pUnk, BOOL *pbEnabled) {
	if (pUnk == NULL)
		return (E_INVALIDARG);
	if (pbEnabled == NULL)
		return (E_POINTER);
	// TODO: add your code here (and comment the line below)
	*pbEnabled = checkEnabled(pUnk);
    CComQIPtr<IAcadBaseObject> object(pUnk);
    AcDbObjectId id;
    if (object && SUCCEEDED(object->GetObjectId(&id))) {
        AcDbObjectPointer<TVS_Entity> entity(id, AcDb::kForRead);
        double area, thickness, mass;
        if (entity.openStatus() == Acad::eOk && entity->getSheetMetal(area,thickness,mass))
            *pbEnabled = FALSE;
    }

	return (S_OK);
}

STDMETHODIMP CTVSPropertyMassDyn::IsPropertyReadOnly(BOOL *pbReadOnly) {
	if (pbReadOnly == NULL)
		return (E_POINTER);
	// TODO: add your code here (and comment the line below)
	*pbReadOnly = FALSE;

	return (S_OK);
}

STDMETHODIMP CTVSPropertyMassDyn::GetDescription(BSTR *pBstrName) {
	if (pBstrName == NULL)
		return (E_POINTER);
	// TODO: add your code here (and comment the line below)
	*pBstrName = ::SysAllocString(L"Masa elementa.");

	return (S_OK);
}

STDMETHODIMP CTVSPropertyMassDyn::GetCurrentValueName(BSTR *pBstrName) {
	if (pBstrName == NULL)
		return (E_POINTER);
	// TODO: add your code here

	//return (S_OK) ; //----- If you do anything in there
	return (E_NOTIMPL);
}

STDMETHODIMP CTVSPropertyMassDyn::GetCurrentValueType(VARTYPE *pVarType) {
	if (pVarType == NULL)
		return (E_POINTER);
	// TODO: add your code here (and comment the line below)
	*pVarType = VT_R8;

	return (S_OK);
}

STDMETHODIMP CTVSPropertyMassDyn::GetCurrentValueData(IUnknown *pUnk, VARIANT *pVarData) {
	if (pUnk == NULL)
		return (E_INVALIDARG);
	if (pVarData == NULL)
		return (E_POINTER);



	AcDbObjectId objId;
	{
		CComQIPtr<IAcadBaseObject> pObj(pUnk);
		pObj->GetObjectId(&objId);
	}


	::VariantInit(pVarData);
	V_VT(pVarData) = VT_R8;

	double retValue = 0.0;

	AcAxDocLock docLoc(acdbCurDwg());
	if (TVSController::get()->tvsPropertyController.getMass(objId, retValue)) {
		V_R8(pVarData) = retValue;
	}

	return (S_OK);
}

STDMETHODIMP CTVSPropertyMassDyn::SetCurrentValueData(IUnknown *pUnk, const VARIANT varData) {
	if (pUnk == NULL)
		return (E_INVALIDARG);

	AcDbObjectId objId;
	{
		CComQIPtr<IAcadBaseObject> pObj(pUnk);
		pObj->GetObjectId(&objId);
	}
	AcAxDocLock docLoc(acdbCurDwg());
	TVSController::get()->tvsPropertyController.setMass(objId, V_R8(&varData));
	return (S_OK);
}

STDMETHODIMP CTVSPropertyMassDyn::Connect(IDynamicPropertyNotify2 *pSink) {
	if (pSink == NULL)
		return (E_POINTER);
	if (m_pNotify) m_pNotify->Release();
	m_pNotify = pSink;
	m_pNotify->AddRef();
	return (S_OK);
}

STDMETHODIMP CTVSPropertyMassDyn::Disconnect() {
	if (m_pNotify) {
		m_pNotify->Release();
		m_pNotify = NULL;
	}
	return (S_OK);
}



//----- ICategorizePropertes
STDMETHODIMP CTVSPropertyMassDyn::MapPropertyToCategory(DISPID dispid, PROPCAT *pPropCat) {
	if (pPropCat == NULL)
		return (E_POINTER);
	// TODO: add your code here (and comment the line below)
	*pPropCat = 0;

	return (S_OK);
}

STDMETHODIMP CTVSPropertyMassDyn::GetCategoryName(PROPCAT propcat, LCID lcid, BSTR *pBstrName) {
	if (pBstrName == NULL)
		return (E_POINTER);
	// TODO: add your code here (and comment the 3 lines below)
	if (propcat != 0)
		return (E_INVALIDARG);
	*pBstrName = ::SysAllocString(L"TVS svojstva");

	return (S_OK);
}

namespace {
bool massObjectId(IUnknown *object, AcDbObjectId &id)
{
    CComQIPtr<IAcadBaseObject> entity(object);
    return entity && SUCCEEDED(entity->GetObjectId(&id)) && !id.isNull();
}
bool nativeMass(const AcDbObjectId &id, double &mass)
{
    AcDbObjectPointer<TVS_Entity> entity(id,AcDb::kForRead);
    double area,thickness;
    return entity.openStatus() == Acad::eOk && entity->getSheetMetal(area,thickness,mass);
}
}

void CTVSSelectionMassDyn::clearSelection()
{
    m_key.clear(); m_objects.clear(); m_total = tvsSelection::Total<AcDbObjectId>();
    m_editorKey.clear(); m_editorTotal = tvsSelection::Total<AcDbObjectId>();
}

bool CTVSSelectionMassDyn::selectionTotal(IUnknown *context, double &mass)
{
    AcDbObjectId contextId;
    if (!massObjectId(context,contextId)) return false;
    CComVariant selection;
    bool inspectorSelection=false;
    if (m_pNotify && SUCCEEDED(m_pNotify->GetCurrentSelectionSet(&selection))
        && V_VT(&selection) == (VT_ARRAY | VT_UNKNOWN) && selection.parray
        && SafeArrayGetDim(selection.parray) == 1) {
        LONG first=0,last=-1;
        if (SUCCEEDED(SafeArrayGetLBound(selection.parray,1,&first))
            && SUCCEEDED(SafeArrayGetUBound(selection.parray,1,&last)) && last >= first) {
            IUnknown **objects=NULL;
            if (SUCCEEDED(SafeArrayAccessData(selection.parray,reinterpret_cast<void**>(&objects)))) {
                inspectorSelection=true;
                const size_t count=static_cast<size_t>(last-first+1);
                const unsigned long revision=TVS_Entity::sheetMetalRevision();
                if (!m_key.matches(objects,count,revision)) {
                    clearSelection();
                    for (size_t i=0;i<count;++i) {
                        m_objects.push_back(CComPtr<IUnknown>(objects[i]));
                        AcDbObjectId id;
                        if (!massObjectId(objects[i],id)) continue;
                        double value=0.0;
                        const bool supported=nativeMass(id,value);
                        m_total.add(id,supported,value);
                    }
                    m_key.set(objects,count,TVS_Entity::sheetMetalRevision());
                }
                SafeArrayUnaccessData(selection.parray);
                if (m_total.contains(contextId) && m_total.count > 0) {
                    mass=m_total.mass;
                    return mass >= 0.0 && mass <= DBL_MAX;
                }
            }
        }
    }
    // Before Connect(), read the existing implied selection without prompting or
    // replacing it. This keeps the total available for a mixed TVS/text selection.
    if (!inspectorSelection) {
        ads_name set;
        if (acedSSGet(L"_I",NULL,NULL,NULL,set)==RTNORM) {
            Adesk::Int32 length=0;
            acedSSLength(set,&length);
            std::vector<AcDbObjectId> ids;
            for (Adesk::Int32 i=0;i<length;++i) {
                ads_name name;
                AcDbObjectId id;
                if (acedSSName(set,i,name)==RTNORM && acdbGetObjectId(id,name)==Acad::eOk)
                    ids.push_back(id);
            }
            acedSSFree(set);
            const AcDbObjectId *data=ids.empty() ? NULL : &ids[0];
            if (!m_editorKey.matches(data,ids.size(),TVS_Entity::sheetMetalRevision())) {
                m_editorTotal=tvsSelection::Total<AcDbObjectId>();
                for (size_t i=0;i<ids.size();++i) {
                    double value=0.0;
                    const bool supported=nativeMass(ids[i],value);
                    m_editorTotal.add(ids[i],supported,value);
                }
                m_editorKey.set(data,ids.size(),TVS_Entity::sheetMetalRevision());
            }
            if (m_editorTotal.contains(contextId) && m_editorTotal.count>0) {
                mass=m_editorTotal.mass;
                return mass>=0.0 && mass<=DBL_MAX;
            }
        }
    }
    return nativeMass(contextId,mass);
}

STDMETHODIMP CTVSSelectionMassDyn::GetGUID(GUID *value)
{
    if (!value) return E_POINTER;
    const GUID guid={0xc53e0c6a,0xd182,0x4cfb,{0x8c,0x39,0xbf,0xe6,0x03,0x1d,0x60,0x5a}};
    *value=guid; return S_OK;
}
STDMETHODIMP CTVSSelectionMassDyn::GetDisplayName(BSTR *value)
{
    if (!value) return E_POINTER;
    *value=::SysAllocString(L"Masa TVS izbora, kg"); return S_OK;
}
STDMETHODIMP CTVSSelectionMassDyn::IsPropertyEnabled(IUnknown *object, BOOL *value)
{
    if (!value) return E_POINTER;
    double mass=0.0;
    *value=selectionTotal(object,mass); return S_OK;
}
STDMETHODIMP CTVSSelectionMassDyn::IsPropertyReadOnly(BOOL *value)
{
    if (!value) return E_POINTER;
    *value=TRUE; return S_OK;
}
STDMETHODIMP CTVSSelectionMassDyn::GetDescription(BSTR *value)
{
    if (!value) return E_POINTER;
    *value=::SysAllocString(L"Zbir mase izabranih krutih TVS kanala i fitinga, bez prirubnica, spojeva i rezerve. Ostali objekti se preskaču.");
    return S_OK;
}
STDMETHODIMP CTVSSelectionMassDyn::GetCurrentValueData(IUnknown *object, VARIANT *value)
{
    if (!value) return E_POINTER;
    ::VariantInit(value);
    double mass=0.0;
    if (!selectionTotal(object,mass)) return E_FAIL;
    V_VT(value)=VT_R8; V_R8(value)=mass; return S_OK;
}
STDMETHODIMP CTVSSelectionMassDyn::SetCurrentValueData(IUnknown *, const VARIANT)
{
    return E_ACCESSDENIED;
}
STDMETHODIMP CTVSSelectionMassDyn::Connect(IDynamicPropertyNotify2 *sink)
{
    clearSelection();
    return CTVSPropertyMassDyn::Connect(sink);
}
STDMETHODIMP CTVSSelectionMassDyn::Disconnect()
{
    clearSelection();
    return CTVSPropertyMassDyn::Disconnect();
}

bool tvsNativeSheetValues(IUnknown *object, double &area, double &thickness, double &mass)
{
    AcDbObjectId id;
    if (!massObjectId(object,id)) return false;
    AcDbObjectPointer<TVS_Entity> entity(id,AcDb::kForRead);
    return entity.openStatus()==Acad::eOk && entity->getSheetMetal(area,thickness,mass);
}
