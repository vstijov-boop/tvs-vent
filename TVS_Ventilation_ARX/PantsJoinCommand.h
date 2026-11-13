#pragma once
// TVS_SPOJ_TKG: inlet duct + two branch ducts (opposite, at right angles to the inlet) joined by
// a TK-G (gaćasti T komad). Plan: geometry/PantsJoin.h. One branch is moved onto the other's
// axis (the one with a free far end), the inlet duct is shortened to the T komad, the branch
// ducts end on its faces. The result is drawn first and confirmed: [Da/Zamijeni/Ne] (Zamijeni:
// the other branch moves; Ne: everything back). One command = one UNDO step.
#include "TVSMoveCommand.h"
#include "SizeFollow.h"
#include "../geometry/PantsJoin.h"
#include "../geometry/TeeShape.h"
#include <vector>
namespace tvsPantsJoinCmd {
struct Pick {AcDbObjectId id;AcGePoint3d first,last;double a,b,elev;bool round;};
inline bool pick(const wchar_t* prompt,Pick& out){
    for(;;){ads_name e;ads_point p;const int rc=acedEntSel(prompt,e,p);
        if(rc==RTCAN||rc==RTNONE)return false;if(rc!=RTNORM)continue;
        AcDbObjectId id;if(acdbGetObjectId(id,e)!=Acad::eOk)continue;
        AcDbObjectPointer<TVS_Pipe> d(id,AcDb::kForRead);
        if(d.openStatus()!=Acad::eOk||!tvsSizeFollow::plainDuct(d.object())||d->getDuctType()==DuctTypeFlex){acutPrintf(L"\nIzaberite običan TVS kanal.");continue;}
        out.id=id;out.first=d->FirstPoint;out.last=d->LastPoint;out.a=d->SizeA;out.b=d->SizeB;out.elev=d->Elev;out.round=d->ThisRound;return true;}
}
inline tvsPantsJoin::V v2(const AcGePoint3d& p){return tvsPantsJoin::V(p.x,p.y);}
// an element of the drawing (not `self`) with a port on the plan point p
inline bool joinedAt(const std::vector<tvsMove::Node>& nodes,const AcDbObjectId& self,const AcGePoint3d& p,double tol){
    for(size_t k=0;k<nodes.size();++k){if(nodes[k].id==self)continue;
        for(size_t j=0;j<nodes[k].ports.size();++j)if(std::hypot(nodes[k].ports[j].x-p.x,nodes[k].ports[j].y-p.y)<=tol)return true;}
    return false;
}
inline const wchar_t* errorText(int e){
    switch(e){
    case tvsPantsJoin::NotPerpendicular:return L"ogranci moraju biti okomiti na ulazni kanal";
    case tvsPantsJoin::NotOpposite:return L"ogranci moraju ići na suprotne strane";
    case tvsPantsJoin::BothJoined:return L"oba ogranka su dalje spojena, a nisu u istoj osi – poravnajte jedan (TVS_MOVE) pa ponovo";
    case tvsPantsJoin::InletShort:return L"ulazni kanal je prekratak za T komad";
    case tvsPantsJoin::BranchShort:return L"ogranak je prekratak (lice T komada bi bilo iza njegovog kraja)";
    case tvsPantsJoin::NearJoined:return L"kraj kanala kod račve je već spojen sa drugim elementom";
    default:return L"kanali nisu prepoznati";}
}
struct Saved {AcDbObjectId id;AcGePoint3d first,last;};
inline void restore(const std::vector<Saved>& saved,AcDbObjectId& tee){
    if(!tee.isNull()){AcDbObjectPointer<TVS_WYE> w(tee,AcDb::kForWrite);if(w.openStatus()==Acad::eOk)w->erase();tee=AcDbObjectId::kNull;}
    for(size_t k=0;k<saved.size();++k){AcDbObjectPointer<TVS_Pipe> p(saved[k].id,AcDb::kForWrite);if(p.openStatus()!=Acad::eOk)continue;
        p->put_FirstPoint(saved[k].first);p->put_Lastpoint(saved[k].last);p->recordGraphicsModified();}
}
inline void setEnd(const AcDbObjectId& id,int end,const AcGePoint3d& to){
    AcDbObjectPointer<TVS_Pipe> p(id,AcDb::kForWrite);if(p.openStatus()!=Acad::eOk)return;
    AcGePoint3d e=end==0?p->FirstPoint:p->LastPoint;e.x=to.x;e.y=to.y;
    if(end==0)p->put_FirstPoint(e);else p->put_Lastpoint(e);p->recordGraphicsModified();
}
// The T komad and the duct ends of plan p; false: not drawn (everything back).
inline bool draw(const Pick& in,const Pick br[2],const tvsPantsJoin::Plan& p,double l,std::vector<Saved>& saved,AcDbObjectId& tee){
    const double f=tvsUnits::factor(in.id.database());
    if(p.moved>=0){AcDbObjectPointer<TVS_Pipe> d(br[p.moved].id,AcDb::kForWrite);if(d.openStatus()!=Acad::eOk)return false;
        AcGeVector3d s(p.shift.x,p.shift.y,0);d->put_FirstPoint(d->FirstPoint+s);d->put_Lastpoint(d->LastPoint+s);d->recordGraphicsModified();}
    double w1=p.w1,B=in.b,A=in.a,B2=in.b,L=tvsTee::pantsLengthPl(l,p.w1,p.w2);
    AcGeVector3d u(p.u.x,p.u.y,0),v(p.v.x,p.v.y,0);AcGePoint3d base(p.base.x,p.base.y,in.first.z);
    TVS_WYE* w=TVS_WYE::add_new(w1,B,A,B2,L,u,v,base,false,false,false);
    if(!w||w->objectId().isNull()){acutPrintf(L"\nTVS_SPOJ_TKG: T komad nije napravljen.");restore(saved,tee);return false;}
    tee=w->objectId();
    tvsTeeFit::Ports ports;
    {AcDbObjectPointer<TVS_WYE> o(tee,AcDb::kForWrite);if(o.openStatus()!=Acad::eOk){restore(saved,tee);return false;}
     {AcDbObjectPointer<TVS_Pipe> d(in.id,AcDb::kForRead);if(d.openStatus()==Acad::eOk){o->setPropertiesFrom(d.object());o->put_Flow(d->get_Flow());}}
     o->put_Elevation(in.elev);
     tvsTeeData::Record r;r.type=tvsTee::Pants;r.l=l;r.second=p.w2;r.radius2=tvsTee::defaultRadius(p.w2);
     if(tvsTeeData::write(o.object(),r)!=Acad::eOk){acutPrintf(L"\nTVS_SPOJ_TKG: tip T komada nije upisan.");o.close();restore(saved,tee);return false;}
     o->recordGraphicsModified();ports=tvsTeeFit::ports(o.object());}
    if(!ports.ok){acutPrintf(L"\nTVS_SPOJ_TKG: TK-G nije moguć za ove širine (%.0f / %.0f / %.0f mm).",A*f,p.w1*f,p.w2*f);restore(saved,tee);return false;}
    // the real ports (TVS_WYE): 2 inlet, 1 +u = branch 0, 0 = branch 1
    setEnd(in.id,p.inletNear==0?0:1,ports.p[2]);
    setEnd(br[0].id,p.branchNear[0],ports.p[1]);
    setEnd(br[1].id,p.branchNear[1],ports.p[0]);
    return true;
}
inline void execute(){
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();if(!db)return;
    const double f=tvsUnits::factor(db),tol=0.1/f,l=tvsTeeData::collarMm()/f;
    Pick in,br[2];
    if(!pick(L"\nIzaberite ulazni kanal: ",in))return;
    if(!pick(L"\nIzaberite prvi ogranak: ",br[0]))return;
    if(!pick(L"\nIzaberite drugi ogranak: ",br[1]))return;
    if(br[0].id==br[1].id||br[0].id==in.id||br[1].id==in.id){acutPrintf(L"\nTVS_SPOJ_TKG: izaberite tri različita kanala.");return;}
    if(in.round||br[0].round||br[1].round){acutPrintf(L"\nTVS_SPOJ_TKG: TK-G je za pravougaone kanale.");return;}
    if(std::fabs(br[0].b-in.b)>1e-6||std::fabs(br[1].b-in.b)>1e-6){
        acutPrintf(L"\nTVS_SPOJ_TKG: visina ogranaka (%.0f / %.0f) mora biti kao visina ulaza (%.0f mm).",br[0].b*f,br[1].b*f,in.b*f);return;}
    if(std::fabs(br[0].elev-in.elev)>1.0/f||std::fabs(br[1].elev-in.elev)>1.0/f){acutPrintf(L"\nTVS_SPOJ_TKG: kanali nisu na istoj koti (TVS_PROVJERA / Svojstva).");return;}
    if(!tvsTee::pantsFeasible(in.a,br[0].a,br[1].a,l)){acutPrintf(L"\nTVS_SPOJ_TKG: ogranci su preuski za ulaz %.0f mm (TK-G nije moguć).",in.a*f);return;}
    std::vector<tvsMove::Node> nodes;AcDbObjectIdArray labels;
    if(!tvsMove::snapshot(db,nodes,labels)){acutPrintf(L"\nTVS_SPOJ_TKG: crtež se ne može pročitati.");return;}
    tvsPantsJoin::Duct din,dbr[2];
    din.a=v2(in.first);din.b=v2(in.last);din.width=in.a;
    for(int k=0;k<2;++k){dbr[k].a=v2(br[k].first);dbr[k].b=v2(br[k].last);dbr[k].width=br[k].a;}
    tvsPantsJoin::Plan p=tvsPantsJoin::plan(din,dbr,l,50.0/f);
    if(p.error==tvsPantsJoin::Degenerate){acutPrintf(L"\nTVS_SPOJ_TKG: %ls.",errorText(p.error));return;}
    // joints at the ends (the plan needs them: near ends free, a far end joined is not moved)
    {const AcGePoint3d inNear=p.inletNear==0?in.first:in.last;din.nearJoined=joinedAt(nodes,in.id,inNear,tol);
     for(int k=0;k<2;++k){const AcGePoint3d n=p.branchNear[k]==0?br[k].first:br[k].last,fr=p.branchNear[k]==0?br[k].last:br[k].first;
        dbr[k].nearJoined=joinedAt(nodes,br[k].id,n,tol);dbr[k].farJoined=joinedAt(nodes,br[k].id,fr,tol);}}
    int prefer=-1;
    for(;;){
        p=tvsPantsJoin::plan(din,dbr,l,50.0/f,prefer);
        if(p.error!=tvsPantsJoin::Ok){acutPrintf(L"\nTVS_SPOJ_TKG: %ls.",errorText(p.error));return;}
        std::vector<Saved> saved;
        {const Pick* all[3]={&in,&br[0],&br[1]};for(int k=0;k<3;++k){Saved s;s.id=all[k]->id;s.first=all[k]->first;s.last=all[k]->last;saved.push_back(s);}}
        AcDbObjectId tee;
        {tvsSizeFollow::applying()=true;const bool ok=draw(in,br,p,l,saved,tee);tvsSizeFollow::applying()=false;if(!ok)return;}
        acedUpdateDisplay();
        if(p.moved>=0)acutPrintf(L"\nOgranak %d pomjeren u osu drugog za %.0f mm.",p.moved+1,std::hypot(p.shift.x,p.shift.y)*f);
        acutPrintf(L"\nTK-G: ulaz %.0f×%.0f, ogranci %.0f / %.0f mm, ulazni kanal skraćen do T komada.",in.a*f,in.b*f,p.w1*f,p.w2*f);
        const bool canSwap=p.moved>=0&&!dbr[1-p.moved].farJoined;
        acedInitGet(0,canSwap?L"Da Zamijeni Ne":L"Da Ne");ACHAR kw[16]=L"";
        const int k=acedGetKword(canSwap?L"\nOvako spojiti? Zamijeni = pomjeri drugi ogranak. [Da/Zamijeni/Ne] <Da>: ":L"\nOvako spojiti? [Da/Ne] <Da>: ",kw,16);
        if(k==RTNONE||(k==RTNORM&&_wcsicmp(kw,L"Da")==0)){acutPrintf(L"\nTVS_SPOJ_TKG: spojeno (U vraća).");return;}
        tvsSizeFollow::applying()=true;restore(saved,tee);tvsSizeFollow::applying()=false;
        if(k==RTNORM&&_wcsicmp(kw,L"Zamijeni")==0){prefer=1-p.moved;continue;}
        acutPrintf(L"\nTVS_SPOJ_TKG: ništa nije promijenjeno.");return;
    }
}
}
