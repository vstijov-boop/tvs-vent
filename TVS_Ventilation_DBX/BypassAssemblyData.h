#pragma once
#ifdef TVS_OFFSET_INTEGRATION
#include "OffsetAssemblyData.h"
namespace tvsBypassData {
struct Record {
    CString guid;int revision;double factor;bool reverse;
    AcDbObjectId members[7],inlet,outlet,obstacle,neighbors[2];int ports[2];
    double policy[4],obstacleSnapshot[8];
    Record():revision(1),factor(0),reverse(false){ports[0]=ports[1]=0;for(int j=0;j<4;++j)policy[j]=0;for(int j=0;j<8;++j)obstacleSnapshot[j]=0;}
};
DLLIMPEXP bool marked(const AcDbObject* object);
DLLIMPEXP tvsOffsetData::Status validate(const AcDbObject* object,bool geometry=true);
DLLIMPEXP Acad::ErrorStatus write(AcDbDatabase* db,Record& record,AcDbObject* members[7],AcDbObjectId& parent);
}
#endif
