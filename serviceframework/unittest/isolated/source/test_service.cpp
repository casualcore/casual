//!
//! test_service.cpp
//!
//! Created on: Mar 9, 2013
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include <xatmi.h>

#include "sf/archive_yaml.h"
#include "sf/buffer.h"
#include "sf/service.h"
#include "sf/server.h"


#include "../include/test_vo.h"


namespace casual
{



   namespace local
   {
      namespace
      {
         template< typename T>
         TPSVCINFO prepareYaml(T& value)
         {
            TPSVCINFO serviceInfo;

            YAML::Emitter emitter;
            sf::archive::yaml::Writer writer( emitter);

            writer << CASUAL_MAKE_NVP( value);

            sf::buffer::X_Octet buffer( "YAML");

            buffer.str( emitter.c_str());

            auto raw = buffer.release();

            serviceInfo.data = raw.buffer;
            serviceInfo.len = raw.size;

            return serviceInfo;

         }


      }

   }

   TEST( casual_sf_service_protocol_yaml, input_deserialization)
   {
      test::SimpleVO someVO;

      someVO.m_long = 456;
      someVO.m_short = 34;
      someVO.m_string = "korv";

      TPSVCINFO serviceInfo = local::prepareYaml( someVO);

      auto service = sf::service::Factory::instance().create( &serviceInfo);

      test::SimpleVO value;

      for( auto reader : service->input().readers)
      {
         (*reader) >> CASUAL_MAKE_NVP( value);
      }

      EXPECT_TRUE( value.m_long == 456);
      EXPECT_TRUE( value.m_short == 34);
      EXPECT_TRUE( value.m_string == "korv");

   }

   TEST( casual_sf_service_protocol_yaml, service_create_input_deserialization)
   {

      test::SimpleVO someVO;

      someVO.m_long = 456;
      someVO.m_short = 34;
      someVO.m_string = "korv";

      TPSVCINFO serviceInfo = local::prepareYaml( someVO);

      auto server = casual::sf::server::create( 0, 0);

      auto service_io = server->createService( &serviceInfo);

      test::SimpleVO value;

      service_io >> CASUAL_MAKE_NVP( value);

      EXPECT_TRUE( value.m_long == 456);
      EXPECT_TRUE( value.m_short == 34);
      EXPECT_TRUE( value.m_string == "korv");

   }


}



