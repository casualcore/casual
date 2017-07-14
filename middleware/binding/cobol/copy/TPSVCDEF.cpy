      * X/Open CAE Specification
      * Distributed Transaction Processing:
      * The XATMI Specification
      * ISBN: 1-85912-130-6
      * X/Open Document Number: C506

      *
      *  TPSVCDEF.cpy
      *
       05 COMM-HANDLE               PIC S9(9) COMP-5.
       05 TPBLOCK-FLAG              PIC S9(9) COMP-5.
               88 TPBLOCK                   VALUE 0.
               88 TPNOBLOCK                 VALUE 1.
       05 TPTRAN-FLAG               PIC S9(9) COMP-5.
               88 TPTRAN                    VALUE 0.
               88 TPNOTRAN                  VALUE 1.
       05 TPREPLY-FLAG              PIC S9(9) COMP-5.
               88 TPREPLY                   VALUE 0.
               88 TPNOREPLY                 VALUE 1.
       05 TPTIME-FLAG               PIC S9(9) COMP-5.
               88 TPTIME                    VALUE 0.
               88 TPNOTIME                  VALUE 1.
       05 TPSIGRSTRT-FLAG           PIC S9(9) COMP-5.
               88 TPNOSIGRSTRT              VALUE 0.
               88 TPSIGRSTRT                VALUE 1.
       05 TPGETANY-FLAG             PIC S9(9) COMP-5.
               88 TPGETHANDLE               VALUE 0.
               88 TPGETANY                  VALUE 1.
       05 TPSENDRECV-FLAG           PIC S9(9) COMP-5.
               88 TPSENDONLY                VALUE 0.
               88 TPRECVONLY                VALUE 1.
       05 TPNOCHANGE-FLAG           PIC S9(9) COMP-5.
               88 TPCHANGE                  VALUE 0.
               88 TPNOCHANGE                VALUE 1.
       05 TPSERVICETYPE-FLAG        PIC S9(9) COMP-5.
               88 TPREQRSP                  VALUE IS 0.
               88 TPCONV                    VALUE IS 1.
      * 05 SERVICE-NAME              PIC X(15).
       05 SERVICE-NAME              PIC X(127).

