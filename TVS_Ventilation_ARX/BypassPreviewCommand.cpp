#include "StdAfx.h"
#ifdef TVS_BYPASS_PREVIEW
#include "BypassPreviewCommand.h"
#include "../geometry/bypass/PreviewPresentation.h"
#include "TVS_Pipe.h"
#include "TVS_TAP.h"
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
#include "BypassSideDetail.h"
#include "BypassSideDetailTest.h"
#include "BypassService.h"
#include "BypassDialogState.h"
static_assert(EN_CHANGE==tvsBypassUx::kEnChange&&CBN_SELCHANGE==tvsBypassUx::kCbnSelChange&&BN_CLICKED==tvsBypassUx::kBnClicked,"Win32 notification codes");
static_assert(RadiusTypeRect_RadiusConstant==tvsBypassUx::RadiusConstant&&RadiusTypeRect_RadiusVariable==tvsBypassUx::RadiusVariable
 &&RadiusTypeRect_RadiusSpecific==tvsBypassUx::RadiusSpecific,"TVS radius modes");
namespace {
using namespace tvsBypassPreview;
static std::wstring wide(const std::string& s){
 int n=MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,NULL,0);if(!n)return L"";
 std::vector<wchar_t> out(n);MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,&out[0],n);return &out[0];
}
static std::wstring number(double x){wchar_t buf[80];swprintf_s(buf,L"%.9g",x);return buf;}
static bool readSegment(const AcDbObjectId& id,Segment& out){
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
static bool readPipe(const ACHAR* prompt,Segment& out,AcDbObjectId& id){
 ads_name name;ads_point pick;if(acedEntSel(prompt,name,pick)!=RTNORM)return false;
 return acdbGetObjectId(id,name)==Acad::eOk&&readSegment(id,out);
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
static const int FieldCount=tvsBypassUx::FieldCount;
static double Options::* const fields[FieldCount]={&Options::ri,&Options::insulation,&Options::obstacleInsulation,&Options::clearance,&Options::reserve,
 &Options::minSlope,&Options::minBridge,&Options::minProjection,&Options::left,&Options::right,&Options::floor,&Options::ceiling,
 &Options::timeMs,&Options::calls,&Options::cells,&Options::depthStep,&Options::bridgeStep,&Options::maxDepth,&Options::depthLevels,
 &Options::bridgeLevels,&Options::positions,&Options::seedSolves,&Options::proposals,&Options::inset,&Options::wTolerance,&Options::dTolerance};
static const wchar_t* labels[FieldCount]={L"Unutrašnji radijus koljena Ri (mm)",L"Izolacija trase, mm *",L"Izolacija prepreke, mm *",L"Obavezan razmak, mm",L"Projektna rezerva, mm",
 L"Min. dužina kosog komada (mm)",L"Min. dužina srednjeg komada C (mm)",L"Min. projekcija, mm",L"Početak prozora s (mm)",L"Kraj prozora s (mm)",L"Donja granica Z, mm (opciono)",L"Gornja granica Z, mm (opciono)",
 L"Vrijeme, ms (max 10000)",L"Checker pozivi (max 128)",L"Ćelije (max 1000000)",L"Korak D, mm",L"Korak C, mm",L"Max. traženi D, mm",L"Nivoi D (max 128)",L"Prijedlozi C (max 16)",L"Položaji (max 16)",L"Seed solve (max 128)",L"Prijedlozi (max 2048)",L"Inset prozora, mm",L"Prag poboljšanja W, mm",L"Prag poboljšanja D, mm"};
struct Dialog {
 Snapshot snapshot;bool reverse,loading,unitFromRecord,unitDefault,advanced,showDimensions,showAxes,allEdges,canAccept;double scaleX,scaleY;Engine engine;View view;Options options;
 std::wstring values[FieldCount];int units,angle,side;bool confirmed;HWND window;HFONT font;
 // Per-document design settings (CDocData) and the TVS radius mode. Geometry, units,
 // window and Z limits always come from the newly selected objects.
 tvsBypassUx::Memory& memory;tvsBypassUx::RadiusSetting radiusSetting;bool riEdited,insulationProposed;std::wstring riSource;
 Dialog(const Snapshot& s,bool r,double factor,bool known,tvsBypassUx::Memory& remembered,const tvsBypassUx::RadiusSetting& radius,bool accept=false):snapshot(s),reverse(r),loading(true),unitFromRecord(known),unitDefault(false),advanced(false),showDimensions(true),showAxes(true),allEdges(false),canAccept(accept),scaleX(1),scaleY(1),units(known?(factor==1?0:factor==10?1:2):-1),angle(1),side(0),confirmed(false),window(NULL),font(NULL),
  memory(remembered),radiusSetting(radius),riEdited(false),insulationProposed(false){
  for(int j=0;j<FieldCount;++j)values[j]=number(options.*fields[j]);
  values[1]=values[2]=L"";values[10]=values[11]=L"";
  insulationProposed=tvsBypassUx::apply(memory,values,angle,side);
  proposeRadius();
  Frame f=engine.frame(s,factor,r);if(known&&f.valid){values[8]=number(f.left);values[9]=number(f.right);}else values[8]=values[9]=L"";
 }
 // Ri from the active TVS radius mode (or Rc = 1.0 x B) for the selected duct; a value
 // the user typed in this dialog is never overwritten. Needs known units.
 void proposeRadius(){
  if(riEdited)return;const double f=factor();if(!(f>0))return;
  const Frame fr=engine.frame(snapshot,f,reverse);if(!fr.valid)return;
  const tvsBypassUx::RadiusProposal p=tvsBypassUx::radius(radiusSetting,fr.height,f);if(!p.ok)return;
  values[0]=number(p.ri);riSource=p.source;
  if(window){const bool was=loading;loading=true;SetDlgItemTextW(window,100,values[0].c_str());loading=was;}
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
   t<<L"Jedinice: "<<(unitFromRecord?L"TVS zapis crteža":unitDefault?L"TVS podrazumijevano mm (crtež nema zapis jedinica)":L"ručni izbor")<<L" — provjerite i potvrdite. s=0 je presjek osa; +s prati "<<(reverse?L"prvi":L"posljednji")<<L" kraj trase.\r\n";
   t<<L"Sve mjere i fizičke kote ispod su u mm. Presjek nije uvećan po visini; crtačka ravan Z="<<f.origin.z<<L" jedinica.";
   if(!riSource.empty())t<<L"   Ri: "<<riSource<<(riEdited?L" (ručno izmijenjen)":L"");
   if(insulationProposed)t<<L"\r\nIzolacije i ostala podešavanja su PRIJEDLOG iz prethodnog unosa u ovom crtežu, ne podatak objekta — provjerite.";
  }else t<<L"Izaberite jedinice crteža i unesite izolacije. Podaci iz objekata ostaju neizmijenjeni.\r\n"<<wide(f.reason);
  SetDlgItemTextW(window,600,t.str().c_str());
 }
 // Repaint only the section area, without erasing: keystrokes no longer flash the dialog.
 void redrawSection(){const SectionLayout layout=sectionLayout(scaleX,scaleY,showDimensions,advanced);
  RECT area={layout.area.left,layout.area.top,layout.area.right,layout.area.bottom};InvalidateRect(window,&area,FALSE);}
 void invalidate(){const bool wasReady=view.ready;engine.invalidate();view=View();EnableWindow(GetDlgItem(window,701),FALSE);EnableWindow(GetDlgItem(window,703),FALSE);EnableWindow(GetDlgItem(window,704),FALSE);SetDlgItemTextW(window,800,L"Ulazi su promijenjeni. Rezultat ne važi; pokrenite računanje.");if(wasReady)redrawSection();}
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
  options.confirmed=true;options.useFloor=!values[10].empty();options.useCeiling=!values[11].empty();options.angle=30+15*angle;options.above=side==1;
  tvsBypassUx::store(memory,values,angle,side); // confirmed design settings of this document
  return true;
 }
 void calculate(bool improve){
  if(!parse()){std::wstring error=text(800);invalidate();SetDlgItemTextW(window,800,error.c_str());return;}
  EnableWindow(GetDlgItem(window,700),FALSE);EnableWindow(GetDlgItem(window,702),FALSE);
  SetDlgItemTextW(window,800,L"Računanje... Rok je kooperativan; aktivni checker poziv se ne prekida. Dijalog čeka završetak.");UpdateWindow(window);
  struct WaitCursor {HCURSOR old;WaitCursor():old(SetCursor(LoadCursor(NULL,IDC_WAIT))){}~WaitCursor(){SetCursor(old);}} wait;
  try{view=engine.calculate(snapshot,factor(),reverse,options,improve);
   refreshResult();}
  catch(...){invalidate();SetDlgItemTextW(window,800,L"Greška računanja. Nema DWG izmjena.");}
  EnableWindow(GetDlgItem(window,700),TRUE);EnableWindow(GetDlgItem(window,702),TRUE);
  EnableWindow(GetDlgItem(window,701),view.ready&&engine.ready());EnableWindow(GetDlgItem(window,703),view.ready&&engine.ready());EnableWindow(GetDlgItem(window,704),canAccept&&view.ready&&engine.ready());redrawSection();
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
   int labelWidth=advanced?172:244,editX=advanced?174:250,editWidth=advanced?92:192;
   SetWindowPos(GetDlgItem(window,1000+j),NULL,(int)(x*scaleX),(int)((y+3)*scaleY),(int)(labelWidth*scaleX),(int)(22*scaleY),SWP_NOZORDER|SWP_SHOWWINDOW);
   SetWindowPos(GetDlgItem(window,100+j),NULL,(int)((x+editX)*scaleX),(int)(y*scaleY),(int)(editWidth*scaleX),(int)(24*scaleY),SWP_NOZORDER|SWP_SHOWWINDOW);
  }
  SetDlgItemTextW(window,601,advanced?L"Razvojni budžeti i numerička dijagnostika.\r\nSva polja ostaju aktivna i kada je ova grupa zatvorena.\r\nOtvorite Osnovne postavke za geometrijske ulaze.":L"Prozor: s=0 je presjek osa. Negativno je prije, pozitivno poslije.\r\nIzolacije morate unijeti (0 kada ih nema). Ostalo: ručno / predlog.\r\nGranice su apsolutne kote spoljnih gabarita; prazno = neaktivno.");
  int left=advanced?590:470,width=1180-left;
  auto move=[&](int id,int x,int y,int w,int h){SetWindowPos(GetDlgItem(window,id),NULL,(int)(x*scaleX),(int)(y*scaleY),(int)(w*scaleX),(int)(h*scaleY),SWP_NOZORDER);};
  move(602,left,90,width,20);move(505,left,112,68,22);move(506,left+76,112,68,22);move(507,left+165,110,width-165,100);
  move(800,left,556,width,109);move(603,left,675,width,34);move(601,12,480,left-30,68);move(503,12,576,left-30,25);
  move(700,12,616,130,30);move(702,156,616,145,30);move(701,315,616,143,30);move(703,12,666,145,30);move(704,165,666,145,30);move(IDCANCEL,318,666,140,30);
  InvalidateRect(window,NULL,TRUE);
 }
 // Window size the user last chose (this AutoCAD session; UI geometry only, never drawing data).
 static SIZE& lastSize(){static SIZE size={0,0};return size;}
 void init(HWND h){window=h;
  RECT work;SystemParametersInfo(SPI_GETWORKAREA,0,&work,0);
  MONITORINFO monitor={};monitor.cbSize=sizeof(monitor);if(GetMonitorInfoW(MonitorFromWindow(GetParent(h),MONITOR_DEFAULTTONEAREST),&monitor))work=monitor.rcWork;
  // Compact default (85 % of the 1220x760 base); resizable, and the chosen size is kept.
  const LONG availableW=work.right-work.left-20,availableH=work.bottom-work.top-20;
  const LONG width=(std::min)(lastSize().cx>0?lastSize().cx:1040L,availableW),height=(std::min)(lastSize().cy>0?lastSize().cy:650L,availableH);
  SetWindowPos(h,NULL,work.left+(work.right-work.left-width)/2,work.top+(work.bottom-work.top-height)/2,width,height,SWP_NOZORDER);
  build();
 }
 // Rebuild all controls for the current client size (initial show and after a resize).
 void rebuild(){capture();while(HWND c=GetWindow(window,GW_CHILD))DestroyWindow(c);if(font){DeleteObject(font);font=NULL;}build();InvalidateRect(window,NULL,TRUE);}
 void build(){HWND h=window;loading=true;
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
  child(L"BUTTON",L"Potvrđujem jedinice i sve osnovne/napredne ulaze",BS_AUTOCHECKBOX|WS_TABSTOP,503,12,576,555,25);CheckDlgButton(h,503,confirmed?BST_CHECKED:BST_UNCHECKED);
  child(L"BUTTON",L"Pronađi prolaz",BS_PUSHBUTTON|WS_TABSTOP,700,12,616,150,30);
  child(L"BUTTON",L"Pokušaj kraći prolaz",BS_PUSHBUTTON|WS_TABSTOP,702,174,616,170,30);
  child(L"BUTTON",L"Pregled u crtežu",BS_PUSHBUTTON|WS_TABSTOP,701,356,616,165,30);EnableWindow(GetDlgItem(h,701),view.ready&&engine.ready());
  child(L"BUTTON",L"Umetni detalj…",BS_PUSHBUTTON|WS_TABSTOP,703,12,666,180,30);EnableWindow(GetDlgItem(h,703),view.ready&&engine.ready());
  child(L"BUTTON",L"Prihvati obilazak",BS_PUSHBUTTON|WS_TABSTOP,704,165,666,145,30);EnableWindow(GetDlgItem(h,704),canAccept&&view.ready&&engine.ready());
  if(!canAccept)ShowWindow(GetDlgItem(h,704),SW_HIDE); // TVS_BYPASS_PREVIEW never writes
  child(L"BUTTON",L"Otkaži / zatvori",BS_PUSHBUTTON|WS_TABSTOP,IDCANCEL,318,666,140,30);
  child(L"BUTTON",L"Napredno / dijagnostika",BS_AUTOCHECKBOX|WS_TABSTOP,504,12,112,280,22);CheckDlgButton(h,504,advanced?BST_CHECKED:BST_UNCHECKED);
  child(L"STATIC",L"Uzdužni presjek — fizičke kote, ista razmjera s/z",0,602,590,90,590,20);
  child(L"BUTTON",L"Kote",BS_AUTOCHECKBOX|WS_TABSTOP,505,590,112,68,22);CheckDlgButton(h,505,showDimensions?BST_CHECKED:BST_UNCHECKED);
  child(L"BUTTON",L"Ose",BS_AUTOCHECKBOX|WS_TABSTOP,506,666,112,68,22);CheckDlgButton(h,506,showAxes?BST_CHECKED:BST_UNCHECKED);
  c=child(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,507,755,110,425,100);
  SendMessage(c,CB_ADDSTRING,0,(LPARAM)L"Konture (dijagnostika, bez zaklanjanja)");
  SendMessage(c,CB_ADDSTRING,0,(LPARAM)L"Sve ivice — dijagnostika (bez zaklanjanja)");SendMessage(c,CB_SETCURSEL,allEdges?1:0,0);
  child(L"EDIT",view.ready?(wide(view.status)+L"\r\n"+wide(view.details)).c_str():L"Rezultat nije izračunat. Nema trajnog upisa.",ES_MULTILINE|ES_READONLY|WS_VSCROLL,800,590,556,590,109);
  child(L"STATIC",canAccept?L"DEV: Prihvati upisuje 7 TVS clanova i cuva krajnje prikljucke.\r\nPrikaz ne racuna tacnu vidljivost/zaklanjanje; detalj je odvojen staticki snimak.":L"DEV preview: staticki detalj je dostupan. Za trajni upis koristite TVS_BYPASS.\r\nPrikaz ne racuna tacnu vidljivost/zaklanjanje.",0,603,590,675,590,34);
  fieldLayout();refreshResult();loading=false;
 }
 void paint(){
  PAINTSTRUCT ps;HDC screen=BeginPaint(window,&ps);
  const SectionLayout layout=sectionLayout(scaleX,scaleY,showDimensions,advanced);
  RECT area={layout.area.left,layout.area.top,layout.area.right,layout.area.bottom};
  // Off-screen section, copied in one blit (falls back to direct drawing).
  RECT client;GetClientRect(window,&client);
  HDC dc=CreateCompatibleDC(screen);HBITMAP bitmap=dc?CreateCompatibleBitmap(screen,(std::max)(1L,client.right),(std::max)(1L,client.bottom)):NULL;
  if(!dc||!bitmap){if(bitmap)DeleteObject(bitmap);if(dc)DeleteDC(dc);dc=screen;bitmap=NULL;}
  HGDIOBJ oldBitmap=bitmap?SelectObject(dc,bitmap):NULL;
  FillRect(dc,&area,(HBRUSH)GetStockObject(WHITE_BRUSH));
  HGDIOBJ oldFont=SelectObject(dc,font);int oldBk=SetBkMode(dc,TRANSPARENT);COLORREF oldColor=GetTextColor(dc);
  if(view.ready&&view.axis.size()==8){
   RECT plot={layout.plot.left,layout.plot.top,layout.plot.right,layout.plot.bottom};
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
    auto notes=captions(view,options);
    LOGFONTW noteSpec={};GetObjectW(font,sizeof(noteSpec),&noteSpec);noteSpec.lfHeight=-(int)(11*(std::min)(scaleX,scaleY));HFONT noteFont=CreateFontIndirectW(&noteSpec);HGDIOBJ previousFont=SelectObject(dc,noteFont);
    for(unsigned j=0;j<notes.size();++j){COLORREF color=notes[j].color==1?RGB(190,40,40):notes[j].color==2?RGB(0,120,65):notes[j].color==3?RGB(130,50,170):RGB(45,45,45);
     const auto& slot=layout.labels[4+j];label(slot.left,slot.top,slot.right-slot.left,slot.bottom-slot.top,wide(notes[j].text),color);}
    SelectObject(dc,previousFont);DeleteObject(noteFont);
   }
  }
  SetTextColor(dc,oldColor);SetBkMode(dc,oldBk);SelectObject(dc,oldFont);
  if(bitmap){BitBlt(screen,area.left,area.top,area.right-area.left,area.bottom-area.top,dc,area.left,area.top,SRCCOPY);SelectObject(dc,oldBitmap);DeleteObject(bitmap);DeleteDC(dc);}
  EndPaint(window,&ps);
 }
 static INT_PTR CALLBACK proc(HWND h,UINT m,WPARAM w,LPARAM l){
  Dialog* self=(Dialog*)GetWindowLongPtr(h,DWLP_USER);
  if(m==WM_INITDIALOG){self=(Dialog*)l;SetWindowLongPtr(h,DWLP_USER,l);self->init(h);return TRUE;}
  if(!self)return FALSE;
  if(m==WM_PAINT){self->paint();return TRUE;}
  if(m==WM_CLOSE){EndDialog(h,IDCANCEL);return TRUE;}
  if(m==WM_GETMINMAXINFO){MINMAXINFO* mm=(MINMAXINFO*)l;mm->ptMinTrackSize.x=820;mm->ptMinTrackSize.y=520;return TRUE;}
  if(m==WM_EXITSIZEMOVE){RECT r;GetWindowRect(h,&r);SIZE now={r.right-r.left,r.bottom-r.top};SIZE& last=lastSize();
   RECT c;GetClientRect(h,&c);if(std::fabs(c.right/1204.0-self->scaleX)>1e-3||std::fabs(c.bottom/720.0-self->scaleY)>1e-3){last=now;self->rebuild();}return TRUE;}
  if(m==WM_COMMAND){int id=LOWORD(w),code=HIWORD(w);
   if(id==IDCANCEL){EndDialog(h,IDCANCEL);return TRUE;}
   if(id==IDOK)return TRUE; // No accidental Enter-based insertion or calculation.
   if(!self->loading&&tvsBypassUx::invalidatesResult(id,code)){
    if(id==100)self->riEdited=true;
    self->capture();self->invalidate();
    if(id==502){self->unitFromRecord=false;self->unitDefault=false;Frame f=self->engine.frame(self->snapshot,self->factor(),self->reverse);self->loading=true;
     if(f.valid){SetDlgItemTextW(h,108,number(f.left).c_str());SetDlgItemTextW(h,109,number(f.right).c_str());}self->loading=false;
     self->proposeRadius();self->capture();self->source();}
    else if(id==100)self->source();
    return TRUE;}
   if(id==504&&code==BN_CLICKED){self->advanced=IsDlgButtonChecked(h,504)==BST_CHECKED;self->fieldLayout();self->refreshResult();return TRUE;}
   if((id==505||id==506)&&code==BN_CLICKED){self->showDimensions=IsDlgButtonChecked(h,505)==BST_CHECKED;self->showAxes=IsDlgButtonChecked(h,506)==BST_CHECKED;self->redrawSection();return TRUE;}
   if(id==507&&code==CBN_SELCHANGE){self->allEdges=SendDlgItemMessage(h,507,CB_GETCURSEL,0,0)==1;return TRUE;}
   if(id==700||id==702){self->calculate(id==702);return TRUE;}
   if((id==701||id==703||(id==704&&self->canAccept))&&self->view.ready&&self->engine.ready()){self->capture();EndDialog(h,id);return TRUE;}
  }return FALSE;
 }
 INT_PTR show(HWND owner){
  const wchar_t title[]=L"TVS — Obilazak kanala (DEV)";
  std::vector<WORD> buffer((sizeof(DLGTEMPLATE)+sizeof(title)+8)/2,0);
  DLGTEMPLATE t={};t.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|DS_MODALFRAME;t.cx=780;t.cy=460;
  memcpy(&buffer[0],&t,sizeof(t));BYTE* p=(BYTE*)&buffer[0]+sizeof(t);p+=4;memcpy(p,title,sizeof(title));
  return DialogBoxIndirectParamW(AfxGetInstanceHandle(),(DLGTEMPLATE*)&buffer[0],owner,proc,(LPARAM)this);
 }
};
}
void tvsBypassDetailTestCommand(){try{tvsSideDetail::nativeFixture();}catch(const std::exception& e){tvsSideDetail::injectedStage=0;acutPrintf(L"\nNATIVE_DETAIL_FIXTURE_FAILED: %ls",wide(e.what()).c_str());}catch(...){tvsSideDetail::injectedStage=0;acutPrintf(L"\nNATIVE_DETAIL_FIXTURE_FAILED. Disposable test drawing may contain completed fixtures.");}}
void tvsBypassDetailCheckCommand(){
 try{auto used=tvsSideDetail::inventory(acdbHostApplicationServices()->workingDatabase());
  acutPrintf(L"\nBocni detalji: %u lokalnih referenci. %ls",static_cast<unsigned>(used.labels.size()),used.duplicated?L"DUPLIKATI: COPY nije nezavisni detalj; uklonite kopije.":L"Oznake/GUID-i bez otkrivenih duplikata. XREF i ponovljene parent instance nisu globalno provjereni.");
 }catch(...){acutPrintf(L"\nProvjera detalja nije zavrsena; zapis je mozda neispravan ili nedostupan.");}
}
static bool runBypass(const AcDbObjectId& active,bool permanent,TVSBypassOutput* output,const TVSBypassRadius& radius){
 try{
  AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();
  if(!db||!db->tilemode())return false;
#ifndef TVS_OFFSET_INTEGRATION
  if(permanent){acutPrintf(L"\nTrajni bypass zahtijeva upareni DEV DBX/ARX sa integracijom.");return false;}
#endif
  Snapshot s;AcDbObjectId route=active,obstacle;
  if(active.isNull()){if(!readPipe(L"\nIzaberite pravi horizontalni TVS kanal: ",s.route,route))return false;}
  else if(!readSegment(active,s.route))return false;
  if(!readPipe(L"\nIzaberite pravougaonu horizontalnu TVS prepreku: ",s.obstacle,obstacle)||route==obstacle)return false;
  bool reverse=false;
  if(active.isNull()){
   acedInitGet(0,L"Prvi Posljednji");ACHAR answer[80]=L"";
   int status=acedGetKword(L"\nSmjer obilaska [Prvi/Posljednji] <Posljednji>: ",answer);
   if(status!=RTNORM&&status!=RTNONE)return false;reverse=_wcsicmp(answer,L"Prvi")==0;
  }
#ifdef TVS_OFFSET_INTEGRATION
  tvsBypassService::Snapshot native;
  if(permanent&&(!tvsBypassService::capture(route,obstacle,reverse,native)||!readSegment(route,s.route)||!readSegment(obstacle,s.obstacle)||!tvsBypassService::current(native)))return false;
#endif
  bool known=false;AcDbDictionary* dict=NULL;
  if(db->getNamedObjectsDictionary(dict,AcDb::kForRead)==Acad::eOk){AcDbObjectId id;
   if(dict->getAt(tvsUnits::recordName(),id)==Acad::eOk){AcDbObjectPointer<AcDbXrecord> record(id,AcDb::kForRead);
    if(record.openStatus()==Acad::eOk){resbuf* data=NULL;if(record->rbChain(&data)==Acad::eOk){
     for(resbuf* p=data;p;p=p->rbnext)if(p->restype==40&&tvsUnits::validFactor(p->resval.rreal))known=true;
     if(data)acutRelRb(data);}}}dict->close();}
  tvsBypassUx::RadiusSetting radiusSetting;radiusSetting.mode=radius.mode;radiusSetting.k=radius.k;radiusSetting.constantDrawing=radius.constant;
  // Units always preselected from TVS: the drawing record, or TVS's own default (no record = mm,
  // e.g. TVS_SETTINGS left on mm does not write a record). The user still confirms them.
  Dialog dialog(s,reverse,tvsUnits::factor(db),true,DocVars.docData().bypassMemory,radiusSetting,permanent);
  dialog.unitFromRecord=known;dialog.unitDefault=!known;HWND owner=acedGetAcadFrame()->GetSafeHwnd();
  for(;;){INT_PTR action=dialog.show(owner);
   if(action==701){
    // Accepting flows: [Prihvati/Dijalog/Odustani]; TVS_BYPASS_PREVIEW: [Dijalog/Odustani].
    const bool canAccept=permanent&&dialog.canAccept;tvsBypassUx::PreviewChoice choice=tvsBypassUx::Cancel;
    {Transient preview;if(!preview.show(dialog.view,dialog.allEdges,dialog.showAxes))return false;
     acutPrintf(L"\nPrivremena osnova BEZ tacne vidljivosti/zaklanjanja. Crtice su simbolika, ne dokaz skrivenih ivica. Original ostaje vidljiv.");
     acedInitGet(0,canAccept?L"Prihvati Dijalog Odustani":L"Dijalog Odustani");ACHAR answer[64]=L"";
     const int status=acedGetKword(canAccept?L"\nPregled [Prihvati/Dijalog/Odustani] <Dijalog>: ":L"\nPregled [Dijalog/Odustani] <Dijalog>: ",answer);
     preview.clear(); // no transient survives Enter, a keyword or Esc
     choice=tvsBypassUx::previewChoice(status==RTNORM,status==RTNONE,answer,canAccept);}
    if(choice==tvsBypassUx::Cancel)return false;
    if(choice==tvsBypassUx::BackToDialog)continue; // same inputs and result
    action=704; // same checked candidate -> the one writer below, with its snapshot checks; no new search
   }
   if(action==703){if(dialog.view.ready&&dialog.engine.ready())tvsSideDetail::insert(db,dialog.view,dialog.options,route,obstacle);
    // Accepting flows (TVS_BYPASS, Obilazak) return to the dialog with the same checked
    // candidate; the static detail stays unlinked. Prihvati re-validates the snapshot.
    if(permanent)continue;
    return false;}
#ifdef TVS_OFFSET_INTEGRATION
   if(action==704&&permanent&&dialog.view.ready&&dialog.engine.ready()){
    tvsBypassService::Commit result;
    if(!tvsBypassService::write(native,dialog.view,dialog.options,result)){
     acutPrintf(L"\nBYPASS nije upisan (faza i razlog su navedeni iznad). Nema djelimicnog upisa.");return false;}
    if(output){output->id=result.output;output->pipe=result.resident;output->elevation=result.elevation;output->angle=result.angle;}
    acutPrintf(L"\nBYPASS upisan: cetiri koljena, dva kosa i H; dvije V1 trojke, jedan roditelj. Pocetna/izlazna kota ista.");
    return true;
   }
#endif
   return false;
  }
 }catch(...){acutPrintf(L"\nBYPASS prekinut. Nepotvrdjena transakcija/grafika je uklonjena.");return false;}
}
void tvsBypassPreviewCommand(const TVSBypassRadius& radius){runBypass(AcDbObjectId::kNull,false,NULL,radius);}
void tvsBypassCommand(const TVSBypassRadius& radius){runBypass(AcDbObjectId::kNull,true,NULL,radius);}
bool tvsBypassDuringDrawing(const AcDbObjectId& route,TVSBypassOutput& result,const TVSBypassRadius& radius){return runBypass(route,true,&result,radius);}
#endif
