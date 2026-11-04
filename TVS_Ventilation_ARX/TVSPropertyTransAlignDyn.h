#pragma once
// Properties palette: "Poravnanje po visini" of a TVS prelaz (Osa / Gore / Dolje). A change is
// written to the transition and TVS_PRELAZ_KOTA asks which side of the network stays.
#include "TVSPropertyMassDyn.h"
#include "TransAlignCommand.h"
class ATL_NO_VTABLE CTVSTransAlignDyn : public CTVSPropertyMassDyn, public IDynamicEnumProperty
{
    static bool idOf(IUnknown* object,AcDbObjectId& id){CComQIPtr<IAcadBaseObject> e(object);return e&&SUCCEEDED(e->GetObjectId(&id))&&!id.isNull();}
public:
    BEGIN_COM_MAP(CTVSTransAlignDyn)
        COM_INTERFACE_ENTRY_IID(IID_IDynamicEnumProperty,IDynamicEnumProperty)
        COM_INTERFACE_ENTRY_CHAIN(CTVSPropertyMassDyn)
    END_COM_MAP()
    STDMETHOD(GetGUID)(GUID* value){
        if(!value)return E_POINTER;
        const GUID guid={0x5d1f2a71,0x7c3e,0x4b8e,{0x9a,0x44,0x2e,0x61,0x0b,0x7d,0x13,0xa5}};*value=guid;return S_OK;}
    STDMETHOD(GetDisplayName)(BSTR* value){if(!value)return E_POINTER;*value=::SysAllocString(L"Poravnanje po visini");return S_OK;}
    STDMETHOD(GetDescription)(BSTR* value){if(!value)return E_POINTER;
        *value=::SysAllocString(L"Prelaz po visini: osa, ravno uz gornju ili ravno uz donju ivicu kanala. Kod promjene program pita koja strana mreze ostaje.");return S_OK;}
    STDMETHOD(IsPropertyEnabled)(IUnknown* object,BOOL* value){
        if(!value)return E_POINTER;AcDbObjectId id;*value=FALSE;
        if(idOf(object,id)){AcDbObjectPointer<TVS_TRANS> t(id,AcDb::kForRead);*value=t.openStatus()==Acad::eOk;}
        return S_OK;}
    STDMETHOD(IsPropertyReadOnly)(BOOL* value){if(!value)return E_POINTER;*value=FALSE;return S_OK;}
    STDMETHOD(GetCurrentValueType)(VARTYPE* type){if(!type)return E_POINTER;*type=VT_I4;return S_OK;}
    STDMETHOD(GetCurrentValueData)(IUnknown* object,VARIANT* value){
        if(!value)return E_POINTER;::VariantInit(value);AcDbObjectId id;if(!idOf(object,id))return E_FAIL;
        AcDbObjectPointer<TVS_TRANS> t(id,AcDb::kForRead);if(t.openStatus()!=Acad::eOk)return E_FAIL;
        V_VT(value)=VT_I4;V_I4(value)=tvsTransAlign::read(t.object());return S_OK;}
    STDMETHOD(SetCurrentValueData)(IUnknown* object,const VARIANT value){
        AcDbObjectId id;if(!idOf(object,id))return E_FAIL;
        CComVariant v(value);if(FAILED(v.ChangeType(VT_I4)))return E_INVALIDARG;
        const int align=V_I4(&v);if(align<0||align>2)return E_INVALIDARG;
        AcApDocument* doc=acDocManager?acDocManager->curDocument():NULL;
        const bool locked=doc&&acDocManager->lockDocument(doc,AcAp::kWrite)==Acad::eOk;
        const bool wasEmpty=tvsTransAlignCmd::pending().empty();bool changed=false;
        {AcDbObjectPointer<TVS_TRANS> t(id,AcDb::kForWrite);if(t.openStatus()==Acad::eOk)changed=tvsTransAlignCmd::apply(t.object(),align);}
        if(locked)acDocManager->unlockDocument(doc);
        if(changed&&wasEmpty)tvsTransAlignCmd::post();
        if(m_pNotify)m_pNotify->OnChanged(static_cast<IDynamicProperty2*>(this));
        return S_OK;}
    // IDynamicEnumProperty
    STDMETHOD(GetNumPropertyValues)(LONG* count){if(!count)return E_POINTER;*count=3;return S_OK;}
    STDMETHOD(GetPropValueName)(LONG index,BSTR* name){if(!name)return E_POINTER;if(index<0||index>2)return E_INVALIDARG;
        static const wchar_t* names[3]={L"Osa",L"Ravno uz gornju ivicu",L"Ravno uz donju ivicu"};*name=::SysAllocString(names[index]);return S_OK;}
    STDMETHOD(GetPropValueData)(LONG index,VARIANT* value){if(!value)return E_POINTER;if(index<0||index>2)return E_INVALIDARG;
        ::VariantInit(value);V_VT(value)=VT_I4;V_I4(value)=index;return S_OK;}
};
