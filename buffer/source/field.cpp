//
// casual_field_buffer.cpp
//
//  Created on: 3 nov 2013
//      Author: Kristone
//

#include "buffer/field.h"

#include "common/environment.h"
#include "common/network_byteorder.h"
#include "common/buffer/pool.h"
#include "common/log.h"

#include "sf/namevaluepair.h"
#include "sf/archive/maker.h"

#include <cstring>

#include <string>
#include <map>
#include <vector>

#include <algorithm>


namespace casual
{
   namespace buffer
   {
      namespace field
      {

         namespace
         {

            template<typename T>
            T parse( const char* const where)
            {
               const auto encoded = *reinterpret_cast< const common::network::type<T>*>( where);
               return common::network::byteorder<T>::decode( encoded);
            }

            template<typename T>
            void write( char* const where, const T value)
            {
               const auto encoded = common::network::byteorder<T>::encode( value);
               std::memcpy( where, &encoded, sizeof( encoded));
            }

            class Buffer
            {
            public:

               typedef long meta_type;

               static constexpr auto meta_size = common::network::bytes<meta_type>();

               template<std::size_t offset>
               class Meta
               {
               public:

                  Meta( char* const where) : m_where( where ? where + offset : nullptr) {}

                  explicit operator bool () const
                  {return m_where != nullptr;}

                  meta_type operator() () const
                  {return parse<meta_type>( m_where);}

                  void operator() ( const meta_type value)
                  {write<meta_type>( m_where, value);}

               private:
                  char* m_where;
               };

               template<std::size_t offset>
               class Data
               {
               public:

                  Data( char* const where) : m_where( where ? where + offset : nullptr) {}

                  explicit operator bool () const
                  {return m_where != nullptr;}

                  char* operator() () const
                  {return m_where;}

               private:
                  char* m_where;
               };

               class Value
               {
               public:

                  Value( char* const buffer) :
                     id( buffer),
                     size( buffer),
                     data( buffer)
                  {}

                  static constexpr auto header() -> decltype(meta_size)
                  {return meta_size + meta_size;}

                  Meta<meta_size * 0> id;
                  Meta<meta_size * 1> size;
                  Data<meta_size * 2> data;

                  bool operator == ( const Value& other) const
                  {return this->data() == other.data();}

                  bool operator != ( const Value& other) const
                  {return this->data() != other.data();}

                  Value next() const
                  {
                     return Value( data ? data() + size() : nullptr);
                  }

               };

               Buffer( char* const buffer) :
                  size( buffer),
                  used( buffer),
                  data( buffer)
               {}

               Meta<meta_size * 0> size;
               Meta<meta_size * 1> used;
               Data<meta_size * 2> data;

               static constexpr auto header() -> decltype(meta_size)
               {return meta_size + meta_size;}

               explicit operator bool () const
               {return data ? true : false;}

               Value first() const
               {
                  return Value( data ? data() : nullptr);
               }

               Value beyond() const
               {
                  return Value( data ? data() + used() - header() : nullptr);
               }
            };

            // TODO: Doesn't have to inherit from anything, but has to implement a set of functions (like pool::basic)
            class Allocator : public common::buffer::pool::basic_pool< common::buffer::Buffer>
            {
            public:

               using types_type = common::buffer::pool::default_pool::types_type;

               static const types_type& types()
               {
                  // The types this pool can manage
                  static const types_type result{{ CASUAL_FIELD, "" }};
                  return result;
               }

               common::platform::raw_buffer_type allocate( const common::buffer::Type& type, const std::size_t size)
               {
                  constexpr auto header = Buffer::header();

                  const auto actual = size < header ? header : size;

                  m_pool.emplace_back( type, actual);

                  auto data = m_pool.back().payload.memory.data();

                  Buffer buffer{ data};
                  buffer.size( actual);
                  buffer.used( header);

                  return data;
               }

               common::platform::raw_buffer_type reallocate( const common::platform::const_raw_buffer_type handle, const std::size_t size)
               {
                  const auto result = find( handle);

                  if( result == std::end( m_pool))
                  {
                     // TODO: shouldn't this be an error (exception) ?
                     return nullptr;
                  }

                  const auto used = Buffer( result->payload.memory.data()).used();

                  const auto actual = size < used ? used : size;

                  //
                  // User may shrink a buffer, but not smaller than what's used
                  //
                  result->payload.memory.resize( actual);

                  Buffer( result->payload.memory.data()).size( actual);

                  return result->payload.memory.data();
               }
            };

         } //

      } // field

   } // buffer

   //
   // Register and define the type that can be used to get the custom pool
   //
   template class common::buffer::pool::Registration< buffer::field::Allocator>;


   namespace buffer
   {
      namespace field
      {

         using pool_type = common::buffer::pool::Registration< Allocator>;

         namespace
         {

            Buffer find_buffer( const char* const handle)
            {
               try
               {
                  auto& buffer = pool_type::pool.get( handle);
                  return Buffer( buffer.payload.memory.data());
               }
               catch( ...)
               {
                  //
                  // TODO: Perhaps have some dedicated field-logging ?
                  //
                  common::error::handler();
                  return Buffer( nullptr);
               }
            }

            int validate_id( const long id, const int type)
            {
               //
               // TODO: Shall we validate with repository/table as well ?
               //

               if( id > CASUAL_FIELD_NO_ID)
               {
                  if( type == id / CASUAL_FIELD_TYPE_BASE)
                  {
                     return CASUAL_FIELD_SUCCESS;
                  }
               }

               return CASUAL_FIELD_INVALID_ID;
            }

            int erase( const char* const handle, const long id)
            {
               if( !(id > CASUAL_FIELD_NO_ID))
               {
                  return CASUAL_FIELD_INVALID_ID;
               }

               Buffer buffer = find_buffer( handle);

               if( !buffer)
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }

               int result = CASUAL_FIELD_NO_OCCURRENCE;

               const Buffer::Value beyond = buffer.beyond();
               Buffer::Value value = buffer.first();

               while( value != beyond)
               {
                  if( value.id() == id)
                  {
                     value.id( CASUAL_FIELD_NO_ID);
                     // We found at least one
                     result = CASUAL_FIELD_SUCCESS;
                  }

                  value = value.next();

               }

               return result;

            }


            int erase( const char* const handle, const long id, long index)
            {
               if( !(id > CASUAL_FIELD_NO_ID))
               {
                  return CASUAL_FIELD_INVALID_ID;
               }

               Buffer buffer = find_buffer( handle);

               if( !buffer)
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }

               const Buffer::Value beyond = buffer.beyond();
               Buffer::Value value = buffer.first();

               while( value != beyond)
               {
                  if( value.id() == id)
                  {
                     if( !index--)
                     {
                        value.id( CASUAL_FIELD_NO_ID);
                        return CASUAL_FIELD_SUCCESS;
                     }

                  }

                  value = value.next();

               }

               return CASUAL_FIELD_NO_OCCURRENCE;

            }



            int add( const char* const handle, const long id, const int type, const char* const data, const long size)
            {
               if( const auto result = validate_id( id, type))
               {
                  return result;
               }

               Buffer buffer = find_buffer( handle);

               if( !buffer)
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }

               const auto total = buffer.used() + Buffer::Value::header() + size;

               if( total > buffer.size())
               {
                  return CASUAL_FIELD_NO_SPACE;
               }

               Buffer::Value value = buffer.beyond();

               value.id( id);
               value.size( size);
               std::memcpy( value.data(), data, size);

               buffer.used( total);

               return CASUAL_FIELD_SUCCESS;
            }

            template<typename T>
            int add( const char* const handle, const long id, const int type, const T data)
            {
               const auto encoded = common::network::byteorder<T>::encode( data);
               return add( handle, id, type, reinterpret_cast<const char*>( &encoded), sizeof encoded);
            }

            int get( const char* const handle, const long id, long index, const int type, const char** data, long* size)
            {
               if( const auto result = validate_id( id, type))
               {
                  return result;
               }

               Buffer buffer = find_buffer( handle);

               if( !buffer)
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }

               const Buffer::Value beyond = buffer.beyond();
               Buffer::Value value = buffer.first();

               while( value != beyond)
               {
                  if( value.id() == id)
                  {
                     if( !index--)
                     {
                        if( data) *data = value.data();
                        if( size) *size = value.size();
                        return CASUAL_FIELD_SUCCESS;
                     }

                  }

                  value = value.next();

               }

               return CASUAL_FIELD_NO_OCCURRENCE;

            }

            template<typename T>
            int get( const char* const handle, const long id, const long index, const int type, T& value)
            {
               const char* data = nullptr;

               if( const auto result = get( handle, id, index, type, &data, nullptr))
               {
                  return result;
               }

               value = common::network::byteorder<T>::decode( *reinterpret_cast< const common::network::type<T>*>( data));

               return CASUAL_FIELD_SUCCESS;
            }


            int first( const char* const handle, long* const id, long* const index)
            {
               const Buffer buffer = find_buffer( handle);

               if( !buffer)
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }

               const Buffer::Value beyond = buffer.beyond();
               const Buffer::Value value = buffer.first();

               if( value != beyond)
               {
                  *id = value.id();
                  *index = 0;
               }
               else
               {
                  return CASUAL_FIELD_NO_OCCURRENCE;
               }

               return CASUAL_FIELD_SUCCESS;

            }

            int next( const char* const handle, long* const id, long* const index)
            {
               //
               // This may be inefficient for large buffers
               //
               // Perhaps we shall store the occurrence as well, but then
               // insertions and removals becomes a bit more cumbersome
               //
               // An other option is to make this and double-linked-list-like
               //

               const Buffer buffer = find_buffer( handle);

               if( !buffer)
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }


               //
               // Store all found id's to estimate index
               //
               std::vector<decltype(Buffer::Value::id())> values;

               const Buffer::Value beyond = buffer.beyond();
               Buffer::Value value = buffer.first();

               while( value != beyond)
               {
                  values.push_back( value.id());

                  value = value.next();

                  if( values.back() == *id)
                  {
                     if( !(*index)--)
                     {
                        if( value != beyond)
                        {
                           *id = value.id();
                           *index = std::count( values.begin(), values.end(), *id);

                           return CASUAL_FIELD_SUCCESS;
                        }
                        else
                        {
                           return CASUAL_FIELD_NO_OCCURRENCE;
                        }
                     }

                  }

               }

               //
               // We couldn't even find the previous one
               //
               return CASUAL_FIELD_INVALID_ID;

            }


            int reset( const char* const handle)
            {
               if( Buffer buffer = find_buffer( handle))
               {
                  buffer.used( Buffer::header());
               }
               else
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }

               return CASUAL_FIELD_SUCCESS;

            }

            int explore( const char* const handle, long* const size, long* const used)
            {
               if( const Buffer buffer = find_buffer( handle))
               {
                  if( size) *size = buffer.size();
                  if( used) *used = buffer.used();
               }
               else
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }

               return CASUAL_FIELD_SUCCESS;

            }

         } //


      } // field

   } // buffer

} // casual


const char* CasualFieldDescription( const int code)
{
   switch( code)
   {
      case CASUAL_FIELD_SUCCESS:
         return "Success";
      case CASUAL_FIELD_INVALID_BUFFER:
         return "Invalid buffer";
      case CASUAL_FIELD_NO_SPACE:
         return "No space";
      case CASUAL_FIELD_NO_OCCURRENCE:
         return "No occurrence";
      case CASUAL_FIELD_UNKNOWN_ID:
         return "Unknown id";
      case CASUAL_FIELD_INVALID_ID:
         return "Invalid id";
      case CASUAL_FIELD_INVALID_TYPE:
         return "Invalid type";
      case CASUAL_FIELD_INTERNAL_FAILURE:
         return "Internal failure";
      default:
         return "Unknown code";
   }
}

int CasualFieldExploreBuffer( const char* buffer, long* const size, long* const used)
{
   return casual::buffer::field::explore( buffer, size, used);
}

int CasualFieldAddChar( char* const buffer, const long id, const char value)
{
   return casual::buffer::field::add( buffer, id, CASUAL_FIELD_CHAR, value);
}

int CasualFieldAddShort( char* const buffer, const long id, const short value)
{
   return casual::buffer::field::add( buffer, id, CASUAL_FIELD_SHORT, value);
}

int CasualFieldAddLong( char* const buffer, const long id, const long value)
{
   return casual::buffer::field::add( buffer, id, CASUAL_FIELD_LONG, value);
}
int CasualFieldAddFloat( char* const buffer, const long id, const float value)
{
   return casual::buffer::field::add( buffer, id, CASUAL_FIELD_FLOAT, value);
}

int CasualFieldAddDouble( char* const buffer, const long id, const double value)
{
   return casual::buffer::field::add( buffer, id, CASUAL_FIELD_DOUBLE, value);
}

int CasualFieldAddString( char* const buffer, const long id, const char* const value)
{
   const auto count = std::strlen( value) + 1;
   return casual::buffer::field::add( buffer, id, CASUAL_FIELD_STRING, value, count);
}

int CasualFieldAddBinary( char* const buffer, const long id, const char* const value, const long count)
{
   return casual::buffer::field::add( buffer, id, CASUAL_FIELD_BINARY, value, count);
}

int CasualFieldAddField( char* const buffer, const long id, const void* const value, const long count)
{
   const int type = id / CASUAL_FIELD_TYPE_BASE;
   return casual::buffer::field::add( buffer, id, type, reinterpret_cast<const char*>(value), count);
}


int CasualFieldGetChar( const char* const buffer, const long id, const long index, char* const value)
{
   return casual::buffer::field::get( buffer, id, index, CASUAL_FIELD_CHAR, *value);
}

int CasualFieldGetShort( const char* const buffer, const long id, const long index, short* const value)
{
   return casual::buffer::field::get( buffer, id, index, CASUAL_FIELD_SHORT, *value);
}

int CasualFieldGetLong( const char* const buffer, const long id, const long index, long* const value)
{
   return casual::buffer::field::get( buffer, id, index, CASUAL_FIELD_LONG, *value);
}

int CasualFieldGetFloat( const char* const buffer, const long id, const long index, float* const value)
{
   return casual::buffer::field::get( buffer, id, index, CASUAL_FIELD_FLOAT, *value);
}

int CasualFieldGetDouble( const char* const buffer, const long id, const long index, double* const value)
{
   return casual::buffer::field::get( buffer, id, index, CASUAL_FIELD_DOUBLE, *value);
}

int CasualFieldGetString( const char* const buffer, const long id, const long index, const char** value)
{
   return casual::buffer::field::get( buffer, id, index, CASUAL_FIELD_STRING, value, nullptr);
}

int CasualFieldGetBinary( const char* const buffer, const long id, const long index, const char** value, long* const count)
{
   return casual::buffer::field::get( buffer, id, index, CASUAL_FIELD_BINARY, value, count);
}

int CasualFieldGetField( const char* buffer, long id, long index, const void** value, long* count)
{
   const int type = id / CASUAL_FIELD_TYPE_BASE;
   return casual::buffer::field::get( buffer, id, index, type, reinterpret_cast<const char**>(value), count);
}

namespace casual
{
   namespace buffer
   {
      namespace field
      {
         namespace
         {
            //
            // TODO: Much better
            //
            // This functionality might be used by some tooling as well
            //

            namespace repository
            {
               std::map<std::string,int> name_to_type()
               {
                  return decltype(name_to_type())
                  {
                     {"short",   CASUAL_FIELD_SHORT},
                     {"long",    CASUAL_FIELD_LONG},
                     {"char",    CASUAL_FIELD_CHAR},
                     {"float",   CASUAL_FIELD_FLOAT},
                     {"double",  CASUAL_FIELD_DOUBLE},
                     {"string",  CASUAL_FIELD_STRING},
                     {"binary",  CASUAL_FIELD_BINARY},
                  };
               }


               std::map<int,std::string> type_to_name()
               {
                  return decltype(type_to_name())
                  {
                     {CASUAL_FIELD_SHORT,    "short"},
                     {CASUAL_FIELD_LONG,     "long"},
                     {CASUAL_FIELD_CHAR,     "char"},
                     {CASUAL_FIELD_FLOAT,    "float"},
                     {CASUAL_FIELD_DOUBLE,   "double"},
                     {CASUAL_FIELD_STRING,   "string"},
                     {CASUAL_FIELD_BINARY,   "binary"},
                  };
               }

               struct field
               {
                  long id; // relative id
                  std::string name;
                  std::string type;

                  template< typename A>
                  void serialize( A& archive)
                  {
                     archive & CASUAL_MAKE_NVP( id);
                     archive & CASUAL_MAKE_NVP( name);
                     archive & CASUAL_MAKE_NVP( type);
                  }
               };

               struct group
               {
                  long base;
                  std::vector< field> fields;

                  template< typename A>
                  void serialize( A& archive)
                  {
                     archive & CASUAL_MAKE_NVP( base);
                     archive & CASUAL_MAKE_NVP( fields);
                  }
               };

               struct mapping
               {
                  long id;
                  std::string name;
               };

               std::vector<group> fetch_groups()
               {
                  decltype(fetch_groups()) groups;

                  try
                  {
                     const auto file = common::environment::variable::get( "CASUAL_FIELD_TABLE");

                     auto archive = sf::archive::reader::makeFromFile( file);

                     archive >> CASUAL_MAKE_NVP( groups);
                  }
                  catch( ...)
                  {
                     // Make sure this is logged once
                     common::error::handler();
                  }

                  return groups;

               }

               std::vector<field> fetch_fields()
               {
                  const auto groups = fetch_groups();

                  decltype(fetch_fields()) fields;

                  for( const auto& group : groups)
                  {
                     for( const auto& field : group.fields)
                     {
                        try
                        {
                           const auto id = name_to_type().at( field.type) * CASUAL_FIELD_TYPE_BASE + group.base + field.id;

                           if( id > CASUAL_FIELD_NO_ID)
                           {
                              fields.push_back( field);
                              fields.back().id = id;
                           }
                           else
                           {
                              // TODO: Much better
                              common::log::error << "id for " << field.name << " is invalid" << std::endl;
                           }
                        }
                        catch( const std::out_of_range&)
                        {
                           // TODO: Much better
                           common::log::error << "type for " << field.name << " is invalid" << std::endl;
                        }
                     }
                  }

                  return fields;

               }

               std::map<std::string,long> name_to_id()
               {
                  const auto fields = fetch_fields();

                  decltype( name_to_id()) result;

                  for( const auto& field : fields)
                  {
                     if( !result.emplace( field.name, field.id).second)
                     {
                        // TODO: Much better
                        common::log::error << "name for " << field.name << " is not unique" << std::endl;
                     }
                  }

                  return result;
               }

               std::map<long,std::string> id_to_name()
               {
                  const auto fields = fetch_fields();

                  decltype( id_to_name()) result;

                  for( const auto& field : fields)
                  {
                     if( !result.emplace( field.id, field.name).second)
                     {
                        // TODO: Much better
                        common::log::error << "id for " << field.name << " is not unique" << std::endl;
                     }
                  }

                  return result;

               }

            } // repository
         } //
      } // field
   } // buffer
} // casual


int CasualFieldNameOfId( const long id, const char** name)
{
   static const auto mapping = casual::buffer::field::repository::id_to_name();

   if( id > CASUAL_FIELD_NO_ID)
   {
      try
      {
         const auto& result = mapping.at( id);
         if( name) *name = result.c_str();
      }
      catch( const std::out_of_range&)
      {
         return CASUAL_FIELD_UNKNOWN_ID;
      }
   }
   else
   {
      return CASUAL_FIELD_INVALID_ID;
   }

   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldIdOfName( const char* const name, long* const id)
{
   static const auto mapping = casual::buffer::field::repository::name_to_id();

   if( name)
   {
      try
      {
         const auto& result = mapping.at( name);
         if( id) *id = result;
      }
      catch( const std::out_of_range&)
      {
         return CASUAL_FIELD_UNKNOWN_ID;
      }
   }
   else
   {
      return CASUAL_FIELD_INVALID_ID;
   }

   return CASUAL_FIELD_SUCCESS;

}

int CasualFieldTypeOfId( const long id, int* const type)
{
   //
   // CASUAL_FIELD_SHORT is 0 'cause FLD_SHORT is 0 and thus we need to
   // represent invalid id with something else than 0
   //

   if( id > CASUAL_FIELD_NO_ID)
   {
      const int result = id / CASUAL_FIELD_TYPE_BASE;

      switch( result)
      {
         case CASUAL_FIELD_SHORT:
         case CASUAL_FIELD_LONG:
         case CASUAL_FIELD_CHAR:
         case CASUAL_FIELD_FLOAT:
         case CASUAL_FIELD_DOUBLE:
         case CASUAL_FIELD_STRING:
         case CASUAL_FIELD_BINARY:
            break;
         default:
            return CASUAL_FIELD_INVALID_ID;
      }

      if( type) *type = result;

   }
   else
   {
      return CASUAL_FIELD_INVALID_ID;
   }

   return CASUAL_FIELD_SUCCESS;

}

int CasualFieldNameOfType( const int type, const char** name)
{
   static const auto mapping = casual::buffer::field::repository::type_to_name();

   try
   {
      const auto& result = mapping.at( type);
      if( name) *name = result.c_str();
   }
   catch( const std::out_of_range&)
   {
      return CASUAL_FIELD_INVALID_TYPE;
   }

   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldTypeOfName( const char* const name, int* const type)
{
   static const auto mapping = casual::buffer::field::repository::name_to_type();

   try
   {
      const auto& result = mapping.at( name);
      if( type) *type = result;
   }
   catch( const std::exception&)
   {
      return CASUAL_FIELD_INVALID_TYPE;
   }

   return CASUAL_FIELD_SUCCESS;
}


int CasualFieldExist( const char* const buffer, const long id, const long index)
{
   //
   // Perhaps we can use casual::buffer::field::next, but not yet
   //
   const int type = id / CASUAL_FIELD_TYPE_BASE;
   return casual::buffer::field::get( buffer, id, index, type, nullptr, nullptr);
}

int CasualFieldRemoveAll( char* const buffer)
{
   return casual::buffer::field::reset( buffer);
}

int CasualFieldRemoveId( char* buffer, const long id)
{
   return casual::buffer::field::erase( buffer, id);
}

int CasualFieldRemoveOccurrence( char* const buffer, const long id, long index)
{
   return casual::buffer::field::erase( buffer, id, index);
}

int CasualFieldCopyBuffer( char* const target, const char* const source)
{
   long used;
   if( const auto result = casual::buffer::field::explore( source, nullptr, &used))
   {
      return result;
   }

   long size;
   if( const auto result = casual::buffer::field::explore( target, &size, nullptr))
   {
      return result;
   }

   if( size < used)
   {
      return CASUAL_FIELD_NO_SPACE;
   }

   std::memcpy( target, source, used);

   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldFirst( const char* const buffer, long* const id, long* const index)
{
   return casual::buffer::field::first( buffer, id, index);
}

int CasualFieldNext( const char* const buffer, long* const id, long* const index)
{

   if( *id == CASUAL_FIELD_NO_ID)
   {
      return casual::buffer::field::first( buffer, id, index);
   }
   else
   {
      return casual::buffer::field::next( buffer, id, index);
   }

}
