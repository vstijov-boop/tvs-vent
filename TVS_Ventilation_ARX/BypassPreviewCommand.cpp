#include "StdAfx.h"
#ifdef TVS_BYPASS_PREVIEW
#include "BypassPreviewCommand.h"
#include "../geometry/bypass/PreviewModel.h"
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
 bool show(const View& v){
  if(!manager||!v.ready)return false;
  clear();lines.reserve(v.plan.size()); // Allocate before registering drawables: cleanup cannot lose a pointer.
  try {for(const auto& c:v.plan){
   std::unique_ptr<AcDbPolyline> p(new AcDbPolyline());
   for(unsigned j=0;j<c.points.size();++j){Point w=v.frame.world(c.points[j].s,c.points[j].z);
    if(p->addVertexAt(j,AcGePoint2d(w.x,w.y))!=Acad::eOk){clear();return false;}}
   p->setElevation(v.frame.origin.z);p->setColorIndex(c.member==7?1:c.insulation?8:3);
   // Never append to a block table, transaction, dictionary or DWG database.
   if(!manager->addTransient(p.get(),kAcGiDirectShortTerm,128,viewports)){clear();return false;}
   lines.push_back(p.release());
  }}catch(...){clear();throw;}
  return true;
 }
};
static const int FieldCount=26;
static double Options::* const fields[FieldCount]={&Options::ri,&Options::insulation,&Options::obstacleInsulation,&Options::clearance,&Options::reserve,
 &Options::minSlope,&Options::minBridge,&Options::minProjection,&Options::left,&Options::right,&Options::floor,&Options::ceiling,
 &Options::timeMs,&Options::calls,&Options::cells,&Options::depthStep,&Options::bridgeStep,&Options::maxDepth,&Options::depthLevels,
 &Options::bridgeLevels,&Options::positions,&Options::seedSolves,&Options::proposals,&Options::inset,&Options::wTolerance,&Options::dTolerance};
static const wchar_t* labels[FieldCount]={L"Ri, mm (ručno)",L"Izolacija trase, mm *",L"Izolacija prepreke, mm *",L"Obavezan razmak, mm",L"Projektna rezerva, mm",
 L"Min. nagibni komad, mm",L"Min. most C, mm",L"Min. projekcija, mm",L"Prozor s lijevo, mm",L"Prozor s desno, mm",L"Donja granica, mm (op.)",L"Gornja granica, mm (op.)",
 L"Vrijeme, ms (max 10000)",L"Checker pozivi (max 128)",L"Ćelije (max 1000000)",L"Korak D, mm",L"Korak C, mm",L"Max. traženi D, mm",L"Nivoi D (max 128)",L"Prijedlozi C (max 16)",L"Položaji (max 16)",L"Seed solve (max 128)",L"Prijedlozi (max 2048)",L"Inset prozora, mm",L"Prag poboljšanja W, mm",L"Prag poboljšanja D, mm"};
struct Dialog {
 Snapshot snapshot;bool reverse,loading,unitFromRecord;double scaleX,scaleY;Engine engine;View view;Options options;
 std::wstring values[FieldCount];int units,angle,side;bool confirmed;HWND window;HFONT font;
 Dialog(const Snapshot& s,bool r,double factor,bool known):snapshot(s),reverse(r),loading(true),unitFromRecord(known),scaleX(1),scaleY(1),units(known?(factor==1?0:factor==10?1:2):-1),angle(1),side(0),confirmed(false),window(NULL),font(NULL){
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
  t<<L"Jedinice: "<<(units<0?L"nedostaje":unitFromRecord?L"iz TVS zapisa, potrebna potvrda":L"ručno, potrebna potvrda")<<L". ";
  t<<L"Iz objekata: osne tačke, A/B, fizičke kote ose, WCS smjerovi i crtački Z.\r\n";
  if(f.valid){t<<L"Trasa "<<f.width<<L" × "<<f.height<<L" mm; osa Z="<<f.z0<<L" mm. Prepreka "<<f.obstacleWidth<<L" × "<<f.obstacleHeight<<L"; osa Z="<<f.obstacleZ<<L" mm.\r\n";
   t<<L"Smjer s=("<<f.ux<<L", "<<f.uy<<L"); crtački Z="<<f.origin.z<<L" jedinica. s=0 je presjek stvarnih osa. +s prati izabrani aktivni kraj.";}
  else t<<L"Nedostaje potvrda jedinica ili nepodržani objekti: "<<wide(f.reason);
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
   std::wstring result=wide(view.status)+L"\r\n"+wide(view.details);SetDlgItemTextW(window,800,result.c_str());}
  catch(...){invalidate();SetDlgItemTextW(window,800,L"Greška računanja. Nema DWG izmjena.");}
  EnableWindow(GetDlgItem(window,700),TRUE);EnableWindow(GetDlgItem(window,702),TRUE);
  EnableWindow(GetDlgItem(window,701),view.ready&&engine.ready());InvalidateRect(window,NULL,TRUE);
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
   child(L"STATIC",labels[j],0,0,x,y+3,172,22);child(L"EDIT",values[j].c_str(),ES_AUTOHSCROLL|WS_TABSTOP,100+j,x+174,y,92,24);}
  child(L"STATIC",L"* Nedostaje: unesite izolacije. Izvor ostalih polja: ručno/predlog.\r\nProzor: fizički mm od presjeka osa; početni predlog iz krajeva trase.\r\nGranice: apsolutne kote spoljnih gabarita; prazno = neaktivno.",0,0,12,510,550,58);
  child(L"BUTTON",L"Potvrđujem jedinice, izolacije, ručne ulaze i razvojne budžete",BS_AUTOCHECKBOX|WS_TABSTOP,503,12,576,555,25);CheckDlgButton(h,503,confirmed?BST_CHECKED:BST_UNCHECKED);
  child(L"BUTTON",L"Pronađi prolaz",BS_PUSHBUTTON|WS_TABSTOP,700,12,616,150,30);
  child(L"BUTTON",L"Pokušaj kraći prolaz",BS_PUSHBUTTON|WS_TABSTOP,702,174,616,170,30);
  child(L"BUTTON",L"Pregled u crtežu",BS_PUSHBUTTON|WS_TABSTOP,701,356,616,165,30);EnableWindow(GetDlgItem(h,701),view.ready&&engine.ready());
  child(L"BUTTON",L"Umetni (nedostupno)",BS_PUSHBUTTON,703,12,666,180,30);EnableWindow(GetDlgItem(h,703),FALSE);
  child(L"BUTTON",L"Otkaži / zatvori",BS_PUSHBUTTON|WS_TABSTOP,IDCANCEL,348,666,173,30);
  child(L"STATIC",L"Uzdužni vertikalni presjek — jednaka skala s i z",0,0,590,94,590,20);
  child(L"EDIT",view.ready?(wide(view.status)+L"\r\n"+wide(view.details)).c_str():L"Rezultat nije izračunat. Nema trajnog upisa.",ES_MULTILINE|ES_READONLY|WS_VSCROLL,800,590,423,590,244);
  child(L"STATIC",L"Razvojni pregled — upis nije implementiran.\r\nSivo: izolacija; boje: lim; crveno: prepreka; crta: osa/zazor.\r\nRačuna se samo pritiskom tastera; tokom proračuna UI čeka.",0,0,590,673,590,42);
  loading=false;
 }
 void paint(){PAINTSTRUCT ps;HDC dc=BeginPaint(window,&ps);RECT area={(LONG)(590*scaleX),(LONG)(118*scaleY),(LONG)(1180*scaleX),(LONG)(412*scaleY)};FillRect(dc,&area,(HBRUSH)GetStockObject(WHITE_BRUSH));
  if(view.ready){double sl=1e100,sr=-1e100,zl=1e100,zr=-1e100;
   for(const auto& c:view.profile)for(const auto& p:c.points){sl=std::fmin(sl,p.s);sr=std::fmax(sr,p.s);zl=std::fmin(zl,p.z);zr=std::fmax(zr,p.z);}
   double scale=std::fmin((area.right-area.left-30)/(sr-sl+1),(area.bottom-area.top-30)/(zr-zl+1));
   double x=area.left+(area.right-area.left-(sr-sl)*scale)/2,y=area.top+(area.bottom-area.top-(zr-zl)*scale)/2;
   for(const auto& c:view.profile){HPEN pen=CreatePen(c.insulation?PS_DOT:PS_SOLID,c.member==9?2:1,c.member==7?RGB(200,35,35):c.insulation?RGB(140,140,140):c.member>=8?RGB(80,80,80):RGB(20,90,160));
    HGDIOBJ old=SelectObject(dc,pen);bool first=true;for(const auto& p:c.points){int px=(int)(x+(p.s-sl)*scale),py=(int)(y+(zr-p.z)*scale);
     if(first)MoveToEx(dc,px,py,NULL);else LineTo(dc,px,py);first=false;}SelectObject(dc,old);DeleteObject(pen);}
   SetBkMode(dc,TRANSPARENT);SelectObject(dc,font);std::wstring label=L"s [mm], z [mm]; W="+number(view.W)+L"; Zprolaza="+number(view.zPass);TextOutW(dc,area.left+6,area.top+4,label.c_str(),(int)label.size());
  }EndPaint(window,&ps);
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
   if(!preview.show(dialog.view)){acutPrintf(L"\nTransient pregled nije moguće prikazati. Nema DWG izmjena.");return;}
   acutPrintf(L"\nPrivremena OSNOVA: projekcije istih sedam tijela i spojeva; njihovo preklapanje u osnovi je očekivano. Nema novih DWG entiteta.");
   ACHAR buffer[256];int r=acedGetString(0,L"\nPregled: Enter vraća dijalog; Esc završava i uklanja pregled: ",buffer);
   preview.clear();if(r!=RTNORM&&r!=RTNONE)break;
  }
 }catch(...){acutPrintf(L"\nGreška razvojnog pregleda. Privremena grafika je uklonjena; nema DWG upisa.");}
}
#endif
