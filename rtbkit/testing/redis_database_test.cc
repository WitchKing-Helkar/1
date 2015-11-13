#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <mutex>
#include <atomic>
#include <set>
#include <thread>
#include <iostream>

#include <soa/service/redis.h>
#include <soa/service/testing/redis_temporary_server.h>

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
   int setKeyTTL( const std::string & key )
   {
      return connection.exec( Redis::Command( "EXPIRE", key, 10 ) ).reply().asInt();
   }
   int setCounterForKey( const std::string & key, const std::string & domain )
   {
      return connection.exec( Redis::Command( "ZADD", root + ":" + key, 0, domain ) ).reply().asInt();
   }
   int getCounterForKey( const std::string & key, const std::string & domain )
   {
      return 1;
   }
   int incCounterForKey( const std::string & key, const std::string & domain )
   {
      return connection.exec( Redis::Command( "ZINCRBY", root + ":" + key, 1, domain ) ).reply().asInt();
   }
private:
   Redis::AsyncConnection connection;

   std::string host;
   int         port;
   std::string root;
};

namespace RTBKIT
{
   BOOST_AUTO_TEST_CASE( redisDatabaseTest )
   {
      Redis::AsyncConnection connection( Redis::Address::tcp( "127.0.0.1" , 6379 ) );
      
      //connection.exec( Redis::Command( "ZADD", "triplelift_augmentor:triplelift_agent", 1, "www.rambler.ru" ) );
      /*
      connection.exec( Redis::Command( "ZADD", key, 2, "b" ) );
      connection.exec( Redis::Command( "ZADD", key, 3, "c" ) );
      */
    
      /*connection.exec( Redis::Command( "ZINCRBY", "triplelift_augmentor:triplelift_agent", 1, "www.rambler.ru" ) );
      std::cout << connection.exec( Redis::Command( "ZRANGE", "triplelift_augmentor:triplelift_agent", 0, -1, "WITHSCORES" ) );
      std::cout << connection.exec( Redis::Command( "ZRANGE", "triplelift_augmentor", 0, -1, "WITHSCORES" ) );*/
      
      
      //connection.exec( Redis::Command( "ZADD", "triplelift_augmentor:triplelift_agent:inner", 1, "www.some.com" ) );
      std::cout << connection.exec( Redis::Command( "ZSCORE",  "triplelift_augmentor:triplelift_agent:inner", "www.some.com" ) ).reply().asJson();
      //std::cout << connection.exec( Redis::Command( "ZINCRBY", "triplelift_augmentor:triplelift_agent:inner", 1, "www.some.com" ) ).reply().asJson();
      /*
      std::cout << connection.exec( Redis::Command( "ZRANGE", key, 0, -1, "WITHSCORES" ) ) << std::endl;
      std::cout << connection.exec( Redis::Command( "ZSCORE", key, "b" ) ) << std::endl;
      std::cout << connection.exec( Redis::Command( "ZINCRBY", key, 1, "b" ) ) << std::endl;
      std::cout << connection.exec( Redis::Command( "ZRANGE", key, 0, -1, "WITHSCORES" ) ) << std::endl;
      */
      //std::cout << connection.exec( Redis::Command( "EXPIRE", key, 15 ) ) << std::endl;
   }
}
