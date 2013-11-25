//
// test_network_byteorder.cpp
//
//  Created on: 7 nov 2013
//      Author: Kristone
//




#include <gtest/gtest.h>

#include "common/network_byteorder.h"

#include <typeinfo>


namespace casual
{
   namespace common
   {
      namespace network
      {

         TEST( casual_common, network_byteorder__confirm_bytes)
         {
            //
            // we could make this as a static check as well ...
            //
            // this is actually some kind of compilar test
            //
            EXPECT_TRUE( bytes<bool>() == 1);
            EXPECT_TRUE( bytes<char>() == 1);
            EXPECT_TRUE( bytes<short>() == 2);
            EXPECT_TRUE( bytes<long>() == 8);
            EXPECT_TRUE( bytes<float>() == 4);
            EXPECT_TRUE( bytes<double>() == 8);
         }


         TEST( casual_common, network_byteorder__confirm_encode_types)
         {
            //
            // we could make this as a static check as well ...
            //
            EXPECT_TRUE( typeid(type<bool>) == typeid(uint8_t));
            EXPECT_TRUE( typeid(type<char>) == typeid(uint8_t));
            EXPECT_TRUE( typeid(type<short>) == typeid(uint16_t));
            EXPECT_TRUE( typeid(type<long>) == typeid(uint64_t));
            EXPECT_TRUE( typeid(type<float>) == typeid(uint32_t));
            EXPECT_TRUE( typeid(type<double>) == typeid(uint64_t));

            EXPECT_TRUE( typeid(type<uint8_t>) == typeid(uint8_t));
            EXPECT_TRUE( typeid(type<uint16_t>) == typeid(uint16_t));
            EXPECT_TRUE( typeid(type<uint32_t>) == typeid(uint32_t));
            EXPECT_TRUE( typeid(type<uint64_t>) == typeid(uint64_t));

         }

         TEST( casual_common, network_byteorder__confirm_decode_types)
         {
            //
            // we could make this as a static check as well ...
            //
            EXPECT_TRUE( typeid(decltype(byteorder<bool>::decode( 0))) == typeid(bool));
            EXPECT_TRUE( typeid(decltype(byteorder<char>::decode( 0))) == typeid(char));
            EXPECT_TRUE( typeid(decltype(byteorder<short>::decode( 0))) == typeid(short));
            EXPECT_TRUE( typeid(decltype(byteorder<long>::decode( 0))) == typeid(long));
            EXPECT_TRUE( typeid(decltype(byteorder<float>::decode( 0))) == typeid(float));
            EXPECT_TRUE( typeid(decltype(byteorder<double>::decode( 0))) == typeid(double));

            EXPECT_TRUE( typeid(decltype(byteorder<uint8_t>::decode( 0))) == typeid(uint8_t));
            EXPECT_TRUE( typeid(decltype(byteorder<uint16_t>::decode( 0))) == typeid(uint16_t));
            EXPECT_TRUE( typeid(decltype(byteorder<uint32_t>::decode( 0))) == typeid(uint32_t));
            EXPECT_TRUE( typeid(decltype(byteorder<uint64_t>::decode( 0))) == typeid(uint64_t));

         }

         TEST( casual_common, network_byteorder__encode_decode)
         {
            {
               const bool initial = true;
               const auto encoded = byteorder<bool>::encode( initial);
               const auto decoded = byteorder<bool>::decode( encoded);
               EXPECT_TRUE( initial == decoded);
            }

            {
               const bool initial = false;
               const auto encoded = byteorder<bool>::encode( initial);
               const auto decoded = byteorder<bool>::decode( encoded);
               EXPECT_TRUE( initial == decoded);
            }

            {
               const char initial = 'a';
               const auto encoded = byteorder<char>::encode( initial);
               const auto decoded = byteorder<char>::decode( encoded);
               EXPECT_TRUE( initial == decoded);
            }

            {
               const short initial = 123;
               const auto encoded = byteorder<short>::encode( initial);
               const auto decoded = byteorder<short>::decode( encoded);
               EXPECT_TRUE( initial == decoded);
            }

            {
               const long initial = 123456;
               const auto encoded = byteorder<long>::encode( initial);
               const auto decoded = byteorder<long>::decode( encoded);
               EXPECT_TRUE( initial == decoded);
            }

            {
               const float initial = 3.13;
               const auto encoded = byteorder<float>::encode( initial);
               const auto decoded = byteorder<float>::decode( encoded);
               EXPECT_TRUE( initial == decoded);
            }

            {
               const double initial = 1234.5678;
               const auto encoded = byteorder<double>::encode( initial);
               const auto decoded = byteorder<double>::decode( encoded);
               EXPECT_TRUE( initial == decoded);
            }
         }



      }
   }
}
