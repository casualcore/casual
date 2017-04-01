      * X/Open CAE Specification
      * Distributed Transaction Processing:
      * The XATMI Specification
      * ISBN: 1-85912-130-6
      * X/Open Document Number: C506

      *
      *  TPSTATUS.cpy
      *
       05 TP-STATUS PIC S9(9) COMP-5.
               88 TPOK                      VALUE 0.
               88 TPEBADDESC                VALUE 2.
               88 TPEBLOCK                  VALUE 3.
               88 TPEINVAL                  VALUE 4.
               88 TPELIMIT                  VALUE 5.
               88 TPENOENT                  VALUE 6.
               88 TPEOS                     VALUE 7.
               88 TPEPROTO                  VALUE 9.
               88 TPESVCERR                 VALUE 10.
               88 TPESVCFAIL                VALUE 11.
               88 TPESYSTEM                 VALUE 12.
               88 TPETIME                   VALUE 13.
               88 TPETRAN                   VALUE 14.
               88 TPEGOTSIG                 VALUE 15.
               88 TPEITYPE                  VALUE 17.
               88 TPEOTYPE                  VALUE 18.
               88 TPEEVENT                  VALUE 22.
               88 TPEMATCH                  VALUE 23.

       05 TPEVENT PIC S9(9) COMP-5.
               88 TPEV-NOEVENT              VALUE 0.
               88 TPEV-DISCONIMM            VALUE 1.
               88 TPEV-SENDONLY             VALUE 2.
               88 TPEV-SVCERR               VALUE 3.
               88 TPEV-SVCFAIL              VALUE 4.
               88 TPEV-SVCSUCC              VALUE 5.

       05 APPL-RETURN-CODE PIC S9(9) COMP-5.

