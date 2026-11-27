#pragma once
// TVS_PAD: the Excel workbook as plain data (geometry/PressureDrop.h result -> sheets). The COM
// writer (TVS_Ventilation_ARX/PressureCommand.h) only copies it: one formula array per sheet,
// formats by ranges, conditional fill, shapes. tests/test_pressure_drop.cpp.
//   "Dionice": friction and Z by formulas (Excel recalculates), lambda as a value (Colebrook),
//   Re hidden; "Dp kraj. elem." typed by the user (yellow); Dp od jedinice cumulative; a hidden
//   helper column carries the largest cumulative value below each dionica, so "Kriticna" and
//   "Visak" (Pa for the damper) follow the user's terminal values.
//   "Fitinzi": zeta on the pd of the dionica (editable; Dionice sums it), ASHRAE key and C.
//   "Sema": single-line plan of the network (flex dashed), dionica numbers, nodes; the critical
//   branch red as exported (terminal Dp = 0).
//   Oprema: Fitinzi rows with Izvor "Oprema" (kind name, tag); inline: zeta on the pd of the
//   dionica as every row; terminal: no zeta, its dp in H (value), "Dp kraj. elem." of the dionica
//   = that cell and its tag is the end label in Sema. "Za ventilator": the reserve is not applied
//   to "Dp kraj. elem." (max over the rows of (cum - kraj) * (1 + rez) + kraj).
//   Resetka na dnu: its tap junction rows (branch / prolaz) have Izvor "Idelchik" like the other
//   Idelchik T rows, the grille row "Oprema"; in Sema the branch is a short stub with the tag.
#include "PressureDrop.h"
#include <string>
#include <vector>
namespace tvsSheet {
struct Format {std::wstring range,number;bool bold,italic,wrap,border;int size;long font,fill;int align;   // align: 0 general, -1 left, 1 centre, 2 right
    Format():bold(false),italic(false),wrap(false),border(false),size(0),font(-1),fill(-1),align(0){}};
struct Condition {std::wstring range,formula;long fill;};
struct Stroke {double x1,y1,x2,y2,weight;long color;bool dash;};
struct Label {double x,y;std::wstring text;double size;long color;bool bold;};
struct Sheet {
    std::wstring name;std::vector<std::vector<std::wstring> > cells;   // "=" formula, "'" text, else number
    std::vector<double> widths;std::vector<int> hidden;std::vector<Format> formats;std::vector<Condition> conditions;
    std::wstring freeze;std::vector<Stroke> strokes;std::vector<Label> labels;bool onePage;   // print: one page (else one page wide)
    Sheet():onePage(false){}
    void put(int row,int col,const std::wstring& v){if(row<1||col<1)return;if(static_cast<int>(cells.size())<row)cells.resize(row);
        std::vector<std::wstring>& r=cells[row-1];if(static_cast<int>(r.size())<col)r.resize(col);r[col-1]=v;}
    std::wstring at(int row,int col)const{return row>=1&&row<=static_cast<int>(cells.size())&&col>=1&&col<=static_cast<int>(cells[row-1].size())?cells[row-1][col-1]:std::wstring();}
    int columns()const{size_t c=0;for(size_t i=0;i<cells.size();++i)if(cells[i].size()>c)c=cells[i].size();return static_cast<int>(c);}
    void format(const Format& f){formats.push_back(f);}
};
struct Workbook {std::vector<Sheet> sheets;int netRow,fanRow;Workbook():netRow(0),fanRow(0){}};   // Dionice: rows of "Dp mreze" / "Za ventilator" (column D)
inline long rgb(int r,int g,int b){return r|(g<<8)|(b<<16);}   // Excel COLORREF (BGR)
inline std::wstring col(int c){std::wstring s;while(c>0){const int m=(c-1)%26;s.insert(s.begin(),static_cast<wchar_t>(L'A'+m));c=(c-1)/26;}return s;}
inline std::wstring cell(int c,int r){return col(c)+std::to_wstring(r);}
inline std::wstring text(const std::wstring& s){return L"'"+s;}
inline std::wstring num(double v,int decimals){wchar_t b[64];swprintf(b,64,L"%.*f",decimals,v);std::wstring s=b;if(s==L"-0"||s.compare(0,3,L"-0.")==0){bool zero=true;for(size_t i=1;i<s.size();++i)if(s[i]!=L'0'&&s[i]!=L'.')zero=false;if(zero)s.erase(0,1);}return s;}
struct Info {std::wstring project,system,date,source;};
// lambda in Excel: laminar 64/Re, else Colebrook-White by Serghides (1984) explicit form (as lambdaOf)
inline std::wstring lambdaFormula(const std::wstring& re,const std::wstring& k,const std::wstring& dh){
    const std::wstring e=L"("+k+L"/(3.7*"+dh+L"))";
    const std::wstring A=L"(-2*LOG10("+e+L"+12/"+re+L"))";
    const std::wstring B=L"(-2*LOG10("+e+L"+2.51*"+A+L"/"+re+L"))";
    const std::wstring C=L"(-2*LOG10("+e+L"+2.51*"+B+L"/"+re+L"))";
    return L"=IF("+re+L"<=0,0,IF("+re+L"<2300,64/"+re+L",1/("+A+L"-("+B+L"-"+A+L")^2/("+C+L"-2*"+B+L"+"+A+L"))^2))";
}
// columns of "Dionice"
enum {cNo=1,cFrom,cTo,cQ,cDim,cA,cB,cDh,cL,cK,cV,cPd,cRe,cLambda,cR,cRL,cZeta,cZ,cTerm,cDp,cCum,cMax,cOver,cCrit,cLast=cCrit};
// tag: suffix of the sheet names (ventilator u mrezi: "usis" / "potis" - two sets in one workbook)
inline Workbook build(const tvsPressure::Result& r,const Info& info,const tvsPressure::Options& o,const std::wstring& tag=std::wstring()){
    Workbook wb;wb.sheets.resize(3);
    const std::wstring nD=tag.empty()?L"Dionice":L"Dionice "+tag,nF=tag.empty()?L"Fitinzi":L"Fitinzi "+tag,nS=tag.empty()?L"\u0160ema":L"\u0160ema "+tag;
    const std::wstring rD=tag.empty()?nD+L"!":L"'"+nD+L"'!",rF=tag.empty()?nF+L"!":L"'"+nF+L"'!";
    const long navy=rgb(0x1F,0x3A,0x5F),blue=rgb(0x31,0x59,0x7F),grey=rgb(0x5B,0x6B,0x7F),yellow=rgb(0xFF,0xF4,0xC2),pink=rgb(0xFD,0xEC,0xEA),red=rgb(0xC0,0x39,0x2B);
    // ---- Dionice ----
    Sheet& s=wb.sheets[0];s.name=nD;
    s.put(1,1,text(L"TVS_VENT \u00B7 Pad pritiska po dionicama"));{Format f;f.range=L"A1";f.bold=true;f.size=14;f.font=navy;s.format(f);}
    const int tRow=6,rhoRow=7,nuRow=8;   // t typed by the user (smoke extraction: hot gas); rho, nu from t
    const std::wstring infoRows[][2]={{L"Projekat",info.project},{L"Sistem",info.system},{L"Datum",info.date}};
    for(int i=0;i<3;++i){s.put(3+i,1,text(infoRows[i][0]));s.put(3+i,2,text(infoRows[i][1]));}
    const std::wstring T=L"($B$"+std::to_wstring(tRow)+L"+273.15)";
    s.put(tRow,1,text(L"t vazduha, \u00B0C"));s.put(tRow,2,num(o.temperature,0));s.put(tRow,3,text(L"\u2190 upi\u0161ite (odimljavanje: temperatura dima); \u03C1 i \u03BD se ra\u010Dunaju, Q je pri toj temperaturi"));
    s.put(rhoRow,1,text(L"\u03C1, kg/m\u00B3"));s.put(rhoRow,2,L"=101325/(287.05*"+T+L")");
    s.put(nuRow,1,text(L"\u03BD, 10\u207B\u2076 m\u00B2/s"));s.put(nuRow,2,L"=1.458E-6*"+T+L"^1.5/("+T+L"+110.4)/$B$"+std::to_wstring(rhoRow)+L"*1E6");
    {Format f;f.range=cell(2,tRow);f.number=L"0";f.align=-1;f.fill=yellow;f.bold=true;s.format(f);}
    {Format f;f.range=cell(3,tRow);f.italic=true;f.size=9;f.font=grey;s.format(f);}
    {Format f;f.range=cell(2,nuRow);f.number=L"0.00";f.align=-1;s.format(f);}{Format f;f.range=cell(2,rhoRow);f.number=L"0.000";f.align=-1;s.format(f);}
    s.put(9,1,text(L"Prora\u010Dun"));s.put(9,2,text(L"R: Darcy-Weisbach, \u03BB Colebrook (Serghides); \u03C1 = p/(R\u00B7T), \u03BC Sutherland (k lim "+num(o.rough,2)+L" mm, fleks "+num(o.flexRough,1)+L" mm); Z = \u03A3\u03B6 \u00B7 pd; \u03B6: ASHRAE DFDB"+(info.source.empty()?L"":L" ("+info.source+L")")));
    {Format f;f.range=L"A3:A9";f.bold=true;f.size=10;f.font=blue;s.format(f);}
    const int h=11,first=h+1,last=h+static_cast<int>(r.d.size());
    const wchar_t* head[]={L"Dionica",L"Od",L"Do",L"Q\nm\u00B3/h",L"Dimenzija\nmm",L"a\nmm",L"b\nmm",L"Dh\nmm",L"L\nm",L"k\nmm",L"v\nm/s",L"pd\nPa",L"Re",L"\u03BB",L"R\nPa/m",L"R\u00B7L\nPa",L"\u03A3\u03B6",L"Z\nPa",L"\u0394p kraj.\nelem. Pa",L"\u0394p\ndionice Pa",L"\u0394p od\njedinice Pa",L"max\nispod",L"Vi\u0161ak\nPa",L"Kriti\u010Dna"};
    for(int c=cNo;c<=cLast;++c)s.put(h,c,text(head[c-1]));
    {Format f;f.range=cell(cNo,h)+L":"+cell(cLast,h);f.bold=true;f.font=rgb(255,255,255);f.fill=navy;f.align=1;f.wrap=true;f.border=true;f.size=10;s.format(f);}
    // Fitinzi rows: by dionica number (stable); terminal oprema -> its row (Dp kraj. elem.)
    std::vector<size_t> order;for(size_t k=0;k<r.f.size();++k)order.push_back(k);
    for(size_t a=1;a<order.size();++a){const size_t x=order[a];size_t j=a;while(j>0&&r.d[r.f[order[j-1]].dionica].number>r.d[r.f[x].dionica].number){order[j]=order[j-1];--j;}order[j]=x;}
    std::vector<int> termRow(r.d.size(),0);std::vector<double> termDp(r.d.size(),0);
    for(size_t k=0;k<order.size();++k){const tvsPressure::Fitting& x=r.f[order[k]];
        if(x.equipment&&x.terminal&&x.ok&&x.dionica>=0&&x.dionica<static_cast<int>(r.d.size())){termRow[x.dionica]=2+static_cast<int>(k);termDp[x.dionica]=x.dp;}}
    // the critical branch as exported (terminal dp = 0, oprema: its dp): cumulative per dionica
    std::vector<double> cum(r.d.size(),0);
    for(size_t i=0;i<r.d.size();++i){const tvsPressure::Dionica& d=r.d[i];double z=0;
        for(size_t k=0;k<r.f.size();++k)if(r.f[k].dionica==static_cast<int>(i))z+=r.f[k].zeta;
        cum[i]=d.r*d.length+z*d.pd+termDp[i]+(d.parent>=0?cum[d.parent]:0);}
    const std::wstring dpNet=L"$D$"+std::to_wstring(last+3);
    for(size_t i=0;i<r.d.size();++i){const tvsPressure::Dionica& d=r.d[i];const int row=first+static_cast<int>(i);const std::wstring R=std::to_wstring(row);
        s.put(row,cNo,num(d.number,0));s.put(row,cFrom,text(d.from));s.put(row,cTo,text(d.to));s.put(row,cQ,num(d.flow,0));
        s.put(row,cDim,text(tvsPressure::dimText(d.s)));s.put(row,cA,num(d.s.a,0));if(!d.s.round)s.put(row,cB,num(d.s.b,0));
        s.put(row,cDh,d.s.round?L"="+cell(cA,row):L"=2*"+cell(cA,row)+L"*"+cell(cB,row)+L"/("+cell(cA,row)+L"+"+cell(cB,row)+L")");
        s.put(row,cL,num(d.length,2));s.put(row,cK,num(d.rough,2));
        s.put(row,cV,d.s.round?L"="+cell(cQ,row)+L"/3600/(PI()*"+cell(cA,row)+L"^2/4000000)":L"="+cell(cQ,row)+L"/3600/("+cell(cA,row)+L"*"+cell(cB,row)+L"/1000000)");
        s.put(row,cPd,L"=$B$"+std::to_wstring(rhoRow)+L"/2*"+cell(cV,row)+L"^2");
        s.put(row,cRe,L"="+cell(cV,row)+L"*"+cell(cDh,row)+L"/1000/($B$"+std::to_wstring(nuRow)+L"*1E-6)");
        s.put(row,cLambda,lambdaFormula(cell(cRe,row),cell(cK,row),cell(cDh,row)));   // follows t (Re)
        s.put(row,cR,L"=IF("+cell(cDh,row)+L">0,"+cell(cLambda,row)+L"/("+cell(cDh,row)+L"/1000)*"+cell(cPd,row)+L",0)");
        s.put(row,cRL,L"="+cell(cR,row)+L"*"+cell(cL,row));
        s.put(row,cZeta,L"=SUMIF("+rF+L"$A:$A,"+cell(cNo,row)+L","+rF+L"$E:$E)");
        s.put(row,cZ,L"="+cell(cZeta,row)+L"*"+cell(cPd,row));
        if(d.terminal)s.put(row,cTerm,termRow[i]?L"="+rF+L"$H$"+std::to_wstring(termRow[i]):std::wstring(L"0"));
        s.put(row,cDp,L"="+cell(cRL,row)+L"+"+cell(cZ,row)+L"+"+cell(cTerm,row));
        s.put(row,cCum,d.parent<0?L"="+cell(cDp,row):L"="+cell(cDp,row)+L"+"+cell(cCum,first+d.parent));
        std::wstring mx=L"=MAX("+cell(cCum,row);for(size_t k=0;k<d.children.size();++k)mx+=L","+cell(cMax,first+d.children[k]);s.put(row,cMax,mx+L")");
        if(d.parent>=0&&r.d[d.parent].children.size()>1){std::wstring m=L"=MAX(";const std::vector<int>& sib=r.d[d.parent].children;
            for(size_t k=0;k<sib.size();++k)m+=(k?L",":L"")+cell(cMax,first+sib[k]);s.put(row,cOver,m+L")-"+cell(cMax,row));}
        s.put(row,cCrit,L"=IF("+cell(cMax,row)+L">="+dpNet+L"-0.001,\"\u2713\",\"\")");
        (void)R;}
    if(last>=first){
        Format body;body.range=cell(cNo,first)+L":"+cell(cLast,last);body.border=true;body.size=10;s.format(body);
        const struct {int c;const wchar_t* f;} nf[]={{cQ,L"#,##0"},{cA,L"0"},{cB,L"0"},{cDh,L"0"},{cL,L"0.00"},{cK,L"0.00"},{cV,L"0.00"},{cPd,L"0.0"},{cRe,L"#,##0"},{cLambda,L"0.0000"},{cR,L"0.00"},{cRL,L"0.0"},{cZeta,L"0.00"},{cZ,L"0.0"},{cTerm,L"0"},{cDp,L"0.0"},{cCum,L"0.0"},{cMax,L"0.0"},{cOver,L"0.0"}};
        for(size_t k=0;k<sizeof(nf)/sizeof(nf[0]);++k){Format f;f.range=cell(nf[k].c,first)+L":"+cell(nf[k].c,last);f.number=nf[k].f;f.align=2;s.format(f);}
        const int centred[]={cNo,cFrom,cTo,cDim,cCrit};
        for(int k=0;k<5;++k){Format f;f.range=cell(centred[k],first)+L":"+cell(centred[k],last);f.align=1;s.format(f);}
        for(size_t i=0;i<r.d.size();++i)if(r.d[i].terminal&&!termRow[i]){Format f;f.range=cell(cTerm,first+static_cast<int>(i));f.fill=yellow;s.format(f);}
        Condition c;c.range=cell(cNo,first)+L":"+cell(cLast,last);c.formula=L"=$"+col(cCrit)+std::to_wstring(first)+L"=\"\u2713\"";c.fill=pink;s.conditions.push_back(c);
        // summary
        const int b=last+2;
        s.put(b,1,text(L"Kriti\u010Dna grana"));{Format f;f.range=cell(1,b);f.bold=true;f.size=11;f.font=navy;s.format(f);}
        s.put(b+1,1,text(L"\u0394p mre\u017Ee, Pa"));s.put(b+1,4,L"=MAX("+cell(cCum,first)+L":"+cell(cCum,last)+L")");
        s.put(b+2,1,text(L"Rezerva, %"));s.put(b+2,4,L"10");
        // no reserve on the terminal elements (as DuctPressDrop): max over the rows of (cum - kraj) * (1 + rez) + kraj
        const std::wstring U=cell(cCum,first)+L":"+cell(cCum,last),S=cell(cTerm,first)+L":"+cell(cTerm,last);
        wb.netRow=b+1;wb.fanRow=b+3;
        s.put(b+3,1,text(L"Za ventilator, Pa"));s.put(b+3,4,L"=SUMPRODUCT(MAX(("+U+L"-"+S+L")*(1+"+cell(4,b+2)+L"/100)+"+S+L"))");
        {Format f;f.range=cell(1,b+1)+L":"+cell(1,b+3);f.bold=true;f.size=10;f.font=blue;s.format(f);}
        {Format f;f.range=cell(4,b+1)+L":"+cell(4,b+3);f.number=L"0.0";f.bold=true;f.size=10;f.align=2;s.format(f);}
        {Format f;f.range=cell(4,b+2);f.number=L"0";f.fill=yellow;s.format(f);}
        // the critical path as exported
        size_t worst=0;for(size_t i=0;i<r.d.size();++i)if(cum[i]>cum[worst])worst=i;
        std::vector<int> path;for(int i=static_cast<int>(worst);i>=0;i=r.d[i].parent)path.insert(path.begin(),i);
        std::wstring p=L"J";for(size_t k=0;k<path.size();++k)p+=L" \u2192 "+r.d[path[k]].to;
        p+=L" (dionice ";for(size_t k=0;k<path.size();++k)p+=(k?L", ":L"")+std::to_wstring(r.d[path[k]].number);p+=L"; pri izvozu, \u0394p kraj. elemenata = 0)";
        s.put(b+4,1,text(L"Put"));s.put(b+4,4,text(p));{Format f;f.range=cell(1,b+4);f.bold=true;f.size=10;f.font=blue;s.format(f);}
        s.put(b+5,1,text(L"Upi\u0161ite \u0394p krajnjih elemenata (\u017Eute \u0107elije) i rezervu; kriti\u010Dna grana i vi\u0161ak za prigu\u0161ivanje se prera\u010Dunavaju. Rezerva se ne primjenjuje na \u0394p krajnjih elemenata."));
        {Format f;f.range=cell(1,b+5);f.italic=true;f.size=9;f.font=grey;s.format(f);}
        int w=b+7;
        if(!r.warnings.empty()){s.put(w++,1,text(L"Upozorenja"));for(size_t k=0;k<r.warnings.size();++k)s.put(w++,1,text(L"\u2022 "+r.warnings[k]));}
    }
    const double widths[]={9,6,6,8,11,6,6,6,7,6,7,7,9,8,7,7,6,7,9,10,10,8,8,9};
    s.widths.assign(widths,widths+cLast);s.hidden.push_back(cRe);s.hidden.push_back(cMax);s.freeze=cell(2,first);
    // ---- Fitinzi ----
    Sheet& f=wb.sheets[1];f.name=nF;
    const wchar_t* fh[]={L"Dionica",L"Element",L"Opis",L"Dimenzija",L"\u03B6 (pd dionice)",L"Izvor",L"C (izvor)",L"\u0394p, Pa",L"Napomena"};
    for(int c=1;c<=9;++c)f.put(1,c,text(fh[c-1]));
    {Format x;x.range=L"A1:I1";x.bold=true;x.font=rgb(255,255,255);x.fill=navy;x.align=1;x.border=true;x.size=10;x.wrap=true;f.format(x);}
    for(size_t k=0;k<order.size();++k){const tvsPressure::Fitting& x=r.f[order[k]];const int row=2+static_cast<int>(k);
        f.put(row,1,num(r.d[x.dionica].number,0));f.put(row,2,text(x.what));f.put(row,3,text(x.note.empty()?std::wstring():x.note));f.put(row,4,text(x.dim));
        if(x.equipment){f.put(row,6,text(L"Oprema"));if(x.ok&&x.c>0)f.put(row,7,num(x.c,4));}
        else{f.put(row,6,text(x.key.empty()?std::wstring(L"\u2013"):(x.key==L"Idelchik"?x.key:L"ASHRAE "+x.key)));if(x.ok)f.put(row,7,num(x.c,4));}
        if(x.equipment&&x.terminal){   // terminal oprema: dp itself (Dionice takes it as "Dp kraj. elem."), no zeta
            if(x.ok)f.put(row,8,num(x.dp,1));else{Format y;y.range=L"H"+std::to_wstring(row);y.fill=yellow;f.format(y);}
            continue;}
        f.put(row,5,num(x.zeta,4));
        f.put(row,8,L"=E"+std::to_wstring(row)+L"*INDEX("+rD+L"$"+col(cPd)+L":$"+col(cPd)+L",MATCH(A"+std::to_wstring(row)+L","+rD+L"$A:$A,0))");
        if(!x.ok){Format y;y.range=L"E"+std::to_wstring(row);y.fill=yellow;f.format(y);}}
    const int fl=1+static_cast<int>(order.size());
    if(fl>=2){Format x;x.range=L"A2:I"+std::to_wstring(fl);x.border=true;x.size=10;f.format(x);
        Format a;a.range=L"A2:A"+std::to_wstring(fl);a.align=1;f.format(a);
        Format e;e.range=L"E2:E"+std::to_wstring(fl);e.number=L"0.000";f.format(e);
        Format g;g.range=L"G2:G"+std::to_wstring(fl);g.number=L"0.000";f.format(g);
        Format hh;hh.range=L"H2:H"+std::to_wstring(fl);hh.number=L"0.0";f.format(hh);
        Format n;n.range=L"I2:I"+std::to_wstring(fl);n.size=9;n.font=grey;f.format(n);}
    f.put(fl+2,1,text(L"\u03B6 je sveden na pd svoje dionice (\u03B6 = \u0394p / pd) i mo\u017Ee se ru\u010Dno ispraviti \u2013 Dionice se same prera\u010Dunaju. \u017Duto: \u03B6 unosite sami."));
    {Format x;x.range=L"A"+std::to_wstring(fl+2);x.italic=true;x.size=9;x.font=grey;f.format(x);}
    const double fw[]={8,18,30,12,10,14,9,9,45};f.widths.assign(fw,fw+9);f.freeze=L"A2";
    // ---- Sema ----
    Sheet& g=wb.sheets[2];g.name=nS;g.onePage=true;
    g.put(1,1,text(L"Jednolinijska \u0161ema mre\u017Ee (osnova)"));{Format x;x.range=L"A1";x.bold=true;x.size=14;x.font=navy;g.format(x);}
    g.put(2,1,text(L"crveno: kriti\u010Dna grana pri izvozu \u00B7 isprekidano: fleks \u00B7 broj: dionica \u00B7 J jedinica, T T komad, P promjena, K kraj"));
    {Format x;x.range=L"A2";x.italic=true;x.size=9;x.font=grey;g.format(x);}
    if(!r.lines.empty()){
        double x0=r.lines[0].a.x,x1=x0,y0=r.lines[0].a.y,y1=y0;
        auto grow=[&](const tvsPressure::XY& p){if(p.x<x0)x0=p.x;if(p.x>x1)x1=p.x;if(p.y<y0)y0=p.y;if(p.y>y1)y1=p.y;};
        for(size_t k=0;k<r.lines.size();++k){grow(r.lines[k].a);grow(r.lines[k].b);}
        const double W=740,H=440,left=30,top=50;double sc=1;
        if(x1-x0>1e-9||y1-y0>1e-9){const double sx=x1-x0>1e-9?W/(x1-x0):1e9,sy=y1-y0>1e-9?H/(y1-y0):1e9;sc=sx<sy?sx:sy;}
        auto X=[&](double x){return left+(x-x0)*sc;};auto Y=[&](double y){return top+(y1-y)*sc;};
        std::vector<char> crit(r.d.size(),0);
        {size_t worst=0;for(size_t i=0;i<r.d.size();++i)if(cum[i]>cum[worst])worst=i;for(int i=static_cast<int>(worst);i>=0;i=r.d[i].parent)crit[i]=1;}
        std::vector<double> longest(r.d.size(),-1);std::vector<tvsPressure::XY> mid(r.d.size());
        for(size_t k=0;k<r.lines.size();++k){const tvsPressure::Line& l=r.lines[k];Stroke t;t.x1=X(l.a.x);t.y1=Y(l.a.y);t.x2=X(l.b.x);t.y2=Y(l.b.y);
            const bool c=l.dionica>=0&&crit[l.dionica];t.weight=c?2.25:1.25;t.color=c?red:navy;t.dash=l.flex;g.strokes.push_back(t);
            const double len=std::hypot(t.x2-t.x1,t.y2-t.y1);if(l.dionica>=0&&len>longest[l.dionica]){longest[l.dionica]=len;mid[l.dionica]=tvsPressure::XY((t.x1+t.x2)/2,(t.y1+t.y2)/2);}}
        for(size_t i=0;i<r.d.size();++i){if(longest[i]<0)continue;Label t;t.x=mid[i].x+3;t.y=mid[i].y-15;t.text=std::to_wstring(r.d[i].number);t.size=9;t.color=crit[i]?red:navy;t.bold=true;g.labels.push_back(t);}
        std::vector<std::wstring> seen;
        for(size_t i=0;i<r.d.size();++i){const tvsPressure::Dionica& d=r.d[i];
            const std::wstring names[2]={d.from,d.to};const tvsPressure::XY at[2]={d.fromAt,d.toAt};
            for(int k=0;k<2;++k){bool dup=false;for(size_t j=0;j<seen.size();++j)if(seen[j]==names[k])dup=true;if(dup)continue;seen.push_back(names[k]);
                Label t;t.x=X(at[k].x)+4;t.y=Y(at[k].y)-13;t.text=k==1&&!d.tag.empty()?d.tag:names[k];t.size=8;t.color=grey;t.bold=false;g.labels.push_back(t);}}
    }
    return wb;
}
// Ventilator u mrezi: "Ventilator" (summary) + the sheets of the suction side (odvod) and of the
// discharge side (dovod); the fan needs the sum of the two "Za ventilator" (each with its reserve).
inline Workbook fanBook(const Workbook& suction,const Workbook& discharge,const Info& info,double qSuction,double qDischarge,const std::wstring& tag){
    Workbook wb;wb.sheets.resize(1);Sheet& s=wb.sheets[0];s.name=L"Ventilator";
    const long navy=rgb(0x1F,0x3A,0x5F),blue=rgb(0x31,0x59,0x7F),grey=rgb(0x5B,0x6B,0x7F),pink=rgb(0xFD,0xEC,0xEA);
    s.put(1,1,text(L"TVS_VENT \u00B7 Ventilator u mre\u017Ei"+(tag.empty()?std::wstring():L" "+tag)));{Format f;f.range=L"A1";f.bold=true;f.size=14;f.font=navy;s.format(f);}
    const std::wstring infoRows[][2]={{L"Projekat",info.project},{L"Datum",info.date}};
    for(int i=0;i<2;++i){s.put(3+i,1,text(infoRows[i][0]));s.put(3+i,2,text(infoRows[i][1]));}
    const int h=6;const wchar_t* head[]={L"Strana",L"Q, m\u00B3/h",L"\u0394p mre\u017Ee, Pa",L"Za ventilator, Pa"};
    for(int c=0;c<4;++c)s.put(h,c+1,text(head[c]));
    {Format f;f.range=cell(1,h)+L":"+cell(4,h);f.bold=true;f.wrap=true;f.border=true;f.align=1;f.font=blue;s.format(f);}
    const Workbook* side[2]={&suction,&discharge};const wchar_t* name[2]={L"Usis (odvod)",L"Potis (dovod)"};const double q[2]={qSuction,qDischarge};
    for(int k=0;k<2;++k){const int row=h+1+k;const std::wstring ref=L"'"+side[k]->sheets[0].name+L"'!";
        s.put(row,1,text(name[k]));s.put(row,2,num(q[k],0));
        s.put(row,3,side[k]->netRow?L"="+ref+L"$D$"+std::to_wstring(side[k]->netRow):std::wstring(L"0"));
        s.put(row,4,side[k]->fanRow?L"="+ref+L"$D$"+std::to_wstring(side[k]->fanRow):std::wstring(L"0"));}
    const int tot=h+3;s.put(tot,1,text(L"Ventilator ukupno"));s.put(tot,3,L"=C"+std::to_wstring(h+1)+L"+C"+std::to_wstring(h+2));s.put(tot,4,L"=D"+std::to_wstring(h+1)+L"+D"+std::to_wstring(h+2));
    {Format f;f.range=cell(1,h+1)+L":"+cell(4,tot);f.border=true;f.size=10;s.format(f);}
    {Format f;f.range=cell(2,h+1)+L":"+cell(2,tot);f.number=L"#,##0";f.align=2;s.format(f);}
    {Format f;f.range=cell(3,h+1)+L":"+cell(4,tot);f.number=L"0.0";f.align=2;s.format(f);}
    {Format f;f.range=cell(1,tot)+L":"+cell(4,tot);f.bold=true;f.fill=pink;s.format(f);}
    int w=tot+2;
    if(qSuction>0&&qDischarge>0&&std::fabs(qSuction-qDischarge)>0.01*(qSuction>qDischarge?qSuction:qDischarge)){
        s.put(w++,1,text(L"Upozorenje: protok usisa i potisa se razlikuje \u2013 kroz ventilator prolazi jedan protok; provjerite krajnje elemente (TVS_PROTOK)."));
        Format f;f.range=cell(1,w-1);f.bold=true;f.font=rgb(0xC0,0x39,0x2B);s.format(f);}
    s.put(w,1,text(L"Usisna strana je ra\u010Dunata kao odvod (krajnji elementi \u2192 ventilator), potisna kao dovod (ventilator \u2192 krajnji elementi). Rezerva i \u0394p krajnjih elemenata se unose na listovima svake strane; ukupno se sabira samo."));
    {Format f;f.range=cell(1,w);f.italic=true;f.size=9;f.font=grey;s.format(f);}
    const double widths[]={22,12,14,16};s.widths.assign(widths,widths+4);
    for(int k=0;k<2;++k)for(size_t j=0;j<side[k]->sheets.size();++j)wb.sheets.push_back(side[k]->sheets[j]);
    return wb;
}
}
