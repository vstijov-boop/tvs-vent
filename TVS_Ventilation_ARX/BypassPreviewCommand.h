#pragma once
#ifdef TVS_BYPASS_PREVIEW
void tvsBypassPreviewCommand();
void tvsBypassDetailCheckCommand();
void tvsBypassDetailTestCommand();
void tvsBypassCommand();
class TVS_Pipe;
struct TVSBypassOutput {TVS_Pipe* pipe;double elevation,angle;TVSBypassOutput():pipe(NULL),elevation(0),angle(0){}};
bool tvsBypassDuringDrawing(const AcDbObjectId& route,TVSBypassOutput& result);
#endif
