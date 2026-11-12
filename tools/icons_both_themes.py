"""Ikone novih komandi citljive i na svijetloj i na tamnoj temi AutoCAD-a (PLAN_RAZVOJA stavka 21).
Cuix nosi jednu sliku po komandi za obje teme, pa se tamni potezi (crno, tamnosivo, tamno ljubicasto)
posvijetle do relativne luminanse 0.24: kontrast ~3:1 prema pozadini obje teme, boja (nijansa) ostaje.
   python3 tools/icons_both_themes.py   (pa tools/add_new_commands_cuix.py)
Idempotentno: svijetliji pikseli se ne diraju."""
import colorsys,pathlib,re,sys
from PIL import Image
ROOT=pathlib.Path(__file__).resolve().parents[1]
LO=0.24
def lum(r,g,b):
    f=lambda c:(c/255/12.92) if c/255<=0.04045 else ((c/255+0.055)/1.055)**2.4
    return 0.2126*f(r)+0.7152*f(g)+0.0722*f(b)
def lift(r,g,b):
    h,l,s=colorsys.rgb_to_hls(r/255,g/255,b/255);lo,hi=l,1.0
    for _ in range(20):
        m=(lo+hi)/2;rr,gg,bb=colorsys.hls_to_rgb(h,m,s)
        if lum(rr*255,gg*255,bb*255)<LO:lo=m
        else:hi=m
    rr,gg,bb=colorsys.hls_to_rgb(h,hi,s);return int(rr*255+.5),int(gg*255+.5),int(bb*255+.5)
def main():
    names=sorted(set(re.findall(r"\('([A-Z_]+)',\s*'",(ROOT/'tools/add_new_commands_cuix.py').read_text(encoding='utf-8'))))
    for n in names:
        for size in (16,32):
            p=ROOT/'Installer/TVS_VENT/icons'/('TVS_%s_%d.png'%(n,size))
            if not p.exists():continue
            im=Image.open(p).convert('RGBA');px=im.load()
            for y in range(im.height):
                for x in range(im.width):
                    r,g,b,a=px[x,y]
                    if a and lum(r,g,b)<LO:px[x,y]=lift(r,g,b)+(a,)
            im.save(p)
if __name__=='__main__':main()
