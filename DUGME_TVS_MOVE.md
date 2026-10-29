# Dugme TVS_MOVE

U TVS_TOOLS.cuix dodato je **Povezano pomjeranje**, sa providnim ikonama 16×16 i
32×32, u toolbaru **Aksonometrija i crtanje**, meniju **Crtanje** i ribbon panelu
**Crtanje**. Makro je `^C^C_TVS_MOVE;` i koristi postojeću ARX komandu.

Motiv: kanal sa dva koljena i plavom dvostranom strelicom za pomjeranje. Ikona
je generisana ugrađenim image_gen alatom, pa prilagođena standardnim dimenzijama.
Kopije su u `Installer/TVS_TOOLS/icons/TVS_MOVE_16.png`, `TVS_MOVE_32.png` i
`TVS_MOVE_128.png`. Izvorni prompt: ortogonalni metalni kanal u obliku obrnutog U,
plava dvostrana vertikalna strelica, providna pozadina, bez teksta, čitljivo u
malim dimenzijama. Skripta održavanja: `tools/add_move_cuix.py`.

Zatvoriti AutoCAD, sačuvati postojeći TVS_TOOLS.cuix van ApplicationPlugins i
zamijeniti fajl na lokaciji:
`C:/ProgramData/Autodesk/ApplicationPlugins/TVS.bundle/Contents/TVS_TOOLS/Resources/TVS_TOOLS.cuix`.
Ako je ikona keširana, pri zatvorenom AutoCAD-u preimenovati TVS_TOOLS.mnr u
TVS_TOOLS.mnr.old. Po potrebi ponovo učitati grupu kroz CUILOAD.

Dugme već radi sa prethodno instaliranim TVS_MOVE ARX-om; ne dodaje nove mogućnosti
u staru binarnu verziju. Korisnik je odobrio kompilaciju koja uključuje novi Enter-default i varijantu mreže. CUIX sam po sebi ne zahtijeva kompilaciju.

Provjereni su ZIP CRC, svi XML dijelovi, jedinstvena nova referenca u svakom UI
mjestu i očuvanje svih prethodnih makroa, referenci i bitmapa, uključujući KGTEZ.
Praktično učitavanje u AutoCAD-u tek treba potvrditi.
