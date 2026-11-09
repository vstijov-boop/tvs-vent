#pragma once
// TVS_ALIGN (faza 3 + povezan kanal "Samo pomjeri"): jednokratno poravnanje TVS elementa prema LINE,
// pravom segmentu polilinije, XLINE ili RAY. Geometrija: AlignGeometry.h (rotacija za najmanji ugao oko
// tacke oslonca = tacka izabrane ivice / ose najbliza kliku, zatim pomjeranje samo upravno na referencu).
//  - slobodan element (kanal, koljeno u osnovi, T komad, prelaz): rotacija + pomjeranje, transformBy;
//  - povezan kanal ili fiting paralelan referenci: mreza kao TVS_MOVE (tvsMove::applyNetwork), susjedni
//    pravi kanali se produze / skrate (T komad klizi duz kanala, koljeno pomjera dionice);
//  - neparalelan povezan element, etaz / obilazak, flex, 3D: odbijeno uz poruku.
// Pregled i upis koriste isti plan; prije upisa crtez se ponovo cita (sameSnapshot); jedna transakcija.
#include "TVSMoveCommand.h"
#include "AlignGeometry.h"
#include <dbxline.h>
#include <dbray.h>
#include <dbpl.h>
#include <dbents.h>
#include <gelnsg3d.h>
#include <cmath>
namespace tvsAlignCmd {
using tvsMoveGeometry::Point;
using tvsMove::Node;
inline Point pt(const AcGePoint3d& p){return Point(p.x,p.y,p.z);}
inline AcGePoint3d ac(const Point& p){return AcGePoint3d(p.x,p.y,p.z);}
inline double& lastOffsetMm(){static double v=0;return v;}

// Temporary marks (acedGrDraw, UCS); gone with the next redraw.
class Marks {
    bool drawn;
public:
    Marks():drawn(false){}
    ~Marks(){if(drawn)acedRedraw(NULL,1);}
    void line(const Point& a,const Point& b,int color){
        ads_point w1={a.x,a.y,a.z},w2={b.x,b.y,b.z},u1,u2;
        if(tvsCoordinates::transformPoint(w1,0,1,u1)!=RTNORM||tvsCoordinates::transformPoint(w2,0,1,u2)!=RTNORM)return;
        acedGrDraw(u1,u2,color,0);drawn=true;
    }
};

// Another TVS port on this element's ports, this element's port inside a pipe, or a port inside this pipe.
inline bool connected(const std::vector<Node>& nodes,int i,double tol){
    const Node& n=nodes[i];
    for(size_t k=0;k<nodes.size();++k){if(static_cast<int>(k)==i)continue;const Node& o=nodes[k];
        for(size_t a=0;a<n.ports.size();++a){
            for(size_t b=0;b<o.ports.size();++b)if(tvsMoveGeometry::pointsNear(n.ports[a],o.ports[b],tol))return true;
            if(o.ports.size()==2&&o.kind==1&&tvsMoveNetwork::interior(n.ports[a],o.ports[0],o.ports[1],tol))return true;
        }
        if(n.kind==1&&n.ports.size()==2)for(size_t b=0;b<o.ports.size();++b)
            if(tvsMoveNetwork::interior(o.ports[b],n.ports[0],n.ports[1],tol))return true;
    }
    return false;
}
inline bool inAssembly(const AcDbObjectId& id){
#ifdef TVS_OFFSET_INTEGRATION
    AcDbObjectPointer<AcDbEntity> e(id,AcDb::kForRead);if(e.openStatus()!=Acad::eOk)return true;
    return tvsOffsetData::marked(e.object())||tvsBypassData::marked(e.object());
#else
    (void)id;return false;
#endif
}

// Chosen element reference: anchor (pivot), direction, segment shown in the drawing.
struct Reference {Point anchor,direction,seg[2];};
inline bool askMode(bool edges,tvsAlign::Reference& mode){
    mode=tvsAlign::PickedEdge;if(!edges){mode=tvsAlign::Axis;return true;}
    ACHAR keyword[132];acedInitGet(0,L"Izabrana Druga Osa");
    const int status=acedGetKword(L"\nReferenca na elementu [Izabrana ivica/Druga ivica/Osa] <Izabrana>: ",keyword);
    if(status!=RTNORM&&status!=RTNONE)return false;
    if(status==RTNORM){
        if(_wcsicmp(keyword,L"Druga")==0)mode=tvsAlign::OtherEdge;
        else if(_wcsicmp(keyword,L"Osa")==0)mode=tvsAlign::Axis;
    }
    return true;
}
inline bool pickReference(const Node& n,const Point& cursor,Reference& r){
    if(n.kind==1){   // pipe: picked edge / other edge / axis along the whole body (as TVS_MOVE)
        tvsAlign::Reference mode;if(!askMode(true,mode))return false;
        const Point ends[2]={n.ports[0],n.ports[1]};
        const tvsMoveGeometry::Reference m=mode==tvsAlign::Axis?tvsMoveGeometry::Axis:mode==tvsAlign::OtherEdge?tvsMoveGeometry::OtherEdge:tvsMoveGeometry::PickedEdge;
        if(!tvsMoveGeometry::referencePoint(ends,cursor,n.a,m,r.anchor)){acutPrintf(L"\nTVS_ALIGN: nije moguće odrediti ivicu kanala.");return false;}
        r.direction=ends[1]-ends[0];
        const Point axis=ends[1]-ends[0];double f=(r.anchor-ends[0]).dot(axis)/axis.dot(axis);
        const Point shift=r.anchor-(ends[0]+axis*f);r.seg[0]=ends[0]+shift;r.seg[1]=ends[1]+shift;
        return true;
    }
    // Fitting: the port nearest the click; its own axis, never one common axis of the fitting.
    int best=-1;double d=0;
    for(size_t k=0;k<n.ports.size()&&k<n.axes.size()&&k<n.sections.size();++k){
        const double dk=std::hypot(n.ports[k].x-cursor.x,n.ports[k].y-cursor.y);
        if(best<0||dk<d){best=static_cast<int>(k);d=dk;}}
    if(best<0)return false;
    const bool transition=n.kind==3;
    if(transition)acutPrintf(L"\nPrelaz: referenca je osa priključka %d.",best+1);
    tvsAlign::Reference mode;if(!askMode(!transition,mode))return false;
    const double width=n.sections[best].a;
    if(!tvsAlign::portReference(n.ports[best],n.axes[best],width,cursor,mode,r.anchor)){
        acutPrintf(L"\nTVS_ALIGN: nije moguće odrediti priključak.");return false;}
    Point u;if(!tvsMoveGeometry::unit(Point(n.axes[best].x,n.axes[best].y,0),u))return false;
    r.direction=u;r.seg[0]=r.anchor;r.seg[1]=r.anchor-u*(width>0?width:1);
    return true;
}

// LINE, straight segment of an LWPOLYLINE, XLINE or RAY in plan: its infinite carrier line.
inline int pickTarget(tvsAlign::Line& line,double tol){
    for(;;){
        ads_name name;ads_point picked;
        const int status=tvsCoordinates::selectEntity(L"\nIzaberite referentnu liniju (LINE, pravi segment polilinije, XLINE, RAY): ",name,picked);
        if(status==RTERROR)continue;   // missed pick
        if(status!=RTNORM)return status;
        AcDbObjectId id;if(acdbGetObjectId(id,name)!=Acad::eOk)continue;
        AcDbEntityPointer e(id,AcDb::kForRead);if(e.openStatus()!=Acad::eOk)continue;
        if(AcDbLine* l=AcDbLine::cast(e.object())){
            if(std::fabs(l->startPoint().z-l->endPoint().z)>tol){acutPrintf(L"\nLinija nije u ravni osnove (krajevi su na različitoj visini).");continue;}
            if(tvsAlign::line(pt(l->startPoint()),pt(l->endPoint()),line))return RTNORM;
            acutPrintf(L"\nLinija nema pravac u osnovi.");continue;
        }
        if(AcDbPolyline* pl=AcDbPolyline::cast(e.object())){
            if(std::fabs(std::fabs(pl->normal().z)-1)>1e-9){acutPrintf(L"\nPolilinija nije u ravni osnove.");continue;}
            const unsigned n=pl->numVerts(),segments=pl->isClosed()?n:(n>0?n-1:0);
            if(segments==0){acutPrintf(L"\nPolilinija nema segmente.");continue;}
            AcGePoint3d on;double param=0;
            if(pl->getClosestPointTo(AcGePoint3d(picked[0],picked[1],picked[2]),AcGeVector3d::kZAxis,on)!=Acad::eOk
                ||pl->getParamAtPoint(on,param)!=Acad::eOk)continue;
            unsigned index=static_cast<unsigned>(std::floor(param+1e-9));if(index>=segments)index=segments-1;
            if(pl->segType(index)!=AcDbPolyline::kLine){acutPrintf(L"\nLučni segment polilinije nije podržan; izaberite pravi segment.");continue;}
            AcGeLineSeg3d seg;if(pl->getLineSegAt(index,seg)!=Acad::eOk)continue;
            if(tvsAlign::line(pt(seg.startPoint()),pt(seg.endPoint()),line))return RTNORM;
            acutPrintf(L"\nSegment nema pravac u osnovi.");continue;
        }
        AcGePoint3d base;AcGeVector3d dir;bool infinite=false;
        if(AcDbXline* x=AcDbXline::cast(e.object())){base=x->basePoint();dir=x->unitDir();infinite=true;}
        else if(AcDbRay* r=AcDbRay::cast(e.object())){base=r->basePoint();dir=r->unitDir();infinite=true;}
        if(infinite){
            if(std::fabs(dir.z)>1e-9){acutPrintf(L"\nXLINE / RAY nije u ravni osnove.");continue;}
            if(tvsAlign::lineFromDirection(pt(base),Point(dir.x,dir.y,0),line))return RTNORM;
            continue;
        }
        if(AcDb2dPolyline::cast(e.object())||AcDb3dPolyline::cast(e.object())){
            acutPrintf(L"\nStara (2D/3D) polilinija nije podržana; koristite LWPOLYLINE (CONVERTPOLY).");continue;}
        acutPrintf(L"\nIzabrani objekat nije LINE, polilinija, XLINE ni RAY.");
    }
}
inline AcGeMatrix3d matrix(const tvsAlign::Plan& p){
    return AcGeMatrix3d::translation(AcGeVector3d(p.delta.x,p.delta.y,0))
        *AcGeMatrix3d::rotation(p.angle,AcGeVector3d::kZAxis,ac(p.pivot));
}

// One prepared input; the preview and the final write both use compute().
struct Input {
    const std::vector<Node>* nodes;const std::vector<tvsMoveNetwork::Body>* bodies;
    int selected;bool linked;Reference ref;tvsAlign::Line target;double offset,tol,minimum;
    bool compute(int sideSign,tvsAlign::Plan& plan,tvsMoveNetwork::Result* network)const{
        if(!tvsAlign::plan(ref.anchor,ref.direction,target,offset,sideSign,1e-6,plan))return false;
        if(!linked)return true;
        if(plan.rotate)return false;
        // Connected element: translation only (TVS_MOVE network). A pipe moves exactly perpendicular to its axis.
        const Node& n=(*nodes)[selected];
        if(n.kind==1){Point axis;if(!tvsMoveGeometry::unit(n.ports[1]-n.ports[0],axis))return false;
            const Point normal(-axis.y,axis.x,0);plan.delta=normal*plan.delta.dot(normal);}
        if(!network)return true;
        return tvsMoveNetwork::plan(*bodies,selected,plan.delta,minimum,tol,*network);
    }
};

class AlignPreview : public AcDbEntity {
public:
    const Input& in;std::vector<int> masks;tvsMove::StretchPreview stretch;
    tvsAlign::Plan plan;bool allowed;
    AlignPreview(const Input& input):in(input),masks(input.nodes->size(),0),stretch(*input.nodes,masks),allowed(false){plan.angle=0;plan.rotate=false;}
    virtual Adesk::Boolean subWorldDraw(AcGiWorldDraw* draw){
        if(in.linked){stretch.subWorldDraw(draw);}
        else{
            AcDbEntityPointer e((*in.nodes)[in.selected].id,AcDb::kForRead);
            if(e.openStatus()==Acad::eOk){const AcGeMatrix3d m=matrix(plan);
                draw->geometry().pushModelTransform(m);draw->geometry().draw(e.object());draw->geometry().popModelTransform();}
        }
        draw->subEntityTraits().setColor(static_cast<Adesk::UInt16>(allowed?3:1));
        AcGePoint3d seg[2]={ac(tvsAlign::apply(plan,in.ref.seg[0])),ac(tvsAlign::apply(plan,in.ref.seg[1]))};
        draw->geometry().polyline(2,seg);
        return Adesk::kTrue;
    }
};
class AlignJig : public AcEdJig {
    const Input& in;AlignPreview preview;AcGePoint3d last;bool sampled;int fallback;
public:
    tvsMoveNetwork::Result result;
    AlignJig(const Input& input,int currentSide):in(input),preview(input),last(ac(input.ref.anchor)),sampled(false),fallback(currentSide){
        setDispPrompt(L"\nStrana odmaka: kliknite stranu (zeleno: dozvoljeno, crveno: nije; Enter: trenutna strana; Esc: odustani): ");
        setUserInputControls(static_cast<UserInputControls>(kAccept3dCoordinates|kNullResponseAccepted));
        update();
    }
    virtual AcDbEntity* entity()const{return const_cast<AlignPreview*>(&preview);}
    virtual DragStatus sampler(){
        AcGePoint3d next;const DragStatus status=acquirePoint(next);if(status!=kNormal)return status;
        if(sampled&&next.distanceTo(last)<in.tol*0.01)return kNoChange;
        sampled=true;last=next;return kNormal;
    }
    int sideSign()const{return tvsAlign::side(in.target,pt(last),in.tol,fallback);}
    virtual Adesk::Boolean update(){
        tvsMoveNetwork::Result r;preview.allowed=in.compute(sideSign(),preview.plan,in.linked?&r:NULL);
        if(in.linked&&r.masks.size()==in.nodes->size()){preview.masks=r.masks;preview.stretch.planned=r.ports;}
        preview.stretch.delta=preview.plan.delta;preview.stretch.allowed=preview.allowed;preview.stretch.problem=r.problem;
        return Adesk::kTrue;
    }
};

inline bool applyFree(AcDbDatabase* db,const std::vector<Node>& nodes,int selected,const tvsAlign::Plan& plan,double factor){
    std::vector<Node> current;AcDbObjectIdArray labels;
    if(!tvsMove::snapshot(db,current,labels)||!tvsMove::sameSnapshot(nodes,current,1e-10/factor)){
        acutPrintf(L"\nTVS_ALIGN: crtež se promijenio tokom unosa. Pokrenite komandu ponovo.");return false;}
    const AcGeMatrix3d m=matrix(plan);
    tvsMove::Transaction tx;if(!tx.good()){acutPrintf(L"\nTVS_ALIGN: nije moguće započeti izmjenu.");return false;}
    AcDbEntity* e=tx.write(nodes[selected].id);
    if(!e){acutPrintf(L"\nTVS_ALIGN: element je zaključan, dio etaža ili nedostupan. Ništa nije promijenjeno.");return false;}
    if(e->transformBy(m)!=Acad::eOk){acutPrintf(L"\nTVS_ALIGN: element nije moguće pomjeriti. Ništa nije promijenjeno.");return false;}
    e->recordGraphicsModified();
    MleaderController controller;   // labels: the arrow follows the element; text (sizes) is unchanged
    for(int i=0;i<labels.length();++i){
        AcDbObjectId target;if(!controller.getObjectId(labels[i],target)||target!=nodes[selected].id)continue;
        AcDbMLeader* leader=AcDbMLeader::cast(tx.write(labels[i]));
        if(!leader){acutPrintf(L"\nTVS_ALIGN: povezanu oznaku nije moguće ažurirati. Ništa nije promijenjeno.");return false;}
        AcArray<int> lines;if(leader->getLeaderLineIndexes(lines)!=Acad::eOk)return false;
        for(int j=0;j<lines.length();++j){AcGePoint3d old;
            if(leader->getFirstVertex(lines[j],old)!=Acad::eOk||leader->setFirstVertex(lines[j],m*old)!=Acad::eOk){
                acutPrintf(L"\nTVS_ALIGN: povezanu oznaku nije moguće ažurirati. Ništa nije promijenjeno.");return false;}}
    }
    if(!tx.commit()){acutPrintf(L"\nTVS_ALIGN: izmjena nije sačuvana.");return false;}
    const double pi=3.14159265358979323846;
    acutPrintf(L"\nTVS_ALIGN: poravnato. Rotacija %.2f°, pomjeraj %.1f mm.",plan.angle*180/pi,plan.delta.length()*factor);
    return true;
}

inline void execute(){
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();if(!db)return;
    ads_name name;ads_point picked;
    if(tvsCoordinates::selectEntity(L"\nIzaberite TVS kanal ili fiting (kliknite blizu željene ivice ili priključka): ",name,picked)!=RTNORM)return;
    AcDbObjectId id;if(acdbGetObjectId(id,name)!=Acad::eOk)return;
    std::vector<Node> nodes;AcDbObjectIdArray labels;
    if(!tvsMove::snapshot(db,nodes,labels)){acutPrintf(L"\nTVS_ALIGN: nije moguće pročitati elemente trenutnog prostora.");return;}
    int selected=-1;for(size_t i=0;i<nodes.size();++i)if(nodes[i].id==id)selected=static_cast<int>(i);
    if(selected<0){acutPrintf(L"\nTVS_ALIGN: izaberite TVS kanal ili fiting u trenutnom prostoru.");return;}
    const Node& n=nodes[selected];
    if(n.kind==0){acutPrintf(L"\nTVS_ALIGN: element nije podržan (flex kanal, vertikalno ili 3D koljeno, nepotpun fiting).");return;}
    if(inAssembly(n.id)){acutPrintf(L"\nTVS_ALIGN: element je dio etaža ili obilaska; pojedinačno poravnanje još nije podržano.");return;}
    const double factor=tvsUnits::factor(db),tol=0.1/factor,minimum=1.0/factor;
    const bool linked=connected(nodes,selected,tol);
    std::vector<tvsMoveNetwork::Body> bodies;std::vector<std::vector<AcDbObjectId> > members;
    if(linked){tvsMove::bodiesFor(nodes,bodies,members,tol);
        if(bodies[selected].kind!=(n.kind==1?tvsMoveNetwork::Pipe:tvsMoveNetwork::Rigid)||!members[selected].empty()){
            acutPrintf(L"\nTVS_ALIGN: element je dio etaža ili obilaska; izaberite element van njih.");return;}}
    Input in;in.nodes=&nodes;in.bodies=&bodies;in.selected=selected;in.linked=linked;in.tol=tol;in.minimum=minimum;
    if(!pickReference(n,Point(picked[0],picked[1],picked[2]),in.ref))return;
    Marks marks;marks.line(in.ref.seg[0],in.ref.seg[1],2);
    if(pickTarget(in.target,tol)!=RTNORM)return;
    {   wchar_t prompt[96];swprintf_s(prompt,L"\nOdmak od reference, mm <%.0f>: ",lastOffsetMm());
        acedInitGet(RSG_NONEG,NULL);double v=0;const int status=acedGetReal(prompt,&v);
        if(status==RTNORM)lastOffsetMm()=v;else if(status!=RTNONE)return;
        in.offset=lastOffsetMm()/factor;}
    const int currentSide=tvsAlign::side(in.target,in.ref.anchor,tol,1);
    tvsAlign::Plan plan;
    if(!in.compute(currentSide,plan,NULL)){
        if(linked&&plan.rotate)acutPrintf(L"\nTVS_ALIGN: povezan element nije paralelan sa referencom; povezani elementi se za sada mogu samo pomjeriti (bez rotacije).");
        else acutPrintf(L"\nTVS_ALIGN: poravnanje nije moguće odrediti.");
        return;}
    AlignJig jig(in,currentSide);
    const AcEdJig::DragStatus status=jig.drag();
    if(status!=AcEdJig::kNormal&&status!=AcEdJig::kNull){acutPrintf(L"\nTVS_ALIGN: odustano, ništa nije promijenjeno.");return;}
    const int sideSign=status==AcEdJig::kNull?currentSide:jig.sideSign();
    if(in.compute(sideSign,plan,NULL)&&!plan.rotate&&plan.delta.length()<=tol){acutPrintf(L"\nTVS_ALIGN: element je već poravnat.");return;}
    tvsMoveNetwork::Result network;
    if(!in.compute(sideSign,plan,linked?&network:NULL)){
        acutPrintf(L"\nTVS_ALIGN: %ls",linked?tvsMove::networkError(network.error):L"poravnanje nije moguće.");
        if(linked)tvsMove::sideBranchProblem(nodes,network,L"TVS_ALIGN");
        return;}
    if(linked)tvsMove::applyNetwork(db,nodes,bodies,members,selected,plan.delta,factor,tol,minimum,L"TVS_ALIGN");
    else applyFree(db,nodes,selected,plan,factor);
}
}
