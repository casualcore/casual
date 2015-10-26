//!
//! switch.h
//!
//! Created on: Jul 7, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_QUEUE_XA_SWITCH_H_
#define CASUAL_QUEUE_XA_SWITCH_H_


extern "C"
{
   extern struct xa_switch_t casual_queue_xa_switch_dynamic;
}

#ifdef __cplusplus

namespace casual
{
   namespace queue
   {
      namespace rm
      {
         int id();
      } // rm
   } // queue
} // casual



#endif

#endif // SWITCH_H_
