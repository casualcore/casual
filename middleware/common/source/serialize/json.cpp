//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/serialize/json.h"
#include "common/serialize/create.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

#include "common/transcode.h"
#include "common/functional.h"
#include "common/buffer/type.h"

// TODO: Move this to makefile
#define RAPIDJSON_HAS_STDSTRING 1

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

#include <iterator>
#include <istream>
#include <functional>

#include <iostream>


namespace casual
{
   namespace common
   {
      namespace serialize
      {
         namespace json
         {
            namespace local
            {
               namespace
               {
                  constexpr auto keys() 
                  {
                     using namespace std::string_view_literals; 
                     return array::make( "json"sv, ".json"sv, "jsn"sv, ".jsn"sv, buffer::type::json);
                  };

                  namespace reader
                  {
                     namespace check
                     {
                        // This is a help to check some to avoid terminate (via assert)
                        template<typename C, typename F>
                        auto trivial( const rapidjson::Value* const value, C&& checker, F&& fetcher)
                        {
                           if( invoke( checker, value))
                              return invoke( fetcher, value);

                           // TODO operations: more information about what type and so on...
                           code::raise::error( code::casual::invalid_node, "unexpected type");
                        }

                        auto string( const rapidjson::Value* const value)
                        {
                           if( invoke( &rapidjson::Value::IsString, value))
                              return std::string_view{ value->GetString(), value->GetStringLength()};

                           // TODO operations: more information about what type and so on...
                           code::raise::error( code::casual::invalid_node, "unexpected type");
                        }
                     } // check

                            
                     const rapidjson::Document& parse( rapidjson::Document& document, const char* json)
                     {
                        // To support empty documents
                        if( ! json || json[ 0] == '\0')
                           document.Parse( "{}");
                        else
                           document.Parse( json);
                        
                        if( document.HasParseError())
                           code::raise::error( code::casual::invalid_document, rapidjson::GetParseError_En( document.GetParseError()));
                        
                        return document;
                     }



                     const rapidjson::Document& parse( rapidjson::Document& document, std::istream& stream)
                     {
                        // note: istreambuf_iterator does not skip whitespace, which is what we want.
                        const std::string buffer{
                           std::istreambuf_iterator<char>(stream),
                           {}};

                        return parse( document, buffer.c_str());
                     }

                     const rapidjson::Document& parse( rapidjson::Document& document, const std::string& json)
                     {
                        return parse( document, json.c_str());
                     }

                     const rapidjson::Document& parse( rapidjson::Document& document, const char* json, const platform::size::type size)
                     {
                        // To ensure null-terminated string
                        const std::string buffer{ json, json + size};
                        return parse( document, buffer.c_str());
                     }

                     const rapidjson::Document& parse( rapidjson::Document& document, const platform::binary::type& json)
                     {
                        if( ! json.empty() && json.back() == '\0')
                           return parse( document, json.data());
                        else
                           return parse( document, json.data(), json.size());
                     }

                     namespace canonical
                     {
                        struct Parser 
                        {
                           using Node = rapidjson::Value;
                           auto operator() ( const Node& document)
                           {
                              deduce( document, nullptr);
                              return std::exchange( m_canonical, {});
                           }

                        private:

                           void deduce( const Node& node, const char* name)
                           {
                              switch( node.GetType())
                              { 
                                 case rapidjson::Type::kNumberType: scalar( node, name); break;
                                 case rapidjson::Type::kStringType: scalar( node, name); break;
                                 case rapidjson::Type::kTrueType: scalar( node, name); break;
                                 case rapidjson::Type::kFalseType: scalar( node, name); break;
                                 case rapidjson::Type::kArrayType: sequence( node, name); break;
                                 case rapidjson::Type::kObjectType: map( node, name); break;
                                 case rapidjson::Type::kNullType: /*???*/ break;
                              }
                           }

                           void scalar( const Node& node, const char* name)
                           {
                              m_canonical.attribute( name);
                           }

                           void sequence( const Node& node, const char* name)
                           {
                              m_canonical.container_start( name);

                              for( auto current = node.Begin(); current != node.End(); ++current)
                                 deduce( *current, nullptr);  
                              
                              m_canonical.container_end();
                           }

                           void map( const Node& node, const char* name)
                           {
                              m_canonical.composite_start( name);

                              for( auto current = node.MemberBegin(); current != node.MemberEnd(); ++current)
                                 deduce( current->value, current->name.GetString());
                              
                              m_canonical.composite_end();
                           }

                           policy::canonical::Representation m_canonical;
                        };

                        auto parse( const rapidjson::Value& document)
                        {
                           return Parser{}( document);
                        }

                     } // canonical

                     class Implementation
                     {
                     public:

                        constexpr static auto archive_type() { return archive::Type::static_need_named;}

                        constexpr static auto keys() { return local::keys();}

                        template< typename... Ts>
                        explicit Implementation( Ts&&... ts) : m_stack{ & reader::parse( m_document, std::forward< Ts>( ts)...)} 
                        {
                           
                        }
                        ~Implementation() = default;

                        std::tuple< platform::size::type, bool> container_start( platform::size::type size, const char* name)
                        {
                           auto node = structured_node( name);

                           if( ! node)
                              return std::make_tuple( 0, false);

                           // This check is to avoid terminate (via assert)
                           if( ! node->IsArray())
                              code::raise::error( code::casual::invalid_node, "expected array - name: ", name ? name : "");

                           // Stack 'em backwards

                           auto range = range::reverse( range::make( node->Begin(), node->End()));

                           for( auto& value : range)
                              m_stack.push_back( &value);

                           return std::make_tuple( range.size(), true);
                        }

                        void container_end( const char*)
                        {
                           m_stack.pop_back();
                        }

                        bool composite_start( const char* name)
                        {
                           if( auto node = structured_node( name))
                           {
                              // This check is to avoid terminate (via assert)
                              if( ! node->IsObject())
                                 code::raise::error( code::casual::invalid_node, "expected object - name: ", name ? name : "");

                              return true;
                           }
                           return false;
                        }

                        void composite_end( const char*)
                        {
                           m_stack.pop_back();
                        }


                        template< typename T>
                        bool read( T& value, const char* name)
                        {
                           if( name)
                           {
                              if( auto child = named_child( name); child && ! child->IsNull())
                              {
                                 read( child, value);               
                                 return true;
                              }
                              return false;
                           }
                           
                           // we assume it is a value from a previous pushed sequence node.
                           // and we consume it.
                           read( m_stack.back(), value);
                           m_stack.pop_back();
                           
                           return true;
                        }

                        policy::canonical::Representation canonical()
                        {
                           return canonical::parse( m_document);
                        }
                     
                     private:

                        const rapidjson::Value* named_child( const char* name)
                        {
                           assert( name);

                           auto node = m_stack.back();

                           if( auto found = node->FindMember( name); found != node->MemberEnd())
                              return &found->value;

                           return nullptr;
                        }
                        
                        const rapidjson::Value* structured_node( const char* name)
                        {
                           // if not named, we assume it is a value from a previous pushed sequence node, or root document
                           if( ! name)
                              return m_stack.back();

                           if( auto node = named_child( name))
                           {
                              // we push it to promote the node to 'current scope'. 
                              // composite_end/container_end will pop it.
                              m_stack.push_back( node);
                              return m_stack.back();
                           }

                           return nullptr;
                        }

                        static void read( const rapidjson::Value* node, bool& value)
                        { value = check::trivial( node, &rapidjson::Value::IsBool, &rapidjson::Value::GetBool); }
                        static void read( const rapidjson::Value* node, short& value)
                        { value = check::trivial( node, &rapidjson::Value::IsInt, &rapidjson::Value::GetInt); }
                        static void read( const rapidjson::Value* node, long& value)
                        { value = check::trivial( node, &rapidjson::Value::IsInt64, &rapidjson::Value::GetInt64); }
                        static void read( const rapidjson::Value* node, long long& value)
                        { value = check::trivial( node, &rapidjson::Value::IsInt64, &rapidjson::Value::GetInt64); }
                        static void read( const rapidjson::Value* node, float& value)
                        { value = check::trivial( node, &rapidjson::Value::IsNumber, &rapidjson::Value::GetDouble); }
                        static void read( const rapidjson::Value* node, double& value)
                        { value = check::trivial( node, &rapidjson::Value::IsNumber, &rapidjson::Value::GetDouble); }
                        static void read( const rapidjson::Value* node, char& value)
                        { value = *transcode::utf8::string::decode( check::string( node)).data(); }
                        static void read( const rapidjson::Value* node, std::string& value)
                        { value = transcode::utf8::string::decode( check::string( node)); }
                        static void read( const rapidjson::Value* node, std::u8string& value)
                        { value = transcode::utf8::cast( check::string( node)); }
                        static void read( const rapidjson::Value* node, platform::binary::type& value)
                        { value = transcode::base64::decode( check::string( node)); }

                        static void read( const rapidjson::Value* node, view::Binary value)
                        { 
                           auto binary = transcode::base64::decode( check::string( node));
                           
                           if( range::size( binary) != range::size( value))
                              code::raise::error( code::casual::invalid_node, "binary size mismatch - wanted: ", range::size( value), " got: ", range::size( binary));

                           algorithm::copy( binary, std::begin( value));
                        }
                  

                        rapidjson::Document m_document;
                        std::vector< const rapidjson::Value*> m_stack;
                     };

                  } // reader
                  
                  
                  namespace writer
                  {
                     template< typename Writer>
                     class basic_implementation
                     {
                     public:

                        constexpr static auto archive_type() { return archive::Type::static_need_named;}

                        constexpr static auto keys() { return local::keys();}
                        
                        explicit basic_implementation()
                           : m_allocator( &m_document.GetAllocator()), m_stack{ &m_document}
                        {
                           m_document.SetObject();
                        }

                        platform::size::type container_start( platform::size::type size, const char* name)
                        {
                           start( name);
                           m_stack.back()->SetArray();

                           return size;
                        }

                        void container_end( const char* name)
                        {
                           end( name);
                        }

                        void composite_start( const char* name)
                        {
                           start( name);
                           m_stack.back()->SetObject();
                        }
                        void composite_end(  const char* name)
                        {
                           end( name);
                        }

                        void start( const char* name)
                        {
                           // Both AddMember and PushBack returns the parent (*this)
                           // instead of a reference to the added value (despite what
                           // the documentation says) and thus we need to do some
                           // cumbersome iterator stuff to get a reference/pointer to it

                           auto& parent = *m_stack.back();

                           if( name)
                           {
                              parent.AddMember( rapidjson::Value( name, *m_allocator), rapidjson::Value(), *m_allocator);
                              m_stack.push_back( &(*(parent.MemberEnd() - 1)).value);
                           }
                           else
                           {
                              switch( parent.GetType())
                              {
                                 case rapidjson::Type::kObjectType:
                                    // do nothing?
                                    break;

                                 default:
                                    // We assume we're in a container
                                    parent.PushBack( rapidjson::Value(), *m_allocator);
                                    m_stack.push_back( &(*(parent.End() - 1)));
                                    break;
                              }

                           }
                        }

                        template< typename T>
                        void write( const T& value, const char* name)
                        {
                           start( name);
                           write( value);
                           end( name);
                        }
                        
                        void end( const char* name)
                        {
                           m_stack.pop_back();
                        }


                        void write( const bool value) { m_stack.back()->SetBool( value); }
                        void write( const char value) { write( std::string{ value}); }
                        void write( const short value) { m_stack.back()->SetInt( value); }
                        void write( const long value) { m_stack.back()->SetInt64( value); }
                        void write( const long long value) { m_stack.back()->SetInt64( value); }
                        void write( const float value) { m_stack.back()->SetDouble( value); }
                        void write( const double value) { m_stack.back()->SetDouble( value); }
                        void write( const std::string& value) { m_stack.back()->SetString( transcode::utf8::string::encode( value), *m_allocator);}
                        void write( const std::u8string& value) { m_stack.back()->SetString( reinterpret_cast< const char*>( value.data()), value.size(), *m_allocator);}
                        void write( const platform::binary::type& value) { m_stack.back()->SetString( transcode::base64::encode( value), *m_allocator);}
                        void write( view::immutable::Binary value) { m_stack.back()->SetString( transcode::base64::encode( value), *m_allocator);}

                        const rapidjson::Document& document() const { return m_document;}

                        auto consume( platform::binary::type& destination)
                        {
                           rapidjson::StringBuffer buffer;
                           Writer writer( buffer);

                           if( m_document.Accept( writer))
                           {
                              destination.assign( buffer.GetString(), buffer.GetString() + buffer.GetSize());
                              reset();
                           }
                           else
                           {
                              // TODO: Better
                              code::raise::error( code::casual::invalid_document, "failed to write document");
                           }
                        }

                        void consume( std::ostream& json)
                        {
                           rapidjson::StringBuffer buffer;
                           Writer writer( buffer);
                           if( m_document.Accept( writer))
                           {
                              json << buffer.GetString();
                              reset();
                           }
                           else
                           {
                              // TODO: Better
                              code::raise::error( code::casual::invalid_document, "failed to write document");
                           }
                        }

                     private:

                        void reset()
                        {
                           rapidjson::Document document;
                           m_document.Swap( document);
                           m_allocator = &m_document.GetAllocator();

                           m_stack.erase( std::begin( m_stack) + 1, std::end( m_stack));
                           m_document.SetObject();
                        }

                        rapidjson::Document m_document;
                        rapidjson::Document::AllocatorType* m_allocator;
                        std::vector< rapidjson::Value*> m_stack;
                     };

                     using Implementation = basic_implementation< rapidjson::Writer< rapidjson::StringBuffer>>;

                     namespace pretty
                     {
                        using Implementation = basic_implementation< rapidjson::PrettyWriter< rapidjson::StringBuffer>>;
                     } // pretty

                  } // writer
               } // <unnamed>
            } // local

            namespace strict
            {
               serialize::Reader reader( const std::string& source) { return create::reader::strict::create< local::reader::Implementation>( source);}
               serialize::Reader reader( std::istream& source) { return create::reader::strict::create< local::reader::Implementation>( source);}
               serialize::Reader reader( const platform::binary::type& source) { return create::reader::strict::create< local::reader::Implementation>( source);}
            } // strict

            namespace relaxed
            {    
               serialize::Reader reader( const std::string& source) { return create::reader::relaxed::create< local::reader::Implementation>( source);}
               serialize::Reader reader( std::istream& source) { return create::reader::relaxed::create< local::reader::Implementation>( source);}
               serialize::Reader reader( const platform::binary::type& source) { return create::reader::relaxed::create< local::reader::Implementation>( source);}
            } // relaxed

            namespace consumed
            {    
               serialize::Reader reader( const std::string& source) { return create::reader::consumed::create< local::reader::Implementation>( source);}
               serialize::Reader reader( std::istream& source) { return create::reader::consumed::create< local::reader::Implementation>( source);}
               serialize::Reader reader( const platform::binary::type& source) { return create::reader::consumed::create< local::reader::Implementation>( source);}
            } // consumed

            namespace pretty
            {
               serialize::Writer writer()
               {
                  return serialize::create::writer::create< local::writer::pretty::Implementation>();
               }
            } // pretty

            serialize::Writer writer()
            {
               return serialize::create::writer::create< local::writer::Implementation>();
            }

         } // json
         
         namespace create
         {
            namespace reader
            {
               template struct Registration< json::local::reader::Implementation>;
            } // writer
            namespace writer
            {
               template struct Registration< json::local::writer::pretty::Implementation>;
            } // writer
         } // create

      } // serialize
   } // common
} // casual
