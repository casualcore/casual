      * X/Open CAE Specification
      * Distributed Transaction Processing: 
      * The XATMI Specification
      * ISBN: 1-85912-130-6
      * X/Open Document Number: C506

      *
      *  TPTYPE.cpy
      *
       05 REC-TYPE                  PIC X(8).
               88 X-OCTET                   VALUE "X_OCTET".
               88 X-COMMON                  VALUE "X_COMMON".
       05 SUB-TYPE                  PIC X(16).
       05 LEN                       PIC S9(9) COMP-5.
               88 NO-LENGTH                 VALUE 0.
       05 TPTYPE-STATUS             PIC S9(9) COMP-5.
               88 TPTYPEOK                  VALUE 0.
               88 TPTRUNCATE                VALUE 1.

