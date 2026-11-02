#pragma once
#ifdef TVS_BYPASS_PREVIEW
void tvsBypassPreviewCommand();
void tvsBypassDetailCheckCommand();
void tvsBypassDetailTestCommand();
void tvsBypassCommand();
class TVS_Pipe;
// id is authoritative for the continuation; pipe is the legacy closed-object pointer
// the drawing loop still carries (same pattern as TVS_Pipe::add_new).
struct TVSBypassOutput {AcDbObjectId id;TVS_Pipe* pipe;double elevation,angle;TVSBypassOutput():pipe(NULL),elevation(0),angle(0){}};
bool tvsBypassDuringDrawing(const AcDbObjectId& route,TVSBypassOutput& result);
#endif
