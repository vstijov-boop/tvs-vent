"""Add the new TVS_VENT commands (presjek, kote, match, prelaz, offset, bypass, T komad, flex)
to TVS_VENT.cuix with their icons, and refresh the MOVE/STRETCH icons. Idempotent."""
import copy, io, pathlib, zipfile
from lxml import etree as E
root = pathlib.Path(__file__).resolve().parents[1]
cuix = root/'Installer'/'TVS_VENT'/'TVS_VENT.cuix'
icons = root/'Installer'/'TVS_VENT'/'icons'
arch = {i.filename: (i, z.read(i.filename)) for z in [zipfile.ZipFile(cuix)] for i in z.infolist()}
def load(n): return E.fromstring(arch[n][1])
def save(n, t): arch[n] = (arch[n][0], E.tostring(t, encoding='UTF-8', xml_declaration=True))
# cmd, label, help, popmenu, ribbon panel, keytip
CMDS = [
 ('OFFSET', 'Promjena kote', 'Etažiranje: promjena visinske kote dionice sa kosim ili vertikalnim prelazom.', 'Crtanje kanala', 'Crtanje kanala', 'OF'),
 ('BYPASS', 'Obilazak prepreke', 'Obilazak prepreke (gore/dolje) na izabranom kanalu.', 'Crtanje kanala', 'Crtanje kanala', 'BY'),
 ('TK_TIP', 'Tip T komada', 'Mijenja tip T komada: običan, sa ogrankom (TK-O) ili gaćasti (TK-G).', 'Crtanje kanala', 'Crtanje kanala', 'TK'),
 ('PRELAZ_KOTA', 'Poravnanje prelaza', 'Poravnanje prelaza po visini (osa/gore/dolje) uz izbor strane mreže koja ostaje.', 'Crtanje kanala', 'Crtanje kanala', 'PK'),
 ('FLEX_MANZETNE', 'Flex manžetne', 'Označava manžetne na krajevima flex crijeva koji se ne nastavljaju u flex.', 'Crtanje kanala', 'Crtanje kanala', 'FM'),
 ('FLEX_MANZETNA', 'Flex manžetna uklj/isklj', 'Uključuje ili isključuje manžetnu na izabranom kraju flex crijeva.', 'Crtanje kanala', 'Crtanje kanala', 'FN'),
 ('MATCH', 'Prenos svojstava', 'Prenosi TVS svojstva izvornog elementa na izabrane elemente ili cijelu povezanu mrežu.', 'Atributi', 'Atributi', 'MA'),
 ('PRESJEK', 'Poprečni presjek', 'Generiše poprečni presjek kroz TVS kanale; ponovnim izborom presjek se osvježava.', 'Crtanje kanala', 'Oznake', 'PR'),
 ('KOTE', 'Prikaz kota', 'Uključuje ili isključuje prikaz visinskih kota na ekranu (ne štampa se).', 'Crtanje kanala', 'Oznake', 'KO'),
 ('ALIGN', 'Poravnanje', 'Poravnava kanal ili fiting (osa ili ivica) prema LINE, pravom segmentu polilinije, XLINE ili RAY, uz odmak i izbor strane.', 'Crtanje kanala', 'Crtanje kanala', 'AL'),
 ('NATPIS', 'Natpis kanala', 'Uključuje ili isključuje natpis dimenzija / protok m3/h / brzina m/s na kanalima (na ekranu, ne štampa se).', 'Crtanje kanala', 'Oznake', 'NA'),
 ('BYPASS_DETALJ', 'Detalj obilaska', 'Bočni detalj postojećeg obilaska prepreke.', 'Crtanje kanala', 'Oznake', 'BD'),
 ('IZVOZ', 'Izvoz bez TVS', 'Kopija crteža u kojoj su TVS elementi zamijenjeni linijama, za korisnike bez TVS_VENT; otvoreni crtež se ne mijenja.', 'Specifikacija', 'Specifikacija', 'IZ'),
 ('PRERACUNAJ', 'Preračunaj jedinice', 'Izbor nacrtan u drugim jedinicama (mm/cm/m), zalijepljen 1:1, preračunava u jedinice ovog crteža (položaj i dimenzije).', 'Crtanje kanala', 'Crtanje kanala', 'PJ'),
 ('KOLJENA_UGAO', 'Ugao koljena', 'Vraća koljena malo odstupljenog ugla na 15/22,5/30/45/60/75/90° (krajevi kanala prate).', 'Crtanje kanala', 'Crtanje kanala', 'KU'),
 ('PROVJERA', 'Provjera mreže', 'Spojevi različitih dimenzija / kota bez prelaza, promašeni priključci, kraj kanala usred kanala, uglovi koljena, kratki kanali; zum na svaki problem.', 'Crtanje kanala', 'Crtanje kanala', 'PV'),
 ('PROTOK', 'Protoci mreže', 'Sabira protoke krajnjih elemenata do jedinice (izbor kraja mreže kod jedinice); krajnjim elementima bez protoka unosi se protok; prikazuje najveću brzinu.', 'Crtanje kanala', 'Crtanje kanala', 'PT'),
 ('SPOJ_TKG', 'Spoj gaćastim T komadom', 'Spaja ulazni kanal i dva suprotna ogranka TK-G T komadom: jedan ogranak se pomjera u osu drugog, ulaz se skraćuje; prikaz prije potvrde.', 'Crtanje kanala', 'Crtanje kanala', 'TG'),
]
def clone(e, cmd, label, key):
    t = copy.deepcopy(e)
    for i, n in enumerate(t.iter()):
        if 'UID' in n.attrib: n.set('UID', n.tag.upper()+'_TVS_'+cmd+'_'+str(i))
        if 'MenuMacroID' in n.attrib: n.set('MenuMacroID', 'MMU_TVS_'+cmd)
        if n.tag in ('Name', 'NameRef', 'TooltipTitle'): n.text = label
        if 'Text' in n.attrib: n.set('Text', label)
        if 'KeyTip' in n.attrib: n.set('KeyTip', key)
    return t
menu = load('MenuGroup.cui')
src = menu.xpath('.//MenuMacro[@UID="MMU_TVS_MOVE"]')[0]
trees = {n: load(n) for n in ('ToolbarRoot.cui', 'PopMenuRoot.cui', 'RibbonRoot.cui')}
tb_tpl = trees['ToolbarRoot.cui'].xpath('.//ToolbarButton[.//MacroRef/@MenuMacroID="MMU_TVS_MOVE"]')[0]
pm_tpl = trees['PopMenuRoot.cui'].xpath('.//PopMenuItem[.//MacroRef/@MenuMacroID="MMU_TVS_MOVE"]')[0]
rb_tpl = trees['RibbonRoot.cui'].xpath('.//RibbonCommandButton[@MenuMacroID="MMU_TVS_MOVE"]')[0]
seps = set()
for cmd, label, helptext, pop, panel, key in CMDS:
    if not menu.xpath('.//MenuMacro[@UID="MMU_TVS_'+cmd+'"]'):
        m = clone(src, cmd, label, key); m.set('UID', 'MMU_TVS_'+cmd)
        m.find('.//Command').text = '^C^C_TVS_'+cmd+';'
        m.find('.//HelpString').text = helptext
        for tag, size in (('SmallImage', 16), ('LargeImage', 32)): m.find('.//'+tag).set('Name', f'TVS_{cmd}_{size}.png')
        menu.find('MacroGroup').append(m)
    ref = './/*[@MenuMacroID="MMU_TVS_'+cmd+'"]'
    t = trees['ToolbarRoot.cui']
    if not t.xpath(ref): t.xpath('.//Toolbar[Name="TVS_VENT"]')[0].append(clone(tb_tpl, cmd, label, key))
    t = trees['PopMenuRoot.cui']
    if not t.xpath(ref):
        target = t.xpath('.//PopMenu[Name="'+pop+'"]')[0]
        if cmd not in seps and cmd in ('OFFSET', 'MATCH', 'PRESJEK'):
            s = E.SubElement(target, 'PopMenuItem', IsSeparator='true', hasDiesel='false', UID='POPMENUITEM_SEP_TVS_'+cmd)
            E.SubElement(s, 'ModifiedRev', MajorVersion='20', MinorVersion='1', UserVersion='1')
            seps.add(cmd)
        target.append(clone(pm_tpl, cmd, label, key))
    t = trees['RibbonRoot.cui']
    if not t.xpath(ref):
        src_panel = t.xpath('.//RibbonPanelSource[@Text="'+panel+'"]')[0]
        src_panel.find('RibbonRow').append(clone(rb_tpl, cmd, label, key))
    for size in (16, 32):
        n = f'TVS_{cmd}_{size}.png'
        arch[n] = (arch[n][0] if n in arch else zipfile.ZipInfo(n, (1980, 1, 1, 0, 0, 0)), (icons/n).read_bytes())
for cmd in ('MOVE', 'STRETCH'):
    for size in (16, 32):
        n = f'TVS_{cmd}_{size}.png'; arch[n] = (arch[n][0], (icons/n).read_bytes())
save('MenuGroup.cui', menu)
for n, t in trees.items(): save(n, t)
buf = io.BytesIO()
with zipfile.ZipFile(buf, 'w') as z:
    for info, data in arch.values(): z.writestr(info, data, compress_type=zipfile.ZIP_DEFLATED)
cuix.write_bytes(buf.getvalue())
with zipfile.ZipFile(cuix) as z:
    assert z.testzip() is None
    for n in z.namelist():
        if n.endswith(('.xml', '.cui', '.rels')): E.fromstring(z.read(n))
    for cmd, *_ in CMDS + [('MOVE',), ('STRETCH',)]:
        mg = E.fromstring(z.read('MenuGroup.cui'))
        assert len(mg.xpath('.//MenuMacro[@UID="MMU_TVS_'+cmd+'"]')) == 1, cmd
        for n in ('ToolbarRoot.cui', 'PopMenuRoot.cui', 'RibbonRoot.cui'):
            assert len(E.fromstring(z.read(n)).xpath('.//*[@MenuMacroID="MMU_TVS_'+cmd+'"]')) == 1, (n, cmd)
        for size in (16, 32): assert f'TVS_{cmd}_{size}.png' in z.namelist()
print(cuix, len(arch), 'entries')
