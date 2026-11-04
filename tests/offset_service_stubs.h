// Test-only SDK substitutes. They do not prove ObjectARX filing or runtime.
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include "../geometry/OffsetOperation.h"
#include "../TVS_Ventilation_ARX/MoveNetworkGeometry.h"
#include "../geometry/RoundElbow.h"
struct AcDbDatabase {};
static AcDbDatabase database;
namespace Acad {enum ErrorStatus {eOk,eInvalid};}
namespace AcDb {enum OpenMode {kForRead,kForWrite};}
struct AcGeVector3d {double x,y,z;AcGeVector3d(double a=0,double b=0,double c=0):x(a),y(b),z(c){}
 double length()const{return std::sqrt(x*x+y*y+z*z);}double dotProduct(const AcGeVector3d& b)const{return x*b.x+y*b.y+z*b.z;}
 AcGeVector3d operator/(double v)const{return AcGeVector3d(x/v,y/v,z/v);}AcGeVector3d operator*(double v)const{return AcGeVector3d(x*v,y*v,z*v);}};
struct AcGePoint3d {double x,y,z;AcGePoint3d(double a=0,double b=0,double c=0):x(a),y(b),z(c){}
 double distanceTo(const AcGePoint3d& b)const{return std::sqrt((x-b.x)*(x-b.x)+(y-b.y)*(y-b.y)+(z-b.z)*(z-b.z));}
 AcGeVector3d operator-(const AcGePoint3d& b)const{return AcGeVector3d(x-b.x,y-b.y,z-b.z);}
 AcGePoint3d operator+(const AcGeVector3d& b)const{return AcGePoint3d(x+b.x,y+b.y,z+b.z);}};
struct AcDbObjectId {int id;static const AcDbObjectId kNull;AcDbObjectId(int i=0):id(i){}bool isNull()const{return !id;}
 bool operator==(const AcDbObjectId& b)const{return id==b.id;}bool operator!=(const AcDbObjectId& b)const{return id!=b.id;}
 unsigned long asOldId()const{return static_cast<unsigned long>(id);} };
const AcDbObjectId AcDbObjectId::kNull;
static const int conPipeStart=1,conPipeEnd=2,DuctTypeStill=0,TypeRoundTap_TapSection=1;
static const int RadiusTypeRect_RadiusConstant=1,RadiusTypeRound_RadiusConstant=2,Form_Up=2,Form_Down=3;
struct TVS_Connector {int cTypeCurrent,cTypeAnother;AcDbObjectId connectionID;AcGePoint3d point;
 TVS_Connector():cTypeCurrent(0),cTypeAnother(0){}};
struct AcDbObject {AcDbObjectId id;virtual ~AcDbObject(){}AcDbObjectId objectId()const{return id;}};
struct TVS_Entity:AcDbObject {std::vector<TVS_Connector> connectors;double SizeA,SizeB,Elev,Flow,WipeoutLength;bool ThisRound;int DuctType;
 TVS_Entity():SizeA(400),SizeB(200),Elev(-400),Flow(12),WipeoutLength(50),ThisRound(false),DuctType(0){}
 void setDatabaseDefaults(){}void assertWriteEnabled(){}void recordGraphicsModified(){}
 Acad::ErrorStatus put_Elevation(double v){Elev=v;return Acad::eOk;}
 static TVS_Entity* cast(AcDbObject* o){return dynamic_cast<TVS_Entity*>(o);} };
struct TVS_Pipe:TVS_Entity {AcGePoint3d FirstPoint,LastPoint;bool locked;
 TVS_Pipe():FirstPoint(-2000,0,37),LastPoint(0,0,37),locked(false){}
 static TVS_Pipe* cast(AcDbObject* o){return dynamic_cast<TVS_Pipe*>(o);} };
struct TVS_TAP:TVS_Entity {int TypeRoundTap,RadiusTypeRect,RadiusTypeRound;double RadiusVariableParameter,RadiusConst,Swectangle;AcGeVector3d Startvector,Normvector;AcGePoint3d MiddlePoint,CenterPoint;
 Acad::ErrorStatus put_Form(int){return Acad::eOk;} };
static std::map<int,AcDbObject*> objects;
static TVS_Pipe fixtureSource;
template<class T>struct AcDbObjectPointer {T* p;AcDbObjectPointer():p(NULL){}
 AcDbObjectPointer(AcDbObjectId id,AcDb::OpenMode):p(objects.count(id.id)?dynamic_cast<T*>(objects[id.id]):NULL){}
 T* operator->(){return p;}T* object(){return p;}void acquire(T* value){p=value;}
 Acad::ErrorStatus openStatus(){return p?Acad::eOk:Acad::eInvalid;} };
struct AcDbBlockTableRecordIterator {std::vector<int> ids;size_t pos;AcDbBlockTableRecordIterator():pos(0){
 for(auto& o:objects)if(dynamic_cast<TVS_Entity*>(o.second))ids.push_back(o.first);}
 bool done(){return pos==ids.size();}void step(){++pos;}
 Acad::ErrorStatus getEntity(AcDbObject*& p,AcDb::OpenMode){p=objects[ids[pos]];return Acad::eOk;} };
typedef AcDbObject AcDbEntity;
struct AcDbBlockTableRecord:AcDbObject {Acad::ErrorStatus newIterator(AcDbBlockTableRecordIterator*& p){p=new AcDbBlockTableRecordIterator;return Acad::eOk;}};
static AcDbBlockTableRecord block;
static int failAppend=0,appendCalls=0,failRecord=0,recordCalls=0,commits=0,aborts=0,txStarts=0;
static bool failCommit=false;
static void (*rollbackHook)()=NULL;
struct Manager {Acad::ErrorStatus getObject(AcDbObject*& out,AcDbObjectId id,AcDb::OpenMode){out=objects[id.id];return out?Acad::eOk:Acad::eInvalid;}};
static Manager manager;
static Manager* actrTransactionManager=&manager;
struct CString {std::wstring text;bool IsEmpty()const{return text.empty();}};
namespace tvsOffsetData {
struct Record {CString guid;int role;AcDbObjectId members[3];double rise;};
static std::map<int,Record> records;
struct WriteScope {WriteScope(){}~WriteScope(){}};
inline CString newGuid(){static int n=0;CString s;s.text=L"new-guid-"+std::to_wstring(++n);return s;}
inline Acad::ErrorStatus write(AcDbObject* o,const Record& r){if(++recordCalls==failRecord)return Acad::eInvalid;records[o->id.id]=r;return Acad::eOk;}
}
namespace tvsOffsetProof {
inline AcGePoint3d pt(const tvsOffset::WorldPoint& p){return AcGePoint3d(p.x,p.y,p.z);}
struct BuildTransaction {
 AcDbBlockTableRecord* space;bool committed;Acad::ErrorStatus status;AcGePoint3d beforeFirst,beforeLast;std::vector<int> created;
 BuildTransaction(AcDbDatabase*):space(&block),committed(false),status(Acad::eOk),beforeFirst(fixtureSource.FirstPoint),beforeLast(fixtureSource.LastPoint){++txStarts;}
 ~BuildTransaction(){if(!committed){++aborts;fixtureSource.FirstPoint=beforeFirst;fixtureSource.LastPoint=beforeLast;
 for(int id:created){delete objects[id];objects.erase(id);tvsOffsetData::records.erase(id);}if(rollbackHook)rollbackHook();}}
 bool append(TVS_Entity* p,AcDbObjectId& id){if(++appendCalls==failAppend){delete p;status=Acad::eInvalid;return false;}id=100+appendCalls;p->id=id;objects[id.id]=p;created.push_back(id.id);return true;}
 bool commit(){if(failCommit){status=Acad::eInvalid;return false;}committed=true;++commits;return true;}
};}
namespace tvsMove {
inline bool editable(TVS_Entity* e){TVS_Pipe* p=TVS_Pipe::cast(e);return p&&!p->locked;}
struct Node {int kind;std::vector<tvsMoveGeometry::Point> ports,axes;std::vector<tvsMoveNetwork::Section> sections;Node():kind(1){}};
inline tvsMoveGeometry::Point point(const AcGePoint3d& p){return tvsMoveGeometry::Point(p.x,p.y,p.z);}
}
static std::map<int,tvsMove::Node> nodes;
inline void acutPrintf(const wchar_t*,...){}
inline const wchar_t* acadErrorStatusText(Acad::ErrorStatus s){return s==Acad::eOk?L"eOk":L"eInvalid";}
namespace tvsOffsetService {
enum Outcome {Applied,NoChange,Cancelled,Failed};
struct Snapshot {AcDbDatabase* db;AcDbObjectId source,owner;tvsOffsetOperation::Snapshot physical;std::wstring fingerprint,network,manufacturer,note;bool round;int typeRoundTap;Snapshot():db(&database),source(1),owner(99),round(false),typeRoundTap(1){}};
struct Commit {AcDbObjectId output,members[3];TVS_Pipe* residentOutput;double elevation,angle,before;Commit():residentOutput(NULL),elevation(0),angle(0),before(0){}};
static Snapshot live;
static bool badCapture=false;
inline bool& failNext(){static bool v=false;return v;}
inline bool reject(const wchar_t*){return false;}
inline bool checked(Acad::ErrorStatus e){return e==Acad::eOk;}
inline bool describe(TVS_Entity* e,tvsMove::Node& n){if(!nodes.count(e->id.id))return false;n=nodes[e->id.id];return true;}
inline void putId(std::wostringstream& s,AcDbObjectId id){s<<id.id;}
inline void putPoint(std::wostringstream& s,const AcGePoint3d& p){s<<p.x<<L','<<p.y<<L','<<p.z<<L';';}
inline bool capture(AcDbObjectId,tvsOffsetOperation::End,Snapshot& s,bool=false){s=live;return !badCapture&&!::fixtureSource.locked;}
inline bool pipeState(TVS_Pipe* p,Snapshot& s,bool=false){s.fingerprint=live.fingerprint;s.round=p&&p->ThisRound;return !::fixtureSource.locked;}
inline void initialize(TVS_Entity*){}
inline void initializePipe(TVS_Pipe* p){p->connectors.resize(2);p->connectors[0].cTypeCurrent=1;p->connectors[1].cTypeCurrent=2;}
inline bool appearance(TVS_Pipe* from,TVS_Entity* to,const Snapshot&){to->Flow=from->Flow;return true;}
}
