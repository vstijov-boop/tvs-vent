#pragma once
// TVS_SPOJ_TKG: inlet duct + two branch ducts (opposite, at right angles to the inlet) joined by
// a TK-G (gaćasti T komad). Plan: geometry/PantsJoin.h. One branch is moved onto the other's
// axis (the one with a free far end), the inlet duct is shortened to the T komad, the branch
// ducts end on its faces. Branches not on one axis: by default nothing moves, the farther branch
// gets a longer straight inlet wall ("vrat", record neck); Pomjeri = one branch moved onto the
// other's axis instead (Zamijeni: the other one). The result is drawn first and confirmed;
// Ne: everything back. One command = one UNDO step.
// Round ducts (user 2026-11-27, geometry/WyeJoin.h): two collinear ducts + a branch at 45 deg ->
// kosa racva TK; inlet + two branches at +-45 deg -> racva Y45 (Soko). Same confirmation.
#include "TVSMoveCommand.h"
#include "SizeFollow.h"
#include "../geometry/PantsJoin.h"
#include "../geometry/WyeJoin.h"
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
    case tvsPantsJoin::NeckShape:return L"razlika osa ogranaka je prevelika za vrat (TK-G se ne može nacrtati) – izaberite Pomjeri";
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
     tvsTeeData::Record r;r.type=tvsTee::Pants;r.l=l;r.second=p.w2;r.radius2=tvsTee::defaultRadius(p.w2);r.neck=p.neck;
     if(tvsTeeData::write(o.object(),r)!=Acad::eOk){acutPrintf(L"\nTVS_SPOJ_TKG: tip T komada nije upisan.");o.close();restore(saved,tee);return false;}
     o->recordGraphicsModified();ports=tvsTeeFit::ports(o.object());}
    if(!ports.ok){acutPrintf(L"\nTVS_SPOJ_TKG: TK-G nije moguć za ove širine (%.0f / %.0f / %.0f mm).",A*f,p.w1*f,p.w2*f);restore(saved,tee);return false;}
    // the real ports (TVS_WYE): 2 inlet, 1 +u = branch 0, 0 = branch 1
    setEnd(in.id,p.inletNear==0?0:1,ports.p[2]);
    setEnd(br[0].id,p.branchNear[0],ports.p[1]);
    setEnd(br[1].id,p.branchNear[1],ports.p[0]);
    return true;
}
// The duct end nearest `to` goes there (one end of each duct, `end` from the plan).
inline void placeEnds(const AcDbObjectId& tee,const Pick* duct[],const int end[],const tvsPantsJoin::V target[],int n){
    tvsTeeFit::Ports ps;{AcDbObjectPointer<TVS_WYE> w(tee,AcDb::kForRead);if(w.openStatus()==Acad::eOk)ps=tvsTeeFit::ports(w.object());}
    for(int k=0;k<n;++k){AcGePoint3d to(target[k].x,target[k].y,0);
        if(ps.ok){int best=0;for(int j=1;j<3;++j)if(std::hypot(ps.p[j].x-to.x,ps.p[j].y-to.y)<std::hypot(ps.p[best].x-to.x,ps.p[best].y-to.y))best=j;to=ps.p[best];}
        setEnd(duct[k]->id,end[k],to);}
}
inline AcDbObjectId newWye(const Pick& like,double sa,double ta,double L,const tvsPantsJoin::V& u,const tvsPantsJoin::V& v,const tvsPantsJoin::V& base,const tvsTeeData::Record& r){
    AcGeVector3d vu(u.x,u.y,0),vv(v.x,v.y,0);AcGePoint3d b(base.x,base.y,like.first.z);double a1=sa,b1=0,a2=ta,b2=0,len=L;
    TVS_WYE* w=TVS_WYE::add_new(a1,b1,a2,b2,len,vu,vv,b,true,true,false);
    if(!w||w->objectId().isNull())return AcDbObjectId::kNull;
    const AcDbObjectId id=w->objectId();
    AcDbObjectPointer<TVS_WYE> o(id,AcDb::kForWrite);if(o.openStatus()!=Acad::eOk)return id;
    {AcDbObjectPointer<TVS_Pipe> d(like.id,AcDb::kForRead);if(d.openStatus()==Acad::eOk){o->setPropertiesFrom(d.object());o->put_Flow(d->get_Flow());}}
    o->put_Elevation(like.elev);
    o->SizeApr=sa;o->SizeBpr=0;o->SizeAotv=ta;o->SizeBotv=0;o->ThisRoundpr=true;o->ThisRoundotv=true;
    o->LengthPl=L;o->Vectpr=vu;o->Vectotv=vv;o->Basepoint=b;
    if(tvsTeeData::write(o.object(),r)!=Acad::eOk){o->erase();return AcDbObjectId::kNull;}
    o->recordGraphicsModified();return id;
}
inline const wchar_t* roundError(int e){
    switch(e){
    case tvsWyeJoin::NotAngled:return L"ogranci moraju biti pod 45° prema ulaznom kanalu";
    case tvsWyeJoin::SameSide:return L"ogranci moraju biti na suprotnim stranama (Y45), odnosno glavni kanali sa obje strane ogranka (kosa račva)";
    case tvsWyeJoin::BothJoined:return L"oba ogranka su dalje spojena, a nisu na mjestu za Y45 – pomjerite jedan (TVS_MOVE) pa ponovo";
    case tvsWyeJoin::InletShort:return L"ulazni kanal je prekratak za račvu";
    case tvsWyeJoin::BranchShort:return L"ogranak je prekratak (lice račve bi bilo iza njegovog kraja)";
    case tvsWyeJoin::NearJoined:return L"kraj kanala kod račve je već spojen sa drugim elementom";
    case tvsWyeJoin::MainSizes:return L"glavni kanal mora biti istog prečnika sa obje strane (prelaz poslije)";
    case tvsWyeJoin::MainShort:return L"glavni kanal je prekratak za kosu račvu";
    default:return L"kanali nisu prepoznati";}
}
inline bool confirm(const AcDbObjectId& tee,std::vector<Saved>& saved,const wchar_t* what){
    acedUpdateDisplay();acutPrintf(L"\n%ls",what);
    ACHAR kw[16]=L"";acedInitGet(0,L"Da Ne");const int k=acedGetKword(L"\nOvako spojiti? [Da/Ne] <Da>: ",kw,16);
    if(k==RTNONE||(k==RTNORM&&_wcsicmp(kw,L"Da")==0)){acutPrintf(L"\nTVS_SPOJ_TKG: spojeno (U vraća).");return true;}
    AcDbObjectId t=tee;tvsSizeFollow::applying()=true;restore(saved,t);tvsSizeFollow::applying()=false;
    acutPrintf(L"\nTVS_SPOJ_TKG: ništa nije promijenjeno.");return false;
}
// Okrugli kanali: kosa racva TK (dva kanala u jednoj osi + ogranak 45) ili racva Y45.
inline void roundJoin(AcDbDatabase* db,const Pick pk[3]){
    const double f=tvsUnits::factor(db),tol=0.1/f,deg=tvsTee::lateralDeg();
    std::vector<tvsMove::Node> nodes;AcDbObjectIdArray labels;
    if(!tvsMove::snapshot(db,nodes,labels)){acutPrintf(L"\nTVS_SPOJ_TKG: crtež se ne može pročitati.");return;}
    tvsWyeJoin::Duct d[3];for(int k=0;k<3;++k){d[k].a=v2(pk[k].first);d[k].b=v2(pk[k].last);d[k].width=pk[k].a;}
    auto joined=[&](int k,int end){return joinedAt(nodes,pk[k].id,end==0?pk[k].first:pk[k].last,tol);};
    std::vector<Saved> saved;for(int k=0;k<3;++k){Saved x;x.id=pk[k].id;x.first=pk[k].first;x.last=pk[k].last;saved.push_back(x);}
    int m[2],b=-1;
    if(tvsWyeJoin::lateralParts(d,2.0/f,m,b)){   // kosa racva TK
        const double A=pk[m[0]].a,w=pk[b].a;
        if(w>A+1e-9){acutPrintf(L"\nTVS_SPOJ_TKG: ogranak (%.0f mm) je veći od kanala (%.0f mm).",w*f,A*f);return;}
        tvsTeeData::Record r;const bool soko=tvsTeeFit::lateralRecord(r,A,w,f);
        tvsTee::Tee t;t.type=tvsTee::Tap;t.round=true;t.A=A;t.C=A;t.w1=w;t.l=r.l;t.margin=r.radius2;t.lateral=deg;
        double before=0,after=0;tvsTee::tapRun(t,before,after);const double L=tvsTee::lateralLengthPl(A,w,deg,r.l);
        tvsWyeJoin::Duct mm[2]={d[m[0]],d[m[1]]};
        tvsWyeJoin::Lateral p=tvsWyeJoin::lateral(mm,d[b],before,after,A/2+L,1.0,50.0/f);
        if(p.error==tvsWyeJoin::Ok||p.error==tvsWyeJoin::NearJoined||p.error==tvsWyeJoin::MainShort||p.error==tvsWyeJoin::BranchShort){
            mm[0].nearJoined=joined(m[0],p.nearEnd[0]);mm[1].nearJoined=joined(m[1],p.nearEnd[1]);tvsWyeJoin::Duct br=d[b];br.nearJoined=joined(b,p.nearEnd[2]);
            p=tvsWyeJoin::lateral(mm,br,before,after,A/2+L,1.0,50.0/f);}
        if(p.error!=tvsWyeJoin::Ok){acutPrintf(L"\nTVS_SPOJ_TKG: %ls.",roundError(p.error));return;}
        tvsSizeFollow::applying()=true;
        const AcDbObjectId tee=newWye(pk[m[0]],A,w,L,p.u,p.d,p.base,r);
        if(tee.isNull()){tvsSizeFollow::applying()=false;acutPrintf(L"\nTVS_SPOJ_TKG: kosa račva nije napravljena.");return;}
        const Pick* ducts[3]={&pk[m[p.up]],&pk[m[1-p.up]],&pk[b]};const int ends[3]={p.nearEnd[p.up],p.nearEnd[1-p.up],p.nearEnd[2]};
        const tvsPantsJoin::V to[3]={p.run0,p.run1,p.face};placeEnds(tee,ducts,ends,to,3);
        tvsSizeFollow::applying()=false;
        wchar_t msg[200];swprintf_s(msg,L"Kosa račva TK 45° Ø%.0f / Ø%.0f (L=%.0f mm%ls).",A*f,w*f,(r.radius2*2+w*std::sqrt(2.0))*f,soko?L", Soko":L"");
        confirm(tee,saved,msg);return;
    }
    // racva Y45: the first pick is the inlet
    const double A=pk[0].a;tvsWyeJoin::Duct br[2]={d[1],d[2]};
    if(pk[1].a>A+1e-9||pk[2].a>A+1e-9){acutPrintf(L"\nTVS_SPOJ_TKG: ogranak je veći od ulaza (Y45). Za kosu račvu izaberite dva kanala u istoj osi i ogranak.");return;}
    tvsTeeData::Record r;double Lpl=0;tvsTeeFit::wyeRecord(r,A,pk[1].a,pk[2].a,f,Lpl);
    tvsWyeJoin::Plan p=tvsWyeJoin::wye(d[0],br,r.radius2,r.l,1.0,50.0/f);
    if(p.error==tvsWyeJoin::Degenerate||p.error==tvsWyeJoin::NotAngled||p.error==tvsWyeJoin::SameSide){
        acutPrintf(L"\nTVS_SPOJ_TKG: %ls (prvi izabrani kanal je ulaz Y45; za kosu račvu: dva kanala u istoj osi + ogranak).",roundError(p.error));return;}
    d[0].nearJoined=joined(0,p.inletNear);
    for(int k=0;k<2;++k){br[k].nearJoined=joined(k+1,p.branchNear[k]);br[k].farJoined=joined(k+1,1-p.branchNear[k]);}
    const int right=p.right;const double w1=pk[1+right].a,w2=pk[2-right].a;
    const bool soko=tvsTeeFit::wyeRecord(r,A,w1,w2,f,Lpl);
    {tvsTee::Tee t;t.type=tvsTee::Pants;t.round=true;t.A=A;t.w1=w1;t.w2=w2;t.l=r.l;t.margin=r.radius2;t.lateral=deg;
     if(!tvsTee::valid(t)){acutPrintf(L"\nTVS_SPOJ_TKG: Y45 nije moguća – ogranci su premali za ulaz %.0f mm.",A*f);return;}}
    p=tvsWyeJoin::wye(d[0],br,r.radius2,r.l,1.0,50.0/f);
    if(p.error!=tvsWyeJoin::Ok){acutPrintf(L"\nTVS_SPOJ_TKG: %ls.",roundError(p.error));return;}
    tvsSizeFollow::applying()=true;
    if(p.moved>=0){AcDbObjectPointer<TVS_Pipe> q(pk[1+p.moved].id,AcDb::kForWrite);
        if(q.openStatus()==Acad::eOk){AcGeVector3d sh(p.shift.x,p.shift.y,0);q->put_FirstPoint(q->FirstPoint+sh);q->put_Lastpoint(q->LastPoint+sh);q->recordGraphicsModified();}}
    const AcDbObjectId tee=newWye(pk[0],w1,A,p.lengthPl,p.u,p.v,p.base,r);
    if(tee.isNull()){AcDbObjectId none;restore(saved,none);tvsSizeFollow::applying()=false;acutPrintf(L"\nTVS_SPOJ_TKG: Y45 nije napravljena.");return;}
    const Pick* ducts[3]={&pk[0],&pk[1],&pk[2]};const int ends[3]={p.inletNear,p.branchNear[0],p.branchNear[1]};
    const tvsPantsJoin::V to[3]={p.inletPort,p.port[0],p.port[1]};placeEnds(tee,ducts,ends,to,3);
    tvsSizeFollow::applying()=false;
    if(p.moved>=0)acutPrintf(L"\nOgranak %d pomjeren duž ulaza za %.0f mm (na mjesto ose Y45).",p.moved+1,std::hypot(p.shift.x,p.shift.y)*f);
    wchar_t msg[200];swprintf_s(msg,L"Račva Y45 Ø%.0f / Ø%.0f, Ø%.0f (L2 = %.0f mm%ls).",A*f,w1*f,w2*f,r.l*f,soko?L", Soko":L", nije u Soko tabeli");
    confirm(tee,saved,msg);
}
inline void execute(){
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();if(!db)return;
    const double f=tvsUnits::factor(db),tol=0.1/f,l=tvsTeeData::collarMm()/f;
    Pick in,br[2];
    if(!pick(L"\nIzaberite ulazni kanal: ",in))return;
    if(in.round)acutPrintf(L"\nOkrugli kanali: Y45 = ulaz + dva ogranka pod 45°; kosa račva = dva kanala u istoj osi + ogranak pod 45°.");
    if(!pick(in.round?L"\nIzaberite drugi kanal: ":L"\nIzaberite prvi ogranak: ",br[0]))return;
    if(!pick(in.round?L"\nIzaberite treći kanal: ":L"\nIzaberite drugi ogranak: ",br[1]))return;
    if(br[0].id==br[1].id||br[0].id==in.id||br[1].id==in.id){acutPrintf(L"\nTVS_SPOJ_TKG: izaberite tri različita kanala.");return;}
    if(std::fabs(br[0].elev-in.elev)>1.0/f||std::fabs(br[1].elev-in.elev)>1.0/f){acutPrintf(L"\nTVS_SPOJ_TKG: kanali nisu na istoj koti (TVS_PROVJERA / Svojstva).");return;}
    if(in.round&&br[0].round&&br[1].round){const Pick all[3]={in,br[0],br[1]};roundJoin(db,all);return;}   // kosa racva TK / Y45
    if(in.round||br[0].round||br[1].round){acutPrintf(L"\nTVS_SPOJ_TKG: kanali moraju biti svi pravougaoni (TK-G) ili svi okrugli (Y45 / kosa račva).");return;}
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
    bool neck=true;   // nothing moves unless asked (Pomjeri)
    for(;;){
        p=tvsPantsJoin::plan(din,dbr,l,50.0/f,prefer,neck);
        if(neck&&p.error==tvsPantsJoin::NeckShape){acutPrintf(L"\nTVS_SPOJ_TKG: %ls.",errorText(p.error));neck=false;continue;}
        if(p.error!=tvsPantsJoin::Ok){acutPrintf(L"\nTVS_SPOJ_TKG: %ls.",errorText(p.error));return;}
        std::vector<Saved> saved;
        {const Pick* all[3]={&in,&br[0],&br[1]};for(int k=0;k<3;++k){Saved s;s.id=all[k]->id;s.first=all[k]->first;s.last=all[k]->last;saved.push_back(s);}}
        AcDbObjectId tee;
        {tvsSizeFollow::applying()=true;const bool ok=draw(in,br,p,l,saved,tee);tvsSizeFollow::applying()=false;if(!ok)return;}
        acedUpdateDisplay();
        if(p.moved>=0)acutPrintf(L"\nOgranak %d pomjeren u osu drugog za %.0f mm.",p.moved+1,std::hypot(p.shift.x,p.shift.y)*f);
        const bool offset=std::fabs(p.e[0])+std::fabs(p.e[1])>1e-9;
        if(offset)acutPrintf(L"\nOgranci ostaju na mjestu; ogranak %d ima duži vrat za %.0f mm.",p.e[0]>0?1:2,(p.e[0]>0?p.e[0]:p.e[1])*f);
        acutPrintf(L"\nTK-G: ulaz %.0f×%.0f, ogranci %.0f / %.0f mm, ulazni kanal skraćen do T komada.",in.a*f,in.b*f,p.w1*f,p.w2*f);
        const bool canSwap=p.moved>=0&&!dbr[1-p.moved].farJoined;
        ACHAR kw[16]=L"";int k=RTNONE;
        if(offset){acedInitGet(0,L"Da Pomjeri Ne");k=acedGetKword(L"\nOvako spojiti? Pomjeri = ogranak u osu drugog. [Da/Pomjeri/Ne] <Da>: ",kw,16);}
        else if(p.moved>=0){acedInitGet(0,canSwap?L"Da Vrat Zamijeni Ne":L"Da Vrat Ne");
            k=acedGetKword(canSwap?L"\nOvako spojiti? Vrat = ogranci ostaju, Zamijeni = pomjeri drugi. [Da/Vrat/Zamijeni/Ne] <Da>: ":L"\nOvako spojiti? Vrat = ogranci ostaju. [Da/Vrat/Ne] <Da>: ",kw,16);}
        else{acedInitGet(0,L"Da Ne");k=acedGetKword(L"\nOvako spojiti? [Da/Ne] <Da>: ",kw,16);}
        if(k==RTNONE||(k==RTNORM&&_wcsicmp(kw,L"Da")==0)){
            // the T komad has the inlet height on every port: prelaz to a branch of another height
            {int portOf[2]={-1,-1};
             {AcDbObjectPointer<TVS_WYE> w(tee,AcDb::kForRead);
              if(w.openStatus()==Acad::eOk){const tvsTeeFit::Ports ps=tvsTeeFit::ports(w.object());
                for(int j=0;j<2&&ps.ok;++j){const tvsTeePort::Neighbour n=tvsTeePort::at(w.object(),ps.p[j],tol);
                    for(int b=0;b<2;++b)if(n.kind==1&&n.id==br[b].id)portOf[b]=j;}}}
             for(int b=0;b<2;++b){if(std::fabs(br[b].b-in.b)<=1e-6)continue;
                if(portOf[b]<0||!tvsTeePort::ensure(tee,portOf[b]))acutPrintf(L"\nOgranak %d: prelaz nije umetnut (prekratak kanal?).",b+1);}}
            acutPrintf(L"\nTVS_SPOJ_TKG: spojeno (U vraća).");return;}
        tvsSizeFollow::applying()=true;restore(saved,tee);tvsSizeFollow::applying()=false;
        if(k==RTNORM&&_wcsicmp(kw,L"Pomjeri")==0){neck=false;prefer=-1;continue;}
        if(k==RTNORM&&_wcsicmp(kw,L"Vrat")==0){neck=true;continue;}
        if(k==RTNORM&&_wcsicmp(kw,L"Zamijeni")==0){prefer=1-p.moved;continue;}
        acutPrintf(L"\nTVS_SPOJ_TKG: ništa nije promijenjeno.");return;
    }
}
}
