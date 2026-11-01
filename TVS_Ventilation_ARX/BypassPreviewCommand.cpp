#include "StdAfx.h"
#ifdef TVS_BYPASS_PREVIEW
#include "BypassPreviewCommand.h"
#include "../geometry/bypass/PreviewPresentation.h"
#include "TVS_Pipe.h"
#include "DrawingUnits.h"
#include "acgiTransient.h"
#include "dbpl.h"
#include "dbobjptr.h"
#include <memory>
#include <sstream>
#include <cstdlib>
#ifdef TVS_OFFSET_INTEGRATION
#include "OffsetAssemblyData.h"
#endif
namespace {
using namespace tvsBypassPreview;
static std::wstring wide(const std::string& s){
 int n=MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,NULL,0);if(!n)return L"";
 std::vector<wchar_t> out(n);MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,&out[0],n);return &out[0];
}
static std::wstring number(double x){wchar_t buf[80];swprintf_s(buf,L"%.9g",x);return buf;}
static bool readPipe(const ACHAR* prompt,Segment& out,AcDbObjectId& id){
 ads_name name;ads_point pick;if(acedEntSel(prompt,name,pick)!=RTNORM)return false;
 if(acdbGetObjectId(id,name)!=Acad::eOk)return false;
 AcDbObjectPointer<TVS_Pipe> p(id,AcDb::kForRead);
 if(p.openStatus()!=Acad::eOk){acutPrintf(L"\nIzaberite pravi TVS kanal, bez XREF-a/bloka.");return false;}
 AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();
 AcDbObjectPointer<AcDbBlockTableRecord> owner(p->ownerId(),AcDb::kForRead);
 if(p->database()!=db||owner.openStatus()!=Acad::eOk||!owner->isLayout()||p->ownerId()!=db->currentSpaceId()){
  acutPrintf(L"\nPotreban je direktan kanal u aktivnom Model prostoru.");return false;}
 bool marked=false;
#ifdef TVS_OFFSET_INTEGRATION
 marked=tvsOffsetData::marked(p.object());
#endif
 out.first=Point(p->FirstPoint.x,p->FirstPoint.y,p->FirstPoint.z);
 out.last=Point(p->LastPoint.x,p->LastPoint.y,p->LastPoint.z);
 out.a=p->SizeA;out.b=p->SizeB;out.elevation=p->get_Elevation();out.rectangular=!p->ThisRound;
 out.rigid=p->DuctType==DuctTypeStill;out.ordinary=!marked;
 return true;
}
class Transient {
 AcGiTransientManager* manager;AcDbIntArray viewports;std::vector<AcDbPolyline*> lines;
public:
 Transient():manager(acgiGetTransientManager()){}
 ~Transient(){clear();}
 void clear(){for(auto p:lines){manager->eraseTransient(p,viewports);delete p;}lines.clear();}
 bool show(const View& v,bool allEdges,bool axes){
  if(!manager||!v.ready)return false;
  clear();lines.reserve(10000);
  try {for(const auto& l:planLines(v,allEdges,axes)){
   double dx=l.b.s-l.a.s,dy=l.b.z-l.a.z,length=std::hypot(dx,dy);if(length<=0)continue;
   const double axisPattern[]={20,4,4,4},seamPattern[]={5,4},diagnosticPattern[]={1,5};
   const double* pattern=l.kind==AxisLine?axisPattern:l.kind==JointSymbol?seamPattern:diagnosticPattern;
   int count=l.kind==AxisLine?4:2;double position=0;unsigned part=0;
   do {
    double next=l.kind==PhysicalOutline?length:(std::min)(length,position+pattern[part%count]);
    if(l.kind==PhysicalOutline||part%2==0){
     if(lines.size()>=10000){clear();return false;}
     std::unique_ptr<AcDbPolyline> p(new AcDbPolyline());
     Point a=v.frame.world(l.a.s+dx*position/length,l.a.z+dy*position/length);
     Point b=v.frame.world(l.a.s+dx*next/length,l.a.z+dy*next/length);
     if(p->addVertexAt(0,AcGePoint2d(a.x,a.y))!=Acad::eOk||p->addVertexAt(1,AcGePoint2d(b.x,b.y))!=Acad::eOk){clear();return false;}
     p->setElevation(v.frame.origin.z);p->setColorIndex(l.member==7?1:l.kind==AxisLine?2:l.kind==JointSymbol?4:l.insulation||l.kind==DiagnosticLine?8:3);
     p->setLineWeight(l.kind==PhysicalOutline?AcDb::kLnWt025:AcDb::kLnWt009);
     if(!manager->addTransient(p.get(),kAcGiDirectShortTerm,128,viewports)){clear();return false;}
     lines.push_back(p.release());
    }
    position=next;++part;
   }while(position<length);
  }}catch(...){clear();throw;}
  return true;
 }

};
static const int FieldCount=26;
static double Options::* const fields[FieldCount]={&Options::ri,&Options::insulation,&Options::obstacleInsulation,&Options::clearance,&Options::reserve,
 &Options::minSlope,&Options::minBridge,&Options::minProjection,&Options::left,&Options::right,&Options::floor,&Options::ceiling,
 &Options::timeMs,&Options::calls,&Options::cells,&Options::depthStep,&Options::bridgeStep,&Options::maxDepth,&Options::depthLevels,
 &Options::bridgeLevels,&Options::positions,&Options::seedSolves,&Options::proposals,&Options::inset,&Options::wTolerance,&Options::dTolerance};
static const wchar_t* labels[FieldCount]={L"Unutrašnji radijus koljena Ri (mm)",L"Izolacija trase, mm *",L"Izolacija prepreke, mm *",L"Obavezan razmak, mm",L"Projektna rezerva, mm",
 L"Min. dužina kosog komada (mm)",L"Min. dužina srednjeg komada C (mm)",L"Min. projekcija, mm",L"Početak raspoloživog prozora s (mm)",L"Kraj raspoloživog prozora s (mm)",L"Donja dozvoljena kota Z (mm, opciono)",L"Gornja dozvoljena kota Z (mm, opciono)",
 L"Vrijeme, ms (max 10000)",L"Checker pozivi (max 128)",L"Ćelije (max 1000000)",L"Korak D, mm",L"Korak C, mm",L"Max. traženi D, mm",L"Nivoi D (max 128)",L"Prijedlozi C (max 16)",L"Položaji (max 16)",L"Seed solve (max 128)",L"Prijedlozi (max 2048)",L"Inset prozora, mm",L"Prag poboljšanja W, mm",L"Prag poboljšanja D, mm"};
struct Dialog {
 Snapshot snapshot;bool reverse,loading,unitFromRecord,advanced,showDimensions,showAxes,allEdges;double scaleX,scaleY;Engine engine;View view;Options options;
 std::wstring values[FieldCount];int units,angle,side;bool confirmed;HWND window;HFONT font;
 Dialog(const Snapshot& s,bool r,double factor,bool known):snapshot(s),reverse(r),loading(true),unitFromRecord(known),advanced(false),showDimensions(true),showAxes(true),allEdges(false),scaleX(1),scaleY(1),units(known?(factor==1?0:factor==10?1:2):-1),angle(1),side(0),confirmed(false),window(NULL),font(NULL){
  for(int j=0;j<FieldCount;++j)values[j]=number(options.*fields[j]);
  values[1]=values[2]=L"";values[10]=values[11]=L"";
  Frame f=engine.frame(s,factor,r);if(known&&f.valid){values[8]=number(f.left);values[9]=number(f.right);}else values[8]=values[9]=L"";
 }
 ~Dialog(){if(font)DeleteObject(font);}
 double factor()const{return units==0?1:units==1?10:units==2?1000:0;}
 HWND child(const wchar_t* klass,const wchar_t* text,DWORD style,int id,int x,int y,int w,int h){
  HWND c=CreateWindowExW(klass==std::wstring(L"EDIT")?WS_EX_CLIENTEDGE:0,klass,text,WS_CHILD|WS_VISIBLE|style,(int)(x*scaleX),(int)(y*scaleY),(int)(w*scaleX),(int)(h*scaleY),window,(HMENU)(INT_PTR)id,AfxGetInstanceHandle(),NULL);
  SendMessage(c,WM_SETFONT,(WPARAM)font,TRUE);return c;
 }
 std::wstring text(int id){wchar_t value[512];GetDlgItemTextW(window,id,value,512);return value;}
 void source(){
  Frame f=engine.frame(snapshot,factor(),reverse);std::wostringstream t;
  if(f.valid){
   t<<L"TRASA "<<f.width<<L" × "<<f.height<<L" mm, osa Z="<<f.z0<<L" mm     |     PREPREKA "<<f.obstacleWidth<<L" × "<<f.obstacleHeight<<L" mm, osa Z="<<f.obstacleZ<<L" mm (iz objekata)\r\n";
   t<<L"Jedinice: "<<(unitFromRecord?L"TVS zapis":L"ručni izbor")<<L" — provjerite i potvrdite. s=0 je presjek osa; +s prati "<<(reverse?L"prvi":L"posljednji")<<L" kraj trase.\r\n";
   t<<L"Sve mjere i fizičke kote ispod su u mm. Presjek nije uvećan po visini; crtačka ravan Z="<<f.origin.z<<L" jedinica.";
  }else t<<L"Izaberite jedinice crteža i unesite izolacije. Podaci iz objekata ostaju neizmijenjeni.\r\n"<<wide(f.reason);
  SetDlgItemTextW(window,600,t.str().c_str());
 }
 void invalidate(){engine.invalidate();view=View();EnableWindow(GetDlgItem(window,701),FALSE);SetDlgItemTextW(window,800,L"Ulazi su promijenjeni. Rezultat ne važi; pokrenite računanje.");InvalidateRect(window,NULL,TRUE);}
 void capture(){for(int j=0;j<FieldCount;++j)values[j]=text(100+j);
  units=(int)SendDlgItemMessage(window,502,CB_GETCURSEL,0,0);angle=(int)SendDlgItemMessage(window,501,CB_GETCURSEL,0,0);side=(int)SendDlgItemMessage(window,500,CB_GETCURSEL,0,0);
  confirmed=IsDlgButtonChecked(window,503)==BST_CHECKED;}
 bool parse(){capture();options=Options();
  if(units<0||angle<0||side<0||!confirmed){SetDlgItemTextW(window,800,L"Izaberite jedinice i potvrdite izvore/ručne postavke.");return false;}
  for(int j=0;j<FieldCount;++j){std::wstring value=values[j];
   if((j==10||j==11)&&value.empty())continue;
   for(auto& c:value)if(c==L',')c=L'.';
   wchar_t* end=NULL;double x=wcstod(value.c_str(),&end);
   if(value.empty()||end==value.c_str()||*end||!finite(x)){
    std::wstring error=L"Nedostaje/nevažeće: ";error+=labels[j];error+=L". Nepoznato nije nula; za nultu izolaciju unesite 0.";
    if(j==7||j>=12){advanced=true;CheckDlgButton(window,504,BST_CHECKED);fieldLayout();}
    SetDlgItemTextW(window,800,error.c_str());SetFocus(GetDlgItem(window,100+j));return false;}
   options.*fields[j]=x;
  }
  options.confirmed=true;options.useFloor=!values[10].empty();options.useCeiling=!values[11].empty();options.angle=30+15*angle;options.above=side==1;return true;
 }
 void calculate(bool improve){
  if(!parse()){std::wstring error=text(800);invalidate();SetDlgItemTextW(window,800,error.c_str());return;}
  EnableWindow(GetDlgItem(window,700),FALSE);EnableWindow(GetDlgItem(window,702),FALSE);
  SetDlgItemTextW(window,800,L"Računanje... Rok je kooperativan; aktivni checker poziv se ne prekida. Dijalog čeka završetak.");UpdateWindow(window);
  try{view=engine.calculate(snapshot,factor(),reverse,options,improve);
   refreshResult();}
  catch(...){invalidate();SetDlgItemTextW(window,800,L"Greška računanja. Nema DWG izmjena.");}
  EnableWindow(GetDlgItem(window,700),TRUE);EnableWindow(GetDlgItem(window,702),TRUE);
  EnableWindow(GetDlgItem(window,701),view.ready&&engine.ready());InvalidateRect(window,NULL,TRUE);
 }
 void refreshResult(){
  std::wstring value=advanced?wide(view.status)+L"\r\n"+wide(view.details):wide(summary(view,options));
  if(!view.ready&&view.status.empty())value=L"Unesite podatke i pokrenite računanje. Rezultat još nije potvrđen.";
  SetDlgItemTextW(window,800,value.c_str());
 }
 void fieldLayout(){
  const int basic[]={0,1,2,3,4,5,6,8,9,10,11};const int expert[]={7,12,13,14,15,16,17,18,19,20,21,22,23,24,25};
  for(int j=0;j<FieldCount;++j){ShowWindow(GetDlgItem(window,100+j),SW_HIDE);ShowWindow(GetDlgItem(window,1000+j),SW_HIDE);}
  const int* list=advanced?expert:basic;int n=advanced?15:11;
  for(int i=0;i<n;++i){int j=list[i],x=12+(advanced?i/8*280:0),y=140+(advanced?i%8:i)*29;
   int labelWidth=advanced?172:306,editX=advanced?174:312,editWidth=advanced?92:230;
   SetWindowPos(GetDlgItem(window,1000+j),NULL,(int)(x*scaleX),(int)((y+3)*scaleY),(int)(labelWidth*scaleX),(int)(22*scaleY),SWP_NOZORDER|SWP_SHOWWINDOW);
   SetWindowPos(GetDlgItem(window,100+j),NULL,(int)((x+editX)*scaleX),(int)(y*scaleY),(int)(editWidth*scaleX),(int)(24*scaleY),SWP_NOZORDER|SWP_SHOWWINDOW);
  }
  SetDlgItemTextW(window,601,advanced?L"Razvojni budžeti i numerička dijagnostika.\r\nSva polja ostaju aktivna i kada je ova grupa zatvorena.\r\nOtvorite Osnovne postavke za geometrijske ulaze.":L"Prozor: s=0 je presjek osa. Negativno je prije, pozitivno poslije.\r\nIzolacije morate unijeti (0 kada ih nema). Ostalo: ručno / predlog.\r\nGranice su apsolutne kote spoljnih gabarita; prazno = neaktivno.");
  InvalidateRect(window,NULL,TRUE);
 }
 void init(HWND h){window=h;loading=true;
  RECT work;SystemParametersInfo(SPI_GETWORKAREA,0,&work,0);
  SetWindowPos(h,NULL,work.left+10,work.top+10,(std::min)(1220L,work.right-work.left-20),(std::min)(760L,work.bottom-work.top-20),SWP_NOZORDER);
  RECT client;GetClientRect(h,&client);scaleX=client.right/1204.0;scaleY=client.bottom/720.0;
  if(!font)font=CreateFontW(-(int)(13*(std::min)(scaleX,scaleY)),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
  child(L"STATIC",L"",SS_LEFT,600,12,10,1180,75);source();
  child(L"STATIC",L"Prolaz:",0,0,12,94,48,22);HWND c=child(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,500,62,90,90,100);
  SendMessage(c,CB_ADDSTRING,0,(LPARAM)L"Ispod");SendMessage(c,CB_ADDSTRING,0,(LPARAM)L"Iznad");SendMessage(c,CB_SETCURSEL,side,0);
  child(L"STATIC",L"Ugao:",0,0,165,94,40,22);c=child(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,501,205,90,70,100);
  for(auto s:{L"30°",L"45°",L"60°"})SendMessage(c,CB_ADDSTRING,0,(LPARAM)s);SendMessage(c,CB_SETCURSEL,angle,0);
  child(L"STATIC",L"Jedinice:",0,0,290,94,62,22);c=child(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,502,355,90,100,110);
  for(auto s:{L"mm",L"cm",L"m"})SendMessage(c,CB_ADDSTRING,0,(LPARAM)s);if(units>=0)SendMessage(c,CB_SETCURSEL,units,0);
  for(int j=0;j<FieldCount;++j){int col=j/13,row=j%13,x=12+col*280,y=128+row*29;
   child(L"STATIC",labels[j],0,1000+j,x,y+3,172,22);child(L"EDIT",values[j].c_str(),ES_AUTOHSCROLL|WS_TABSTOP,100+j,x+174,y,92,24);}
  child(L"STATIC",L"",0,601,12,480,550,68);
  child(L"BUTTON",L"Potvrđujem jedinice, izolacije, ručne ulaze i razvojne budžete",BS_AUTOCHECKBOX|WS_TABSTOP,503,12,576,555,25);CheckDlgButton(h,503,confirmed?BST_CHECKED:BST_UNCHECKED);
  child(L"BUTTON",L"Pronađi prolaz",BS_PUSHBUTTON|WS_TABSTOP,700,12,616,150,30);
  child(L"BUTTON",L"Pokušaj kraći prolaz",BS_PUSHBUTTON|WS_TABSTOP,702,174,616,170,30);
  child(L"BUTTON",L"Pregled u crtežu",BS_PUSHBUTTON|WS_TABSTOP,701,356,616,165,30);EnableWindow(GetDlgItem(h,701),view.ready&&engine.ready());
  child(L"BUTTON",L"Umetni (nedostupno)",BS_PUSHBUTTON,703,12,666,180,30);EnableWindow(GetDlgItem(h,703),FALSE);
  child(L"BUTTON",L"Otkaži / zatvori",BS_PUSHBUTTON|WS_TABSTOP,IDCANCEL,348,666,173,30);
  child(L"BUTTON",L"Napredno / dijagnostika",BS_AUTOCHECKBOX|WS_TABSTOP,504,12,112,280,22);CheckDlgButton(h,504,advanced?BST_CHECKED:BST_UNCHECKED);
  child(L"STATIC",L"Uzdužni presjek — fizičke kote, ista razmjera s/z",0,0,590,90,590,20);
  child(L"BUTTON",L"Kote",BS_AUTOCHECKBOX|WS_TABSTOP,505,590,112,68,22);CheckDlgButton(h,505,showDimensions?BST_CHECKED:BST_UNCHECKED);
  child(L"BUTTON",L"Ose",BS_AUTOCHECKBOX|WS_TABSTOP,506,666,112,68,22);CheckDlgButton(h,506,showAxes?BST_CHECKED:BST_UNCHECKED);
  c=child(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,507,755,110,425,100);
  SendMessage(c,CB_ADDSTRING,0,(LPARAM)L"Konture (dijagnostika, bez zaklanjanja)");
  SendMessage(c,CB_ADDSTRING,0,(LPARAM)L"Sve ivice — dijagnostika (bez zaklanjanja)");SendMessage(c,CB_SETCURSEL,allEdges?1:0,0);
  child(L"EDIT",view.ready?(wide(view.status)+L"\r\n"+wide(view.details)).c_str():L"Rezultat nije izračunat. Nema trajnog upisa.",ES_MULTILINE|ES_READONLY|WS_VSCROLL,800,590,536,590,129);
  child(L"STATIC",L"DEV: upis nije implementiran. Osnova je dijagnostička, bez zaklanjanja.\r\nIzabrana originalna trasa ostaje vidljiva ispod transient pregleda.",0,0,590,675,590,34);
  fieldLayout();refreshResult();loading=false;
 }
 void paint(){
  PAINTSTRUCT ps;HDC dc=BeginPaint(window,&ps);
  RECT area={(LONG)(590*scaleX),(LONG)(136*scaleY),(LONG)(1180*scaleX),(LONG)(526*scaleY)};
  FillRect(dc,&area,(HBRUSH)GetStockObject(WHITE_BRUSH));
  HGDIOBJ oldFont=SelectObject(dc,font);int oldBk=SetBkMode(dc,TRANSPARENT);COLORREF oldColor=GetTextColor(dc);
  if(view.ready&&view.axis.size()==8){
   RECT plot=area;plot.left+=(LONG)(55*scaleX);plot.right-=(LONG)(20*scaleX);plot.top+=(LONG)(30*scaleY);plot.bottom-=(LONG)((showDimensions?145:30)*scaleY);
   double sl=1e100,sr=-1e100,zl=1e100,zr=-1e100;
   for(const auto& c:view.profile)for(const auto& p:c.points){sl=std::fmin(sl,p.s);sr=std::fmax(sr,p.s);zl=std::fmin(zl,p.z);zr=std::fmax(zr,p.z);}
   if(options.useFloor)zl=std::fmin(zl,options.floor);if(options.useCeiling)zr=std::fmax(zr,options.ceiling);
   double scale=std::fmin((plot.right-plot.left)/(sr-sl+1),(plot.bottom-plot.top)/(zr-zl+1));
   double x=plot.left+(plot.right-plot.left-(sr-sl)*scale)/2,y=plot.top+(plot.bottom-plot.top-(zr-zl)*scale)/2;
   auto px=[&](double s){return (int)(x+(s-sl)*scale);};auto py=[&](double z){return (int)(y+(zr-z)*scale);};
   auto line=[&](int x1,int y1,int x2,int y2,COLORREF color,int style=PS_SOLID){HPEN p=CreatePen(style,1,color);HGDIOBJ old=SelectObject(dc,p);MoveToEx(dc,x1,y1,NULL);LineTo(dc,x2,y2);SelectObject(dc,old);DeleteObject(p);};
   auto label=[&](int x1,int y1,int w,int h,const std::wstring& text,COLORREF color){RECT r={x1,y1,x1+w,y1+h};SetTextColor(dc,color);DrawTextW(dc,text.c_str(),(int)text.size(),&r,DT_CENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_VCENTER);};
   for(const auto& c:view.profile){
    if(c.kind==AxisLine&&!showAxes)continue;
    if(c.member==9&&!showDimensions)continue;
    COLORREF color=c.member==7?RGB(190,40,40):c.member==9?RGB(0,120,65):c.insulation?RGB(145,145,145):c.kind==AxisLine?RGB(110,110,110):RGB(20,85,155);
    HPEN pen=CreatePen(c.kind==AxisLine?PS_DASHDOT:c.insulation?PS_DOT:PS_SOLID,1,color);HGDIOBJ old=SelectObject(dc,pen);
    bool first=true;for(const auto& p:c.points){if(first)MoveToEx(dc,px(p.s),py(p.z),NULL);else LineTo(dc,px(p.s),py(p.z));first=false;}SelectObject(dc,old);DeleteObject(pen);
   }
   if(options.useFloor)line(px(sl),py(options.floor),px(sr),py(options.floor),RGB(130,50,170),PS_DASH);
   if(options.useCeiling)line(px(sl),py(options.ceiling),px(sr),py(options.ceiling),RGB(130,50,170),PS_DASH);
   if(showDimensions){
    HPEN markerPen=CreatePen(PS_SOLID,1,RGB(0,120,65));HGDIOBJ previousPen=SelectObject(dc,markerPen),previousBrush=SelectObject(dc,GetStockObject(HOLLOW_BRUSH));
    for(const auto& point:{view.witnessBody,view.witnessObstacle})Ellipse(dc,px(point.s)-3,py(point.z)-3,px(point.s)+3,py(point.z)+3);
    SelectObject(dc,previousBrush);SelectObject(dc,previousPen);DeleteObject(markerPen);
    const int h=(int)(18*scaleY),cw=(area.right-area.left)/2;
    label(area.left,area.top,cw,h,L"Početna osa Z="+wide(displayNumber(view.axis.front().z))+L" mm",RGB(20,85,155));
    label(area.left+cw,area.top,cw,h,L"Završna osa Z="+wide(displayNumber(view.axis.back().z))+L" mm",RGB(20,85,155));
    auto tick=[&](int a,int b){line(a-3,b-3,a+3,b+3,RGB(65,65,65));};
    int wy=plot.bottom+(int)(18*scaleY),cy=plot.bottom+(int)(42*scaleY);
    int l=px(view.axis.front().s),r=px(view.axis.back().s);line(l,wy,r,wy,RGB(65,65,65));tick(l,wy);tick(r,wy);
    label(l,wy-h,r-l,h,L"Ukupni zahvat W="+wide(displayNumber(view.W))+L" mm",RGB(35,35,35));
    l=px(view.axis[3].s);r=px(view.axis[4].s);line(l,cy,r,cy,RGB(65,65,65));tick(l,cy);tick(r,cy);
    int labelWidth=(int)(320*scaleX);
    label((std::max)((int)area.left,(std::min)((int)area.right-labelWidth,(l+r-labelWidth)/2)),cy-h,labelWidth,h,L"Srednji komad C="+wide(displayNumber(view.C))+L" mm",RGB(35,35,35));
    int dx=area.left+(int)(15*scaleX);line(dx,py(view.axis.front().z),dx,py(view.zPass),RGB(65,65,65));tick(dx,py(view.axis.front().z));tick(dx,py(view.zPass));
    // D label stays in its own left rail; no text is placed over the body.
    std::wstring d=L"D="+wide(displayNumber(view.D));
    LOGFONTW lf={};GetObjectW(font,sizeof(lf),&lf);lf.lfEscapement=900;lf.lfOrientation=900;HFONT vertical=CreateFontIndirectW(&lf);
    HGDIOBJ old=SelectObject(dc,vertical);TextOutW(dc,dx-(int)(12*scaleX),(py(view.axis.front().z)+py(view.zPass))/2+(int)(30*scaleY),d.c_str(),(int)d.size());SelectObject(dc,old);DeleteObject(vertical);
    auto notes=captions(view,options);int rowH=(int)(15*scaleY),top=area.bottom-6*rowH;
    for(unsigned j=0;j<notes.size();++j){COLORREF color=notes[j].color==1?RGB(190,40,40):notes[j].color==2?RGB(0,120,65):notes[j].color==3?RGB(130,50,170):RGB(45,45,45);
     label(area.left+(j%2)*cw,top+(j/2)*rowH,cw,rowH,wide(notes[j].text),color);}
   }
  }
  SetTextColor(dc,oldColor);SetBkMode(dc,oldBk);SelectObject(dc,oldFont);EndPaint(window,&ps);
 }
 static INT_PTR CALLBACK proc(HWND h,UINT m,WPARAM w,LPARAM l){
  Dialog* self=(Dialog*)GetWindowLongPtr(h,DWLP_USER);
  if(m==WM_INITDIALOG){self=(Dialog*)l;SetWindowLongPtr(h,DWLP_USER,l);self->init(h);return TRUE;}
  if(!self)return FALSE;
  if(m==WM_PAINT){self->paint();return TRUE;}
  if(m==WM_CLOSE){EndDialog(h,IDCANCEL);return TRUE;}
  if(m==WM_COMMAND){int id=LOWORD(w),code=HIWORD(w);
   if(id==IDCANCEL){EndDialog(h,IDCANCEL);return TRUE;}
   if(id==IDOK)return TRUE; // No accidental Enter-based insertion or calculation.
   if(!self->loading&&((id>=100&&id<100+FieldCount&&code==EN_CHANGE)||((id==500||id==501||id==502)&&code==CBN_SELCHANGE)||(id==503&&code==BN_CLICKED))){
    self->capture();self->invalidate();
    if(id==502){self->unitFromRecord=false;Frame f=self->engine.frame(self->snapshot,self->factor(),self->reverse);self->loading=true;
     if(f.valid){SetDlgItemTextW(h,108,number(f.left).c_str());SetDlgItemTextW(h,109,number(f.right).c_str());}self->loading=false;self->source();}return TRUE;}
   if(id==504&&code==BN_CLICKED){self->advanced=IsDlgButtonChecked(h,504)==BST_CHECKED;self->fieldLayout();self->refreshResult();return TRUE;}
   if((id==505||id==506)&&code==BN_CLICKED){self->showDimensions=IsDlgButtonChecked(h,505)==BST_CHECKED;self->showAxes=IsDlgButtonChecked(h,506)==BST_CHECKED;InvalidateRect(h,NULL,TRUE);return TRUE;}
   if(id==507&&code==CBN_SELCHANGE){self->allEdges=SendDlgItemMessage(h,507,CB_GETCURSEL,0,0)==1;return TRUE;}
   if(id==700||id==702){self->calculate(id==702);return TRUE;}
   if(id==701&&self->view.ready&&self->engine.ready()){self->capture();EndDialog(h,701);return TRUE;}
  }return FALSE;
 }
 INT_PTR show(HWND owner){
  const wchar_t title[]=L"TVS — Obilazak kanala (DEV preview)";
  std::vector<WORD> buffer((sizeof(DLGTEMPLATE)+sizeof(title)+8)/2,0);
  DLGTEMPLATE t={};t.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME;t.cx=780;t.cy=460;
  memcpy(&buffer[0],&t,sizeof(t));BYTE* p=(BYTE*)&buffer[0]+sizeof(t);p+=4;memcpy(p,title,sizeof(title));
  return DialogBoxIndirectParamW(AfxGetInstanceHandle(),(DLGTEMPLATE*)&buffer[0],owner,proc,(LPARAM)this);
 }
};
}
void tvsBypassPreviewCommand(){
 try{
  AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();
  if(!db||db->tilemode()==false){acutPrintf(L"\nDEV pregled radi samo u Model prostoru.");return;}
  acutPrintf(L"\nRazvojni pregled — upis nije implementiran. Izabrani objekti ostaju neizmijenjeni.");
  Snapshot s;AcDbObjectId route,obstacle;
  if(!readPipe(L"\nIzaberite pravi horizontalni TVS kanal kao referencu nove trase: ",s.route,route))return;
  if(!readPipe(L"\nIzaberite jednu pravougaonu horizontalnu TVS prepreku: ",s.obstacle,obstacle))return;
  if(route==obstacle){acutPrintf(L"\nTrasa i prepreka moraju biti različiti objekti.");return;}
  acedInitGet(0,L"Prvi Posljednji");ACHAR answer[80]=L"";
  int status=acedGetKword(L"\nAktivni kraj / smjer +s [Prvi/Posljednji] <Posljednji>: ",answer);
  if(status!=RTNORM&&status!=RTNONE)return;
  bool reverse=_wcsicmp(answer,L"Prvi")==0;
  bool known=false;AcDbDictionary* dict=NULL;
  if(db->getNamedObjectsDictionary(dict,AcDb::kForRead)==Acad::eOk){AcDbObjectId id;if(dict->getAt(tvsUnits::recordName(),id)==Acad::eOk){
    AcDbXrecord* record=NULL;if(acdbOpenObject(record,id,AcDb::kForRead)==Acad::eOk){resbuf* data=NULL;
     if(record->rbChain(&data)==Acad::eOk){for(resbuf* p=data;p;p=p->rbnext)if(p->restype==40&&tvsUnits::validFactor(p->resval.rreal))known=true;if(data)acutRelRb(data);}record->close();}}
   dict->close();}
  Dialog dialog(s,reverse,tvsUnits::factor(db),known);
  HWND owner=acedGetAcadFrame()->GetSafeHwnd();
  while(dialog.show(owner)==701){
   Transient preview;
   if(!preview.show(dialog.view,dialog.allEdges,dialog.showAxes)){acutPrintf(L"\nTransient pregled nije moguće prikazati. Nema DWG izmjena.");return;}
   acutPrintf(L"\nPrivremena OSNOVA — DIJAGNOSTIKA, BEZ ZAKLANJANJA. Pune: gabariti; crtice: simbolički spojevi/pomoćne linije; dugo-kratko: ose. Originalna trasa ostaje vidljiva. Nema DWG upisa.");
   ACHAR buffer[256];int r=acedGetString(0,L"\nPregled: Enter vraća dijalog; Esc završava i uklanja pregled: ",buffer);
   preview.clear();if(r!=RTNORM&&r!=RTNONE)break;
  }
 }catch(...){acutPrintf(L"\nGreška razvojnog pregleda. Privremena grafika je uklonjena; nema DWG upisa.");}
}
#endif
