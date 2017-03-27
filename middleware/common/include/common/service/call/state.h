//!
//! casual
//!

#ifndef CASUAL_COMMON_SERVICE_CALL_STATE_H_
#define CASUAL_COMMON_SERVICE_CALL_STATE_H_



#include "common/signal.h"
#include "common/timeout.h"
#include "common/platform.h"
#include "common/uuid.h"

namespace casual
{
   namespace common
   {
      namespace service
      {

         namespace call
         {
            using descriptor_type = platform::descriptor::type;


            struct State
            {

               struct Pending
               {
                  struct Descriptor
                  {
                     Descriptor( descriptor_type descriptor, bool active = true)
                       : descriptor( descriptor), active( active) {}

                     descriptor_type descriptor;
                     bool active;
                     Uuid correlation;
                     common::Timeout timeout;

                     friend bool operator == ( descriptor_type cd, const Descriptor& d) { return cd == d.descriptor;}
                     friend bool operator == ( const Descriptor& d, descriptor_type cd) { return cd == d.descriptor;}
                  };


                  Pending();

                  //!
                  //! Reserves a descriptor and associates it to message-correlation
                  //!
                  Descriptor& reserve( const Uuid& correlation);

                  void unreserve( descriptor_type descriptor);

                  bool active( descriptor_type descriptor) const;

                  const Descriptor& get( descriptor_type descriptor) const;
                  const Descriptor& get( const Uuid& correlation) const;

                  //!
                  //! Tries to discard descriptor, throws if fail.
                  //!
                  void discard( descriptor_type descriptor);

                  signal::timer::Deadline deadline( descriptor_type descriptor, const platform::time::point::type& now) const;

                  //!
                  //! @returns true if there are no pending replies or associated transactions.
                  //!  Thus, it's ok to do a service-forward
                  //!
                  bool empty() const;

               private:

                  Descriptor& reserve();

                  std::vector< Descriptor> m_descriptors;

               } pending;
            };


         } // call
      } // service

   } // common


} // casual

#endif // STATE_H_
