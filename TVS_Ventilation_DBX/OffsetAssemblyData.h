#pragma once
#ifdef TVS_OFFSET_INTEGRATION
#include "dbmain.h"
#include "dbidmap.h"
namespace tvsOffsetData {
enum Role { FirstElbow=0,SlopedStraight=1,SecondElbow=2 };
enum Status { Legacy,Valid,Incomplete,Corrupt,UnsupportedVersion,GeometryMismatch };
struct Record {int version,role;CString guid;AcDbObjectId members[3];double rise;
    Record():version(1),role(-1),rise(0){} };
DLLIMPEXP Status read(const AcDbObject* object,Record& record);
DLLIMPEXP Acad::ErrorStatus write(AcDbObject* object,const Record& record);
DLLIMPEXP CString newGuid();
DLLIMPEXP Status validate(const AcDbObject* object,bool geometry=true);
DLLIMPEXP bool physicalLength(const AcDbObject* object,double projected,double& physical);
DLLIMPEXP bool marked(const AcDbObject* object);
DLLIMPEXP bool editable(const AcDbObject* object);
DLLIMPEXP void permitClone(const AcDbObjectId& id);
DLLIMPEXP void clearClonePermits();
DLLIMPEXP bool allowTransform(const AcDbObject* object,const AcGeMatrix3d& matrix);
DLLIMPEXP Acad::ErrorStatus remapClones(AcDbIdMapping& map);
DLLIMPEXP const ACHAR* statusText(Status status);
// Internal construction scope is process-local, never serialized.
class DLLIMPEXP WriteScope {public:WriteScope();~WriteScope();private:WriteScope(const WriteScope&);WriteScope& operator=(const WriteScope&);};
}
#endif
