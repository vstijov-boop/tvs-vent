; Read-only standard-DWG inspection. Does not require the TVS plug-in.
(defun c:TVS_DETAIL_AUDIT (/ rec name ent data typ dims polys notes unknown refs)
 (setq dims 0 polys 0 notes 0 unknown 0)
 (setq rec (tblnext "BLOCK" T))
 (while rec
  (setq name (cdr (assoc 2 rec)))
  (if (wcmatch name "TVS_SIDE_*")
   (progn
    (setq ent (entnext (tblobjname "BLOCK" name)))
    (while (and ent (/= "ENDBLK" (cdr (assoc 0 (entget ent)))))
     (setq data (entget ent) typ (cdr (assoc 0 data)))
     (cond ((= typ "DIMENSION") (setq dims (1+ dims)))
           ((= typ "LWPOLYLINE") (setq polys (1+ polys)))
           ((= typ "MTEXT") (setq notes (1+ notes)))
           (T (setq unknown (1+ unknown))))
     (setq ent (entnext ent)))))
  (setq rec (tblnext "BLOCK")))
 (setq refs (ssget "_X" '((0 . "INSERT") (2 . "TVS_SIDE_*") (410 . "Model"))))
 (princ (strcat "\nDETAIL_AUDIT refs=" (itoa (if refs (sslength refs) 0)) " dimensions=" (itoa dims) " polylines=" (itoa polys) " mtext=" (itoa notes) " unknown=" (itoa unknown)))
 (princ))
(princ)

