"""Add TVS_MOVE to the existing tools CUIX, preserving all previous commands/assets."""
import copy
import io
import pathlib
import sys
import zipfile
from lxml import etree as E
from PIL import Image

root = pathlib.Path(__file__).resolve().parents[1]
cuix = root / 'Installer/TVS_TOOLS/TVS_TOOLS.cuix'
icons = cuix.parent / 'icons'
if len(sys.argv) > 1:
    source = Image.open(sys.argv[1]).convert('RGBA')
    source = source.crop(source.getchannel('A').getbbox())
    for size in (16, 32, 128):
        inner = source.copy()
        inner.thumbnail((size-2, size-2), Image.Resampling.LANCZOS)
        canvas = Image.new('RGBA', (size, size))
        canvas.alpha_composite(inner, ((size-inner.width)//2, (size-inner.height)//2))
        canvas.save(icons / f'TVS_MOVE_{size}.png')
with zipfile.ZipFile(cuix) as z:
    entries = {i.filename: (i, z.read(i.filename)) for i in z.infolist()}

def load(name):
    return E.fromstring(entries[name][1])

def duplicate(element):
    result = copy.deepcopy(element)
    for node in result.iter():
        for key, value in list(node.attrib.items()):
            node.set(key, value.replace('KGTEZ', 'MOVE'))
        if node.text:
            node.text = node.text.replace('Masa kanala', 'Povezano pomjeranje')
    return result

def save(name, tree):
    entries[name] = (entries[name][0], E.tostring(tree, encoding='UTF-8', xml_declaration=True))

menu = load('MenuGroup.cui')
if not menu.xpath('.//MenuMacro[@UID="MMU_TVS_MOVE"]'):
    old = menu.xpath('.//MenuMacro[@UID="MMU_TVS_KGTEZ"]')[0]
    new = duplicate(old)
    new.find('.//Command').text = '^C^C_TVS_MOVE;'
    new.find('.//HelpString').text = 'TVS_MOVE — pomjera kanal uz prilagođavanje povezanih dionica. Izbor izabrane ivice, druge ivice ili ose.'
    old.getparent().append(new)
    save('MenuGroup.cui', menu)

for filename, tag, destination in (
    ('ToolbarRoot.cui', 'ToolbarButton', './/Toolbar[Name="Aksonometrija i crtanje"]'),
    ('PopMenuRoot.cui', 'PopMenuItem', './/PopMenu[Name="Crtanje"]'),
    ('RibbonRoot.cui', 'RibbonRowPanel', './/RibbonPanelSource[@Text="Crtanje"]//RibbonRow')):
    tree = load(filename)
    if tree.xpath('.//*[@MenuMacroID="MMU_TVS_MOVE"]'):
        continue
    ref = tree.xpath('.//*[@MenuMacroID="MMU_TVS_KGTEZ"]')[0]
    old = ref
    while old.tag != tag:
        old = old.getparent()
    new = duplicate(old)
    for node in new.iter():
        for key in ('Text', 'KeyTip'):
            if key in node.attrib:
                node.set(key, 'MV' if key == 'KeyTip' else 'Povezano pomjeranje')
    tree.xpath(destination)[0].append(new)
    save(filename, tree)

for size in (16, 32):
    name = f'TVS_MOVE_{size}.png'
    entries[name] = (zipfile.ZipInfo(name), (icons/name).read_bytes())
buffer = io.BytesIO()
with zipfile.ZipFile(buffer, 'w', zipfile.ZIP_DEFLATED) as out:
    for name, (info, data) in entries.items():
        out.writestr(info, data, compress_type=zipfile.ZIP_DEFLATED)
cuix.write_bytes(buffer.getvalue())
with zipfile.ZipFile(cuix) as z:
    assert z.testzip() is None
    for name in z.namelist():
        if name.endswith(('.cui', '.xml', '.rels')):
            E.fromstring(z.read(name))
    for name in ('MenuGroup.cui', 'ToolbarRoot.cui', 'PopMenuRoot.cui', 'RibbonRoot.cui'):
        tree = E.fromstring(z.read(name))
        refs = tree.xpath('.//MenuMacro[@UID="MMU_TVS_MOVE"]') if name == 'MenuGroup.cui' else tree.xpath('.//*[@MenuMacroID="MMU_TVS_MOVE"]')
        assert len(refs) == 1, (name, len(refs))
print(cuix)
