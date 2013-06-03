//!
//! test_xatmi_call.cpp
//!
//! Created on: Jun 2, 2013
//!     Author: Lazan
//!

#include <gtest/gtest.h>


#include "sf/xatmi_call.h"

#include "../include/test_vo.h"

namespace casual
{
   namespace sf
   {
      namespace xatmi
      {
         namespace mockup
         {
            namespace
            {
               struct xatmi_call_mirror
               {
                  void operator() ( const std::string& service, buffer::Base& input, buffer::Base& output, long flags) const
                  {
                     output.reset( input.release());
                  }
               };

               struct xatmi_send_receive_mirror
               {
                  service::call_descriptor_type operator() ( const std::string& service, buffer::Base& input, long flags)
                  {
                     m_inputBuffer = std::move( input);
                     return 10;
                  }

                  bool operator() ( service::call_descriptor_type& cd, buffer::Base& output, long flags)
                  {
                     if( cd == 10)
                     {
                        output.reset( m_inputBuffer.release());
                        return true;
                     }
                     return false;
                  }

               private:
                  buffer::Base m_inputBuffer;
               };

            }
         }


         TEST( casual_sf_xatmi_call, sync_binary_mockup_mirror)
         {
            service::basic_sync< service::policy::Binary, 0, mockup::xatmi_call_mirror> service( "someService");


            test::SimpleVO input;
            service << CASUAL_MAKE_NVP( input);

            auto reply = service.call();
            test::SimpleVO output;

            reply >> CASUAL_MAKE_NVP( output);


            EXPECT_TRUE( input.m_long == output.m_long);

         }


      }
   }
}

