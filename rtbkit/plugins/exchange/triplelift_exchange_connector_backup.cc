#include "triplelift_exchange_connector.h"

#include "rtbkit/plugins/exchange/http_auction_handler.h"

#include "rtbkit/common/currency.h"

using namespace std;
using namespace Datacratic;

namespace RTBKIT {

    BOOST_STATIC_ASSERT(hasFromJson<Datacratic::Id>::value == true);
    BOOST_STATIC_ASSERT(hasFromJson<int>::value == false);
    
    Logging::Category TripleLiftExchangeConnectorLogs::trace( "TripleLift Exchange Connector" );
    Logging::Category TripleLiftExchangeConnectorLogs::error( "[ERROR] TripleLift Exchange Connector error", TripleLiftExchangeConnectorLogs::trace );
    
    
    /*****************************************************************************/
    /* TRIPLELIFT EXCHANGE CONNECTOR                                             */
    /*****************************************************************************/

    TripleLiftExchangeConnector::
    TripleLiftExchangeConnector( ServiceBase & owner, const std::string & name ) :
        HttpExchangeConnector( name, owner ),
        configuration( "triplelift" )
    {
        this->auctionResource = "/auctions";
        this->auctionVerb     = "POST";

        path = "/home/dev/rtbkit/rtbkit_log.txt";
        std::ofstream file( path, std::ios_base::app );
        file << " *** TRIPLELIFT CONNECTOR START IT'S WORK" << std::endl;
        file.close();
        
        parser.reset( new TripleLiftBidRequestParser() );
        initCreativeConfiguration();
    }

    TripleLiftExchangeConnector::
    TripleLiftExchangeConnector( const std::string & name, std::shared_ptr<ServiceProxies> proxies ) :
          HttpExchangeConnector( name, proxies ),
          configuration( "triplelift" )
    {
        this->auctionResource = "/auctions";
        this->auctionVerb     = "POST";

        path = "/home/dev/rtbkit/rtbkit_log.txt";
        std::ofstream file( path, std::ios_base::app );
        file << " *** TRIPLELIFT CONNECTOR START IT'S WORK" << std::endl;
        file.close();
        
        parser.reset( new TripleLiftBidRequestParser() );
        initCreativeConfiguration();
    }

    void
    TripleLiftExchangeConnector::
    initCreativeConfiguration()
    {
        configuration.addField(
            "nurl",
            []( const Json::Value & value, CreativeInfo & data )
            {
                Datacratic::jsonDecode( value, data.nurl );

                return true;
            } ).snippet();
    }


    std::shared_ptr<BidRequest>
    TripleLiftExchangeConnector::
    parseBidRequest( HttpAuctionHandler & connection,
                     const HttpHeader  & header,
                     const std::string & payload )
    {
        std::ofstream file( path, std::ios_base::app );

        static unsigned int bid_counter = 0;

        file << " *** TRIPLELIFT CATCH SOME BIDREQUEST #"
             << bid_counter++
             << " :"
             << std::endl
             << payload
             << std::endl
             << "----------------------------------------------------------------------------------------------"
             << std::endl;
        

        std::shared_ptr<BidRequest> res;
        /*
        if ( header.contentType != "application/json" )
        {
            file << " *** TRIPLELIFT FAILED - non-JSON request : " << header.contentType << std::endl;
            file.close();
            connection.sendErrorResponse( "non-JSON request" );
            return res;
        }
        */
        if ( !Json::parse( payload ).toString().length() )
        {
            file << " *** TRIPLELIFT FAILED - non-JSON request" << std::endl;
            file.close();
            
            connection.sendErrorResponse( "non-JSON request" );
            return res;
        }
        
        ML::Parse_Context context( "Bid Request", payload.c_str(), payload.size() );
        res.reset( parser.get()->parseBidRequest( context, exchangeName(), exchangeName() ) );

        file << " *** TRIPLELIFT FINISHED PARSING BIDREQUEST" << std::endl;
        file.close();
        
        return res;
    }

    Json::Value
    TripleLiftExchangeConnector::
    getAdm( const Json::Value& reqAssets, const Json::Value& confAssets ) const
    {
        Json::Value response, currAsset;

        ConfigAssets assets;

        fillConfigAssets( confAssets, assets );
        for ( auto & r : reqAssets )
        {
            currAsset.clear();

            if ( r.isMember( "title" ) )
            {
                currAsset["title"]["text"] = assets.title.text;
                currAsset["id"] = r["id"].asInt();
                response.append( currAsset );

                continue;
            }
            if ( r.isMember( "data" ) )
            {
                currAsset["data"]["value"] = assets.data[r["data"]["type"].asInt()].value;
                currAsset["id"] = r["id"].asInt();
                response.append( currAsset );

                continue;
            }
            if ( r.isMember( "img" ) )
            {
                int type = r["img"]["type"].asInt();

                if ( assets.image.find( type ) != assets.image.end() )
                {
                    currAsset["img"]["url"] = assets.image[type].link;
                    currAsset["img"]["w"]   = assets.image[type].w;
                    currAsset["img"]["h"]   = assets.image[type].h;
                    currAsset["id"] = r["id"].asInt();

                    response.append( currAsset );
                }
            }
        }

        return response;
    }

    void
    TripleLiftExchangeConnector::
    setSeatBid( Auction const & auction,
                int spotNum,
                TripleLift::BidResponse_2point3 & response ) const
    {
        auto  resp = auction.getCurrentData()->winningResponse( spotNum );

        const AgentConfig * config = std::static_pointer_cast<const AgentConfig>( resp.agentConfig ).get();
        int creativeIndex = resp.agentCreativeIndex;

        Creative creative = config->creatives.at( creativeIndex );
        TripleLift::ResponseSeatBid_2point3 seatbid;

        {
            TripleLift::ResponseBid_2point3 bid;

            bid.id    = Datacratic::Id( resp.creativeId );
            bid.impid = Datacratic::Id( 1 );                           /*   auction.request->imp[spotNum].id   */
            bid.price.val = getAmountIn<CPM>( resp.price.maxPrice );

            std::string nurl = "";

            Datacratic::jsonDecode( config->creatives[creativeIndex].providerConfig["triplelift"]["nurl"], nurl );
            bid.nurl = Datacratic::Url( configuration.expand( nurl, {creative, resp, *auction.request} ) );

            if ( auction.request->toJson()["imp"][spotNum].isMember( "native" ) )
            {
                auto reqAssets  = auction.request->toJson()["imp"][spotNum]["native"]["request"]["assets"];
                auto confAssets = config->providerConfig["triplelift"]["native"]["assets"];

                Json::Value value;
                value["native"]["ver"]    = 1;
                value["native"]["assets"] = getAdm( reqAssets, confAssets );
                
                value["native"]["link"]   = Json::parse( replacer.replace_linear( config->providerConfig["triplelift"]["link"].toString(), *auction.request ) );

                if ( config->providerConfig["triplelift"].isMember( "ext" ) )
                {
                    value["native"]["ext"]  = config->providerConfig["triplelift"]["ext"];
                }
                if ( config->providerConfig["triplelift"].isMember( "imptrackers" ) )
                {
                    value["native"]["imptrackers"]  = config->providerConfig["triplelift"]["imptrackers"];
                }

                bid.adm = value.toString();
            }

            seatbid.bid.push_back( bid );
        };
        response.seatbid.push_back( seatbid );
    }

    HttpResponse
    TripleLiftExchangeConnector::
    getResponse( const HttpAuctionHandler & connection,
                 const HttpHeader & requestHeader,
                 const Auction & auction ) const
    {
        std::ofstream file( path, std::ios_base::app );
        const Auction::Data * current = auction.getCurrentData();

        if ( current->hasError() )
        {
            file << " *** SOME ERROR OCCURRED DURING RESPONSE CREATING : "
                 << std::endl
                 << current->error
                 << " : "
                 << current->details
                 << "----------------------------------------------------------------------------------------------"
                 << std::endl;
            file.close();
            
            return getErrorResponse( connection, current->error + ": " + current->details );
        }
        TripleLift::BidResponse_2point3 response;

        response.id = Datacratic::Id( auction.id.toString() );

        bool validator = false;

        // Create a spot for each of the bid responses
        for ( unsigned int spotNum = 0; spotNum < current->responses.size(); ++spotNum )
        {
            if ( !current->hasValidResponse( spotNum ) )
            {
                continue;
            }
            setSeatBid( auction, spotNum, response );
            validator = true;
        }

        if ( !validator )
        {
            file << " *** NO ADMISSIBLE RESPONSES "
                 << std::endl
                 << "----------------------------------------------------------------------------------------------"
                 << std::endl;
            file.close();
            
            return HttpResponse(204, "none", "");
        }

        static Datacratic::DefaultDescription<TripleLift::BidResponse_2point3> desc;
        std::ostringstream stream;
        StreamJsonPrintingContext context( stream );
        desc.printJsonTyped( &response, context );

        file << " *** TRIPLELIFT RESPONSE CREATED : "
             << std::endl
             << stream.str()
             << std::endl
             << "----------------------------------------------------------------------------------------------"
             << std::endl;
        file.close();
        
        return HttpResponse( 200, "application/json", stream.str() );
    }

    ExchangeConnector::ExchangeCompatibility
    TripleLiftExchangeConnector::
    getCreativeCompatibility( const Creative & creative,
                              bool includeReasons ) const
    {
        ExchangeCompatibility result;
        result.setCompatible();

        configuration.handleCreativeCompatibility( creative, includeReasons );

        return result;
    }

    void
    TripleLiftExchangeConnector::
    fillConfigAssets( const Json::Value & json,
                      ConfigAssets & assets ) const
    {
        for ( const Json::Value & val : json )
        {
            if ( val.isMember( "title" ) )
            {
                assets.title = val["title"]["text"].asString();

                continue;
            }
            if ( val.isMember( "data" ) )
            {
                int type = val["data"]["type"].asInt();
                assets.data[type] = ConfigAssets::DataAsset( val["data"]["value"].asString() );

                continue;
            }
            if ( val.isMember( "img" ) )
            {
                int type =  val["img"]["type"].asInt();
                assets.image[type] = ConfigAssets::ImageAsset( val["img"]["w"].asInt(), val["img"]["h"].asInt(), val["img"]["url"].asString() );

                continue;
            }
        }
    }
}

namespace
{
    using namespace RTBKIT;

    struct AtInit
    {
        AtInit()
        {
            ExchangeConnector::registerFactory<TripleLiftExchangeConnector>();
        }
    } atInit;
}


