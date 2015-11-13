/** augmentor_redis_ex.h                                 -*- C++ -*-
    Borysiuk Sviatoslav, 24 sep 2015
    Copyright (c) 2013 Datacratic.  All rights reserved.

    Interface of my Augmentor for TripleLift exchange
*/

#pragma once

#include <rtbkit/core/agent_configuration/agent_configuration_listener.h>
#include <rtbkit/plugins/augmentor/augmentor_base.h>
#include <soa/service/zmq_named_pub_sub.h>
#include <soa/service/service_base.h>
#include <soa/service/redis.h>

#include <string>
#include <memory>

namespace RTBKIT
{
   struct RedisDatabaseInterface
   {
      RedisDatabaseInterface() :
         host( "127.0.0.1" ),
         port( 6379 ),
         root( "triplelift_augmentor" )
      {
         connection.connect( Redis::Address::tcp( host, port ) );
      }
      RedisDatabaseInterface( const std::string & _host = "127.0.0.1", const int & _port = 6379 ) :
         host( _host ),
         port( _port ),
         root( "triplelift_augmentor" )
      {
         connection.connect( Redis::Address::tcp( host, port ) );
      }
      void setCounterForKey( const std::string & key, const std::string & domain )
      {
         connection.execMulti( { Redis::Command( "ZADD", root + ":" + key, 1, domain ), Redis::Command( "EXPIRE", root + ":" + key, 24 * 60 * 60 ) } );
         /*
         connection.exec( Redis::Command( "ZADD", root + ":" + key, 1, domain ) );
         connection.exec( Redis::Command( "EXPIRE", root + ":" + key, 24 * 60 * 60 ) );
         */
      }
      Json::Value getCounterForKey( const std::string & key, const std::string & domain )
      {
         return connection.exec( Redis::Command( "ZSCORE", root + ":" + key, domain ) ).reply().asJson();
      }
      bool incCounterForKey( const std::string & key, const std::string & domain )
      {
         return static_cast<bool>( std::atoi( connection.exec( Redis::Command( "ZINCRBY", root + ":" + key, 1, domain ) ).reply().asJson().asString().c_str() ) );
      }
      
      std::string host;
      int         port;
      std::string root;
   private:
      Redis::AsyncConnection connection;
   };

   struct TripleLiftAugmentor :
      public RTBKIT::SyncAugmentor
   {

      TripleLiftAugmentor( std::shared_ptr<Datacratic::ServiceProxies> services,
                           const std::string& serviceName,
                           const std::string& augmentorName = "triplelift-augmentor-ex" );

      void init();

   private:

      virtual RTBKIT::AugmentationList onRequest( const RTBKIT::AugmentationRequest & request );
      
      size_t getCap( const std::string & augmentor,
                     const std::string & agent,
                     const RTBKIT::AgentConfigEntry& config) const;  // get augmentation maxPerDay value
                    
      RTBKIT::RedisDatabaseInterface         database;               // asynchronous connection with Redis database
      RTBKIT::AgentConfigurationListener     agentConfig;
      Datacratic::ZmqNamedMultipleSubscriber palEvents;
   };
} // namespace RTBKIT