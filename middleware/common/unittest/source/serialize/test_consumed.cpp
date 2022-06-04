//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "common/serialize/create.h"


namespace casual
{
   namespace common
   {
      struct archive_maker : public ::testing::TestWithParam<const char*> 
      {
         auto param() const { return ::testing::TestWithParam<const char*>::GetParam();}
      };

      INSTANTIATE_TEST_SUITE_P( common_serialize_consumed_archive,
            archive_maker,
            ::testing::Values("yaml", "json", "xml"));

      TEST_P( archive_maker, default_ctor)
      {
         common::unittest::Trace trace;

         auto writer = serialize::create::writer::from( this->param());

         {
            int int_value = 42;
            writer << CASUAL_NAMED_VALUE( int_value);
         }


         EXPECT_NO_THROW({
            auto stream = writer.consume< std::stringstream>();
            auto archive = serialize::create::reader::consumed::from( this->param(), stream);
         });
      }

      TEST_P( archive_maker, validate_basic__expect_throw)
      {
         common::unittest::Trace trace;

         auto writer = serialize::create::writer::from( this->param());

         {
            int int_value = 42;
            writer << CASUAL_NAMED_VALUE( int_value);
         }

         auto stream = writer.consume< std::stringstream>();

         EXPECT_CODE({
            auto archive = serialize::create::reader::consumed::from( this->param(), stream);
            archive.validate();
         }, code::casual::invalid_configuration) << "stream: " << stream.str();
      }

      TEST_P( archive_maker, validate_sequence__expect_throw)
      {
         common::unittest::Trace trace;

         auto writer = serialize::create::writer::from( this->param());

         {
            std::vector< int> sequence{ 42, 43};
            writer << CASUAL_NAMED_VALUE( sequence);
         }

         auto stream = writer.consume< std::stringstream>();

         EXPECT_CODE({
            auto archive = serialize::create::reader::consumed::from(  this->param(), stream);
            archive.validate();
         }, code::casual::invalid_configuration) << "stream: " << stream.str();
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
                  CASUAL_SERIALIZE( my_long);
                  CASUAL_SERIALIZE( my_string);
                  CASUAL_SERIALIZE( sequence);
               )
            };
         } // <unnamed>
      } // local

      TEST_P( archive_maker, validate_composite__expect_throw)
      {
         common::unittest::Trace trace;

         auto writer = serialize::create::writer::from( this->param());
         
         {
            local::Composite composite;
            writer << CASUAL_NAMED_VALUE( composite);
         }

         auto stream = writer.consume< std::stringstream>();

         EXPECT_CODE({
            auto archive = serialize::create::reader::consumed::from(  this->param(), stream);
            archive.validate();
         }, code::casual::invalid_configuration);
      }



      TEST_P( archive_maker, consume_archive__validate_sequence__expect_no_throw)
      {
         common::unittest::Trace trace;
         
         const std::vector< int> expected{ 42, 43};

         auto writer = serialize::create::writer::from( this->param());

         {
            auto& sequence = expected;
            writer << CASUAL_NAMED_VALUE( sequence);
         }

         auto stream = writer.consume< std::stringstream>();
         auto reader = serialize::create::reader::consumed::from( this->param(), stream);

         std::vector< int> sequence;

         reader >> CASUAL_NAMED_VALUE( sequence);

         EXPECT_NO_THROW({  
            reader.validate();
         });

         EXPECT_TRUE( sequence == expected);
      }

      TEST_P( archive_maker, consume_archive__validate_unnamed_sequence__expect_no_throw)
      {
         common::unittest::Trace trace;
         
         const std::vector< int> expected{ 42, 43};

         auto writer = serialize::create::writer::from( this->param());
         writer << expected;

         auto stream = writer.consume< std::stringstream>();

         auto textual = stream.str();
         auto reader = serialize::create::reader::consumed::from( this->param(), stream);

         std::vector< int> sequence;

         reader >> sequence;

         EXPECT_NO_THROW({  
            reader.validate();
         }) << "textual: " << textual;

         EXPECT_TRUE( sequence == expected);
      }



      TEST_P( archive_maker, consume_archive__composite_validate__expect_no_throw)
      {
         common::unittest::Trace trace;
         
         auto writer = serialize::create::writer::from( this->param());

         {
            local::Composite composite;
            composite.my_long = 42;
            composite.my_string = "foo";
            composite.sequence = { 42, 43};

            writer << CASUAL_NAMED_VALUE( composite);
         }

         auto stream = writer.consume< std::stringstream>();
         auto reader = serialize::create::reader::consumed::from( this->param(), stream);

         local::Composite composite;

         reader >> CASUAL_NAMED_VALUE( composite);

         EXPECT_NO_THROW({  
            reader.validate();
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
                  CASUAL_SERIALIZE( my_long);
                  CASUAL_SERIALIZE( composite);
                  CASUAL_SERIALIZE( sequence);
               )
            };
         } // <unnamed>
      } // local

      TEST_P( archive_maker, consume_archive__nested_validate__expect_no_throw)
      {
         common::unittest::Trace trace;
         
         auto writer = serialize::create::writer::from( this->param());

         {
            local::Nested nested;
            nested.my_long = 42;
            nested.composite.my_long = 43;
            nested.composite.my_string = "foo";
            nested.composite.sequence = { 42, 43};
            nested.sequence.push_back( nested.composite);

            writer << CASUAL_NAMED_VALUE( nested);
         }

         auto stream = writer.consume< std::stringstream>();
         auto reader = serialize::create::reader::consumed::from( this->param(), stream);

         local::Nested nested;

         reader >> CASUAL_NAMED_VALUE( nested);

         EXPECT_NO_THROW({  
            reader.validate();
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
                        CASUAL_SERIALIZE( value1);
                        CASUAL_SERIALIZE( value2);
                        CASUAL_SERIALIZE( active);
                     )
                  };

                  std::string name = "charlie";
                  long age = 42;
                  Nested composite= { 423, 56, true};
                  std::vector< Nested> nested = { { 43, 345, false}, { 6666, 232, true}};

                  std::vector< long> plain_sequence = { 3, 43, 4, 543, 5, 34, 345, 3};

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( name);
                     CASUAL_SERIALIZE( age);
                     CASUAL_SERIALIZE( composite);
                     CASUAL_SERIALIZE( nested);
                     CASUAL_SERIALIZE( plain_sequence);
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
                     CASUAL_SERIALIZE( extra_name);
                  )
               };
               
            } // input
            
         } // <unnamed>
      } // local

      TEST_P( archive_maker, complex_input__deserialize_slightly_different_structure___expect_throw)
      {
         common::unittest::Trace trace;

         auto writer = serialize::create::writer::from( this->param());
         
         // input
         {
            local::input::Composite composite;
            writer << CASUAL_NAMED_VALUE( composite);
         }

         EXPECT_CODE(
         {
            auto stream = writer.consume< std::stringstream>();
            auto reader = serialize::create::reader::consumed::from( this->param(), stream);
            local::structure::Composite composite;
            reader >> CASUAL_NAMED_VALUE( composite);
            reader.validate();

         }, code::casual::invalid_configuration);
      }
   } // common
} // casual