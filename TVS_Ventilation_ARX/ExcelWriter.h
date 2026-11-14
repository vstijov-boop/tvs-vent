#pragma once
// TVS_PAD: copies a tvsSheet::Workbook (geometry/PressureSheet.h) into a new Excel workbook by
// late-bound COM (Excel.Application, as TVS_ExcelSp). One formula array per sheet (Range.Formula
// takes en-US formulas; "'" keeps text as text), formats by ranges, conditional fill, shapes.
#include "../geometry/PressureSheet.h"
#include <oleauto.h>
#include <string>
#include <vector>
namespace tvsExcel {
class Var {   // VARIANT holder
public:
    VARIANT v;
    Var(){VariantInit(&v);}
    explicit Var(const std::wstring& s){VariantInit(&v);v.vt=VT_BSTR;v.bstrVal=SysAllocString(s.c_str());}
    explicit Var(double d){VariantInit(&v);v.vt=VT_R8;v.dblVal=d;}
    explicit Var(long l){VariantInit(&v);v.vt=VT_I4;v.lVal=l;}
    explicit Var(bool b){VariantInit(&v);v.vt=VT_BOOL;v.boolVal=b?VARIANT_TRUE:VARIANT_FALSE;}
    static Var missing(){Var m;m.v.vt=VT_ERROR;m.v.scode=DISP_E_PARAMNOTFOUND;return m;}
    Var(const Var& o){VariantInit(&v);VariantCopy(&v,const_cast<VARIANT*>(&o.v));}
    Var& operator=(const Var& o){if(this!=&o){VariantClear(&v);VariantCopy(&v,const_cast<VARIANT*>(&o.v));}return *this;}
    ~Var(){VariantClear(&v);}
    IDispatch* disp()const{return v.vt==VT_DISPATCH?v.pdispVal:NULL;}
};
// args in the natural order (reversed for DISPPARAMS). LCID en-US (0x0409): Excel reads Formula and
// numbers in the locale of the call, so a user locale with ';' / ',' would reject IF(a,b,c).
inline HRESULT invoke(IDispatch* d,const wchar_t* name,WORD type,Var* result,const std::vector<Var>& args=std::vector<Var>()){
    if(!d)return E_POINTER;
    DISPID id;LPOLESTR n=const_cast<LPOLESTR>(name);
    HRESULT hr=d->GetIDsOfNames(IID_NULL,&n,1,0x0409,&id);if(FAILED(hr))return hr;
    std::vector<VARIANT> a(args.size());for(size_t i=0;i<args.size();++i)a[i]=args[args.size()-1-i].v;
    DISPPARAMS dp={a.empty()?NULL:&a[0],NULL,static_cast<UINT>(a.size()),0};DISPID put=DISPID_PROPERTYPUT;
    if(type&(DISPATCH_PROPERTYPUT|DISPATCH_PROPERTYPUTREF)){dp.cNamedArgs=1;dp.rgdispidNamedArgs=&put;}
    VARIANT out;VariantInit(&out);
    hr=d->Invoke(id,IID_NULL,0x0409,type,&dp,&out,NULL,NULL);
    if(result){VariantClear(&result->v);result->v=out;}else VariantClear(&out);
    return hr;
}
inline Var get(IDispatch* d,const wchar_t* name,const std::vector<Var>& args=std::vector<Var>()){Var r;invoke(d,name,DISPATCH_PROPERTYGET|DISPATCH_METHOD,&r,args);return r;}
inline HRESULT put(IDispatch* d,const wchar_t* name,const Var& value){return invoke(d,name,DISPATCH_PROPERTYPUT,NULL,std::vector<Var>(1,value));}
inline Var call(IDispatch* d,const wchar_t* name,const std::vector<Var>& args=std::vector<Var>()){Var r;invoke(d,name,DISPATCH_METHOD,&r,args);return r;}
inline Var range(IDispatch* sheet,const std::wstring& a1){return get(sheet,L"Range",std::vector<Var>(1,Var(a1)));}
inline void formatRange(IDispatch* sheet,const tvsSheet::Format& f){
    Var r=range(sheet,f.range);IDispatch* d=r.disp();if(!d)return;
    if(!f.number.empty())put(d,L"NumberFormat",Var(f.number));
    if(f.bold||f.italic||f.size||f.font>=0){Var font=get(d,L"Font");
        if(f.bold)put(font.disp(),L"Bold",Var(true));if(f.italic)put(font.disp(),L"Italic",Var(true));
        if(f.size)put(font.disp(),L"Size",Var(static_cast<long>(f.size)));if(f.font>=0)put(font.disp(),L"Color",Var(f.font));}
    if(f.fill>=0){Var in=get(d,L"Interior");put(in.disp(),L"Color",Var(f.fill));}
    if(f.align){const long a=f.align<0?-4131L:f.align==1?-4108L:-4152L;put(d,L"HorizontalAlignment",Var(a));put(d,L"VerticalAlignment",Var(-4108L));}
    if(f.wrap)put(d,L"WrapText",Var(true));
    if(f.border){Var b=get(d,L"Borders");put(b.disp(),L"LineStyle",Var(1L));put(b.disp(),L"Color",Var(tvsSheet::rgb(0xAE,0xB9,0xC7)));}
}
inline void cells(IDispatch* sheet,const tvsSheet::Sheet& s){
    const long rows=static_cast<long>(s.cells.size()),cols=s.columns();if(rows<1||cols<1)return;
    SAFEARRAYBOUND b[2];b[0].lLbound=1;b[0].cElements=rows;b[1].lLbound=1;b[1].cElements=cols;
    SAFEARRAY* a=SafeArrayCreate(VT_VARIANT,2,b);if(!a)return;
    for(long r=1;r<=rows;++r)for(long c=1;c<=cols;++c){const std::wstring t=s.at(r,c);if(t.empty())continue;
        Var x(t);long idx[2]={r,c};SafeArrayPutElement(a,idx,&x.v);}
    Var arr;arr.v.vt=VT_ARRAY|VT_VARIANT;arr.v.parray=a;
    Var r=range(sheet,L"A1:"+tvsSheet::cell(cols,rows));
    if(SUCCEEDED(put(r.disp(),L"Formula",arr)))return;
    // a refused cell stops the array: cell by cell, a bad one as text
    for(long row=1;row<=rows;++row)for(long c=1;c<=cols;++c){const std::wstring t=s.at(row,c);if(t.empty())continue;
        Var one=range(sheet,tvsSheet::cell(c,row));if(FAILED(put(one.disp(),L"Formula",Var(t))))put(one.disp(),L"Formula",Var(L"'"+t));}
}
inline void shapes(IDispatch* sheet,const tvsSheet::Sheet& s){
    if(s.strokes.empty()&&s.labels.empty())return;
    Var sh=get(sheet,L"Shapes");IDispatch* d=sh.disp();if(!d)return;
    for(size_t k=0;k<s.strokes.size();++k){const tvsSheet::Stroke& t=s.strokes[k];
        std::vector<Var> a;a.push_back(Var(t.x1));a.push_back(Var(t.y1));a.push_back(Var(t.x2));a.push_back(Var(t.y2));
        Var line=call(d,L"AddLine",a);if(!line.disp())continue;Var l=get(line.disp(),L"Line");
        Var fc=get(l.disp(),L"ForeColor");put(fc.disp(),L"RGB",Var(t.color));put(l.disp(),L"Weight",Var(t.weight));
        if(t.dash)put(l.disp(),L"DashStyle",Var(4L));}   // msoLineDash
    for(size_t k=0;k<s.labels.size();++k){const tvsSheet::Label& t=s.labels[k];
        std::vector<Var> a;a.push_back(Var(1L));a.push_back(Var(t.x));a.push_back(Var(t.y));a.push_back(Var(t.size*0.7*(t.text.size()+1)+4));a.push_back(Var(t.size*1.6));
        Var box=call(d,L"AddTextbox",a);IDispatch* bx=box.disp();if(!bx)continue;
        Var fill=get(bx,L"Fill");put(fill.disp(),L"Visible",Var(0L));Var ln=get(bx,L"Line");put(ln.disp(),L"Visible",Var(0L));
        Var tf=get(bx,L"TextFrame");put(tf.disp(),L"MarginLeft",Var(0.0));put(tf.disp(),L"MarginRight",Var(0.0));put(tf.disp(),L"MarginTop",Var(0.0));put(tf.disp(),L"MarginBottom",Var(0.0));
        Var ch=call(tf.disp(),L"Characters");put(ch.disp(),L"Text",Var(t.text));
        Var font=get(ch.disp(),L"Font");put(font.disp(),L"Size",Var(t.size));put(font.disp(),L"Color",Var(t.color));if(t.bold)put(font.disp(),L"Bold",Var(true));}
}
// false: Excel not available. The workbook is left open and visible (not saved).
inline bool write(const tvsSheet::Workbook& wb){
    const HRESULT init=CoInitialize(NULL);
    CLSID clsid;if(FAILED(CLSIDFromProgID(L"Excel.Application",&clsid))){if(SUCCEEDED(init))CoUninitialize();return false;}
    IDispatch* app=NULL;if(FAILED(CoCreateInstance(clsid,NULL,CLSCTX_LOCAL_SERVER,IID_IDispatch,reinterpret_cast<void**>(&app)))||!app){if(SUCCEEDED(init))CoUninitialize();return false;}
    {   // every Var released before CoUninitialize
    put(app,L"ScreenUpdating",Var(false));
    Var old=get(app,L"SheetsInNewWorkbook");put(app,L"SheetsInNewWorkbook",Var(static_cast<long>(wb.sheets.size())));
    Var books=get(app,L"Workbooks");Var book=call(books.disp(),L"Add");
    if(old.v.vt!=VT_EMPTY)put(app,L"SheetsInNewWorkbook",old);
    Var sheets=get(book.disp(),L"Worksheets");
    put(app,L"DisplayAlerts",Var(false));
    // every sheet named first: a formula on Dionice that refers to Fitinzi before it exists makes
    // Excel ask for an external file ("Update Values: Fitinzi")
    for(size_t i=0;i<wb.sheets.size();++i){Var sheet=get(sheets.disp(),L"Item",std::vector<Var>(1,Var(static_cast<long>(i+1))));if(sheet.disp())put(sheet.disp(),L"Name",Var(wb.sheets[i].name));}
    for(size_t i=0;i<wb.sheets.size();++i){const tvsSheet::Sheet& s=wb.sheets[i];
        Var sheet=get(sheets.disp(),L"Item",std::vector<Var>(1,Var(static_cast<long>(i+1))));IDispatch* d=sheet.disp();if(!d)continue;
        cells(d,s);
        for(size_t k=0;k<s.formats.size();++k)formatRange(d,s.formats[k]);
        for(size_t k=0;k<s.widths.size();++k){Var c=get(d,L"Columns",std::vector<Var>(1,Var(static_cast<long>(k+1))));put(c.disp(),L"ColumnWidth",Var(s.widths[k]));}
        for(size_t k=0;k<s.hidden.size();++k){Var c=get(d,L"Columns",std::vector<Var>(1,Var(static_cast<long>(s.hidden[k]))));put(c.disp(),L"Hidden",Var(true));}
        for(size_t k=0;k<s.conditions.size();++k){Var r=range(d,s.conditions[k].range);Var fcs=get(r.disp(),L"FormatConditions");
            std::vector<Var> a;a.push_back(Var(2L));a.push_back(Var::missing());a.push_back(Var(s.conditions[k].formula));
            Var fc=call(fcs.disp(),L"Add",a);Var in=get(fc.disp(),L"Interior");put(in.disp(),L"Color",Var(s.conditions[k].fill));}
        shapes(d,s);
        Var ps=get(d,L"PageSetup");put(ps.disp(),L"Orientation",Var(2L));put(ps.disp(),L"Zoom",Var(false));put(ps.disp(),L"FitToPagesWide",Var(1L));if(s.onePage)put(ps.disp(),L"FitToPagesTall",Var(1L));else put(ps.disp(),L"FitToPagesTall",Var(false));
        if(!s.freeze.empty()){call(d,L"Activate");Var c=range(d,s.freeze);call(c.disp(),L"Select");Var w=get(app,L"ActiveWindow");put(w.disp(),L"FreezePanes",Var(true));}
    }
    Var first=get(sheets.disp(),L"Item",std::vector<Var>(1,Var(1L)));call(first.disp(),L"Activate");
    put(app,L"DisplayAlerts",Var(true));put(app,L"ScreenUpdating",Var(true));put(app,L"Visible",Var(true));put(app,L"UserControl",Var(true));
    }
    app->Release();
    if(SUCCEEDED(init))CoUninitialize();
    return true;
}
}
