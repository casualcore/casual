//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <common/unittest.h>

#include "common/network/byteorder.h"

#include <typeinfo>


namespace casual
{
   namespace common
   {
      namespace network
      {

         TEST( casual_common, network_byteorder__confirm_bytes)
         {
            common::unittest::Trace trace;

            //
            // we could make this as a static check as well ...
            //
            // this is actually some kind of compilar test
            //
            EXPECT_TRUE( byteorder::bytes<bool>() == 1);
            EXPECT_TRUE( byteorder::bytes<char>() == 1);
            EXPECT_TRUE( byteorder::bytes<short>() == 2);
            EXPECT_TRUE( byteorder::bytes<int>() == 4);
            EXPECT_TRUE( byteorder::bytes<long>() == 8);
            EXPECT_TRUE( byteorder::bytes<long long>() == 8);
            EXPECT_TRUE( byteorder::bytes<float>() == 4);
            EXPECT_TRUE( byteorder::bytes<double>() == 8);
         }


         TEST( casual_common, network_byteorder__confirm_encode_types)
         {
            common::unittest::Trace trace;

            //
            // we could make this as a static check as well ...
            //
            EXPECT_TRUE( typeid( byteorder::type<bool>) == typeid(uint8_t));
            EXPECT_TRUE( typeid( byteorder::type<char>) == typeid(uint8_t));
            EXPECT_TRUE( typeid( byteorder::type<short>) == typeid(uint16_t));
            EXPECT_TRUE( typeid( byteorder::type<int>) == typeid(uint32_t));
            EXPECT_TRUE( typeid( byteorder::type<long>) == typeid(uint64_t));
            EXPECT_TRUE( typeid( byteorder::type<long long>) == typeid(uint64_t));
            EXPECT_TRUE( typeid( byteorder::type<float>) == typeid(uint32_t));
            EXPECT_TRUE( typeid( byteorder::type<double>) == typeid(uint64_t));
         }

         //
         // This will always succeed
         //
         TEST( casual_common, network_byteorder__confirm_decode_types)
         {
            common::unittest::Trace trace;

            //
            // we could make this as a static check as well ...
            //
            EXPECT_TRUE( typeid(decltype(byteorder::decode<bool>( 0))) == typeid(bool));
            EXPECT_TRUE( typeid(decltype(byteorder::decode<char>( 0))) == typeid(char));
            EXPECT_TRUE( typeid(decltype(byteorder::decode<short>( 0))) == typeid(short));
            EXPECT_TRUE( typeid(decltype(byteorder::decode<int>( 0))) == typeid(int));
            EXPECT_TRUE( typeid(decltype(byteorder::decode<long>( 0))) == typeid(long));
            EXPECT_TRUE( typeid(decltype(byteorder::decode<long long>( 0))) == typeid(long long));
            EXPECT_TRUE( typeid(decltype(byteorder::decode<float>( 0))) == typeid(float));
            EXPECT_TRUE( typeid(decltype(byteorder::decode<double>( 0))) == typeid(double));

         }

         TEST( casual_common, network_byteorder__encode_decode)
         {
            common::unittest::Trace trace;

            {
               const bool initial = true;
               const auto encoded = byteorder::encode( initial);
               const auto decoded = byteorder::decode< bool>( encoded);
               EXPECT_TRUE( initial == decoded);
            }

            {
               const bool initial = false;
               const auto encoded = byteorder::encode( initial);
               const auto decoded = byteorder::decode<bool>( encoded);
               EXPECT_TRUE( initial == decoded);
            }

            {
               const char initial = 'a';
               const auto encoded = byteorder::encode( initial);
               const auto decoded = byteorder::decode<char>( encoded);
               EXPECT_TRUE( initial == decoded);
            }

            {
               const short initial = 123;
               const auto encoded = byteorder::encode( initial);
               const auto decoded = byteorder::decode<short>( encoded);
               EXPECT_TRUE( initial == decoded);
            }

            {
               const long initial = 123456;
               const auto encoded = byteorder::encode( initial);
               const auto decoded = byteorder::decode<long>( encoded);
               EXPECT_TRUE( initial == decoded);
            }

            {
               const float initial = 3.13;
               const auto encoded = byteorder::encode( initial);
               const auto decoded = byteorder::decode<float>( encoded);
               EXPECT_TRUE( initial == decoded) << "initial: " << initial << " - decoded: " << decoded << " - type: " << typeid( encoded).name();
            }

            {
               const double initial = 1234.5678;
               const auto encoded = byteorder::encode( initial);
               const auto decoded = byteorder::decode< double>( encoded);
               EXPECT_TRUE( initial == decoded) << "initial: " << initial << " - decoded: " << decoded;
            }


            {
               const std::size_t initial = 1234;
               const auto encoded = byteorder::size::encode( initial);
               const auto decoded = byteorder::size::decode<std::size_t>( encoded);
               EXPECT_TRUE( initial == decoded) << "initial: " << initial << " - decoded: " << decoded;
            }
         }



      }
   }
}
