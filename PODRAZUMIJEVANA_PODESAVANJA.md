# Podrazumijevana podešavanja koljena

Dogovor korisnika, 29.10.2026: ovo su standardna početna podešavanja za rad.

- Pravougaoni kanal: uključeno `R=Dpr*`, koeficijent **0,5**.
- Konstantni radijus: sačuvana vrijednost **150 mm**, opcija `R=` isključena.
- Okrugli kanal: uključeno `R=Dpr*`; `R=` i `Segmentno (ВСН)` isključeni.

Početne vrijednosti su u `TVS_Ventilation_ARX/Settings.h`. Koeficijent je zajednički
za pravougaona i okrugla koljena. Fiksnih 150 mm se pretvara u jedinice crteža kao
ranije, a koeficijent 0,5 ostaje bezdimenzioni. Postojeći fitinzi se ne mijenjaju.

Izmjena je prvobitno sačuvana bez kompilacije (`[skip ci]`), prema zahtjevu
korisnika. Korisnik je zatim 29.10.2026. odobrio kompilaciju zajedno sa prvom
verzijom TVS_MOVE. Nove početne vrijednosti ulaze u te pakete. Prethodno preuzeti
UCS paketi ih nemaju.
