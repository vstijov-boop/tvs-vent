#pragma once
#include "resource.h"
#include "TVSPropertDynBase.h"
#include "TVS_Ventilation_ARX_i.h"
#include "SelectionMass.h"



//----- CTVSPropertyMassDyn
class ATL_NO_VTABLE CTVSPropertyMassDyn :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CTVSPropertyMassDyn, &CLSID_TVSPropertyMassDyn>,
	public ISupportErrorInfo,
	public IDispatchImpl<ITVSPropertyMassDyn, &IID_ITVSPropertyMassDyn, &LIBID_TVSTVS_Ventilation_ARXLib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
	public ICategorizeProperties,
	public IDynamicProperty2,
	public TVSPropertyDynBase
{
public:
	CTVSPropertyMassDyn() : m_pNotify(NULL) {
	}

	DECLARE_REGISTRY_RESOURCEID(IDR_TVSPROPERTYMassDYN)

	BEGIN_COM_MAP(CTVSPropertyMassDyn)
		COM_INTERFACE_ENTRY(ITVSPropertyMassDyn)
		COM_INTERFACE_ENTRY(IDispatch)
		COM_INTERFACE_ENTRY(ISupportErrorInfo)
		COM_INTERFACE_ENTRY(ICategorizeProperties)
		COM_INTERFACE_ENTRY(IDynamicProperty2)
	END_COM_MAP()

	//----- ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);


	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct() {
		return (S_OK);
	}

	void FinalRelease() {
        Disconnect();
	}

	IDynamicPropertyNotify2 *m_pNotify;

public:
	//IDynamicProperty2
	STDMETHOD(GetGUID)(GUID* propGUID);
	STDMETHOD(GetDisplayName)(BSTR* bstrName);
	STDMETHOD(IsPropertyEnabled)(IUnknown *pUnk, BOOL* pbEnabled);
	STDMETHOD(IsPropertyReadOnly)(BOOL* pbReadonly);
	STDMETHOD(GetDescription)(BSTR* bstrName);
	STDMETHOD(GetCurrentValueName)(BSTR* pbstrName);
	STDMETHOD(GetCurrentValueType)(VARTYPE* pVarType);
	STDMETHOD(GetCurrentValueData)(IUnknown *pUnk, VARIANT* pvarData);
	STDMETHOD(SetCurrentValueData)(IUnknown *pUnk, const VARIANT varData);
	STDMETHOD(Connect)(IDynamicPropertyNotify2* pSink);
	STDMETHOD(Disconnect)();
	//ICategorizePropery
	STDMETHOD(MapPropertyToCategory)(DISPID dispid, PROPCAT* ppropcat);
	STDMETHOD(GetCategoryName)(PROPCAT propcat, LCID lcid, BSTR* pbstrName);
	//ITVSPropertyMassDyn

};

OBJECT_ENTRY_AUTO(__uuidof(TVSPropertyMassDyn), CTVSPropertyMassDyn)
//OPM_DYNPROP_OBJECT_ENTRY_AUTO(CTVSPropertyMassDyn, AcDbObject)

// A distinct, read-only property shared by all entity classes. The value is the
// total of the exact selection supplied by the Property Inspector notification sink.
class ATL_NO_VTABLE CTVSSelectionMassDyn : public CTVSPropertyMassDyn
{
    std::vector<CComPtr<IUnknown> > m_objects;
    tvsSelection::CacheKey<IUnknown*> m_key;
    tvsSelection::Total<AcDbObjectId> m_total;
    tvsSelection::CacheKey<AcDbObjectId> m_editorKey;
    tvsSelection::Total<AcDbObjectId> m_editorTotal;
    bool selectionTotal(IUnknown *context, double &mass);
    void clearSelection();
public:
    STDMETHOD(GetGUID)(GUID *value);
    STDMETHOD(GetDisplayName)(BSTR *value);
    STDMETHOD(IsPropertyEnabled)(IUnknown *object, BOOL *value);
    STDMETHOD(IsPropertyReadOnly)(BOOL *value);
    STDMETHOD(GetDescription)(BSTR *value);
    STDMETHOD(GetCurrentValueData)(IUnknown *object, VARIANT *value);
    STDMETHOD(SetCurrentValueData)(IUnknown *object, const VARIANT value);
    STDMETHOD(Connect)(IDynamicPropertyNotify2 *sink);
    STDMETHOD(Disconnect)();
};

bool tvsNativeSheetValues(IUnknown *object, double &area, double &thickness, double &mass);

template<int Kind> class ATL_NO_VTABLE CTVSNativeSheetDyn : public CTVSPropertyMassDyn
{
public:
    STDMETHOD(GetGUID)(GUID *value) {
        if (!value) return E_POINTER;
        const GUID guid={0xc53e0c6b+Kind,0xd182,0x4cfb,{0x8c,0x39,0xbf,0xe6,0x03,0x1d,0x60,0x5a}};
        *value=guid; return S_OK;
    }
    STDMETHOD(GetDisplayName)(BSTR *value) {
        if (!value) return E_POINTER;
        static const wchar_t *names[]={L"Površina lima, m²",L"Debljina lima, mm",L"Masa elementa, kg"};
        *value=::SysAllocString(names[Kind]); return S_OK;
    }
    STDMETHOD(GetDescription)(BSTR *value) {
        if (!value) return E_POINTER;
        *value=::SysAllocString(L"Obračun čeličnog lima iz geometrije TVS elementa, bez prirubnica, spojeva i rezerve.");
        return S_OK;
    }
    STDMETHOD(IsPropertyEnabled)(IUnknown *object, BOOL *value) {
        if (!value) return E_POINTER;
        double area,thickness,mass;
        *value=tvsNativeSheetValues(object,area,thickness,mass); return S_OK;
    }
    STDMETHOD(IsPropertyReadOnly)(BOOL *value) {
        if (!value) return E_POINTER;
        *value=TRUE; return S_OK;
    }
    STDMETHOD(GetCurrentValueData)(IUnknown *object, VARIANT *value) {
        if (!value) return E_POINTER;
        ::VariantInit(value);
        double area,thickness,mass;
        if (!tvsNativeSheetValues(object,area,thickness,mass)) return E_FAIL;
        V_VT(value)=VT_R8;
        V_R8(value)=Kind==0 ? area : Kind==1 ? thickness : mass;
        return S_OK;
    }
    STDMETHOD(SetCurrentValueData)(IUnknown *, const VARIANT) { return E_ACCESSDENIED; }
};
