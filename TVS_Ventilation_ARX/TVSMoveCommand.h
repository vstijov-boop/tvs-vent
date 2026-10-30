#pragma once
#include "CoordinateInput.h"
#include "MoveGeometry.h"
#include "MoveNetworkGeometry.h"
#include "TVS_Pipe.h"
#include "TVS_TAP.h"
#include "TVS_TRANS.h"
#include "TVS_WYE.h"
#include "MleaderController.h"
#include <actrans.h>
#include <dbmleader.h>
#include <vector>

namespace tvsMove {
using tvsMoveGeometry::Point;
inline Point point(const AcGePoint3d& p){return Point(p.x,p.y,p.z);}
inline AcGePoint3d acad(const Point& p){return AcGePoint3d(p.x,p.y,p.z);}
struct Node {
    AcDbObjectId id;
    int kind; // 1 pipe, 2 elbow, 3 transition, 4 wye, 0 unsupported
    std::vector<Point> ports,axes;
    std::vector<tvsMoveNetwork::Section> sections;
    Point middle;
    double a,b,elevation;
    bool round;
    Node():kind(0),a(0),b(0),elevation(0),round(false){}
};
inline bool describe(AcDbEntity* entity,Node& n){
    TVS_Entity* tvs=TVS_Entity::cast(entity);if(!tvs)return false;
    n.id=entity->objectId();
    if(TVS_Pipe* p=TVS_Pipe::cast(entity)){
        n.a=p->SizeA;n.b=p->SizeB;n.round=p->ThisRound;n.elevation=p->Elev;
        n.ports.push_back(point(p->FirstPoint));n.ports.push_back(point(p->LastPoint));
        n.axes.push_back(n.ports[1]-n.ports[0]);n.axes.push_back(n.axes[0]);
        if(p->DuctType==DuctTypeStill)n.kind=1;
    }else if(TVS_TAP* p=TVS_TAP::cast(entity)){
        n.a=p->SizeA;n.b=p->SizeB;n.round=p->ThisRound;n.elevation=p->Elev;
        n.middle=point(p->MiddlePoint);
        double radius=p->Radius;
        const int type=static_cast<int>(p->ThisRound?p->RadiusTypeRound:p->RadiusTypeRect);
        const bool variable=p->ThisRound?type==RadiusTypeRound_RadiusVariable:type==RadiusTypeRect_RadiusVariable;
        const bool constant=p->ThisRound?type==RadiusTypeRound_RadiusConstant:type==RadiusTypeRect_RadiusConstant;
        const bool specific=p->ThisRound?type==RadiusTypeRound_RadiusSpecific:type==RadiusTypeRect_RadiusSpecific;
        if(variable)radius=p->SizeA*p->RadiusVariableParameter;
        if(constant)radius=p->RadiusConst;
        if(specific)radius=tvsUnits::mm(p->SizeA,p)<355?p->SizeA:p->SizeA/2;
        Point ports[2];
        if(p->DuctType==DuctTypeStill && p->get_Form()==Form_Direct
            && std::fabs(p->Startvector.z)<1e-8 && std::fabs(p->Normvector.z-1)<1e-8
            && std::fabs(p->Normvector.x)<1e-8 && std::fabs(p->Normvector.y)<1e-8
            && (!p->ThisRound || variable || constant)
            && tvsMoveGeometry::elbowPorts(n.middle,Point(p->Startvector.x,p->Startvector.y,0),
                p->Swectangle,radius+p->SizeA/2,ports)){
            n.kind=2;n.ports.push_back(ports[0]);n.ports.push_back(ports[1]);
            n.axes.push_back(ports[0]-n.middle);n.axes.push_back(ports[1]-n.middle);
        }else{
            // Read-only fallback only blocks ambiguous contacts; never move these fittings.
            n.ports.push_back(point(p->MA));n.ports.push_back(point(p->MC));
        }
    }else if(TVS_TRANS* p=TVS_TRANS::cast(entity)){
        n.middle=point(p->FirstPoint);n.elevation=p->Elev;
        n.a=p->SizeAp1;n.b=p->SizeBp1;n.round=p->ThisRoundp1;
        Point last=point(p->FirstPoint)+Point(p->VectTr.x,p->VectTr.y,0),side;
        if(tvsMoveGeometry::unit(Point(p->VectTr.y,-p->VectTr.x,p->VectTr.z),side)){
            double shift=0;
            if(p->TransType==2)shift=(p->SizeAp1-p->SizeAp2)/2;
            if(p->TransType==3)shift=(p->SizeAp2-p->SizeAp1)/2;
            last=last+Point(side.x,side.y,0)*shift;
        }
        n.ports.push_back(point(p->FirstPoint));n.ports.push_back(last);
        n.axes.push_back(Point(p->VectTr.x,p->VectTr.y,p->VectTr.z));n.axes.push_back(n.axes[0]);
        n.sections.push_back(tvsMoveNetwork::Section(p->SizeAp1,p->SizeBp1,p->ThisRoundp1,p->Elev));
        n.sections.push_back(tvsMoveNetwork::Section(p->SizeAp2,p->SizeBp2,p->ThisRoundp2,p->Elev));
        Point direction;
        if(std::fabs(p->VectTr.z)<1e-8
            &&tvsMoveGeometry::unit(n.ports[1]-n.ports[0],direction))n.kind=3;
    }else if(TVS_WYE* p=TVS_WYE::cast(entity)){
        n.middle=point(p->Basepoint);n.a=p->SizeApr;n.b=p->SizeBpr;
        n.round=p->ThisRoundpr;n.elevation=p->Elev;
        // Reconstruct the three WYE ports without depending on a previous redraw.
        Point mainAxis,branchAxis;
        if(tvsMoveGeometry::unit(Point(p->Vectpr.x,p->Vectpr.y,0),mainAxis)
            &&tvsMoveGeometry::unit(Point(p->Vectotv.x,p->Vectotv.y,0),branchAxis)){
            Point base=point(p->Basepoint);double half=p->LengthPl+p->SizeAotv/2;
            n.ports.push_back(base-mainAxis*half);n.ports.push_back(base+mainAxis*half);
            n.ports.push_back(base+branchAxis*(p->SizeApr/2+p->LengthPl));
            n.axes.push_back(mainAxis);n.axes.push_back(mainAxis);n.axes.push_back(branchAxis);
            n.sections.push_back(tvsMoveNetwork::Section(p->SizeApr,p->SizeBpr,p->ThisRoundpr,p->Elev));
            n.sections.push_back(n.sections[0]);
            n.sections.push_back(tvsMoveNetwork::Section(p->SizeAotv,p->SizeBotv,p->ThisRoundotv,p->Elev));
            if(std::fabs(p->Vectpr.z)<1e-8&&std::fabs(p->Vectotv.z)<1e-8
                &&std::fabs(mainAxis.dot(branchAxis))<1e-6)n.kind=4;
        }
    }
    if(n.sections.empty())for(size_t i=0;i<n.ports.size();++i)
        n.sections.push_back(tvsMoveNetwork::Section(n.a,n.b,n.round,n.elevation));
    return true;
}
struct Contact {int node,port;Contact():node(-1),port(-1){}};
inline bool uniqueContact(const std::vector<Node>& nodes,int owner,int port,double tol,Contact& found){
    int count=0;
    for(size_t i=0;i<nodes.size();++i)if(static_cast<int>(i)!=owner)
        for(size_t j=0;j<nodes[i].ports.size();++j)
            if(tvsMoveGeometry::pointsNear(nodes[owner].ports[port],nodes[i].ports[j],tol)){
                ++count;found.node=static_cast<int>(i);found.port=static_cast<int>(j);
            }
    return count==1;
}
inline bool section(const Node& a,const Node& b,double tol){
    return a.round==b.round && a.a>0 && b.a>0 && (a.round || (a.b>0&&b.b>0))
        && std::fabs(a.a-b.a)<=tol && (a.round||std::fabs(a.b-b.b)<=tol)
        && std::fabs(a.elevation-b.elevation)<=tol;
}
inline bool snapshot(AcDbDatabase* db,std::vector<Node>& nodes,AcDbObjectIdArray& labels){
    AcDbObjectPointer<AcDbBlockTableRecord> space(db->currentSpaceId(),AcDb::kForRead);
    if(space.openStatus()!=Acad::eOk)return false;
    AcDbBlockTableRecordIterator* it=NULL;if(space->newIterator(it)!=Acad::eOk)return false;
    bool ok=true;
    for(;!it->done();it->step()){
        AcDbEntity* e=NULL;if(it->getEntity(e,AcDb::kForRead)!=Acad::eOk){ok=false;break;}
        Node n;if(describe(e,n))nodes.push_back(n);
        if(AcDbMLeader::cast(e))labels.append(e->objectId());
        e->close();
    }
    delete it;return ok;
}
inline bool editable(AcDbEntity* e){
    AcDbObjectPointer<AcDbLayerTableRecord> layer(e->layerId(),AcDb::kForRead);
    return layer.openStatus()==Acad::eOk&&!layer->isLocked();
}
class Transaction {
    AcTransactionManager* manager;
    bool active;
public:
    Transaction():manager(actrTransactionManager),active(false){if(manager)active=manager->startTransaction()!=NULL;}
    ~Transaction(){if(active)manager->abortTransaction();}
    bool good()const{return active;}
    AcDbEntity* write(const AcDbObjectId& id){
        AcDbObject* obj=NULL;
        if(!active||manager->getObject(obj,id,AcDb::kForWrite)!=Acad::eOk)return NULL;
        AcDbEntity* e=AcDbEntity::cast(obj);return e&&editable(e)?e:NULL;
    }
    bool commit(){if(!active)return false;if(manager->endTransaction()!=Acad::eOk)return false;active=false;return true;}
};
// A port touching the interior of one of the three pipes is a side branch.
inline bool sideBranches(const std::vector<Node>& nodes,const int pipes[3],double tol){
    for(int k=0;k<3;++k){const Node& p=nodes[pipes[k]];
        for(size_t i=0;i<nodes.size();++i)if(static_cast<int>(i)!=pipes[k])
            for(size_t j=0;j<nodes[i].ports.size();++j){const Point& q=nodes[i].ports[j];
                if(tvsMoveGeometry::onSegment(q,p.ports[0],p.ports[1],tol)
                    && !tvsMoveGeometry::pointsNear(q,p.ports[0],tol)&&!tvsMoveGeometry::pointsNear(q,p.ports[1],tol))return true;
            }
    }return false;
}
// Prepare associated TVS label text and arrow updates in the same transaction.
inline bool updateLabels(Transaction& tx,const AcDbObjectIdArray& labels,const AcDbObjectId ids[5],
                         const tvsMoveGeometry::Plan& plan,const Point fixed[2],const int neighborPorts[2]){
    MleaderController controller;
    for(int i=0;i<labels.length();++i){
        AcDbObjectId target;if(!controller.getObjectId(labels[i],target))continue;
        int which=-1;for(int j=0;j<5;++j)if(target==ids[j])which=j;
        if(which<0)continue;
        CString format,message;
        if(!controller.getFormat(labels[i],format)||!controller.getStringMessage(target,format,message))return false;
        AcDbMLeader* leader=AcDbMLeader::cast(tx.write(labels[i]));if(!leader)return false;
        AcDbMText* text=leader->mtext();if(!text)return false;
        text->setContents(message.GetString());
        Acad::ErrorStatus textStatus=leader->setMText(text);delete text;
        if(textStatus!=Acad::eOk)return false;
        AcArray<int> lines;if(leader->getLeaderLineIndexes(lines)!=Acad::eOk)return false;
        for(int j=0;j<lines.length();++j){
            AcGePoint3d old;if(leader->getFirstVertex(lines[j],old)!=Acad::eOk)return false;
            Point next=point(old);
            if(which==0){
                // Position along the selected segment, preserved when oblique elbows change its length.
                Point old0=plan.selected[0]-plan.delta[0],old1=plan.selected[1]-plan.delta[1],v=old1-old0;
                double fraction=(next-old0).dot(v)/v.dot(v);if(fraction<0)fraction=0;if(fraction>1)fraction=1;
                next=plan.selected[0]+(plan.selected[1]-plan.selected[0])*fraction;
            }else if(which==1||which==2)next=next+plan.delta[which-1];
            else{
                const int k=which-3;Point a=neighborPorts[k]==0?plan.attached[k]:fixed[k];
                Point b=neighborPorts[k]==0?fixed[k]:plan.attached[k],v=b-a;
                double fraction=(next-a).dot(v)/v.dot(v);if(fraction<0)fraction=0;if(fraction>1)fraction=1;
                next=a+v*fraction;
            }
            if(leader->setFirstVertex(lines[j],acad(next))!=Acad::eOk)return false;
        }
    }return true;
}
inline bool destinationPoint(const Node& main,const ads_point picked,Point& origin,ads_point destination){
    Point ends[2]={main.ports[0],main.ports[1]},cursor(picked[0],picked[1],picked[2]);
    tvsMoveGeometry::Reference reference=tvsMoveGeometry::PickedEdge;
    ACHAR keyword[132];
    acedInitGet(0,L"Izabrana Druga Osa");
    int status=acedGetKword(L"\nReferenca pomjeranja [Izabrana/Druga/Osa] <Izabrana>: ",keyword);
    if(status!=RTNORM && status!=RTNONE)return false;
    if(status==RTNORM){
        if(_wcsicmp(keyword,L"Druga")==0||_wcsicmp(keyword,L"D")==0)reference=tvsMoveGeometry::OtherEdge;
        else if(_wcsicmp(keyword,L"Osa")==0||_wcsicmp(keyword,L"O")==0)reference=tvsMoveGeometry::Axis;
    }

    if(!tvsMoveGeometry::referencePoint(ends,cursor,main.a,reference,origin)){
        acutPrintf(L"\nTVS_MOVE: nije moguće odrediti ivicu kanala.");return false;}
    ads_point base={origin.x,origin.y,origin.z};
    const ACHAR* prompt=reference==tvsMoveGeometry::Axis
        ?L"\nZadajte novu poziciju ose: "
        :reference==tvsMoveGeometry::OtherEdge
            ?L"\nZadajte novu poziciju druge ivice: "
            :L"\nZadajte novu poziciju izabrane ivice: ";
    acedInitGet(RSG_NONULL,NULL);
    if(tvsCoordinates::getPoint(base,prompt,destination)!=RTNORM)return false;
    return true;
}

inline bool sameSnapshot(const std::vector<Node>& before,const std::vector<Node>& after,double epsilon){
    if(before.size()!=after.size())return false;
    for(size_t i=0;i<before.size();++i){const Node& a=before[i];const Node& b=after[i];
        if(a.id!=b.id||a.kind!=b.kind||a.a!=b.a||a.b!=b.b||a.elevation!=b.elevation||a.round!=b.round
            ||a.ports.size()!=b.ports.size()||a.axes.size()!=b.axes.size()||a.sections.size()!=b.sections.size()
            ||!tvsMoveGeometry::pointsNear(a.middle,b.middle,epsilon))return false;
        for(size_t p=0;p<a.ports.size();++p)if(!tvsMoveGeometry::pointsNear(a.ports[p],b.ports[p],epsilon))return false;
        for(size_t p=0;p<a.axes.size();++p)if(!tvsMoveGeometry::pointsNear(a.axes[p],b.axes[p],epsilon))return false;
        for(size_t p=0;p<a.sections.size();++p){
            const tvsMoveNetwork::Section& x=a.sections[p];const tvsMoveNetwork::Section& y=b.sections[p];
            if(x.a!=y.a||x.b!=y.b||x.elevation!=y.elevation||x.round!=y.round)return false;
        }
    }return true;
}
inline bool simpleChain(const std::vector<Node>& nodes,int selected,double tol){
    Contact elbow[2],neighbor[2];int ids[5]={selected,-1,-1,-1,-1};
    for(int i=0;i<2;++i){
        if(!uniqueContact(nodes,selected,i,tol,elbow[i])||nodes[elbow[i].node].kind!=2)return false;
        if(!uniqueContact(nodes,elbow[i].node,1-elbow[i].port,tol,neighbor[i])||nodes[neighbor[i].node].kind!=1)return false;
        ids[i+1]=elbow[i].node;ids[i+3]=neighbor[i].node;
    }
    for(int i=0;i<5;++i)for(int j=0;j<i;++j)if(ids[i]==ids[j])return false;
    const int pipes[3]={selected,neighbor[0].node,neighbor[1].node};
    if(sideBranches(nodes,pipes,tol))return false;
    for(size_t i=0;i<nodes.size();++i)if(nodes[i].kind==4)
        for(int p=0;p<3;++p)if(tvsMoveNetwork::interior(nodes[i].middle,nodes[pipes[p]].ports[0],nodes[pipes[p]].ports[1],tol))return false;
    return true;
}
inline const ACHAR* networkError(tvsMoveNetwork::Error e){
    switch(e){
    case tvsMoveNetwork::Ambiguous:return L"Priključak ima više mogućih veza.";
    case tvsMoveNetwork::Unsupported:return L"Veza vodi do nepodržanog elementa ili izabrana dionica ima slobodan kraj.";
    case tvsMoveNetwork::SectionMismatch:return L"Presjeci ili elevacije na spoju ne odgovaraju.";
    case tvsMoveNetwork::NotTangent:return L"Ose nisu usklađene na priključku.";
    case tvsMoveNetwork::NonPlanar:return L"Mreža nije u zajedničkoj ravni osnove.";
    case tvsMoveNetwork::TooShort:return L"Pomjeranje bi previše skratilo ili obrnulo dionicu.";
    case tvsMoveNetwork::SideBranch:return L"Postoji priključak usred nepodijeljenog kanala.";
    case tvsMoveNetwork::NewContact:return L"Nova pozicija dodiruje drugi priključak.";
    case tvsMoveNetwork::Cycle:return L"Zatvorene petlje nisu obuhvaćene ovom varijantom.";
    case tvsMoveNetwork::TooMany:return L"Pomjeranje obuhvata više od 256 elemenata.";
    default:return L"Nije moguće izračunati povezano pomjeranje.";
    }
}
inline bool networkLabels(Transaction& tx,const std::vector<Node>& nodes,const AcDbObjectIdArray& labels,
                          const tvsMoveNetwork::Result& result,const Point& delta){
    MleaderController controller;
    for(int i=0;i<labels.length();++i){
        AcDbObjectId target;if(!controller.getObjectId(labels[i],target))continue;
        int index=-1;
        for(size_t j=0;j<nodes.size();++j)if(nodes[j].id==target&&result.masks[j]){index=static_cast<int>(j);break;}
        if(index<0)continue;
        CString format,text;
        if(!controller.getFormat(labels[i],format)||!controller.getStringMessage(target,format,text))return false;
        AcDbMLeader* leader=AcDbMLeader::cast(tx.write(labels[i]));if(!leader)return false;
        AcDbMText* content=leader->mtext();if(!content)return false;
        content->setContents(text.GetString());Acad::ErrorStatus status=leader->setMText(content);delete content;
        if(status!=Acad::eOk)return false;
        AcArray<int> lines;if(leader->getLeaderLineIndexes(lines)!=Acad::eOk)return false;
        for(int j=0;j<lines.length();++j){
            AcGePoint3d old;if(leader->getFirstVertex(lines[j],old)!=Acad::eOk)return false;
            Point updated=point(old)+delta;
            if(nodes[index].kind==1){
                Point v=nodes[index].ports[1]-nodes[index].ports[0];
                double fraction=(point(old)-nodes[index].ports[0]).dot(v)/v.dot(v);
                if(fraction<0)fraction=0;
                if(fraction>1)fraction=1;
                updated=result.ports[index][0]+(result.ports[index][1]-result.ports[index][0])*fraction;
            }
            if(leader->setFirstVertex(lines[j],acad(updated))!=Acad::eOk)return false;
        }
    }return true;
}
inline void executeNetwork(AcDbDatabase* db,const std::vector<Node>& nodes,
                           int selected,const ads_point picked,double factor,double tol,double minimum){
    Point origin;ads_point destination;
    if(!destinationPoint(nodes[selected],picked,origin,destination))return;
    Point axis;
    if(!tvsMoveGeometry::unit(nodes[selected].ports[1]-nodes[selected].ports[0],axis))return;
    Point normal(-axis.y,axis.x,0),target(destination[0],destination[1],destination[2]);
    Point delta=normal*((target-origin).dot(normal));
    if(!tvsMoveGeometry::valid(delta)||delta.length()<=tol){acutPrintf(L"\nTVS_MOVE: nema bočnog pomjeranja.");return;}
    std::vector<tvsMoveNetwork::Body> bodies;
    for(size_t i=0;i<nodes.size();++i){
        tvsMoveNetwork::Body b;b.ports=nodes[i].ports;b.axes=nodes[i].axes;b.sections=nodes[i].sections;
        b.kind=nodes[i].kind==1?tvsMoveNetwork::Pipe:nodes[i].kind>=2?tvsMoveNetwork::Rigid:tvsMoveNetwork::Blocked;
        if(nodes[i].kind==4)b.junctions.push_back(nodes[i].middle);
        bodies.push_back(b);
    }
    tvsMoveNetwork::Result result;
    if(!tvsMoveNetwork::plan(bodies,selected,delta,minimum,tol,result)){
        acutPrintf(L"\nTVS_MOVE: %s",networkError(result.error));return;}
    std::vector<Node> current;AcDbObjectIdArray currentLabels;
    if(!snapshot(db,current,currentLabels)||!sameSnapshot(nodes,current,1e-10/factor)){
        acutPrintf(L"\nTVS_MOVE: crtež se promijenio tokom unosa. Pokrenite komandu ponovo.");return;}
    Transaction tx;if(!tx.good())return;
    std::vector<AcDbEntity*> entities(nodes.size(),static_cast<AcDbEntity*>(NULL));
    for(size_t i=0;i<nodes.size();++i)if(result.masks[i]){
        entities[i]=tx.write(nodes[i].id);
        if(!entities[i]){acutPrintf(L"\nTVS_MOVE: povezani element je zaključan ili nedostupan. Izmjena je poništena.");return;}
    }
    AcGeMatrix3d translation;translation.setToTranslation(AcGeVector3d(delta.x,delta.y,delta.z));
    int stretched=0;
    for(size_t i=0;i<nodes.size();++i)if(entities[i]){
        if(nodes[i].kind==1){
            TVS_Pipe* pipe=TVS_Pipe::cast(entities[i]);
            if(!pipe||pipe->put_FirstPoint(acad(result.ports[i][0]))!=Acad::eOk
                ||pipe->put_Lastpoint(acad(result.ports[i][1]))!=Acad::eOk)return;
            if(result.masks[i]!=3)++stretched;
        }else if(entities[i]->transformBy(translation)!=Acad::eOk){
            acutPrintf(L"\nTVS_MOVE: fiting nije moguće pomjeriti. Izmjena je poništena.");return;}
        entities[i]->recordGraphicsModified();
    }
    if(!networkLabels(tx,nodes,currentLabels,result,delta)){
        acutPrintf(L"\nTVS_MOVE: povezanu oznaku nije moguće ažurirati. Izmjena je poništena.");return;}
    if(!tx.commit())return;
    acutPrintf(L"\nTVS_MOVE: povezana mreža — %d elemenata; prilagođeno dužina: %d. Pomjeraj: %.2f mm.",result.changed,stretched,delta.length()*factor);
}

inline void execute(){
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();
    if(!db)return;
    ads_name name;ads_point picked;
    if(tvsCoordinates::selectEntity(L"\nIzaberite pravu dionicu (kliknite bliže željenoj ivici): ",name,picked)!=RTNORM)return;
    AcDbObjectId selected;if(acdbGetObjectId(selected,name)!=Acad::eOk)return;
    std::vector<Node> nodes;AcDbObjectIdArray labels;
    if(!snapshot(db,nodes,labels)){acutPrintf(L"\nTVS_MOVE: nije moguće pročitati elemente trenutnog prostora.");return;}
    int center=-1;for(size_t i=0;i<nodes.size();++i)if(nodes[i].id==selected)center=static_cast<int>(i);
    if(center<0||nodes[center].kind!=1){acutPrintf(L"\nTVS_MOVE: izaberite kruti TVS kanal u trenutnom prostoru.");return;}
    const double factor=tvsUnits::factor(db),tol=0.1/factor,minimum=1.0/factor;
    if(!simpleChain(nodes,center,tol)){
        executeNetwork(db,nodes,center,picked,factor,tol,minimum);return;
    }
    const Node& main=nodes[center];Contact elbows[2],neighbors[2];int neighborPorts[2];
    Point attached[2],fixed[2];
    for(int i=0;i<2;++i){
        if(!uniqueContact(nodes,center,i,tol,elbows[i])||nodes[elbows[i].node].kind!=2){
            acutPrintf(L"\nTVS_MOVE: svaki kraj mora imati tačno jedno podržano koljeno u osnovi.");return;}
        const Node& elbow=nodes[elbows[i].node];
        if(!uniqueContact(nodes,elbows[i].node,1-elbows[i].port,tol,neighbors[i])||nodes[neighbors[i].node].kind!=1){
            acutPrintf(L"\nTVS_MOVE: iza svakog koljena mora biti jedna prava kruta dionica.");return;}
        const Node& neighbor=nodes[neighbors[i].node];neighborPorts[i]=neighbors[i].port;
        attached[i]=neighbor.ports[neighborPorts[i]];fixed[i]=neighbor.ports[1-neighborPorts[i]];
        if(!section(main,elbow,tol)||!section(main,neighbor,tol)){
            acutPrintf(L"\nTVS_MOVE: dimenzije, oblik presjeka i elevacija moraju odgovarati na spojevima.");return;}
        Point dir,along;
        if(!tvsMoveGeometry::unit(main.ports[1]-main.ports[0],dir)
            ||!tvsMoveGeometry::unit(main.ports[i]-elbow.middle,along)||std::fabs(dir.dot(along))<1-1e-6
            ||!tvsMoveGeometry::unit(attached[i]-fixed[i],dir)
            ||!tvsMoveGeometry::unit(attached[i]-elbow.middle,along)||std::fabs(dir.dot(along))<1-1e-6){
            acutPrintf(L"\nTVS_MOVE: ose kanala nisu tangentne na koljena.");return;}
    }
    int indexes[5]={center,elbows[0].node,elbows[1].node,neighbors[0].node,neighbors[1].node};
    for(int i=0;i<5;++i)for(int j=0;j<i;++j)if(indexes[i]==indexes[j]){
        acutPrintf(L"\nTVS_MOVE: zatvorene petlje nisu podržane u ovom koraku.");return;}
    const int pipes[3]={center,neighbors[0].node,neighbors[1].node};
    if(sideBranches(nodes,pipes,tol)){acutPrintf(L"\nTVS_MOVE: postoji bočni priključak; ovaj korak podržava samo niz od pet elemenata.");return;}
    Point ends[2]={main.ports[0],main.ports[1]},origin;ads_point destination;
    if(!destinationPoint(main,picked,origin,destination))return;
    tvsMoveGeometry::Plan plan;
    if(!tvsMoveGeometry::plan(ends,attached,fixed,origin,Point(destination[0],destination[1],destination[2]),minimum,tol,plan)){
        acutPrintf(L"\nTVS_MOVE: nema bočnog pomjeranja ili bi nastala prekratka/obrnuta dionica ili nepodržana 3D geometrija.");return;}
    // Prevent an unconnected external port at the new moving joints or pipe interior.
    for(size_t i=0;i<nodes.size();++i){bool own=false;for(int k=0;k<5;++k)if(static_cast<int>(i)==indexes[k])own=true;
        if(own)continue;
        for(size_t j=0;j<nodes[i].ports.size();++j){const Point& q=nodes[i].ports[j];
            bool conflict=tvsMoveGeometry::onSegment(q,plan.selected[0],plan.selected[1],tol);
            for(int k=0;k<2;++k)conflict=conflict||tvsMoveGeometry::pointsNear(q,plan.attached[k],tol)
                ||(tvsMoveGeometry::onSegment(q,fixed[k],plan.attached[k],tol)&&!tvsMoveGeometry::pointsNear(q,fixed[k],tol));
            if(conflict){acutPrintf(L"\nTVS_MOVE: nova pozicija dodiruje drugi priključak; pomjeranje je odbijeno.");return;}
        }
    }
    std::vector<Node> current;AcDbObjectIdArray currentLabels;
    if(!snapshot(db,current,currentLabels)||current.size()!=nodes.size()){
        acutPrintf(L"\nTVS_MOVE: crtež se promijenio tokom unosa. Pokrenite komandu ponovo.");return;}
    if(!sameSnapshot(nodes,current,1e-10/factor)){
        acutPrintf(L"\nTVS_MOVE: geometrija se promijenila tokom unosa. Pokrenite komandu ponovo.");return;}
    labels=currentLabels;
    Transaction tx;if(!tx.good()){acutPrintf(L"\nTVS_MOVE: nije moguće započeti izmjenu.");return;}
    AcDbEntity* objects[5];AcDbObjectId ids[5];
    for(int i=0;i<5;++i){ids[i]=nodes[indexes[i]].id;objects[i]=tx.write(ids[i]);if(!objects[i]){
        acutPrintf(L"\nTVS_MOVE: element je zaključan ili nije dostupan. Ništa nije promijenjeno.");return;}}
    TVS_Pipe* moved=TVS_Pipe::cast(objects[0]);
    if(!moved||moved->put_FirstPoint(acad(plan.selected[0]))!=Acad::eOk||moved->put_Lastpoint(acad(plan.selected[1]))!=Acad::eOk)return;
    for(int i=0;i<2;++i){
        TVS_TAP* elbow=TVS_TAP::cast(objects[i+1]);TVS_Pipe* neighbor=TVS_Pipe::cast(objects[i+3]);
        if(!elbow||!neighbor||elbow->put_Centerpoint(acad(nodes[elbows[i].node].middle+plan.delta[i]))!=Acad::eOk)return;
        Acad::ErrorStatus status=neighborPorts[i]==0?neighbor->put_FirstPoint(acad(plan.attached[i])):neighbor->put_Lastpoint(acad(plan.attached[i]));
        if(status!=Acad::eOk)return;
    }
    for(int i=0;i<5;++i)objects[i]->recordGraphicsModified();
    if(!updateLabels(tx,labels,ids,plan,fixed,neighborPorts)){
        acutPrintf(L"\nTVS_MOVE: povezanu oznaku nije moguće ažurirati. Cijela izmjena je poništena.");return;}
    if(!tx.commit()){acutPrintf(L"\nTVS_MOVE: izmjena nije sačuvana.");return;}
    acutPrintf(L"\nTVS_MOVE: pomjerena dionica i dva koljena; susjedne dužine prilagođene. Bočni pomjeraj: %.2f mm.",plan.offset*factor);
}
inline void executeStretch(){
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();if(!db)return;
    const double factor=tvsUnits::factor(db),tol=0.1/factor,minimum=1.0/factor;
    ads_point corner1,corner2;
    if(acedGetPoint(NULL,L"\nPrvi ugao crossing okvira (presijecite prave dionice): ",corner1)!=RTNORM)return;
    if(acedGetCorner(corner1,L"\nSuprotni ugao okvira: ",corner2)!=RTNORM)return;
    std::vector<Node> nodes;AcDbObjectIdArray labels;
    if(!snapshot(db,nodes,labels))return;
    std::vector<tvsMoveNetwork::Body> bodies;std::vector<int> masks;
    for(size_t i=0;i<nodes.size();++i){
        tvsMoveNetwork::Body b;b.ports=nodes[i].ports;b.axes=nodes[i].axes;b.sections=nodes[i].sections;
        b.kind=nodes[i].kind==1?tvsMoveNetwork::Pipe:nodes[i].kind>=2?tvsMoveNetwork::Rigid:tvsMoveNetwork::Blocked;
        if(nodes[i].kind==4)b.junctions.push_back(nodes[i].middle);
        bodies.push_back(b);int mask=0;
        for(size_t j=0;j<b.ports.size();++j){
            ads_point w={b.ports[j].x,b.ports[j].y,b.ports[j].z},u;
            if(tvsCoordinates::transformPoint(w,0,1,u)!=RTNORM)return;
            if(u[0]>=(corner1[0]<corner2[0]?corner1[0]:corner2[0])-tol&&u[0]<=(corner1[0]>corner2[0]?corner1[0]:corner2[0])+tol
                &&u[1]>=(corner1[1]<corner2[1]?corner1[1]:corner2[1])-tol&&u[1]<=(corner1[1]>corner2[1]?corner1[1]:corner2[1])+tol)
                mask|=1<<static_cast<int>(j);
        }
        if(mask&&b.kind!=tvsMoveNetwork::Pipe&&mask!=(1<<static_cast<int>(b.ports.size()))-1){
            acutPrintf(L"\nTVS_STRETCH: okvir presijeca fiting. Obuhvatite sve njegove priključke ili ga izostavite.");return;}
        masks.push_back(mask);
    }
    bool any=false;for(size_t i=0;i<masks.size();++i)if(masks[i])any=true;
    if(!any){acutPrintf(L"\nTVS_STRETCH: okvir ne obuhvata nijedan TVS priključak.");return;}
    ads_point base,target;
    if(tvsCoordinates::getPoint(NULL,L"\nBazna tačka pomjeranja: ",base)!=RTNORM)return;
    if(tvsCoordinates::getPoint(base,L"\nNova pozicija: ",target)!=RTNORM)return;
    Point delta(target[0]-base[0],target[1]-base[1],target[2]-base[2]);
    tvsMoveNetwork::Result result;
    if(!tvsMoveNetwork::planSelection(bodies,masks,delta,minimum,tol,result)){
        acutPrintf(L"\nTVS_STRETCH: %s",networkError(result.error));return;}
    std::vector<Node> current;AcDbObjectIdArray currentLabels;
    if(!snapshot(db,current,currentLabels)||!sameSnapshot(nodes,current,1e-10/factor)){
        acutPrintf(L"\nTVS_STRETCH: crtež se promijenio tokom unosa. Pokrenite komandu ponovo.");return;}
    Transaction tx;if(!tx.good())return;
    std::vector<AcDbEntity*> entities(nodes.size(),static_cast<AcDbEntity*>(NULL));
    for(size_t i=0;i<nodes.size();++i)if(result.masks[i]){
        entities[i]=tx.write(nodes[i].id);
        if(!entities[i]){acutPrintf(L"\nTVS_STRETCH: povezani element je zaključan ili nedostupan. Izmjena je poništena.");return;}
    }
    AcGeMatrix3d translation;translation.setToTranslation(AcGeVector3d(delta.x,delta.y,delta.z));
    int stretched=0;
    for(size_t i=0;i<nodes.size();++i)if(entities[i]){
        if(nodes[i].kind==1){
            TVS_Pipe* pipe=TVS_Pipe::cast(entities[i]);
            if(!pipe||pipe->put_FirstPoint(acad(result.ports[i][0]))!=Acad::eOk
                ||pipe->put_Lastpoint(acad(result.ports[i][1]))!=Acad::eOk)return;
            if(result.masks[i]!=3)++stretched;
        }else if(entities[i]->transformBy(translation)!=Acad::eOk){
            acutPrintf(L"\nTVS_STRETCH: fiting nije moguće pomjeriti. Izmjena je poništena.");return;}
        entities[i]->recordGraphicsModified();
    }
    if(!networkLabels(tx,nodes,currentLabels,result,delta)){
        acutPrintf(L"\nTVS_STRETCH: povezanu oznaku nije moguće ažurirati. Izmjena je poništena.");return;}
    if(!tx.commit())return;
    acutPrintf(L"\nTVS_STRETCH: povezana mreža — %d elemenata; prilagođeno dužina: %d. Pomjeraj: %.2f mm.",result.changed,stretched,delta.length()*factor);
}
}
