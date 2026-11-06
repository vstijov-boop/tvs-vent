"""Source of TVS_Ventilation_ARX/acrxEntryPoint.cpp as one text: the command fragments
(commands/*.inl, included inside the app class) expanded at their #include lines, so source
contract tests see the same text as before the file was split (plan 10)."""
import pathlib,re
ROOT=pathlib.Path(__file__).resolve().parents[1]
ARX=ROOT/'TVS_Ventilation_ARX'
def entry_source():
    text=(ARX/'acrxEntryPoint.cpp').read_text(encoding='utf-8-sig')
    out=[]
    for line in text.split('\n'):
        m=re.match(r'^\t#include "commands/(\w+)\.inl"',line)
        if m:out.append((ARX/'commands'/(m.group(1)+'.inl')).read_text(encoding='utf-8-sig')[:-1])
        else:out.append(line)
    return '\n'.join(out)
