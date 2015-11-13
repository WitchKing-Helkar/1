/** augmentor_redis_ex.cc                                -*- C++ -*-
    Borysiuk Sviatoslav, 24 sep 2015
    Copyright (c) 2013 Datacratic.  All rights reserved.
*/

#include "augmentor_redis_ex.h"

#include "rtbkit/core/agent_configuration/agent_configuration_listener.h"
#include "rtbkit/core/agent_configuration/agent_config.h"
#include "rtbkit/plugins/augmentor/augmentor_base.h"
#include "rtbkit/common/bid_request.h"
#include "soa/service/zmq_named_pub_sub.h"
#include "jml/utils/exc_assert.h"

#include <unordered_map>
#include <mutex>

using namespace std;

namespace RTBKIT 
{
   TripleLiftAugmentor::
   TripleLiftAugmentor( std::shared_ptr<Datacratic::ServiceProxies> services,
                        const string & serviceName,
                        const string & augmentorName ) :
      SyncAugmentor( augmentorName, serviceName, services ),
      database( static_cast<std::string>( "127.0.0.1" ), 6379 ),   // create connection to Redis database
      agentConfig( getZmqContext() ),
      palEvents( getZmqContext() )
   {
      recordHit( "up" );
   }

   void
   TripleLiftAugmentor::
   init()
   {   
      SyncAugmentor::init( 2 );

      agentConfig.init( getServices()->config );
      addSource( "TripleLiftAugmentor::agentConfig", agentConfig );

      palEvents.init( getServices()->config );

      palEvents.messageHandler = [&] ( const vector<zmq::message_t> & msg )
      {
         RTBKIT::AccountKey account( msg[19].toString() );
         RTBKIT::UserIds uids = RTBKIT::UserIds::createFromString( msg[15].toString() );

         recordHit( "wins" );
      };

      palEvents.connectAllServiceProviders( "rtbPostAuctionService", "logger", { "MATCHEDWIN" } );
      addSource( "TripleLiftAugmentor::palEvents", palEvents );
   }

   RTBKIT::AugmentationList
   TripleLiftAugmentor::
   onRequest( const RTBKIT::AugmentationRequest & request )
   {
//    std::ofstream file( "/home/dev/rtbkit/rtbkit_log.txt", std::ios_base::app );
      recordHit( "requests" );

      RTBKIT::AugmentationList result;

      for ( const string & agent : request.agents ) 
      {

         RTBKIT::AgentConfigEntry config = agentConfig.getAgentEntry( agent );

         if ( !config.valid() ) // проверяем допустимость конфигураций
         {
            recordHit( "invalid-config" );
            continue;
         }
         const RTBKIT::AccountKey & account = config.config->account;
         
         std::string domain = request.bidRequest->site->domain.rawString();         
         Json::Value value  = database.getCounterForKey( account.toString(), domain );
         
         if ( !value.isNull() )
         {
            unsigned int counter = getCap( request.augmentor, account.toString(), config );
//          file << " *** counter for this site already exist : " 
//               << database.root 
//               << ":" 
//               << account.toString() 
//               << ":" 
//               << domain 
//               << " = " 
//               << value.asString()
//               << std::endl;
//          file << " *** currently agent max cap = "
//               << std::to_string( counter )
//               << std::endl;
            if ( std::atoi( value.asString().c_str() ) < counter )
            {
//             file << " *** passed augmentor : "
//                  << database.incCounterForKey( account.toString(), domain )
//                  << std::endl;
               result[account].tags.insert( "pass-triplelift-augmentor-ex" );
               recordHit( "accounts." + account[0] + ".passed" );
            }
            else
            {
//             file << " *** dropped by augmentor"
//                  << std::endl;
               result[account].tags.insert( "drop-triplelift-augmentor-ex" );
               recordHit( "accounts." + account[0] + ".capped" );
            }
         }
         else
         {
//          file << " *** counter have not exist - created successfully : " 
//               << database.root
//               << ":" 
//               << account.toString() 
//               << ":" 
//               << domain 
//               << std::endl;
            database.setCounterForKey( account.toString(), domain );
            result[account].tags.insert( "pass-triplelift-augmentor-ex" );
            recordHit( "accounts." + account[0] + ".passed" );
//          file << " *** passed by augmentor"
//               << std::endl;
         }
      }
//    file.close();
      return result;
   }

   size_t
   TripleLiftAugmentor::
   getCap( const string & augmentor,
           const string & agent,
           const RTBKIT::AgentConfigEntry & config) const
   {    
//    std::ofstream file( "/home/dev/rtbkit/rtbkit_log.txt", std::ios_base::app );
      auto & augms = config.config->augmentations;
      auto find = std::find_if( 
         augms.begin(), 
         augms.end(), 
         [&augmentor/*, &file*/]( const RTBKIT::AugmentationConfig & augm )
         { 
//          file << "     *** augm.name : " 
//               << augm.name 
//               << std::endl
//               << "     *** augmentor : " 
//               << augmentor 
//               << std::endl;
            return augm.name == augmentor; 
         } 
      );
      if ( find != augms.end() )
      {
//       file.close();
         return find->config["maxPerDay"].asInt();
      }
      else
      {
//       file.close();
         return 0;
      }
   }
} // namespace RTBKIT
