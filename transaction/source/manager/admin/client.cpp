//!
//! client.cpp
//!
//! Created on: Jun 14, 2015
//!     Author: Lazan
//!



#include "common/arguments.h"
#include "common/environment.h"


#include "transaction/manager/admin/transactionvo.h"



#include "sf/xatmi_call.h"
#include "sf/archive/log.h"


namespace casual
{
   namespace transaction
   {
      namespace manager
      {

         namespace admin
         {
            namespace call
            {

               vo::State state()
               {
                  sf::xatmi::service::binary::Sync service( ".casual.transaction.state");
                  auto reply = service();

                  vo::State serviceReply;

                  reply >> CASUAL_MAKE_NVP( serviceReply);

                  return serviceReply;
               }

            } // call


            namespace global
            {
               bool porcelain = false;

               bool no_color = false;
               bool no_header = false;

            } // global


            common::terminal::format::formatter< vo::Transaction> transaction_formatter()
            {
               struct format_global
               {
                  std::string operator () ( const vo::Transaction& value) const { return value.trid.global; }
               };

               struct format_branch
               {
                  std::string operator () ( const vo::Transaction& value) const { return value.trid.branch; }
               };

               struct format_owner
               {
                  std::string operator () ( const vo::Transaction& value) const { return std::to_string( value.trid.owner.pid); }
               };

               struct format_state
               {
                  std::string operator () ( const vo::Transaction& value)
                  { return common::error::xa::error( value.state);}
               };

               struct format_resources
               {
                  std::string operator () ( const vo::Transaction& value)
                  { std::ostringstream out;  out << common::range::make( value.resources); return out.str();}
               };


               return {
                  { global::porcelain, ! global::no_color, ! global::no_header},
                  common::terminal::format::column( "global", format_global{}, common::terminal::color::yellow),
                  common::terminal::format::column( "branch", format_branch{}, common::terminal::color::grey),
                  common::terminal::format::column( "owner", format_owner{}, common::terminal::color::white, common::terminal::format::Align::right),
                  common::terminal::format::column( "state", format_state{}, common::terminal::color::green, common::terminal::format::Align::left),
                  common::terminal::format::column( "resources", format_resources{}, common::terminal::color::magenta, common::terminal::format::Align::left)
               };
            }

            namespace dispatch
            {
               void list_transactions()
               {
                  auto state = call::state();

                  auto formatter = transaction_formatter();

                  formatter.print( std::cout, state.transactions);
               }

               void list_pending()
               {
                  auto state = call::state();

                  sf::archive::log::Writer debug{ std::cout};

                  debug << CASUAL_MAKE_NVP( state.pending.requests);
                  debug << CASUAL_MAKE_NVP( state.persistent.requests);
                  debug << CASUAL_MAKE_NVP( state.persistent.replies);

               }

            } // dispatch


            int main( int argc, char **argv)
            {
               common::Arguments arguments{ R"(
   usage: 
     
   )"};

               arguments.add(
                  common::argument::directive( {"--no-header"}, "do not print headers", global::no_header),
                  common::argument::directive( {"--no-color"}, "do not use color", global::no_color),
                  common::argument::directive( {"--porcelain"}, "Easy to parse format", global::porcelain),
                  common::argument::directive( { "-lt", "--list-transactions" }, "list current transactions", &dispatch::list_transactions),
                  common::argument::directive( { "-lp", "--list-pending" }, "list pending tasks", &dispatch::list_pending)
                  );


               try
               {
                  arguments.parse( argc, argv);

               }
               catch( const std::exception& exception)
               {
                  std::cerr << "error: " << exception.what() << std::endl;
                  return 10;
               }
               return 0;
            }

         } // admin
      } // manager
   } // transaction
} // casual

int main( int argc, char **argv)
{
   return casual::transaction::manager::admin::main( argc, argv);
}

