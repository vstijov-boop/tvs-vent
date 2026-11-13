#pragma once
// TVS_SPOJ_TKO: inlet duct, prolaz duct (parallel, on the other side) and a branch duct at right
// angles joined by a TK-O (T komad sa ogrankom). Plan: geometry/BranchJoin.h. Ivica: the prolaz on
// the inlet axis (moved across when it is not) or with one edge in line with the inlet edge
// (opposite the branch / on the branch side: one run wall of the T komad is straight, record
// "neck" = prolaz offset). The result is drawn first; [Da/Ivica/Ne] (Ivica: the next alignment;
// Ne: everything back). One command = one UNDO step.
#include "PantsJoinCommand.h"
#include "../geometry/BranchJoin.h"
namespace tvsBranchJoinCmd {
using tvsPantsJoinCmd::Pick;using tvsPantsJoinCmd::Saved;
inline tvsBranchJoin::V v2(const AcGePoint3d& p){return tvsBranchJoin::V(p.x,p.y);}
inline const wchar_t* errorText(int e){
    switch(e){
    case tvsBranchJoin::NotParallel:return L"ulazni i prolazni kanal moraju biti paralelni, prolaz u nastavku ulaza";
    case tvsBranchJoin::ProlazBehind:return L"prolazni kanal mora biti ispred ulaznog (u smjeru strujanja)";
    case tvsBranchJoin::NotPerpendicular:return L"ogranak mora biti okomit na ulazni kanal";
    case tvsBranchJoin::BranchSide:return L"ogranak počinje na drugoj strani voda";
    case tvsBranchJoin::ProlazJoined:return L"prolazni kanal je dalje spojen, a treba ga pomjeriti – izaberite drugu ivicu ili ga poravnajte (TVS_MOVE)";
    case tvsBranchJoin::InletShort:return L"ulazni kanal je prekratak za T komad";
    case tvsBranchJoin::ProlazShort:return L"prolazni kanal je prekratak (lice T komada bi bilo iza njegovog kraja)";
    case tvsBranchJoin::BranchShort:return L"ogranak je prekratak (lice T komada bi bilo iza njegovog kraja)";
    case tvsBranchJoin::NearJoined:return L"kraj kanala kod T komada je već spojen sa drugim elementom";
    default:return L"kanali nisu prepoznati";}
}
inline const wchar_t* alignName(int a){return a==1?L"prolaz poravnat uz ivicu ulaza nasuprot ogranku":a==2?L"prolaz poravnat uz ivicu ulaza na strani ogranka":L"prolaz u osi ulaza";}
inline bool draw(const Pick& in,const Pick& pr,const Pick& br,const tvsBranchJoin::Plan& p,double l,std::vector<Saved>& saved,AcDbObjectId& tee){
    const double f=tvsUnits::factor(in.id.database());
    if(p.moved){AcDbObjectPointer<TVS_Pipe> d(pr.id,AcDb::kForWrite);if(d.openStatus()!=Acad::eOk)return false;
        AcGeVector3d s(p.shift.x,p.shift.y,0);d->put_FirstPoint(d->FirstPoint+s);d->put_Lastpoint(d->LastPoint+s);d->recordGraphicsModified();}
    double A=in.a,B=in.b,W=br.a,BW=in.b,L=p.lengthPl;
    AcGeVector3d u(p.u.x,p.u.y,0),v(p.v.x,p.v.y,0);AcGePoint3d base(p.base.x,p.base.y,in.first.z);
    TVS_WYE* w=TVS_WYE::add_new(A,B,W,BW,L,u,v,base,false,false,false);
    if(!w||w->objectId().isNull()){acutPrintf(L"\nTVS_SPOJ_TKO: T komad nije napravljen.");tvsPantsJoinCmd::restore(saved,tee);return false;}
    tee=w->objectId();tvsTeeFit::Ports ports;
    {AcDbObjectPointer<TVS_WYE> o(tee,AcDb::kForWrite);if(o.openStatus()!=Acad::eOk){tvsPantsJoinCmd::restore(saved,tee);return false;}
     {AcDbObjectPointer<TVS_Pipe> d(in.id,AcDb::kForRead);if(d.openStatus()==Acad::eOk){o->setPropertiesFrom(d.object());o->put_Flow(d->get_Flow());}}
     o->put_Elevation(in.elev);
     tvsTeeData::Record r;r.type=tvsTee::Branch;r.l=l;r.second=pr.a;r.neck=p.xo;
     if(tvsTeeData::write(o.object(),r)!=Acad::eOk){acutPrintf(L"\nTVS_SPOJ_TKO: tip T komada nije upisan.");o.close();tvsPantsJoinCmd::restore(saved,tee);return false;}
     o->recordGraphicsModified();ports=tvsTeeFit::ports(o.object());}
    if(!ports.ok){acutPrintf(L"\nTVS_SPOJ_TKO: TK-O nije moguć za ove širine (%.0f / %.0f / %.0f mm).",A*f,pr.a*f,W*f);tvsPantsJoinCmd::restore(saved,tee);return false;}
    // the real ports (TVS_WYE): 0 inlet, 1 prolaz, 2 branch
    tvsPantsJoinCmd::setEnd(in.id,p.inletNear,ports.p[0]);
    tvsPantsJoinCmd::setEnd(pr.id,p.prolazNear,ports.p[1]);
    tvsPantsJoinCmd::setEnd(br.id,p.branchNear,ports.p[2]);
    return true;
}
inline void execute(){
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();if(!db)return;
    const double f=tvsUnits::factor(db),tol=0.1/f,l=tvsTeeData::collarMm()/f;
    Pick in,pr,br;
    if(!tvsPantsJoinCmd::pick(L"\nIzaberite ulazni kanal: ",in))return;
    if(!tvsPantsJoinCmd::pick(L"\nIzaberite prolazni kanal: ",pr))return;
    if(!tvsPantsJoinCmd::pick(L"\nIzaberite ogranak: ",br))return;
    if(in.id==pr.id||in.id==br.id||pr.id==br.id){acutPrintf(L"\nTVS_SPOJ_TKO: izaberite tri različita kanala.");return;}
    if(in.round||pr.round||br.round){acutPrintf(L"\nTVS_SPOJ_TKO: TK-O je za pravougaone kanale (okrugli ogranak: TVS_TK_TIP, štucna).");return;}
    if(std::fabs(pr.elev-in.elev)>1.0/f||std::fabs(br.elev-in.elev)>1.0/f){acutPrintf(L"\nTVS_SPOJ_TKO: kanali nisu na istoj koti (TVS_PROVJERA / Svojstva).");return;}
    if(pr.a>in.a+1e-9){acutPrintf(L"\nTVS_SPOJ_TKO: prolaz (%.0f) je širi od ulaza (%.0f mm) – zamijenite ulaz i prolaz.",pr.a*f,in.a*f);return;}
    if(br.a>in.a+1e-9){acutPrintf(L"\nTVS_SPOJ_TKO: ogranak (%.0f) je širi od ulaza (%.0f mm).",br.a*f,in.a*f);return;}
    std::vector<tvsMove::Node> nodes;AcDbObjectIdArray labels;
    if(!tvsMove::snapshot(db,nodes,labels)){acutPrintf(L"\nTVS_SPOJ_TKO: crtež se ne može pročitati.");return;}
    tvsBranchJoin::Duct din,dpr,dbr;
    din.a=v2(in.first);din.b=v2(in.last);din.width=in.a;dpr.a=v2(pr.first);dpr.b=v2(pr.last);dpr.width=pr.a;dbr.a=v2(br.first);dbr.b=v2(br.last);dbr.width=br.a;
    tvsBranchJoin::Plan p=tvsBranchJoin::plan(din,dpr,dbr,0,l,50.0/f);
    if(p.error==tvsBranchJoin::Degenerate||p.error==tvsBranchJoin::NotParallel||p.error==tvsBranchJoin::NotPerpendicular||p.error==tvsBranchJoin::ProlazBehind){
        acutPrintf(L"\nTVS_SPOJ_TKO: %ls.",errorText(p.error));return;}
    {const AcGePoint3d i0=p.inletNear==0?in.first:in.last,p0=p.prolazNear==0?pr.first:pr.last,p1=p.prolazNear==0?pr.last:pr.first,b0=p.branchNear==0?br.first:br.last;
     din.nearJoined=tvsPantsJoinCmd::joinedAt(nodes,in.id,i0,tol);dpr.nearJoined=tvsPantsJoinCmd::joinedAt(nodes,pr.id,p0,tol);
     dpr.farJoined=tvsPantsJoinCmd::joinedAt(nodes,pr.id,p1,tol);dbr.nearJoined=tvsPantsJoinCmd::joinedAt(nodes,br.id,b0,tol);}
    int align=p.suggested>=0?p.suggested:0;   // what the drawing already has, else the axis
    for(int tries=0;;++tries){
        p=tvsBranchJoin::plan(din,dpr,dbr,align,l,50.0/f);
        if(p.error==tvsBranchJoin::ProlazJoined&&tries<3){align=(align+1)%3;continue;}   // try the next alignment that needs no move
        if(p.error!=tvsBranchJoin::Ok){acutPrintf(L"\nTVS_SPOJ_TKO: %ls.",errorText(p.error));return;}
        std::vector<Saved> saved;
        {const Pick* all[3]={&in,&pr,&br};for(int k=0;k<3;++k){Saved s;s.id=all[k]->id;s.first=all[k]->first;s.last=all[k]->last;saved.push_back(s);}}
        AcDbObjectId tee;
        {tvsSizeFollow::applying()=true;const bool ok=draw(in,pr,br,p,l,saved,tee);tvsSizeFollow::applying()=false;if(!ok)return;}
        acedUpdateDisplay();
        if(p.moved)acutPrintf(L"\nProlazni kanal pomjeren za %.0f mm.",std::hypot(p.shift.x,p.shift.y)*f);
        acutPrintf(L"\nTK-O: ulaz %.0f, prolaz %.0f, ogranak %.0f mm, visina %.0f; %ls.",in.a*f,pr.a*f,br.a*f,in.b*f,alignName(align));
        if(std::fabs(pr.b-in.b)>1e-6)acutPrintf(L"\n  prolaz: visina %.0f – poslije potvrde prelaz %.0f → %.0f.",pr.b*f,in.b*f,pr.b*f);
        if(std::fabs(br.b-in.b)>1e-6)acutPrintf(L"\n  ogranak: visina %.0f – poslije potvrde prelaz %.0f → %.0f.",br.b*f,in.b*f,br.b*f);
        acedInitGet(0,L"Da Ivica Ne");ACHAR kw[16]=L"";
        const int k=acedGetKword(L"\nOvako spojiti? Ivica = osa / ivica nasuprot ogranku / ivica na strani ogranka. [Da/Ivica/Ne] <Da>: ",kw,16);
        if(k==RTNONE||(k==RTNORM&&_wcsicmp(kw,L"Da")==0)){
            // the T komad has the inlet height on every port: prelaz to a duct of another height
            if(std::fabs(pr.b-in.b)>1e-6&&!tvsTeePort::ensure(tee,1))acutPrintf(L"\nProlaz: prelaz nije umetnut (prekratak kanal?).");
            if(std::fabs(br.b-in.b)>1e-6&&!tvsTeePort::ensure(tee,2))acutPrintf(L"\nOgranak: prelaz nije umetnut (prekratak kanal?).");
            acutPrintf(L"\nTVS_SPOJ_TKO: spojeno (U vraća).");return;}
        tvsSizeFollow::applying()=true;tvsPantsJoinCmd::restore(saved,tee);tvsSizeFollow::applying()=false;
        if(k==RTNORM&&_wcsicmp(kw,L"Ivica")==0){align=(align+1)%3;tries=0;continue;}
        acutPrintf(L"\nTVS_SPOJ_TKO: ništa nije promijenjeno.");return;
    }
}
}
