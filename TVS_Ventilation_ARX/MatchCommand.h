#pragma once
// TVS_MATCH: TVS properties of one source duct onto selected TVS elements (MATCHPROP does not
// know them). Groups (settings dialog IDD_MATCHSET): Dimenzije (ducts only), Kota (axis / top / bottom of the
// source), Tip (kruti / flex, ducts only), Prikaz (maskiranje, ivice, 1D), Protok, Sloj (layer,
// colour, linetype). After a width change, each end of a changed duct that meets an element of
// another width gets a transition (TeeNeighbours.h); elevation steps to elements outside the
// selection are listed. Targets: a selection, or Mreza = every TVS element connected to one pick;
// with Mreza the elevation is not equalised: the whole network moves by the difference between
// the source and the picked element, so existing level changes (Z prelazi, obilasci) stay.
#include "../TVS_Ventilation_DBX/TVS_Pipe.h"
#include "../TVS_Ventilation_DBX/TVS_TRANS.h"
#include "../TVS_Ventilation_DBX/DrawingUnits.h"
#include "TeeNeighbours.h"
#include "TVSMoveCommand.h"
#include "ElevationShift.h"
#include "TransAlignCommand.h"
#include <utility>
#include "resource.h"
#include "adui.h"
#include <set>
#include <vector>
namespace tvsMatch {
struct Options {bool size,elevation,type,display,flow,layer;int reference; // 0 osa, 1 vrh, 2 dno
    Options():size(true),elevation(true),type(true),display(true),flow(false),layer(true),reference(0){}};
inline Options& options(){static Options o;return o;}
// Kept per user (HKCU\Software\TVS_Vent\MatchOptions): bits D K T P O S, reference, Mreza.
inline const wchar_t* regKey(){return L"Software\\TVS_Vent";}
inline void load(Options& o,bool& network){
    HKEY k=NULL;if(RegOpenKeyExW(HKEY_CURRENT_USER,regKey(),0,KEY_READ,&k)!=ERROR_SUCCESS)return;
    DWORD v=0,size=sizeof(v),type=0;
    if(RegQueryValueExW(k,L"MatchOptions",NULL,&type,reinterpret_cast<LPBYTE>(&v),&size)==ERROR_SUCCESS&&type==REG_DWORD&&(v&0x80000000u)){
        o.size=(v&1)!=0;o.elevation=(v&2)!=0;o.type=(v&4)!=0;o.display=(v&8)!=0;o.flow=(v&16)!=0;o.layer=(v&32)!=0;
        o.reference=static_cast<int>((v>>8)&3);if(o.reference>2)o.reference=0;network=(v&(1u<<12))!=0;}
    RegCloseKey(k);
}
inline void save(const Options& o,bool network){
    const DWORD v=0x80000000u|(o.size?1:0)|(o.elevation?2:0)|(o.type?4:0)|(o.display?8:0)|(o.flow?16:0)|(o.layer?32:0)|(static_cast<DWORD>(o.reference)<<8)|(network?1u<<12:0);
    HKEY k=NULL;if(RegCreateKeyExW(HKEY_CURRENT_USER,regKey(),0,NULL,0,KEY_WRITE,NULL,&k,NULL)!=ERROR_SUCCESS)return;
    RegSetValueExW(k,L"MatchOptions",0,REG_DWORD,reinterpret_cast<const BYTE*>(&v),sizeof(v));RegCloseKey(k);
}
// Settings dialog shown on every TVS_MATCH (after the source is picked).
class Dialog : public CAdUiBaseDialog {
public:
    Options o;bool network;
    Dialog():CAdUiBaseDialog(IDD_MATCHSET,NULL,NULL),network(false){}
    virtual BOOL OnInitDialog(){
        CAdUiBaseDialog::OnInitDialog();
        CheckDlgButton(1101,o.size);CheckDlgButton(1102,o.elevation);CheckDlgButton(1104,o.type);
        CheckDlgButton(1105,o.display);CheckDlgButton(1106,o.flow);CheckDlgButton(1107,o.layer);
        if(CComboBox* c=static_cast<CComboBox*>(GetDlgItem(1103))){c->AddString(L"Osa");c->AddString(L"Vrh (gornja ivica)");c->AddString(L"Dno (donja ivica)");c->SetCurSel(o.reference);}
        CheckRadioButton(1108,1109,network?1109:1108);
        return TRUE;
    }
    virtual void OnOK(){
        o.size=IsDlgButtonChecked(1101)==BST_CHECKED;o.elevation=IsDlgButtonChecked(1102)==BST_CHECKED;o.type=IsDlgButtonChecked(1104)==BST_CHECKED;
        o.display=IsDlgButtonChecked(1105)==BST_CHECKED;o.flow=IsDlgButtonChecked(1106)==BST_CHECKED;o.layer=IsDlgButtonChecked(1107)==BST_CHECKED;
        if(CComboBox* c=static_cast<CComboBox*>(GetDlgItem(1103))){const int r=c->GetCurSel();if(r>=0&&r<=2)o.reference=r;}
        network=IsDlgButtonChecked(1109)==BST_CHECKED;
        CAdUiBaseDialog::OnOK();
    }
};
inline bool ask(bool& network){
    static bool loaded=false;static bool lastNetwork=false;
    if(!loaded){load(options(),lastNetwork);loaded=true;}
    Dialog dg;dg.o=options();dg.network=lastNetwork;
    {CAcModuleResourceOverride resources;if(dg.DoModal()!=IDOK)return false;}
    options()=dg.o;lastNetwork=network=dg.network;save(dg.o,dg.network);return true;
}
struct Source {double A,B,elev,flow,wipeLength;bool round,wipe,grani,oneD;int ductType;AcDbObjectId id;};
inline double height(double A,double B){return B>0?B:A;}   // round: diameter
inline double targetAxis(const Source& s,double A,double B,int ref){
    if(ref==1)return s.elev+height(s.A,s.B)/2-height(A,B)/2;
    if(ref==2)return s.elev-height(s.A,s.B)/2+height(A,B)/2;
    return s.elev;
}
// Mreza: every TVS element reachable from one picked element through coinciding ports.
inline bool network(AcDbDatabase* db,double tol,std::vector<AcDbObjectId>& out){
    ads_name name;ads_point pick;if(acedEntSel(L"\nIzaberite jedan element mreze: ",name,pick)!=RTNORM)return false;
    AcDbObjectId start;if(acdbGetObjectId(start,name)!=Acad::eOk)return false;
    std::vector<tvsMove::Node> nodes;AcDbObjectIdArray labels;if(!tvsMove::snapshot(db,nodes,labels))return false;
    std::vector<int> seen(nodes.size(),0),queue;
    for(size_t i=0;i<nodes.size();++i)if(nodes[i].id==start){seen[i]=1;queue.push_back(static_cast<int>(i));}
    if(queue.empty()){acutPrintf(L"\nIzabrani objekat nije TVS element.");return false;}
    for(size_t q=0;q<queue.size();++q){const tvsMove::Node& a=nodes[queue[q]];
        for(size_t k=0;k<nodes.size();++k){if(seen[k])continue;bool touch=false;
            for(size_t i=0;i<a.ports.size()&&!touch;++i)for(size_t j=0;j<nodes[k].ports.size()&&!touch;++j){
                const AcGePoint3d x=tvsMove::acad(a.ports[i]),y=tvsMove::acad(nodes[k].ports[j]);
                touch=std::hypot(x.x-y.x,x.y-y.y)<=tol;}
            if(touch){seen[k]=1;queue.push_back(static_cast<int>(k));}}}
    for(size_t q=0;q<queue.size();++q)out.push_back(nodes[queue[q]].id);
    acutPrintf(L"\nMreza: %d povezanih elemenata.",static_cast<int>(out.size()));
    return true;
}
inline void run(){
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();if(!db)return;
    const double f=tvsUnits::factor(db),tol=2.0/f;
    Source src;
    for(;;){
        ads_name name;ads_point pick;
        const int rc=acedEntSel(L"\nIzaberite izvorni TVS kanal: ",name,pick);
        if(rc!=RTNORM)return;
        AcDbObjectId id;if(acdbGetObjectId(id,name)!=Acad::eOk)return;
        AcDbObjectPointer<TVS_Pipe> p(id,AcDb::kForRead);
        if(p.openStatus()!=Acad::eOk){acutPrintf(L"\nIzvor mora biti TVS kanal.");continue;}
        src.A=p->SizeA;src.B=p->SizeB;src.elev=p->Elev;src.flow=p->Flow;src.wipeLength=p->WipeoutLength;
        src.round=p->ThisRound;src.wipe=p->Wipeout;src.grani=p->Grani;src.oneD=p->This1D;src.ductType=p->DuctType;src.id=id;
        break;
    }
    std::vector<AcDbObjectId> targets;bool whole=false;
    if(!ask(whole))return;   // settings dialog: what is transferred, Izbor / Mreza
    if(whole){if(!network(db,tol,targets))return;}
    else{
        ads_name ss;acutPrintf(L"\nIzaberite ciljne TVS elemente: ");
        if(acedSSGet(NULL,NULL,NULL,NULL,ss)!=RTNORM)return;
        Adesk::Int32 count=0;acedSSLength(ss,&count);
        for(Adesk::Int32 i=0;i<count;++i){ads_name en;AcDbObjectId id;if(acedSSName(ss,i,en)==RTNORM&&acdbGetObjectId(id,en)==Acad::eOk)targets.push_back(id);}
        acedSSFree(ss);
    }
    const Options o=options();
    std::set<AcDbObjectId> selected;std::vector<AcDbObjectId> widened,raised;
    std::vector<std::pair<AcDbObjectId,double> > offsetBefore;   // end axis offset of target transitions
    int changed=0,skippedShape=0;
    for(size_t i=0;i<targets.size();++i){
        const AcDbObjectId id=targets[i];if(id==src.id)continue;
        AcDbObjectPointer<AcDbEntity> e(id,AcDb::kForWrite);if(e.openStatus()!=Acad::eOk)continue;
        TVS_Entity* t=TVS_Entity::cast(e.object());if(!t)continue;
        selected.insert(id);
        TVS_Pipe* pipe=TVS_Pipe::cast(e.object());
        bool any=false;
        TVS_TRANS* trans=TVS_TRANS::cast(e.object());
        if(trans){offsetBefore.push_back(std::make_pair(id,tvsTransAlignCmd::endOffset(trans)));
            // Dimenzije on a prelaz: both ends take the source section (it joins ducts of that size now)
            if(o.size&&trans->ThisRoundp1==src.round&&trans->ThisRoundp2==src.round
               &&(std::fabs(trans->SizeAp1-src.A)>1e-9||std::fabs(trans->SizeBp1-src.B)>1e-9||std::fabs(trans->SizeAp2-src.A)>1e-9||std::fabs(trans->SizeBp2-src.B)>1e-9)){
                trans->put_SizeAp1(src.A);trans->put_SizeBp1(src.B);trans->put_SizeAp2(src.A);trans->put_SizeBp2(src.B);any=true;}
            else if(o.size&&(trans->ThisRoundp1!=src.round||trans->ThisRoundp2!=src.round))++skippedShape;
        }
        if(o.size&&pipe){
            if(pipe->ThisRound!=src.round)++skippedShape;
            else if(std::fabs(pipe->SizeA-src.A)>1e-9||std::fabs(pipe->SizeB-src.B)>1e-9){
                if(pipe->put_SizeA(src.A)==Acad::eOk&&pipe->put_SizeB(src.B)==Acad::eOk){widened.push_back(id);any=true;}
            }
        }
        if(o.type&&pipe&&pipe->DuctType!=src.ductType){pipe->setFlex(src.ductType==DuctTypeFlex);any=true;}
        if(o.display){t->put_Wipeout(src.wipe);t->put_WipeoutLength(src.wipeLength);t->put_Grani(src.grani);t->put_This1D(src.oneD);any=true;}
        if(o.flow){t->put_Flow(src.flow);any=true;}
        if(o.layer){AcDbObjectPointer<TVS_Pipe> s(src.id,AcDb::kForRead);if(s.openStatus()==Acad::eOk){e->setPropertiesFrom(s.object());any=true;}}
        if(o.elevation&&!whole){
            const TVS_TRANS* tr=TVS_TRANS::cast(e.object());   // prelaz: its start section (Elev = axis at the start)
            const double axis=tr?targetAxis(src,tr->SizeAp1,tr->SizeBp1,o.reference):targetAxis(src,t->SizeA,t->SizeB,o.reference);
            if(std::fabs(axis-t->Elev)>1e-9&&t->put_Elevation(axis)==Acad::eOk){raised.push_back(id);any=true;}
        }
        if(any){e->recordGraphicsModified();++changed;}
    }
    // Mreza: shift the whole network by the difference picked element -> source reference
    if(o.elevation&&whole&&!targets.empty()&&targets[0]!=src.id){
        double delta=0;
        {AcDbObjectPointer<AcDbEntity> e(targets[0],AcDb::kForRead);TVS_Entity* t=e.openStatus()==Acad::eOk?TVS_Entity::cast(e.object()):NULL;
         if(t)delta=targetAxis(src,t->SizeA,t->SizeB,o.reference)-t->Elev;}
        if(std::fabs(delta)>1e-9){
            const int shifted=tvsElevationShift::shift(targets,delta,src.id);
            acutPrintf(L"\nMreza pomjerena za %.0f mm (%d elemenata); postojece promjene kote su sacuvane.",delta*f,shifted);
        }
    }
    // Mreza: a prelaz whose end offset changed (other heights now) moves the network beyond it,
    // so the uniform shift above stays continuous across it
    if(o.elevation&&whole)
        for(size_t i=0;i<offsetBefore.size();++i){
            double now=0;{AcDbObjectPointer<TVS_TRANS> t(offsetBefore[i].first,AcDb::kForRead);if(t.openStatus()!=Acad::eOk)continue;now=tvsTransAlignCmd::endOffset(t.object());}
            const double d=now-offsetBefore[i].second;if(std::fabs(d)<=1e-9)continue;
            std::vector<AcDbObjectId> ids;bool loop=false;
            if(tvsElevationShift::side(db,offsetBefore[i].first,1,tol,ids,loop)&&!loop)tvsElevationShift::shift(ids,d,src.id);
        }
    // width steps at the ends of changed ducts
    for(size_t i=0;i<widened.size();++i){tvsTeeFit::farEnd(AcDbObjectId::kNull,widened[i],true,tol,f);tvsTeeFit::farEnd(AcDbObjectId::kNull,widened[i],false,tol,f);}
    // elevation steps to elements outside the selection
    int steps=0;
    if(!raised.empty()){
        std::vector<tvsMove::Node> nodes;AcDbObjectIdArray labels;tvsMove::snapshot(db,nodes,labels);
        std::set<AcDbObjectId> moved(raised.begin(),raised.end());
        for(size_t i=0;i<nodes.size();++i){if(!moved.count(nodes[i].id))continue;
            for(size_t j=0;j<nodes[i].ports.size();++j)for(size_t k=0;k<nodes.size();++k){
                if(k==i||selected.count(nodes[k].id))continue;
                for(size_t m=0;m<nodes[k].ports.size();++m){
                    const AcGePoint3d a=tvsMove::acad(nodes[i].ports[j]),b=tvsMove::acad(nodes[k].ports[m]);
                    if(std::hypot(a.x-b.x,a.y-b.y)>tol)continue;
                    const double dz=nodes[i].elevation-nodes[k].elevation;
                    if(std::fabs(dz)>1e-9){++steps;acutPrintf(L"\nZ razlika %.0f mm prema elementu van izbora kod (%.1f, %.1f): potreban je Z prelaz.",dz*f,a.x,a.y);}
                }
            }}
    }
    acutPrintf(L"\nTVS_MATCH: promijenjeno elemenata %d.",changed);
    if(skippedShape)acutPrintf(L" Dimenzije nisu prenesene na %d kanala drugog oblika (okrugli/pravougaoni).",skippedShape);
    if(steps)acutPrintf(L"\nZ razlika na %d mjesta; izaberite i susjedne elemente ili dodajte Z prelaz (TVS_OFFSET).",steps);
}
}
