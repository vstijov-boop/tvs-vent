#pragma once
// TVS_OZNAKE: printed duct labels (size/flow, TVS_Ventilation_DBX/DuctLabel.h) on/off per drawing,
// default position (iznad / sredina / ispod), drawing scale for the text height, and the position
// of single labels (xdata TVS_OZNAKA on the duct). Makes layer and text style AT-DIM_KANAL.
#include "../TVS_Ventilation_DBX/DuctLabel.h"
#include "MLeaderSettings.h"
namespace tvsLabelCmd {
inline const wchar_t* positionName(int p){return p==tvsDuctLabel::Below?L"ispod kanala":p==tvsDuctLabel::Middle?L"na sredini kanala":L"iznad kanala";}
inline void ensureLayerAndStyle(AcDbDatabase* db){
    {AcDbLayerTable* t=NULL;if(db->getLayerTable(t,AcDb::kForRead)==Acad::eOk&&t){
        if(!t->has(tvsDuctLabel::layerName())&&t->upgradeOpen()==Acad::eOk){AcDbLayerTableRecord* r=new AcDbLayerTableRecord();r->setName(tvsDuctLabel::layerName());
            AcCmColor c;c.setColorIndex(7);r->setColor(c);if(t->add(r)==Acad::eOk)r->close();else delete r;}
        t->close();}}
    {AcDbTextStyleTable* t=NULL;if(db->getTextStyleTable(t,AcDb::kForRead)==Acad::eOk&&t){
        if(!t->has(tvsDuctLabel::layerName())&&t->upgradeOpen()==Acad::eOk){AcDbTextStyleTableRecord* r=new AcDbTextStyleTableRecord();r->setName(tvsDuctLabel::layerName());
            r->setFileName(L"romans.shx");r->setXScale(0.7);r->setTextSize(0);if(t->add(r)==Acad::eOk)r->close();else delete r;}
        t->close();}}
}
inline void regen(){AcApDocument* doc=acDocManager?acDocManager->curDocument():NULL;if(doc)acDocManager->sendStringToExecute(doc,L"_.REGENALL\n",false,true,false);}
inline int askPosition(const wchar_t* prompt,bool withDefault){
    acedInitGet(0,withDefault?L"Iznad Sredina Ispod Podrazumijevano":L"Iznad Sredina Ispod");ACHAR kw[32]=L"";
    const int k=acedGetKword(prompt,kw,32);if(k!=RTNORM)return -2;
    if(!_wcsicmp(kw,L"Iznad"))return tvsDuctLabel::Above;if(!_wcsicmp(kw,L"Sredina"))return tvsDuctLabel::Middle;if(!_wcsicmp(kw,L"Ispod"))return tvsDuctLabel::Below;
    return tvsDuctLabel::Default;
}
inline void single(AcDbDatabase* db){
    acutPrintf(L"\nIzaberite kanale čiji natpis se pomjera: ");
    ads_name ss;resbuf* filter=acutBuildList(RTDXF0,L"TVS_PIPE",RTNONE);const int rc=acedSSGet(NULL,NULL,NULL,filter,ss);acutRelRb(filter);
    if(rc!=RTNORM){acutPrintf(L"\nNišta nije izabrano.");return;}
    Adesk::Int32 n=0;acedSSLength(ss,&n);
    const int p=askPosition(L"\nNatpis [Iznad/Sredina/Ispod/Podrazumijevano] <Iznad>: ",true);
    const int pos=p==-2?tvsDuctLabel::Above:p;
    acdbRegApp(tvsDuctLabel::app());int done=0;
    for(Adesk::Int32 i=0;i<n;++i){ads_name e;if(acedSSName(ss,i,e)!=RTNORM)continue;AcDbObjectId id;if(acdbGetObjectId(id,e)!=Acad::eOk)continue;
        AcDbObjectPointer<AcDbEntity> o(id,AcDb::kForWrite);if(o.openStatus()!=Acad::eOk)continue;
        resbuf* x=pos==tvsDuctLabel::Default?acutBuildList(1001,tvsDuctLabel::app(),RTNONE):acutBuildList(1001,tvsDuctLabel::app(),1070,pos,RTNONE);
        if(x&&o->setXData(x)==Acad::eOk){o->recordGraphicsModified();++done;}if(x)acutRelRb(x);}
    acedSSFree(ss);
    acutPrintf(L"\nTVS_OZNAKE: %d natpisa %ls.",done,pos==tvsDuctLabel::Default?L"vraćeno na podrazumijevani položaj":positionName(pos));
    (void)db;
}
inline void execute(){
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();if(!db)return;
    tvsDuctLabel::Settings s=tvsDuctLabel::read(db);
    if(!s.on){MLeaderSettings m;bool known=false;{AcDbDictionary* nod=NULL;if(db->getNamedObjectsDictionary(nod,AcDb::kForRead)==Acad::eOk&&nod){known=nod->has(tvsDuctLabel::key());nod->close();}}
        if(!known)s.scale=m.scale;}   // the first time: the scale of the TVS labels
    acutPrintf(L"\nTVS_OZNAKE: %ls; položaj %ls; razmjera 1:%.0f (tekst %.1f mm na papiru = %.0f mm).",s.on?L"uključene":L"isključene",positionName(s.position),s.scale,tvsDuctLabel::paperMm(),tvsDuctLabel::paperMm()*s.scale);
    acedInitGet(0,s.on?L"Iskljuci Polozaj Razmjera Pojedinacno":L"Ukljuci Polozaj Razmjera Pojedinacno");ACHAR kw[32]=L"";
    const int k=acedGetKword(s.on?L"\nOznake kanala [Iskljuci/Polozaj/Razmjera/Pojedinacno] <Iskljuci>: ":L"\nOznake kanala [Ukljuci/Polozaj/Razmjera/Pojedinacno] <Ukljuci>: ",kw,32);
    if(k==RTCAN||k==RTERROR)return;
    if(k==RTNONE||!_wcsicmp(kw,L"Ukljuci")||!_wcsicmp(kw,L"Iskljuci")){s.on=!s.on;}
    else if(!_wcsicmp(kw,L"Polozaj")){const int p=askPosition(L"\nPodrazumijevani položaj [Iznad/Sredina/Ispod] <Iznad>: ",false);if(p==RTCAN)return;s.position=p<0?tvsDuctLabel::Above:p;}
    else if(!_wcsicmp(kw,L"Razmjera")){wchar_t pr[80];swprintf_s(pr,80,L"\nRazmjera 1: <%.0f>: ",s.scale);acedInitGet(RSG_NOZERO|RSG_NONEG,NULL);double v=s.scale;
        const int g=acedGetReal(pr,&v);if(g==RTCAN)return;if(g==RTNORM&&v>0&&v<=100000)s.scale=v;}
    else if(!_wcsicmp(kw,L"Pojedinacno")){single(db);return;}
    if(s.on)ensureLayerAndStyle(db);
    if(!tvsDuctLabel::write(db,s)){acutPrintf(L"\nTVS_OZNAKE: podešavanje nije upisano.");return;}
    acutPrintf(L"\nTVS_OZNAKE: %ls; položaj %ls; razmjera 1:%.0f.",s.on?L"uključene (sloj AT-DIM_KANAL)":L"isključene",positionName(s.position),s.scale);
    regen();
}
}
