# Dugmad TVS_MOVE i TVS_STRETCH

30.10.2026: obje komande nalaze se u TVS_VENT.cuix: toolbar TVS_VENT,
meni Crtanje kanala i ribbon panel Crtanje kanala. TVS_MOVE je uklonjen iz
TVS_TOOLS.cuix, gdje KGTEZ i ostale postojeće komande ostaju.

- Povezano pomjeranje: ^C^C_TVS_MOVE;
- Istezanje mreže: ^C^C_TVS_STRETCH;

Ispravan naziv je TVS_STRETCH. Providne PNG ikone 16/32 ugrađene su u CUIX,
a kopije 16/32/128 su u Installer/TVS_VENT/icons. Skripta za održavanje:
tools/add_connected_cuix.py; stari add_move_cuix.py poziva novu skriptu.

Nova ikona je generisana ugrađenim image_gen alatom. Prompt: Use case:
stylized-concept. Asset: single AutoCAD toolbar raster icon, square, transparent
background. A thick simple steel-gray rectangular ventilation duct runs
horizontally from a fixed dark bracket on left to a bright blue movable end on
right. A bold blue horizontal right-pointing arrow above indicates stretching
length. Minimal orthographic 3D beveled metal style, strong dark outlines, very
few details, readable when reduced to 16 and 32 pixels. Centered with narrow
transparent margin. No letters, numbers, words, watermark or background.
This is the TVS_STRETCH icon.

Zamijeniti oba CUIX fajla uz zatvoren AutoCAD, nakon čuvanja prethodnih kopija:
- Contents/TVS_VENT/Resources/TVS_VENT.cuix
- Contents/TVS_TOOLS/Resources/TVS_TOOLS.cuix
unutar C:/ProgramData/Autodesk/ApplicationPlugins/TVS.bundle/.
Ako prikaz ostane stari, pri zatvorenom CAD-u preimenovati odgovarajuće .mnr
fajlove u .mnr.old i po potrebi učitati grupe kroz CUILOAD.

CUIX ne zahtijeva kompilaciju. TVS_MOVE radi sa postojećim ARX-om; novo dugme
TVS_STRETCH će raditi tek nakon budućeg builda. Build nije pokrenut.
Provjereni ZIP/XML, reference i očuvanje ostalih makroa/bitmapa; stvarni izgled
u AutoCAD-u treba potvrditi.
