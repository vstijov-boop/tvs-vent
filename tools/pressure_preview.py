"""Preview of the TVS_PAD workbook model (tests/test_pressure_drop dump) as .xlsx, like the COM
writer does it in Excel: python3 tools/pressure_preview.py dump.txt out.xlsx"""
import sys
from openpyxl import Workbook
from openpyxl.styles import Font,PatternFill,Alignment,Border,Side
from openpyxl.formatting.rule import FormulaRule
from openpyxl.drawing.image import Image
def bgr(c):c=int(c);return '%02X%02X%02X'%(c&255,(c>>8)&255,(c>>16)&255)
wb=Workbook();wb.remove(wb.active);ws=None;shapes={}
thin=Side(style='thin',color='AEB9C7')
for line in open(sys.argv[1],encoding='utf-8'):
    f=line.rstrip('\n').split('\t')
    if f[0]=='SHEET':ws=wb.create_sheet(f[1]);shapes[ws.title]=([],[]);continue
    if f[0]=='C':
        v=f[3].replace('\\n','\n');r,c=int(f[1]),int(f[2])
        if v.startswith("'"):ws.cell(r,c,v[1:])
        elif v.startswith('='):ws.cell(r,c,v)
        else:
            try:ws.cell(r,c,float(v))
            except ValueError:ws.cell(r,c,v)
    elif f[0]=='W':
        from openpyxl.utils import get_column_letter as L;ws.column_dimensions[L(int(f[1]))].width=float(f[2])
    elif f[0]=='H':
        from openpyxl.utils import get_column_letter as L;ws.column_dimensions[L(int(f[1]))].hidden=True
    elif f[0]=='F':
        rng,num,bold,ital,wrap,border,size,font,fill,align=f[1:11]
        for row in ws[rng] if ':' in rng else [[ws[rng]]]:
            for c in row:
                ft=c.font;c.font=Font(name='Calibri',bold=bold=='1' or ft.b,italic=ital=='1' or ft.i,size=int(size) if int(size) else (ft.sz or 10),color=bgr(font) if int(font)>=0 else ft.color)
                if num:c.number_format=num
                if int(fill)>=0:c.fill=PatternFill('solid',fgColor=bgr(fill))
                if border=='1':c.border=Border(left=thin,right=thin,top=thin,bottom=thin)
                a=int(align)
                if a or wrap=='1':c.alignment=Alignment(horizontal={-1:'left',1:'center',2:'right'}.get(a,c.alignment.horizontal),vertical='center',wrap_text=wrap=='1' or c.alignment.wrap_text)
    elif f[0]=='K':ws.conditional_formatting.add(f[1],FormulaRule(formula=[f[2][1:]],fill=PatternFill('solid',fgColor=bgr(f[3]),bgColor=bgr(f[3]))))
    elif f[0]=='Z':ws.freeze_panes=f[1]
    elif f[0]=='S':shapes[ws.title][0].append([float(x) for x in f[1:6]]+[bgr(f[6]),f[7]=='1'])
    elif f[0]=='L':shapes[ws.title][1].append((float(f[1]),float(f[2]),float(f[3]),bgr(f[4]),f[5]=='1',f[6]))
# shapes: openpyxl has no shapes, so the schematic is a picture here (Excel gets real lines)
for name,(st,lb) in shapes.items():
    if not st:continue
    from PIL import Image as P,ImageDraw,ImageFont
    k=2;W=int(max(max(s[0],s[2]) for s in st)+80);H=int(max(max(s[1],s[3]) for s in st)+60)
    im=P.new('RGB',(W*k,H*k),'white');d=ImageDraw.Draw(im)
    try:fb=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf',int(9*k*1.3));fr=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',int(8*k*1.3))
    except OSError:fb=fr=ImageFont.load_default()
    for x1,y1,x2,y2,w,c,dash in st:
        col='#'+c
        if dash:
            import math;L_=math.hypot(x2-x1,y2-y1);n=max(1,int(L_/6))
            for i in range(0,n,2):a=i/n;b=min(1,(i+1)/n);d.line([((x1+(x2-x1)*a)*k,(y1+(y2-y1)*a)*k),((x1+(x2-x1)*b)*k,(y1+(y2-y1)*b)*k)],fill=col,width=int(w*k))
        else:d.line([(x1*k,y1*k),(x2*k,y2*k)],fill=col,width=int(w*k))
    for x,y,s,c,b,t in lb:d.text((x*k,y*k),t,fill='#'+c,font=fb if b else fr)
    p='/tmp/claude-0/pad/sema_%s.png'%abs(hash(name));im.save(p)
    img=Image(p);img.width,img.height=W,H;wb[name].add_image(img,'A4')
for s in wb.worksheets:
    s.page_setup.orientation='landscape';s.sheet_properties.pageSetUpPr.fitToPage=True;s.page_setup.fitToWidth=1;s.page_setup.fitToHeight=0
wb.save(sys.argv[2])
