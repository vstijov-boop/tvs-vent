#pragma once
#ifdef TVS_OFFSET_INTEGRATION
#include <map>
#include <vector>
namespace tvsOffsetSession {
struct Event {AcDbObjectId source,output;double before;};
inline std::map<AcDbDatabase*,std::vector<Event> >& history(){static std::map<AcDbDatabase*,std::vector<Event> > data;return data;}
inline void applied(AcDbDatabase* db,const AcDbObjectId& source,const AcDbObjectId& output,double before){Event e;e.source=source;e.output=output;e.before=before;history()[db].push_back(e);}
inline void afterUndoRedo(const ACHAR* command){
    if(_wcsicmp(command,L"UNDO")&&_wcsicmp(command,L"U")&&_wcsicmp(command,L"REDO")&&_wcsicmp(command,L"MREDO"))return;
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();
    std::map<AcDbDatabase*,std::vector<Event> >::iterator found=history().find(db);if(found==history().end()||found->second.empty())return;
    for(std::vector<Event>::reverse_iterator it=found->second.rbegin();it!=found->second.rend();++it){
        AcDbObjectPointer<TVS_Pipe> output(it->output,AcDb::kForRead);
        if(output.openStatus()==Acad::eOk){globalElevMid=output->get_Elevation();return;}}
    AcDbObjectPointer<TVS_Pipe> source(found->second.front().source,AcDb::kForRead);
    globalElevMid=source.openStatus()==Acad::eOk?source->get_Elevation():found->second.front().before;
}
}
#endif
