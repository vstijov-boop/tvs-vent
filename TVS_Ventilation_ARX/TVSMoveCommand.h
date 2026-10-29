#pragma once
#include "CoordinateInput.h"
#include "MoveGeometry.h"
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
    int kind; // 1 rigid pipe, 2 supported elbow, 0 unsupported connector owner
    std::vector<Point> ports;
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
        }else{
            // Read-only fallback only blocks ambiguous contacts; never move these fittings.
            n.ports.push_back(point(p->MA));n.ports.push_back(point(p->MC));
        }
    }else if(TVS_TRANS* p=TVS_TRANS::cast(entity)){
        Point last=point(p->FirstPoint)+Point(p->VectTr.x,p->VectTr.y,0),side;
        if(tvsMoveGeometry::unit(Point(p->VectTr.y,-p->VectTr.x,p->VectTr.z),side)){
            double shift=0;
            if(p->TransType==2)shift=(p->SizeAp1-p->SizeAp2)/2;
            if(p->TransType==3)shift=(p->SizeAp2-p->SizeAp1)/2;
            last=last+Point(side.x,side.y,0)*shift;
        }
        n.ports.push_back(point(p->FirstPoint));n.ports.push_back(last);
    }else if(TVS_WYE* p=TVS_WYE::cast(entity)){
        // Reconstruct the three WYE ports without depending on a previous redraw.
        Point mainAxis,branchAxis;
        if(tvsMoveGeometry::unit(Point(p->Vectpr.x,p->Vectpr.y,0),mainAxis)
            &&tvsMoveGeometry::unit(Point(p->Vectotv.x,p->Vectotv.y,0),branchAxis)){
            Point base=point(p->Basepoint);double half=p->LengthPl+p->SizeAotv/2;
            n.ports.push_back(base-mainAxis*half);n.ports.push_back(base+mainAxis*half);
            n.ports.push_back(base+branchAxis*(p->SizeApr/2+p->LengthPl));
            n.ports.push_back(base); // Block a side takeoff at its actual junction as well.
        }
    }
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
inline void execute(){
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();
    if(!db)return;
    ads_name name;ads_point picked;
    if(tvsCoordinates::selectEntity(L"\nIzaberite pravu dionicu između dva koljena: ",name,picked)!=RTNORM)return;
    AcDbObjectId selected;if(acdbGetObjectId(selected,name)!=Acad::eOk)return;
    std::vector<Node> nodes;AcDbObjectIdArray labels;
    if(!snapshot(db,nodes,labels)){acutPrintf(L"\nTVS_MOVE: nije moguće pročitati elemente trenutnog prostora.");return;}
    int center=-1;for(size_t i=0;i<nodes.size();++i)if(nodes[i].id==selected)center=static_cast<int>(i);
    if(center<0||nodes[center].kind!=1){acutPrintf(L"\nTVS_MOVE: izaberite kruti TVS kanal u trenutnom prostoru.");return;}
    const double factor=tvsUnits::factor(db),tol=0.1/factor,minimum=1.0/factor;
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
    Point ends[2]={main.ports[0],main.ports[1]},cursor(picked[0],picked[1],picked[2]);
    tvsMoveGeometry::Reference reference=tvsMoveGeometry::PickedEdge;
    ACHAR keyword[132];
    acedInitGet(0,L"Izabrana Druga Osa");
    int status=acedGetKword(L"\nReferenca pomjeranja [Izabrana/Druga/Osa] <Izabrana>: ",keyword);
    if(status!=RTNORM && status!=RTNONE)return;
    if(status==RTNORM){
        if(_wcsicmp(keyword,L"Druga")==0||_wcsicmp(keyword,L"D")==0)reference=tvsMoveGeometry::OtherEdge;
        else if(_wcsicmp(keyword,L"Osa")==0||_wcsicmp(keyword,L"O")==0)reference=tvsMoveGeometry::Axis;
    }
    Point origin;ads_point destination;
    if(!tvsMoveGeometry::referencePoint(ends,cursor,main.a,reference,origin)){
        acutPrintf(L"\nTVS_MOVE: nije moguće odrediti ivicu kanala.");return;}
    ads_point base={origin.x,origin.y,origin.z};
    const ACHAR* prompt=reference==tvsMoveGeometry::Axis
        ?L"\nZadajte novu poziciju ose: "
        :reference==tvsMoveGeometry::OtherEdge
            ?L"\nZadajte novu poziciju druge ivice: "
            :L"\nZadajte novu poziciju izabrane ivice: ";
    acedInitGet(RSG_NONULL,NULL);
    if(tvsCoordinates::getPoint(base,prompt,destination)!=RTNORM)return;
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
    for(size_t i=0;i<nodes.size();++i){
        const Node& a=nodes[i];const Node& b=current[i];
        bool same=a.id==b.id&&a.kind==b.kind&&a.ports.size()==b.ports.size()
            &&a.a==b.a&&a.b==b.b&&a.round==b.round&&a.elevation==b.elevation
            &&tvsMoveGeometry::pointsNear(a.middle,b.middle,1e-10/factor);
        for(size_t j=0;same&&j<a.ports.size();++j)same=tvsMoveGeometry::pointsNear(a.ports[j],b.ports[j],1e-10/factor);
        if(!same){acutPrintf(L"\nTVS_MOVE: geometrija se promijenila tokom unosa. Pokrenite komandu ponovo.");return;}
    }
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
}
