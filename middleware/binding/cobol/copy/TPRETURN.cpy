      * Based on description of the copy-text TPRETURN in:
      * X/Open CAE Specification
      * Distributed Transaction Processing:
      * The XATMI Specification
      * ISBN: 1-85912-130-6
      * X/Open Document Number: C506

      * The document does not contain a listing of the copy
      * but describes what it does and how it should be used.
      *
      * The only discussion point is if the code sequence
      * should include "EXIT PROGRAM" or if it should use
      * "GOBACK". The C506 mentions that there is an EXIT PROGRAM
      * in the copy-text.
      * "GOBACK" is an extension that most (if not all) 
      * COBOL compilers have, and is a "universal" replacement
      * of various return mechanisms. It can be used in
      * both main and subprograms. EXIT PROGRAM shall
      * be a NOP if a "CALL" is not active. The question is then
      * how the code checks if a CALL is active, and if this test works
      * when the code is called from the CRM (e.g. Casual) that 
      * usually is not COBOL code. To be safe this code includes
      * a GOBACK after the EXIT PROGRAM.
      *
      *  TPRETURN.cpy
      *
           CALL "TPRETURN" USING
                TPSVCRET-REC
                TPTYPE-REC
                DATA-REC
                TPSTATUS-REC
           EXIT PROGRAM.
      *    display "after EXIT PROGRAM in TPRETURN"
           GOBACK.
      