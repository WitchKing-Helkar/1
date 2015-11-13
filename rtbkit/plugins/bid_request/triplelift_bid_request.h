#pragma once

#include "rtbkit/common/bid_request.h"
#include "rtbkit/plugins/bid_request/triplelift.h"
#include "rtbkit/plugins/bid_request/triplelift_parsing.h"
#include "triplelift.h"
#include "jml/utils/parse_context.h"
#include "iostream"
#include <rtbkit/plugins/bid_request/openrtb_bid_request.h>
#include "soa/service/logs.h"

namespace RTBKIT 
{
    struct TripleLiftBidRequestLogs {
        static Logging::Category error;
        static Logging::Category trace;
    };

    struct TripleLiftBidRequestParser {
         
        std::string path;
        TripleLiftBidRequestParser()
        {
           path = "/home/dev/rtbkit/rtbkit_log.txt";
        }
        
        BidRequest *
        parseBidRequest( const std::string & jsonValue,
                         const std::string & provider,
                         const std::string & exchange = "" );

        BidRequest *
        parseBidRequest( ML::Parse_Context & context,
                         const std::string & provider,
                         const std::string & exchange = "" );

    protected:
        BidRequest *
        fromTripleLift( TripleLift::BidRequest && req,
                        const std::string & provider,
                        const std::string & exchange );

        virtual void onNativeAssetImage( TripleLift::NativeAssetImage & image );
        virtual void onNativeAssetTitle( TripleLift::NativeAssetTitle & title );
        virtual void onNativeAssetData( TripleLift::NativeAssetData & data );
        virtual void onNativeAsset( TripleLift:: NativeAsset & asset );
        virtual void onNativeRequest( TripleLift:: NativeRequest & native );
        virtual void onNative( TripleLift:: Native & native );
        virtual void onNativeFromTripleLift( TripleLift::Banner & banner );
        virtual void onBanner( TripleLift:: Banner & banner );
        virtual void onImpression( TripleLift::Impression & imp );
        virtual void onPublisher( TripleLift:: Publisher & publisher );
        virtual void onSite( TripleLift::Site & site );
        virtual void onApplication( TripleLift::App & app );
        virtual void onGeo( TripleLift::Geo & geo );
        virtual void onDevice( TripleLift::Device & device );
        virtual void onUser( TripleLift::User & user );
        virtual void onBidRequest( TripleLift::BidRequest & req );

    private:
        std::unique_ptr<BidRequest> request;
    };

}
