#pragma once

#include <set>
#include <unordered_map>
#include <rtbkit/plugins/exchange/http_exchange_connector.h>
#include <rtbkit/plugins/exchange/http_auction_handler.h>
#include <rtbkit/plugins/bid_request/triplelift.h>
#include "rtbkit/plugins/bid_request/triplelift_parsing.h"
#include "rtbkit/plugins/bid_request/triplelift_bid_request.h"

#include "rtbkit/common/creative_configuration.h"

namespace RTBKIT
{  
    struct TripleLiftExchangeConnectorLogs {
        static Logging::Category error;
        static Logging::Category trace;
    };

    struct TripleLiftExchangeConnector : public HttpExchangeConnector
    {
        TripleLiftExchangeConnector( ServiceBase & owner, const std::string & name);
        TripleLiftExchangeConnector( const std::string & name, std::shared_ptr<ServiceProxies> proxies );

        static std::string exchangeNameString() { return "triplelift"; }

        std::string exchangeName() const { return exchangeNameString(); }

        std::unique_ptr<TripleLiftBidRequestParser> parser;

        std::shared_ptr<BidRequest>
        parseBidRequest( HttpAuctionHandler & connection,
                         const HttpHeader   & header,
                         const std::string  & payload );

        double getTimeAvailableMs( HttpAuctionHandler & connection,
                                   const HttpHeader   & header,
                                   const std::string  & payload )
        {
            return 100;
        }

        double getRoundTripTimeMs( HttpAuctionHandler & handler,
                                   const HttpHeader   & header )
        {
            return 5;
        }

        HttpResponse
        getResponse( const HttpAuctionHandler & connection,
                     const HttpHeader & requestHeader,
                     const Auction    & auction ) const;
        void
        setSeatBid( Auction const & auction,
                    int spotNum,
                    TripleLift::BidResponse_2point3 & response ) const;

        Json::Value
        getAdm( const Json::Value & reqAssets,
                const Json::Value & confAssets ) const;

        struct ConfigAssets
        {
            struct ImageAsset
            {
                ImageAsset( const int & _w = -1, const int & _h = -1, const std::string &_link = "" ) :
                    w( _w ),
                    h( _h ),
                    link( _link )
                {}
                int w;
                int h;
                std::string link;
            };
            struct DataAsset
            {
                DataAsset( const std::string _value = "" ) :
                    value( _value )
                {}
                std::string value;
            };
            struct TitleAsset
            {
                TitleAsset( const std::string _text = "" ) :
                    text( _text )
                {}
                std::string text;
            };
            typedef unsigned int key;
            std::unordered_map<key,DataAsset>  data;
            std::unordered_map<key,ImageAsset> image;
            TitleAsset title;
        };

        void
        fillConfigAssets( const Json::Value & json,
                          ConfigAssets & assets ) const;

        struct CreativeInfo
        {
            std::string nurl;
        };

        typedef CreativeConfiguration<CreativeInfo> TripleLiftCreativeConfiguration;
        TripleLiftCreativeConfiguration configuration;

        ExchangeConnector::ExchangeCompatibility
        getCreativeCompatibility( const Creative & creative,
                                  bool includeReasons ) const;

        std::string path;

        ExchangeConnector::ExchangeCompatibility
        getCampaignCompatibility( const AgentConfig & config,
                                  bool includeReasons ) const
        {

            ExchangeCompatibility result;
            result.setCompatible();

            if ( !config.providerConfig.isMember( "triplelift" ) )
            {
                result.setIncompatible("providerConfig.triplelift hasn't found",
                                       includeReasons);
                return result;
            }
            else
            {/*
                if ( !config.providerConfig["triplelift"]["iurl"].asString().size() )
                {
                    result.setIncompatible("providerConfig.triplelift.iurl is null",
                                           includeReasons);
                    return result;
                }*/
            }

            std::ofstream file( path, std::ios_base::app );
            file << " *** TRIPLELIFT EXCHANGE CONNECTOR TAKE NEW AGENT CONFIGURATION : "
                 << std::endl
                 << " *** "
                 << config.toJson().toString()
                 << "      --------------------------------------"
                    "--------------------------------------------"
                    "--------------------------------------------"
                    "--------------------------------------------"
                    "-----------------------------"
                 << std::endl;

            file.close();
            
            return result;
        }

        void initCreativeConfiguration();

        Datacratic::List<ConfigAssets> agentConfigsList;
        
         struct BidRequestMacrosReplacer
        {
            BidRequestMacrosReplacer()
            {
                init();
            }

            std::string replace_regex( const std::string & input,
                                       const BidRequest  & bidrequest ) const
            {
                std::string output = input;
                for ( auto & val : expander_ )
                {
                    std::string regex  = val.first,
                                format = "${1}" + val.second( bidrequest ) + "${3}";
                    boost::regex  xRegEx("(.*)(\\%\\{" + regex + "\\})(.*)");
                    output = boost::regex_replace( output, xRegEx, format, boost::match_default | boost::format_perl);
                }
                return output;
            }

            std::string replace_linear( const std::string & input,
                                        const BidRequest  & bidrequest ) const
            {
                std::string output = input;

                int at = 0;
                while ( 1 )
                {
                    int macros_begin = output.find( "%{", at );
                    if ( macros_begin != std::string::npos )
                    {
                        int macros_end = output.find( "}", macros_begin );
                        if ( macros_end != std::string::npos )
                        {
                            std::string macros( output.begin() + macros_begin + 2, output.begin() + macros_end );
                            if ( expander_.find( macros ) != expander_.end() )
                            {
                                int try_begin  = output.find( "%{", macros_begin + 1, macros_end - macros_begin + 1  );
                                if ( try_begin != std::string::npos && try_begin < macros_end )
                                {
                                   at = try_begin;
                                   LOG( TripleLiftExchangeConnectorLogs::trace )
                                     << " *** try_begin - Find the beginning of the macro %{, but end isn't exist"
                                     << std::endl;
                                   continue;
                                }
                                std::string replace = expander_.find( macros )->second( bidrequest );
                                output.replace( macros_begin, macros_end - macros_begin + 1, replace );
                                at = macros_begin + replace.length();
                            }
                            else
                            {
                                THROW( TripleLiftExchangeConnectorLogs::error )
                                   << " *** Couldn't find macros %{"
                                   << macros
                                   << "} in the list"
                                   << std::endl;
                            }
                        }
                        else
                        {
                            LOG( TripleLiftExchangeConnectorLogs::trace )
                               << " *** macros_end - Find the beginning of the macro %{, but end isn't exist"
                               << std::endl;
                            break;
                        }
                    }
                    else
                    {
                        LOG( TripleLiftExchangeConnectorLogs::trace )
                           << " *** macros_begin - There wasn't found any more macros"
                           << std::endl;
                        break;
                    }
                }
                return output;
            }

        private:
            void init()
            {
                expander_ = {
                    {
                        "exchange",
                        []( const BidRequest & bidrequest ) { return bidrequest.exchange; }
                    },
                    {
                        "bidrequest.id",
                        []( const BidRequest & bidrequest ) { return bidrequest.auctionId.toString(); }
                    },
                    {
                        "bidrequest.user.id",
                        []( const BidRequest & bidrequest ) -> std::string { if ( bidrequest.user ) { return bidrequest.user->id.toString(); } return ""; }
                    },
                    {
                        "bidrequest.publisher.id",
                        []( const BidRequest & bidrequest )
                        {
                            auto const& br = bidrequest;
                            if (br.site && br.site->publisher) {
                                return br.site->publisher->id.toString();
                            } else if (br.app && br.app->publisher) {
                                return br.app->publisher->id.toString();
                            } else {
                                std::cerr << "In bid request: " << br.toJson().toString()
                                          << " no publisher id found" << std::endl;

                                throw std::runtime_error( "No publisher id available" );
                            }
                        }
                    },
                    {
                        "bidrequest.timestamp",
                        []( const BidRequest & bidrequest ) { return std::to_string( bidrequest.timestamp.secondsSinceEpoch() ); }
                    }
                };
            }
            typedef std::function<std::string( const BidRequest & bidrequest )> ExpanderCallable;
            typedef std::map<std::string, ExpanderCallable> ExpanderMap;
            ExpanderMap expander_;
        } replacer;
    };
}
