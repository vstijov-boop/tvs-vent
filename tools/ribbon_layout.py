"""TVS_Vent ribbon layout (user decision 2026-11-23, after the BGTOOLS example): panels by topic,
the main command of a panel as a large button with text, the others as small icon-only buttons in
up to three rows (tooltip = name + help). Rebuilds every RibbonPanelSource of the TVS_Vent tab in
Installer/TVS_VENT/TVS_VENT.cuix from LAYOUT; commands, macros, icons, toolbar and menus are not
touched. Idempotent. New commands: add them to LAYOUT (tools/add_new_commands_cuix.py calls this).
"""
import io, pathlib, zipfile
try:
    from lxml import etree as E
except ImportError:   # tests read LAYOUT without lxml (CI)
    E = None

root = pathlib.Path(__file__).resolve().parents[1]
cuix = root/'Installer'/'TVS_VENT'/'TVS_VENT.cuix'

# panel title, large button (macro name or None), small buttons (macro names, in order)
LAYOUT = [
    ('Crtanje', 'Crtanje kanala', ['Spajanje koljenom', 'Spajanje T-komadom', 'Spoj T komadom sa ogrankom',
                                   'Spoj gaćastim T komadom', 'Presijecanje kanala', 'Izmjena svojstava']),
    ('Izmjena mreže', None, ['Povezano pomjeranje', 'Istezanje mreže', 'Poravnanje', 'Promjena kote',
                             'Obilazak prepreke', 'Poravnanje prelaza', 'Tip T komada', 'Ugao koljena',
                             'Preračunaj jedinice']),
    ('Flex', None, ['Flex manžetne', 'Flex manžetna uklj/isklj']),
    ('Oprema', 'Oprema na mreži', ['Priključenje rešetke', 'Svojstva opreme', 'Odveži opremu']),
    ('Proračun', 'Pad pritiska', ['Protoci mreže', 'Provjera mreže', 'Izvoz u DuctPressDrop']),
    ('Oznake', 'Oznaka elementa', ['Ažuriranje oznaka', 'Oznake kanala', 'Natpis kanala', 'Prikaz kota',
                                   'Poprečni presjek', 'Detalj obilaska']),
    ('Specifikacija', 'Specifikacija', ['Izvoz specifikacije u Excel', 'Izvoz bez TVS']),
    ('Svojstva i podešavanja', 'Podešavanje crtanja', ['Dodavanje svojstava', 'Upravljač svojstvima',
                                                       'Atributi u svojstva', 'Prenos svojstava',
                                                       'Razlaganje TVS objekata']),
]
ROWS = 3


def rev(parent):
    E.SubElement(parent, 'ModifiedRev', MajorVersion='20', MinorVersion='1', UserVersion='1')


def button(parent, uid, macro, label, style, key):
    b = E.SubElement(parent, 'RibbonCommandButton', UID=uid, Id='AcRibbonCommandButton', Text=label,
                     ButtonStyle=style, MenuMacroID=macro, KeyTip=key)
    t = E.SubElement(b, 'TooltipTitle', xlate='true', UID=uid.replace('RBN', 'TT', 1))
    t.text = label
    rev(b)


def build(ribbon, menu):
    old_keys = {b.get('MenuMacroID'): b.get('KeyTip') or '' for b in ribbon.iter('RibbonCommandButton')}
    macros = {}
    for m in menu.iter('MenuMacro'):   # a name used twice (Specifikacija...): the macro already on the ribbon wins
        name, uid = m.findtext('.//Name'), m.get('UID')
        if name not in macros or (uid in old_keys and macros[name] not in old_keys):
            macros[name] = uid
    used = [n for _, big, small in LAYOUT for n in ([big] if big else []) + small]
    missing = [n for n in used if n not in macros]
    assert not missing, ('nema makroa', missing)
    left_out = [n for n, uid in macros.items() if uid in old_keys and n not in used]
    assert not left_out, ('komanda je bila u ribbonu, a nema je u LAYOUT', left_out)

    panels = ribbon.find('RibbonPanelSourceCollection')
    tab = ribbon.xpath('.//RibbonTabSource[@Text="TVS_Vent"]')[0]
    for p in list(panels):
        if p.tag == 'RibbonPanelSource':
            panels.remove(p)
    for r in tab.findall('RibbonPanelSourceReference'):
        tab.remove(r)
    for i, (title, big, small) in enumerate(LAYOUT, 1):
        pid = f'RBN_TVS_PANEL_{i}'
        p = E.SubElement(panels, 'RibbonPanelSource', UID=pid, Text=title, HiddenInEditor='false')
        rev(p)
        n = E.SubElement(p, 'Name', xlate='true', UID=f'XLS_TVS_PANEL_{i}')
        n.text = title
        row = E.SubElement(p, 'RibbonRow', UID=f'{pid}_ROW')
        rev(row)
        if big:
            button(row, f'RBN_TVS_{i}_BIG', macros[big], big, 'LargeWithText', old_keys.get(macros[big], ''))
        if small:
            rp = E.SubElement(row, 'RibbonRowPanel', UID=f'{pid}_ROWPANEL', ResizeStyle='None',
                              ResizePriority='100', TopJustify='True')
            rev(rp)
            rows = min(ROWS, len(small))
            per = -(-len(small) // rows)   # columns: fill row by row, at most three rows
            for r in range(rows):
                sub = E.SubElement(rp, 'RibbonRow', UID=f'{pid}_R{r}')
                rev(sub)
                for k, name in enumerate(small[r*per:(r+1)*per]):
                    button(sub, f'RBN_TVS_{i}_{r}_{k}', macros[name], name, 'SmallWithoutText',
                           old_keys.get(macros[name], ''))
        ref = E.SubElement(tab, 'RibbonPanelSourceReference', UID=f'RBN_TVS_REF_{i}', PanelId=pid,
                           ResizeStyle='Default')
        rev(ref)


def main():
    arch = {i.filename: (i, z.read(i.filename)) for z in [zipfile.ZipFile(cuix)] for i in z.infolist()}
    ribbon = E.fromstring(arch['RibbonRoot.cui'][1])
    menu = E.fromstring(arch['MenuGroup.cui'][1])
    build(ribbon, menu)
    arch['RibbonRoot.cui'] = (arch['RibbonRoot.cui'][0],
                              E.tostring(ribbon, encoding='UTF-8', xml_declaration=True))
    buf = io.BytesIO()
    with zipfile.ZipFile(buf, 'w') as z:
        for info, data in arch.values():
            z.writestr(info, data, compress_type=zipfile.ZIP_DEFLATED)
    cuix.write_bytes(buf.getvalue())
    with zipfile.ZipFile(cuix) as z:
        assert z.testzip() is None
        E.fromstring(z.read('RibbonRoot.cui'))
    print(cuix, 'ribbon:', len(LAYOUT), 'panela')


if __name__ == '__main__':
    main()
