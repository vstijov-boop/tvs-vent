"""Place MOVE/STRETCH in TVS_VENT and remove the old MOVE entry from TVS_TOOLS."""
import copy
import io
import pathlib
import zipfile
from lxml import etree as E
root=pathlib.Path(__file__).resolve().parents[1]
paths={g:root/'Installer'/g/(g+'.cuix') for g in ('TVS_TOOLS','TVS_VENT')}
archives={g:{i.filename:(i,z.read(i.filename)) for i in z.infolist()} for g,p in paths.items() for z in [zipfile.ZipFile(p)]}
def load(g,n):return E.fromstring(archives[g][n][1])
def save(g,n,t):archives[g][n]=(archives[g][n][0],E.tostring(t,encoding='UTF-8',xml_declaration=True))
def clone(e,cmd,label):
 t=copy.deepcopy(e)
 for i,n in enumerate(t.iter()):
  if 'UID' in n.attrib:n.set('UID',n.tag.upper()+'_TVS_'+cmd+'_'+str(i))
  if 'MenuMacroID' in n.attrib:n.set('MenuMacroID','MMU_TVS_'+cmd)
  if n.tag in ('Name','NameRef','TooltipTitle','CLICommand'):n.text=label
  if 'Text' in n.attrib:n.set('Text',label)
  if 'KeyTip' in n.attrib:n.set('KeyTip','MV' if cmd=='MOVE' else 'ST')
 return t
menu=load('TVS_VENT','MenuGroup.cui')
source=load('TVS_TOOLS','MenuGroup.cui').xpath('.//MenuMacro[@UID="MMU_TVS_MOVE"]')
if not source:source=menu.xpath('.//MenuMacro[@UID="MMU_TVS_MOVE"]')
assert source
for cmd,label,helptext in [('MOVE','Povezano pomjeranje','Pomjera kanal uz prilagođavanje povezanih dionica. Izbor ivice ili ose.'),('STRETCH','Istezanje mreže','Okvirom izaberite priključke pokretnog dijela mreže. Spoljašnji krajevi ostaju fiksni; granične dionice mijenjaju dužinu.')]:
 if not menu.xpath('.//MenuMacro[@UID="MMU_TVS_'+cmd+'"]'):
  macro=clone(source[0],cmd,label);macro.set('UID','MMU_TVS_'+cmd)
  macro.find('.//Command').text='^C^C_TVS_'+cmd+';'
  macro.find('.//HelpString').text=helptext
  for tag,size in [('SmallImage',16),('LargeImage',32)]:macro.find('.//'+tag).set('Name',f'TVS_{cmd}_{size}.png')
  menu.find('MacroGroup').append(macro)
 for name,tag,dest in [('ToolbarRoot.cui','ToolbarButton','.//Toolbar[Name="TVS_VENT"]'),('PopMenuRoot.cui','PopMenuItem','.//PopMenu[Name="Crtanje kanala"]'),('RibbonRoot.cui','RibbonCommandButton','.//RibbonPanelSource[@Text="Crtanje kanala"]/RibbonRow')]:
  t=load('TVS_VENT',name)
  if not t.xpath('.//*[@MenuMacroID="MMU_TVS_'+cmd+'"]'):
   target=t.xpath(dest)[0];template=target.find('.//'+tag)
   target.append(clone(template,cmd,label));save('TVS_VENT',name,t)
 for size in (16,32):
  name=f'TVS_{cmd}_{size}.png'
  archives['TVS_VENT'][name]=(zipfile.ZipInfo(name),(root/'Installer/TVS_VENT/icons'/name).read_bytes())
save('TVS_VENT','MenuGroup.cui',menu)
t=load('TVS_VENT','[Content_Types].xml')
for n in t:
 if n.get('Extension')=='png':n.set('ContentType','image/png')
save('TVS_VENT','[Content_Types].xml',t)
for name,tag in [('MenuGroup.cui','MenuMacro'),('ToolbarRoot.cui','ToolbarButton'),('PopMenuRoot.cui','PopMenuItem'),('RibbonRoot.cui','RibbonRowPanel')]:
 t=load('TVS_TOOLS',name)
 for ref in t.xpath('.//*[@MenuMacroID="MMU_TVS_MOVE"] | .//MenuMacro[@UID="MMU_TVS_MOVE"]'):
  while ref.tag!=tag:ref=ref.getparent()
  ref.getparent().remove(ref)
 save('TVS_TOOLS',name,t)
for size in (16,32):archives['TVS_TOOLS'].pop(f'TVS_MOVE_{size}.png',None)
for group,p in paths.items():
 buf=io.BytesIO()
 with zipfile.ZipFile(buf,'w') as z:
  for info,data in archives[group].values():z.writestr(info,data,compress_type=zipfile.ZIP_DEFLATED)
 p.write_bytes(buf.getvalue())
 with zipfile.ZipFile(p) as z:
  assert z.testzip() is None
  for name in z.namelist():
   if name.endswith(('.xml','.cui','.rels')):E.fromstring(z.read(name))
  for cmd in ('MOVE','STRETCH'):
   for name in ('MenuGroup.cui','ToolbarRoot.cui','PopMenuRoot.cui','RibbonRoot.cui'):
    t=E.fromstring(z.read(name));refs=t.xpath('.//*[@MenuMacroID="MMU_TVS_'+cmd+'"] | .//MenuMacro[@UID="MMU_TVS_'+cmd+'"]')
    assert len(refs)==(1 if group=='TVS_VENT' else 0),(group,name,cmd)
 print(p)
