#pragma once
#ifdef TVS_BYPASS_PREVIEW
// Active TVS rectangular elbow radius mode (RadiusTypeRect_*), read by the caller in
// acrxEntryPoint.cpp where the live Settings.h globals belong.
struct TVSBypassRadius {int mode;double k,constant;TVSBypassRadius():mode(0),k(0),constant(0){}};
void tvsBypassPreviewCommand(const TVSBypassRadius& radius);
void tvsBypassDetailCheckCommand();
void tvsBypassDetailTestCommand();
void tvsBypassCommand(const TVSBypassRadius& radius);
class TVS_Pipe;
// id is authoritative for the continuation; pipe is the legacy closed-object pointer
// the drawing loop still carries (same pattern as TVS_Pipe::add_new).
struct TVSBypassOutput {AcDbObjectId id;TVS_Pipe* pipe;double elevation,angle;TVSBypassOutput():pipe(NULL),elevation(0),angle(0){}};
bool tvsBypassDuringDrawing(const AcDbObjectId& route,TVSBypassOutput& result,const TVSBypassRadius& radius);
#endif
