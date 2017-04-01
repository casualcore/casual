      *****************************************************
      * X/Open CAE Specification
      * Distributed Transaction Processing:
      * The XATMI Specification
      * ISBN: 1-85912-130-6
      * X/Open Document Number: C506
      *****************************************************
        IDENTIFICATION DIVISION.
        PROGRAM-ID. SIMPCLI.
        ENVIRONMENT DIVISION.
        CONFIGURATION SECTION.
        DATA DIVISION.
        WORKING-STORAGE SECTION.
        01 TPTYPE-REC. 
           COPY TPTYPE.

        01 TPSTATUS-REC. 
           COPY TPSTATUS.

        01 TPSVCDEF-REC. 
           COPY TPSVCDEF.

        01  SEND-STRING            PIC X(30) VALUE SPACES.
        01  RECV-STRING            PIC X(30) VALUE SPACES.

      ******************************************************
      * Start program
      ******************************************************
        PROCEDURE DIVISION.
        START-CSIMPCL.
           ACCEPT SEND-STRING FROM COMMAND-LINE.
           MOVE LENGTH OF SEND-STRING TO LEN.
      
           DISPLAY "Start".
      
           DISPLAY "Call TPCALL".
           MOVE SPACE TO RECV-STRING.
           DISPLAY "SEND-STRING: |" SEND-STRING "|".
           PERFORM DO-TPCALL. 
           DISPLAY "RECV-STRING: |" RECV-STRING "|".

           DISPLAY "Call TPACALL".
           DISPLAY "SEND-STRING: |" SEND-STRING "|".
           PERFORM DO-TPACALL. 
           DISPLAY "COMM-HANDLE: |" COMM-HANDLE "|".

           DISPLAY "Call TPGETRPLY".
           MOVE SPACE TO RECV-STRING.
           PERFORM DO-TPGETRPLY. 
           DISPLAY "RECV-STRING: |" RECV-STRING "|".

           DISPLAY "End".
           PERFORM EXIT-PROGRAM. 

      *****************************************************
      *  Issue a TPACALL
      *****************************************************
        DO-TPACALL.
      *    MOVE 2 TO LEN.
           MOVE "X_OCTET" TO REC-TYPE.
      
           MOVE "TOUPPER" TO SERVICE-NAME.
           SET TPBLOCK TO TRUE.
           SET TPNOTRAN TO TRUE.
           SET TPREPLY TO TRUE.
           SET TPNOTIME TO TRUE.
           SET TPSIGRSTRT TO TRUE.
       
           CALL "TPACALL" USING TPSVCDEF-REC 
                TPTYPE-REC 
                SEND-STRING
                TPSTATUS-REC. 
      
           IF NOT TPOK
                DISPLAY "TPCALL Failed"
           END-IF.
      
      *****************************************************
      *  Issue a TPGETRPLY
      *****************************************************
        DO-TPGETRPLY.
      *    MOVE 2 TO LEN.
           MOVE "X_OCTET" TO REC-TYPE.
      
           MOVE "TOUPPER" TO SERVICE-NAME.
           SET TPBLOCK TO TRUE.
           SET TPNOTRAN TO TRUE.
           SET TPGETHANDLE TO TRUE.
           SET TPNOCHANGE TO TRUE.
           SET TPNOTIME TO TRUE.
           SET TPSIGRSTRT TO TRUE.
       
           CALL "TPGETRPLY" USING TPSVCDEF-REC 
                TPTYPE-REC 
                RECV-STRING
                TPSTATUS-REC. 
      
           IF NOT TPOK
                DISPLAY "TPCALL Failed"
           END-IF.
      
      *****************************************************
      *  Issue a TPCALL
      *****************************************************
        DO-TPCALL.
      *    MOVE 2 TO LEN.
           MOVE "X_OCTET" TO REC-TYPE.
      
           MOVE "TOUPPER" TO SERVICE-NAME.
           SET TPBLOCK TO TRUE.
           SET TPNOTRAN TO TRUE.
           SET TPNOTIME TO TRUE.
           SET TPSIGRSTRT TO TRUE.
           SET TPCHANGE TO TRUE.
       
           CALL "TPCALL" USING TPSVCDEF-REC 
                TPTYPE-REC 
                SEND-STRING
                TPTYPE-REC 
                RECV-STRING
                TPSTATUS-REC. 
      
           IF NOT TPOK
                DISPLAY "TPCALL Failed"
           END-IF.
      
      *****************************************************
      *Leave Application
      *****************************************************
        EXIT-PROGRAM.
           STOP RUN.

