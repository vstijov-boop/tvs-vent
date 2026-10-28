;;; KGTEZ.lsp - specifikacija lima: tezina pravougaonih kanala i fitinga (TVS-VENT)
;;; Komanda: KGTEZ
;;; TVS mm/cm/m: koristi jedinice iz TVS_SETTINGS, bez rucnog faktora.
;;;   1) oznaci elemente  2) dijalog (sistem, rezerva %, visina slova, debljina 2 mm)
;;;   3) tabela u AutoCAD-u (Enter = bez tabele)  4) izvoz: Excel / CSV / Ne
;;; Racuna: TVS_Pipe (kanal), TVS_TAP (koljeno), TVS_TRANS (redukcija), TVS_WYE (T-komad)
;;; Okrugli elementi (SizeB <= 0) i ne-TVS objekti se ne racunaju.

(vl-load-com)

;; ---- PODESAVANJA ----------------------------------------------------
(setq *tvs-rmode* "unutra")  ; koljeno: "unutra" = R je unutrasnji radijus, "os" = R je radijus ose
(setq *tvs-rk* 0.5)          ; R = k * Dpr
(setq *tvs-rho* 7850.0)      ; gustina celika kg/m3
(setq *tvs-wye-mode* "otv")  ; duzina glavnog dijela T-komada: "otv" = Dotv + 2*L, "dpr" = k*Dpr
(setq *tvs-wye-k* 1.0)       ; koristi se samo u modu "dpr"
(setq *tvs-wye-hole* T)      ; T = oduzmi otvor odvojka (Dotv x Hotv) od glavnog dijela T-komada (kao TVS)
(setq *tvs-elbow-geom* T)    ; T = radijus koljena se cita iz geometrije (kopija + explode + brisanje)
(setq *tvs-summary-margin* 0.5) ; margina zbirnih celija kao dio visine slova
(setq *tvs-debug* nil)        ; T = ispisuje Ri, Ro, Rc i povrsinu za svako koljeno
(setq *tvs-deb-fix* 2.0)     ; debljina (mm) kad je u dijalogu ukljucena opcija "debljina 2 mm"

;; vrijednosti dijaloga (pamte se do kraja sesije, osim sistema)
(setq *tvs-rez*  20.0)       ; rezerva na tezinu u %
(setq *tvs-th*   2.5)        ; visina slova na papiru u mm
(setq *tvs-deb2* nil)        ; T = svi elementi debljine *tvs-deb-fix*
(setq *tvs-sys*  "")         ; naziv sistema (opcionalno)
(setq *tvs-tab*  T)          ; T = napravi tabelu u AutoCAD-u
(setq *tvs-xls*  nil)        ; T = izvezi u Excel

;; Debljina lima (mm) prema vecoj stranici presjeka (mm) - PRIMJER, zamijeni svojom tablicom
(defun tvs-debljina (mx)
  (cond (*tvs-deb2*      *tvs-deb-fix*)
        ((<= mx 500)  0.6)
        ((<= mx 1000) 0.8)
        ((<= mx 1500) 1.0)
        (T 1.2)))

;; TVS_SETTINGS stores millimetres per drawing unit in this DWG Xrecord.
;; TVS COM dimensions (Dpr, SizeB, Length...) remain physical millimetres.
;; Only radii read from exploded drawing geometry require conversion here.
(setq *tvs-mm-per-unit* 1.0)
(defun tvs-drawing-unit-factor ( / data value)
  (setq data (dictsearch (namedobjdict) "TVS_DRAWING_UNITS")
        value (cdr (assoc 40 data)))
  (if (member value '(1.0 10.0 1000.0)) value 1.0))

;; Zajednicka razmjera sa TVS tabelama. Paper space koristi mm papira.
(defun tvs-table-scale-record ()
  (if (= (getvar "CVPORT") 1) "TVS_TABLE_SCALE_PAPER" "TVS_TABLE_SCALE_MODEL"))
(defun tvs-table-scale-default ( / data value)
  (setq data (dictsearch (namedobjdict) (tvs-table-scale-record))
        value (cdr (assoc 40 data)))
  (if (and (numberp value) (> value 0.0)) value
    (if (= (getvar "CVPORT") 1) 1.0 50.0)))
(defun tvs-save-table-scale (scale / key data ent ed)
  (setq key (tvs-table-scale-record)
        data (dictsearch (namedobjdict) key))
  (if data
    (progn
      (setq ent (cdr (assoc -1 data)) ed (entget ent))
      (entmod (if (assoc 40 ed) (subst (cons 40 scale) (assoc 40 ed) ed)
                (append ed (list (cons 40 scale))))))
    (progn
      (setq ent (entmakex (list '(0 . "XRECORD") '(100 . "AcDbXrecord")
                                '(280 . 1) (cons 40 scale))))
      (if ent (dictadd (namedobjdict) key ent)))))
(defun tvs-table-text-height (paper-height scale mm-per-unit)
  (/ (* paper-height scale) mm-per-unit))
(defun tvs-prompt-table-height ( / previous scale units height)
  (setq previous (tvs-table-scale-default))
  (initget 6)
  (setq scale (getreal (strcat "\nRazmjera tabele 1:<" (rtos previous 2 2)
                              "> (50 za 1:50, 1 za 1:1): ")))
  (if (null scale) (setq scale previous))
  (setq units (if (= (getvar "CVPORT") 1) 1.0 *tvs-mm-per-unit*)
        height (tvs-table-text-height *tvs-th* scale units))
  (tvs-save-table-scale scale)
  (princ (strcat "\nVisina teksta: " (rtos *tvs-th* 2 2) " mm na papiru; "
                 (rtos height 2 5) " jedinica crteza."))
  height)

;; ---- POMOCNE --------------------------------------------------------
(defun tvs-fmt (x)
  (if (equal x (fix x) 1e-6) (itoa (fix x)) (rtos x 2 1)))

(defun tvs-cell->str (v)
  (cond ((= (type v) 'STR)  v)
        ((= (type v) 'INT)  (itoa v))
        ((= (type v) 'REAL) (rtos v 2 2))
        (T "")))

;; ---- DIJALOG --------------------------------------------------------
(defun tvs-dlg-validate ( / r h)
  (setq r (distof (vl-string-subst "." "," (get_tile "rez")))
        h (distof (vl-string-subst "." "," (get_tile "th"))))
  (cond
    ((or (null r) (< r 0.0))
     (set_tile "error" "Rezerva mora biti broj >= 0.") (mode_tile "rez" 2) nil)
    ((or (null h) (<= h 0.0))
     (set_tile "error" "Visina slova mora biti broj > 0.") (mode_tile "th" 2) nil)
    (T
     (setq *tvs-rez*  r
           *tvs-th*   h
           *tvs-deb2* (= (get_tile "deb2") "1")
           *tvs-tab*  (= (get_tile "tab") "1")
           *tvs-xls*  (= (get_tile "xls") "1")
           *tvs-sys*  (vl-string-trim " " (get_tile "sys")))
     T)))

;; vraca T ako je korisnik potvrdio (OK)
(defun tvs-dialog ( / dcl f id res)
  (setq dcl (vl-filename-mktemp "kgtez" nil ".dcl")
        f   (open dcl "w"))
  (foreach l '("kgtez : dialog {"
               "  label = \"KGTEZ - Specifikacija lima\";"
               "  : edit_box { key = \"sys\"; label = \"Sistem (opcionalno):\"; edit_width = 26; }"
               "  : edit_box { key = \"rez\"; label = \"Rezerva na tezinu [%]:\"; edit_width = 8; }"
               "  : edit_box { key = \"th\";  label = \"Visina slova na papiru (mm):\"; edit_width = 8; }"
               "  : toggle   { key = \"deb2\"; label = \"Svi kanali i fitinzi debljine 2 mm\"; }"
               "  : boxed_column { label = \"Izlaz\";"
               "    : toggle { key = \"tab\"; label = \"Tabela u AutoCAD-u\"; }"
               "    : toggle { key = \"xls\"; label = \"Izvoz u Excel\"; }"
               "  }"
               "  : errtile { width = 36; }"
               "  ok_cancel;"
               "}")
    (write-line l f))
  (close f)
  (setq id (load_dialog dcl))
  (if (new_dialog "kgtez" id)
    (progn
      (set_tile "sys"  "")
      (set_tile "rez"  (tvs-fmt *tvs-rez*))
      (set_tile "th"   (tvs-fmt *tvs-th*))
      (set_tile "deb2" (if *tvs-deb2* "1" "0"))
      (set_tile "tab"  (if *tvs-tab* "1" "0"))
      (set_tile "xls"  (if *tvs-xls* "1" "0"))
      (action_tile "accept" "(if (tvs-dlg-validate) (done_dialog 1))")
      (action_tile "cancel" "(done_dialog 0)")
      (setq res (start_dialog)))
    (setq res 0))
  (unload_dialog id)
  (vl-file-delete dcl)
  (= res 1))

;; ---- GEOMETRIJA -----------------------------------------------------
;; vraca (tip redniTip dimenzije povrsina_m2 maxDim_mm duzina_m) ili nil
;; unutrasnji i vanjski radijus koljena iz geometrije: kopira koljeno, razbije kopiju,
;; procita lukove (ARC), obrise sve. Vraca (Ri Ro) ili nil.
(defun tvs-elbow-rg (obj / old cp en ss i ed rs)
  (setq old (getvar "CMDECHO"))
  (setvar "CMDECHO" 0)
  (setq cp (vla-Copy obj)
        en (vlax-vla-object->ename cp))
  (command "_.explode" en)
  (setq ss (ssget "_P") rs nil)
  (if ss
    (repeat (setq i (sslength ss))
      (setq ed (entget (ssname ss (setq i (1- i)))))
      (if (= (cdr (assoc 0 ed)) "ARC")
        (setq rs (cons (* *tvs-mm-per-unit* (cdr (assoc 40 ed))) rs)))
      (entdel (cdr (assoc -1 ed)))))
  (setvar "CMDECHO" old)
  (if (>= (length rs) 2)
    (list (apply 'min rs) (apply 'max rs))
    nil))

(defun tvs-info (obj on / a b a2 b2 rc lm len ang rg ar)
  (setq a (vlax-get-property obj 'Dpr)
        b (vlax-get-property obj 'SizeB))
  (cond
    ((<= b 0.0) nil)                                   ; okrugli - ne racuna se
    ;; ravni kanal
    ((= on "TVS_Pipe")
     (setq len (vlax-get-property obj 'Length))
     (list "Kanal" 1
           (strcat (tvs-fmt a) "x" (tvs-fmt b))
           (/ (* 2.0 (+ a b) len) 1.0e6)
           (max a b)
           (/ len 1000.0)))
    ;; koljeno: obim * radijus ose * ugao
    ((= on "TVS_TAP")
     (setq ang (vlax-get-property obj 'Angle)
           rg  (if *tvs-elbow-geom* (tvs-elbow-rg obj) nil)   ; (Ri Ro) iz geometrije koljena
           rc  (cond (rg (/ (+ (car rg) (cadr rg)) 2.0))      ; radijus ose iz crteza
                     ((= *tvs-rmode* "os") (* *tvs-rk* a))
                     (T (+ (* *tvs-rk* a) (/ a 2.0))))
           ar  (/ (* 2.0 (+ a b) rc (* ang (/ pi 180.0))) 1.0e6))
     (if *tvs-debug*
       (princ (strcat "\n[koljeno " (tvs-fmt a) "x" (tvs-fmt b) " ugao " (rtos ang 2 2)
                      (if rg (strcat "  Ri=" (rtos (car rg) 2 1) "  Ro=" (rtos (cadr rg) 2 1))
                             "  (bez geometrije)")
                      "  Rc=" (rtos rc 2 1) "  P=" (rtos ar 2 3) " m2]")))
     (list "Koljeno" 2
           (strcat (tvs-fmt a) "x" (tvs-fmt b) " ugao " (rtos ang 2 1))
           ar
           (max a b)
           nil))
    ;; redukcija: srednji obim * duzina
    ((= on "TVS_TRANS")
     (setq a2 (vlax-get-property obj 'Dpr2)
           b2 (vlax-get-property obj 'SizeB2))
     (list "Redukcija" 3
           (strcat (tvs-fmt a) "x" (tvs-fmt b) " - " (tvs-fmt a2) "x" (tvs-fmt b2)
                   " L" (tvs-fmt (vlax-get-property obj 'Length)))
           (/ (* (+ a b a2 b2) (vlax-get-property obj 'Length)) 1.0e6)
           (max a b a2 b2)
           nil))
    ;; T-komad: glavni dio + odvojak (Length)
    ((= on "TVS_WYE")
     (setq a2  (vlax-get-property obj 'Dpr2)
           b2  (vlax-get-property obj 'SizeB2)
           len (vlax-get-property obj 'Length)
           lm  (if (= *tvs-wye-mode* "dpr")
                 (* *tvs-wye-k* a)
                 (+ a2 (* 2.0 len))))
     (list "T-komad" 4
           (strcat (tvs-fmt a) "x" (tvs-fmt b) " / " (tvs-fmt a2) "x" (tvs-fmt b2))
           (/ (- (+ (* 2.0 (+ a b) lm) (* 2.0 (+ a2 b2) len))
                 (if *tvs-wye-hole* (* a2 b2) 0.0))      ; otvor odvojka u glavnom dijelu
              1.0e6)
           (max a b a2 b2)
           nil))
    (T nil)))

;; ---- AKUMULACIJA ----------------------------------------------------
;; red: (key ord maxd tip dim t0 kom duzina povrsina tezina)
(defun tvs-add (inf / tip ord dim pov mx len t0 tez key e)
  (setq tip (nth 0 inf) ord (nth 1 inf) dim (nth 2 inf)
        pov (nth 3 inf) mx  (nth 4 inf) len (nth 5 inf)
        t0  (tvs-debljina mx)
        tez (* pov (/ t0 1000.0) *tvs-rho*)
        key (strcat tip "|" dim "|" (rtos t0 2 2))
        e   (assoc key *tvs-rows*))
  (if e
    (setq *tvs-rows*
      (subst (list key ord mx tip dim t0
                   (1+ (nth 6 e))
                   (+ (nth 7 e) (if len len 0.0))
                   (+ (nth 8 e) pov)
                   (+ (nth 9 e) tez))
             e *tvs-rows*))
    (setq *tvs-rows*
      (cons (list key ord mx tip dim t0 1 (if len len 0.0) pov tez)
            *tvs-rows*))))

;; matrica podataka: zaglavlje + redovi + UKUPNO + REZERVA + UKUPNO SA REZERVOM
(defun tvs-matrix (rows / m k tK tL tA tW)
  (setq m (list (list "Rb." "Opis" "Dimenzije (mm)" "Deb. (mm)" "Kom"
                      "Duzina (m)" "Povrsina (m2)" "Tezina (kg)"))
        k 0 tK 0 tL 0.0 tA 0.0 tW 0.0)
  (foreach r rows
    (setq k (1+ k)
          tK (+ tK (nth 6 r))
          tL (+ tL (nth 7 r))
          tA (+ tA (nth 8 r))
          tW (+ tW (nth 9 r))
          m  (append m
               (list (list k (nth 3 r) (nth 4 r) (nth 5 r) (nth 6 r)
                           (if (= (nth 3 r) "Kanal") (nth 7 r) nil)
                           (nth 8 r) (nth 9 r))))))
  (append m
    (list (list "" "UKUPNO" "" nil tK tL tA tW)
          (list "" (strcat "REZERVA " (tvs-fmt *tvs-rez*) "%") "" nil nil nil nil
                (* tW (/ *tvs-rez* 100.0)))
          (list "" "UKUPNO SA REZERVOM" "" nil nil nil nil
                (* tW (+ 1.0 (/ *tvs-rez* 100.0)))))))

;; ---- TABELA U AUTOCAD-U ---------------------------------------------
(defun tvs-table (mat pt th title / doc sp nr nc tbl r c w rr lbl x rowv v)
  (setq doc (vla-get-ActiveDocument (vlax-get-acad-object))
        sp  (if (> (getvar "CVPORT") 1)
              (vla-get-ModelSpace doc)
              (vla-get-PaperSpace doc))
        nr  (+ 1 (length mat))        ; naslov + zaglavlje + redovi + ukupno
        nc  8
        tbl (vla-AddTable sp (vlax-3d-point pt) nr nc (* 2.0 th) (* 8.0 th)))
  (vl-catch-all-apply 'vla-SetTextHeight (list tbl 7 th))
  ;; Stil tabele moze imati margine i minimalne redove u drugim jedinicama.
  ;; Prvo smanji slova i margine, a konacne dimenzije postavi nakon sadrzaja.
  (vla-put-HorzCellMargin tbl (* 0.2 th))
  (vla-put-VertCellMargin tbl (* 0.15 th))
  (vla-SetText tbl 0 0 title)
  (setq r 1)
  (foreach rowv mat
    (setq c 0)
    (foreach v rowv
      (vla-SetText tbl r c (tvs-cell->str v))
      (vl-catch-all-apply 'vla-SetCellAlignment (list tbl r c 5))
      (setq c (1+ c)))
    (setq r (1+ r)))
  ;; REZERVA i UKUPNO SA REZERVOM: spoji kolone Rb. ... Povrsina u jednu celiju (tekst desno)
  (foreach rr (list (- nr 2) (- nr 1))
    (setq lbl (tvs-cell->str (nth 1 (nth (- rr 1) mat))))
    (vla-SetText tbl rr 1 "")
    (vla-SetText tbl rr 0 lbl)
    (vl-catch-all-apply 'vla-MergeCells (list tbl rr rr 0 6))
    (vl-catch-all-apply 'vla-SetCellAlignment (list tbl rr 0 6))
    ;; Lijeva i desna margina zbirnih celija prate visinu slova.
    (vl-catch-all-apply 'vla-SetMargin
      (list tbl rr 0 10 (* *tvs-summary-margin* th))))
  ;; AddTable moze povecati pocetne dimenzije zbog naslijedjenog stila.
  ;; Ponovo ih postavi tek kada su slova, margine i spajanja podeseni.
  (setq w (list 5 18 24 8 6 10 11 10)
        c 0)
  (foreach x w
    (vla-SetColumnWidth tbl c (* x th))
    (setq c (1+ c)))
  (setq r 0)
  (repeat nr
    (vla-SetRowHeight tbl r (* 2.0 th))
    (setq r (1+ r)))
  tbl)

;; ---- IZVOZ: EXCEL ---------------------------------------------------
(defun tvs-excel (mat / xl wbs wb ws cells r c)
  (setq xl (vl-catch-all-apply 'vlax-get-or-create-object (list "Excel.Application")))
  (if (or (vl-catch-all-error-p xl) (null xl))
    nil
    (progn
      (vla-put-Visible xl :vlax-true)
      (setq wbs   (vlax-get-property xl 'Workbooks)
            wb    (vlax-invoke-method wbs 'Add)
            ws    (vlax-get-property wb 'ActiveSheet)
            cells (vlax-get-property ws 'Cells)
            r 0)
      (foreach rowv mat
        (setq r (1+ r) c 0)
        (foreach v rowv
          (setq c (1+ c))
          (vlax-put-property cells 'Item r c (if v v ""))))
      (vl-catch-all-apply
        '(lambda ( / rng)
           (foreach adr (list "A1:H1" "A2:H2"
                              (strcat "A" (itoa (- r 2)) ":H" (itoa r)))
             (setq rng (vlax-get-property ws 'Range adr))
             (vlax-put-property (vlax-get-property rng 'Font) 'Bold :vlax-true))
           (vlax-invoke-method (vlax-get-property ws 'Columns) 'AutoFit)))
      (vlax-release-object cells)
      (vlax-release-object ws)
      (vlax-release-object wb)
      (vlax-release-object wbs)
      (vlax-release-object xl)
      T)))

;; ---- IZVOZ: CSV (separator ; i decimalni zarez - otvara se direktno u Excelu) ----
(defun tvs-csv (mat / fn f line)
  (if (setq fn (getfiled "Sacuvaj CSV" "KGTEZ" "csv" 1))
    (if (setq f (open fn "w"))
      (progn
        (foreach rowv mat
          (setq line "")
          (foreach v rowv
            (setq line (strcat line
                         (vl-string-subst "," "." (tvs-cell->str v))
                         ";")))
          (write-line (substr line 1 (1- (strlen line))) f))
        (close f)
        fn)
      nil)
    nil))

;; ---- KOMANDA --------------------------------------------------------
(defun c:KGTEZ ( / ss i obj on r nn nskip mat mat2 tot pt ch fn title table-height)
  (setq *tvs-mm-per-unit* (tvs-drawing-unit-factor))
  (princ (strcat "\nKGTEZ: jedinice crteza = "
                 (cond ((= *tvs-mm-per-unit* 10.0) "cm")
                       ((= *tvs-mm-per-unit* 1000.0) "m")
                       (T "mm"))
                 "; dimenzije i debljina lima u mm."))
  (setq *tvs-rows* nil nn 0 nskip 0)
  (if (setq ss (ssget))
    (if (tvs-dialog)
      (progn
        (repeat (setq i (sslength ss))
          (setq obj (vlax-ename->vla-object (ssname ss (setq i (1- i))))
                on  (vla-get-ObjectName obj))
          (cond
            ((not (wcmatch on "TVS_*")) (setq nn (1+ nn)))
            (T
             (setq r (vl-catch-all-apply 'tvs-info (list obj on)))
             (if (and r (not (vl-catch-all-error-p r)))
               (tvs-add r)
               (setq nskip (1+ nskip))))))
        (if (null *tvs-rows*)
          (princ "\nNema obracunatih TVS elemenata u izboru.")
          (progn
            ;; sortiranje: tip, najveca dimenzija, opis
            (setq *tvs-rows*
              (vl-sort *tvs-rows*
                '(lambda (a b)
                   (cond ((/= (nth 1 a) (nth 1 b)) (< (nth 1 a) (nth 1 b)))
                         ((/= (nth 2 a) (nth 2 b)) (< (nth 2 a) (nth 2 b)))
                         (T (< (nth 4 a) (nth 4 b)))))))
            (setq mat   (tvs-matrix *tvs-rows*)
                  tot   (nth (- (length mat) 3) mat)
                  title (if (/= *tvs-sys* "")
                          (strcat "SPECIFIKACIJA LIMA - SISTEM: " *tvs-sys*)
                          "SPECIFIKACIJA LIMA - KANALI I FITINZI")
                  mat2  (cons (list title) mat))
            (princ (strcat "\nObracunato redova: " (itoa (length *tvs-rows*))
                           "  |  ukupno: "
                           (rtos (nth 4 tot) 2 0) " kom, "
                           (rtos (nth 6 tot) 2 2) " m2, "
                           (rtos (nth 7 tot) 2 1) " kg, sa rezervom "
                           (tvs-fmt *tvs-rez*) "%: "
                           (rtos (nth 7 (last mat)) 2 1) " kg"
                           "  |  ignorisano ne-TVS: " (itoa nn)
                           "  |  preskoceno TVS (okrugli/nepodrzani): " (itoa nskip)))
            ;; tabela u crtezu
            (if *tvs-tab*
              (progn
                (setq table-height (tvs-prompt-table-height))
                (if (setq pt (getpoint "\nTacka umetanja tabele: "))
                  (tvs-table mat pt table-height title))))
            ;; izvoz u Excel (ako Excel nije dostupan, nudi CSV)
            (if *tvs-xls*
              (if (tvs-excel mat2)
                (princ "\nIzvezeno u Excel.")
                (progn
                  (princ "\nExcel nije dostupan - izvozim u CSV.")
                  (if (setq fn (tvs-csv mat2))
                    (princ (strcat "\nSacuvano: " fn)))))))))
      (princ "\nOtkazano.")))
  (princ))

(princ "\nKGTEZ ucitan. Komanda: KGTEZ")
(princ)
