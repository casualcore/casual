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
                     m_inputBuffer = &input;
                     return 10;
                  }

                  bool operator() ( service::call_descriptor_type& cd, buffer::Base& output, long flags)
                  {
                     if( cd == 10)
                     {
                        output.reset( m_inputBuffer->release());
                        return true;
                     }
                     return false;
                  }

               private:
                  static buffer::Base* m_inputBuffer;
               };

               buffer::Base* xatmi_send_receive_mirror::m_inputBuffer = nullptr;

            }
         }


         TEST( casual_sf_xatmi_call, sync_binary_mockup_mirror)
         {
            typedef service::basic_sync< service::policy::Binary, 0, mockup::xatmi_call_mirror> Service;
            Service service( "someService");


            test::SimpleVO input;
            service << CASUAL_MAKE_NVP( input);

            auto reply = service.call();
            test::SimpleVO output;

            reply >> CASUAL_MAKE_NVP( output);


            EXPECT_TRUE( input.m_long == output.m_long);

         }

         TEST( casual_sf_xatmi_call, async_blocking_binary_xatmi_send_receive_mirror)
         {
            typedef service::basic_async< service::policy::Binary, 0, mockup::xatmi_send_receive_mirror> Service;
            Service service( "someService");


            test::SimpleVO input;
            service << CASUAL_MAKE_NVP( input);

            // async send
            service.send();

            // async recive
            auto reply = service.receive();
            test::SimpleVO output;

            reply >> CASUAL_MAKE_NVP( output);

            EXPECT_TRUE( input.m_long == output.m_long);

         }

         TEST( casual_sf_xatmi_call, async_non_blocking_binary_xatmi_send_receive_mirror)
         {
            typedef service::basic_async< service::policy::Binary, TPNOBLOCK, mockup::xatmi_send_receive_mirror> Service;
            Service service( "someService");


            test::SimpleVO input;
            service << CASUAL_MAKE_NVP( input);

            // async send
            service.send();

            // async no-block recive
            auto reply = service.receive();

            if( ! reply.empty())
            {
               test::SimpleVO output;

               reply.front() >> CASUAL_MAKE_NVP( output);

               EXPECT_TRUE( input.m_long == output.m_long);
            }

         }


      }
   }
}

