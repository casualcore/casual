/** 
 ** Copyright (c) 2015, The casual project
 **
 ** This software is licensed under the MIT license, https://opensource.org/licenses/MIT
 **/

#pragma once

#include <xatmi.h>

#ifdef __cplusplus
extern "C"
{
#endif

   typedef struct header_data_s
   {
      char key[80];
      char value[80];
   } header_data_type;

   typedef struct header_s
   {
      header_data_type *data;
      long size;
   } header_type;

   typedef struct buffer_s
   {
      char *data;
      long size;
   } buffer_type;

   typedef struct casual_buffer_s
   {
      //!
      //! header information
      //!
      header_type header_in;
      header_type header_out;

      //!
      //! actual data
      //!
      buffer_type payload;
      buffer_type parameter;

      //!
      //! state
      //!
      long context;

      //!
      //! misc
      //!
      char service[ XATMI_SERVICE_NAME_LENGTH];
      char protocol[80];

      long descriptor;
      long lookup_key;
      long code;
   } casual_buffer_type;

   //!
   //! Communicate with service manager
   //!
   long casual_xatmi_lookup( casual_buffer_type* data);
   //!
   //! Make the call
   //!
   long casual_xatmi_send( casual_buffer_type* data);
   //!
   //! Get the response
   //!
   long casual_xatmi_receive( casual_buffer_type* data);
   //!
   //! Abort the call
   //!
   long casual_xatmi_cancel( casual_buffer_type* data);

   enum
   {
      OK,
      AGAIN,
      ERROR
   };
   enum xatmi_context
   {
      cTPINIT,
      cTPALLOC,
      cTPACALL,
      cTPGETRPLY
   };

#ifdef __cplusplus
}
#endif
