#include "stdafx.h"
#include "PropertyAdmin.h"
#include "../TVS_Ventilation_DBX/TVS_Pipe.h"
#include "../TVS_Ventilation_DBX/TVS_TAP.h"
#include "../TVS_Ventilation_DBX/TVS_TRANS.h"
#include "../TVS_Ventilation_DBX/TVS_WYE.h"

bool PropertyAdmin::m_bInitialized=false;
AcRxClass *PropertyAdmin::m_pClass=AcDbEntity::desc();

namespace {
struct Registration {
    CComPtr<IPropertyManager2> manager;
    CComObject<CTVSSelectionMassDyn> *selection;
    CComObject<CTVSNativeSheetDyn<0> > *area;
    CComObject<CTVSNativeSheetDyn<1> > *thickness;
    CComObject<CTVSNativeSheetDyn<2> > *mass;
    Registration():selection(NULL),area(NULL),thickness(NULL),mass(NULL) {}
};
std::vector<Registration*> registrations;
CComPtr<IPropertyManager2> managerFor(AcRxClass *type)
{
    if (!type) return CComPtr<IPropertyManager2>();
    IUnknown *raw=GET_OPMPROPERTY_MANAGER(type);
    CComQIPtr<IPropertyManager2> manager(raw);
    if (raw) raw->Release();
    return manager;
}
template<class T> bool add(CComPtr<IPropertyManager2> &manager,T *&property)
{
    if (FAILED(T::CreateInstance(&property))) return false;
    property->AddRef();
    if (SUCCEEDED(manager->AddProperty(static_cast<IDynamicProperty2*>(property)))) return true;
    property->Release(); property=NULL; return false;
}
template<class T> void remove(CComPtr<IPropertyManager2> &manager,T *&property)
{
    if (!property) return;
    if (manager) manager->RemoveProperty(static_cast<IDynamicProperty2*>(property));
    property->Disconnect(); property->Release(); property=NULL;
}
}

void PropertyAdmin::initialize()
{
    if (m_bInitialized) return;
    // Explicit mappings for every TVS wrapper avoid relying on base-class
    // property inheritance. Stable property GUIDs allow mixed selections.
    AcRxClass *types[]={m_pClass,TVS_Pipe::desc(),TVS_TAP::desc(),TVS_TRANS::desc(),TVS_WYE::desc()};
    for (size_t i=0;i<sizeof(types)/sizeof(types[0]);++i) {
        Registration *item=new Registration;
        registrations.push_back(item);
        item->manager=managerFor(types[i]);
        bool ok=item->manager!=NULL;
        if (ok && i>0) ok=add(item->manager,item->area) && add(item->manager,item->thickness)
            && add(item->manager,item->mass);
        if (ok) ok=add(item->manager,item->selection);
        if (!ok) {
            uninitialize();
            acutPrintf(L"\nTVS: registracija prikaza mase nije uspjela.");
            return;
        }
    }
    m_bInitialized=true;
}

void PropertyAdmin::uninitialize()
{
    for (size_t i=0;i<registrations.size();++i) {
        Registration *item=registrations[i];
        remove(item->manager,item->selection); remove(item->manager,item->mass);
        remove(item->manager,item->thickness); remove(item->manager,item->area);
        delete item;
    }
    registrations.clear();
    m_bInitialized=false;
}
