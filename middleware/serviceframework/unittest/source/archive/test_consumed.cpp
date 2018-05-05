//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "serviceframework/archive/maker.h"


namespace casual
{
   namespace serviceframework
   {
      struct archive_maker : public ::testing::TestWithParam<const char*> 
      {
         auto param() const { return ::testing::TestWithParam<const char*>::GetParam();}
      };

      INSTANTIATE_TEST_CASE_P(casual_sf_consumed_archive,
            archive_maker,
            ::testing::Values("yaml", "json", "xml"));

      TEST_P( archive_maker, default_ctor)
      {
         common::unittest::Trace trace;

         std::stringstream stream;

         {
            auto archive = archive::writer::from::name( stream, this->param());
            int int_value = 42;

            archive << CASUAL_MAKE_NVP( int_value);
         }


         EXPECT_NO_THROW({
            auto archive = archive::reader::consumed::from::name( stream, this->param());
         });
      }

      TEST_P( archive_maker, validate_basic__expect_throw)
      {
         common::unittest::Trace trace;

         std::stringstream stream;

         {
            auto archive = archive::writer::from::name( stream, this->param());
            int int_value = 42;

            archive << CASUAL_MAKE_NVP( int_value);
         }

         EXPECT_THROW({
            auto archive = archive::reader::consumed::from::name( stream, this->param());
            archive.validate();
         }, common::exception::casual::invalid::Configuration) << "stream: " << stream.str();
      }

      TEST_P( archive_maker, validate_sequence__expect_throw)
      {
         common::unittest::Trace trace;

         std::stringstream stream;

         {
            auto archive = archive::writer::from::name( stream, this->param());
            std::vector< int> sequence{ 42, 43};

            archive << CASUAL_MAKE_NVP( sequence);
         }

         EXPECT_THROW({
            auto archive = archive::reader::consumed::from::name( stream, this->param());
            archive.validate();
         }, common::exception::casual::invalid::Configuration) << "stream: " << stream.str();
      }
   
      namespace local
      {
         namespace
         {
            struct Composite
            {
               long my_long = 0;
               std::string my_string;
               std::vector< int> sequence;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  archive & CASUAL_MAKE_NVP( my_long);
                  archive & CASUAL_MAKE_NVP( my_string);
                  archive & CASUAL_MAKE_NVP( sequence);
               )
            };
         } // <unnamed>
      } // local

      TEST_P( archive_maker, validate_composite__expect_throw)
      {
         common::unittest::Trace trace;

         std::stringstream stream;
         
         {
            auto archive = archive::writer::from::name( stream, this->param());
            local::Composite composite;
            archive << CASUAL_MAKE_NVP( composite);
         }

         EXPECT_THROW({
            auto archive = archive::reader::consumed::from::name( stream, this->param());
            archive.validate();
         }, common::exception::casual::invalid::Configuration);
      }



      TEST_P( archive_maker, consume_archive__validate_sequence__expect_no_throw)
      {
         common::unittest::Trace trace;
         
         const std::vector< int> expected{ 42, 43};

         std::stringstream stream;

         {
            auto archive = archive::writer::from::name( stream, this->param());
            auto& sequence = expected;

            archive << CASUAL_MAKE_NVP( sequence);
         }

         auto archive = archive::reader::consumed::from::name( stream, this->param());

         std::vector< int> sequence;

         archive >> CASUAL_MAKE_NVP( sequence);

         EXPECT_NO_THROW({  
            archive.validate();
         });

         EXPECT_TRUE( sequence == expected);
      }



      TEST_P( archive_maker, consume_archive__composite_validate__expect_no_throw)
      {
         common::unittest::Trace trace;
         
         std::stringstream stream;

         {
            auto archive = archive::writer::from::name( stream, this->param());
            local::Composite composite;
            composite.my_long = 42;
            composite.my_string = "foo";
            composite.sequence = { 42, 43};

            archive << CASUAL_MAKE_NVP( composite);
         }


         auto archive = archive::reader::consumed::from::name( stream, this->param());

         local::Composite composite;

         archive >> CASUAL_MAKE_NVP( composite);

         EXPECT_NO_THROW({  
            archive.validate();
         });

         std::vector< int> expected{ 42, 43};

         EXPECT_TRUE( composite.my_long == 42);
         EXPECT_TRUE( composite.my_string == "foo");
         EXPECT_TRUE( composite.sequence == expected);
      }

      namespace local
      {
         namespace
         {
            struct Nested
            {
               long my_long = 0;
               local::Composite composite;
               std::vector< local::Composite> sequence;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  archive & CASUAL_MAKE_NVP( my_long);
                  archive & CASUAL_MAKE_NVP( composite);
                  archive & CASUAL_MAKE_NVP( sequence);
               )
            };
         } // <unnamed>
      } // local

      TEST_P( archive_maker, consume_archive__nested_validate__expect_no_throw)
      {
         common::unittest::Trace trace;
         
         std::stringstream stream;

         {
            auto archive = archive::writer::from::name( stream, this->param());
            local::Nested nested;
            nested.my_long = 42;
            nested.composite.my_long = 43;
            nested.composite.my_string = "foo";
            nested.composite.sequence = { 42, 43};
            nested.sequence.push_back( nested.composite);

            archive << CASUAL_MAKE_NVP( nested);
         }


         auto archive = archive::reader::consumed::from::name( stream, this->param());

         local::Nested nested;

         archive >> CASUAL_MAKE_NVP( nested);

         EXPECT_NO_THROW({  
            archive.validate();
         });

         std::vector< int> expected{ 42, 43};

         EXPECT_TRUE( nested.my_long == 42);
         EXPECT_TRUE( nested.composite.my_long == 43);
         EXPECT_TRUE( nested.composite.my_string == "foo");
         EXPECT_TRUE( nested.sequence.size() == 1);
      }

      namespace local
      {
         namespace
         {
            namespace structure
            {
               struct Composite
               {
                  struct Nested
                  {
                     long value1 = 0;
                     short value2 = 0;
                     bool active = false;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                        archive & CASUAL_MAKE_NVP( value1);
                        archive & CASUAL_MAKE_NVP( value2);
                        archive & CASUAL_MAKE_NVP( active);
                     )
                  };

                  std::string name = "charlie";
                  long age = 42;
                  Nested composit = { 423, 56, true};
                  std::vector< Nested> nested = { { 43, 345, false}, { 6666, 232, true}};

                  std::vector< long> plain_sequence = { 3, 43, 4, 543, 5, 34, 345, 3};

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     archive & CASUAL_MAKE_NVP( name);
                     archive & CASUAL_MAKE_NVP( age);
                     archive & CASUAL_MAKE_NVP( composit);
                     archive & CASUAL_MAKE_NVP( nested);
                     archive & CASUAL_MAKE_NVP( plain_sequence);
                  )
               };
               
            } // structure

            namespace input
            {
               struct Composite : structure::Composite
               {
                  std::string extra_name = "poop";
                  
                  CASUAL_CONST_CORRECT_SERIALIZE(
                     structure::Composite::serialize( archive);
                     archive & CASUAL_MAKE_NVP( extra_name);
                  )
               };
               
            } // input
            
         } // <unnamed>
      } // local

      TEST_P( archive_maker, complex_input__deserialize_slightly_different_structure___expect_throw)
      {
         common::unittest::Trace trace;

         std::stringstream stream;
         
         // input
         {
            auto archive = archive::writer::from::name( stream, this->param());
            local::input::Composite composite;
            archive << CASUAL_MAKE_NVP( composite);
         }

         EXPECT_THROW(
         {
            auto archive = archive::reader::consumed::from::name( stream, this->param());
            local::structure::Composite composite;
            archive >> CASUAL_MAKE_NVP( composite);
            archive.validate();

         }, common::exception::casual::invalid::Configuration);
      }
   } // serviceframework
} // casual