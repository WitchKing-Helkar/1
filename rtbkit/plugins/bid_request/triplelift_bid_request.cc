#include "triplelift_bid_request.h"
#include "triplelift.h"
#include "triplelift_parsing.h"
#include "jml/utils/json_parsing.h"

#include "rtbkit/openrtb/openrtb.h"

namespace RTBKIT
{
    Logging::Category TripleLiftBidRequestLogs::trace( "TripleLift Bid Request Parser" );
    Logging::Category TripleLiftBidRequestLogs::error( "[ERROR] TripleLift Bid Request Parser error", TripleLiftBidRequestLogs::trace );

    BidRequest *
    TripleLiftBidRequestParser::
    fromTripleLift( TripleLift::BidRequest && req,
                    const std::string & provider,
                    const std::string & exchange )
    {
        request.reset( new RTBKIT::BidRequest() );
        
        static Datacratic::DefaultDescription<TripleLift::BidRequest> desc;
        std::ostringstream stream;
        StreamJsonPrintingContext context( stream );
        desc.printJsonTyped( &req, context );
             
        onBidRequest( req );
        
        request->auctionId = Datacratic::Id( req.id.toString() );
        request->auctionType = Datacratic::AuctionType::SECOND_PRICE;
        request->timeAvailableMs = 100;
        request->timestamp = Datacratic::Date::now();
        request->isTest = false;
        request->provider = provider;
        request->exchange = (exchange.empty() ? provider : exchange);

        request->ipAddress = *req.device->ip.get();
        request->userAgent = *req.device->ua.get();

        return request.release();
    }

    namespace
    {
        static DefaultDescription<TripleLift::BidRequest> desc;
    }

    BidRequest *
    TripleLiftBidRequestParser::
    parseBidRequest( const std::string & jsonValue,
                     const std::string & provider,
                     const std::string & exchange )
    {
        StructuredJsonParsingContext jsonContext( jsonValue );

        TripleLift::BidRequest req;
        desc.parseJson( &req, jsonContext );

        return fromTripleLift( std::move( req ), provider, exchange );
    }

    BidRequest *
    TripleLiftBidRequestParser::
    parseBidRequest( ML::Parse_Context & context,
                     const std::string & provider,
                     const std::string & exchange )
    {
        StreamingJsonParsingContext jsonContext( context );
        TripleLift::BidRequest req;
        desc.parseJson( &req, jsonContext );

        return fromTripleLift( std::move( req ), provider, exchange );
    }

    void
    TripleLiftBidRequestParser::
    onNativeAssetImage( TripleLift::NativeAssetImage & image )
    {
        BidRequest * ptr = request.get();
        int currImp   = ptr->imp.size() - 1,
            currAsset = ptr->imp[currImp].native->request->assets.size() - 1;

        if ( image.h )
        {
            if ( image.h->value() <= 0 )
            {
                THROW( TripleLiftBidRequestLogs::error )
                        << "br.imp["
                        << currImp
                        << "].native.request.assets["
                        << currAsset
                        << "].img.h"
                        << std::endl
                        << "    *** has invalid value, height isn't positive ***"
                        << std::endl;
            }
            ptr->imp[currImp].native->request->assets[currAsset].img->h.val = image.h->value();
        }
        if ( image.w )
        {
            if ( image.w->value() <= 0 )
            {
                THROW( TripleLiftBidRequestLogs::error )
                        << "br.imp["
                        << currImp
                        << "].native.request.assets["
                        << currAsset
                        << "].img.w"
                        << std::endl
                        << "    *** has invalid value, width isn't positive ***"
                        << std::endl;
            }
            ptr->imp[currImp].native->request->assets[currAsset].img->w.val = image.w->value();
        }
        if ( image.hmin.value() == 0 )
        {
            LOG( TripleLiftBidRequestLogs::trace )
                    << "br.imp["
                    << currImp
                    << "].native.request.assets["
                    << currAsset
                    << "].img.hmin"
                    << std::endl
                    << "    *** has zero height value ***"
                    << std::endl;
            ptr->imp[currImp].native->request->assets[currAsset].img->hmin.val = 1;
        }
        else
        {
            ptr->imp[currImp].native->request->assets[currAsset].img->hmin.val = image.hmin.value();
        }
        ptr->imp[currImp].native->request->assets[currAsset].img->wmin.val = image.wmin.value();
        int type = image.type.value();
        if ( type == -1 )
        {
            LOG( TripleLiftBidRequestLogs::trace )
                    << "br.imp["
                    << currImp
                    << "].native.request.assets["
                    << currAsset
                    << "].img.type"
                    << std::endl
                    << "    *** has UNSPECIFIED type value ***"
                    << std::endl;
        }
        else
        if ( type != 2 && type != 3 )
        {
            THROW( TripleLiftBidRequestLogs::error )
                    << "br.imp["
                    << currImp
                    << "].native.request.assets["
                    << currAsset
                    << "].img.type"
                    << std::endl
                    << "    *** has invalid type value ***"
                    << std::endl;
        }
        ptr->imp[currImp].native->request->assets[currAsset].img->type.val = type;
    }

    void
    TripleLiftBidRequestParser::
    onNativeAssetTitle( TripleLift::NativeAssetTitle & title )
    {
        BidRequest * ptr = request.get();
        int currImp   = ptr->imp.size() - 1,
            currAsset = ptr->imp[currImp].native->request->assets.size() - 1;
        if ( title.len.value() <= 0 )
        {
            THROW( TripleLiftBidRequestLogs::error )
                    << "br.imp["
                    << currImp
                    << "].native.request.assets["
                    << currAsset
                    << "].title.len"
                    << std::endl
                    << "    *** has invalid value, len isn't positive ***"
                    << std::endl;
        }
        ptr->imp[currImp].native->request->assets[currAsset].title->len.val = title.len.value();
        
    }

    void
    TripleLiftBidRequestParser::
    onNativeAssetData( TripleLift::NativeAssetData & data )
    {
        BidRequest * ptr = request.get();
        int currImp   = ptr->imp.size() - 1,
            currAsset = ptr->imp[currImp].native->request->assets.size() - 1;

        int type = data.type.value();
        if ( type == -1 )
        {
            LOG( TripleLiftBidRequestLogs::trace )
                    << "br.imp["
                    << currImp
                    << "].native.request.assets["
                    << currAsset
                    << "].data.type"
                    << std::endl
                    << "    *** has UNSPECIFIED type value ***"
                    << std::endl;
        }
        else
        if ( type != 1 && type != 2 )
        {
            THROW( TripleLiftBidRequestLogs::error )
                    << "br.imp["
                    << currImp
                    << "].native.request.assets["
                    << currAsset
                    << "].data.type"
                    << std::endl
                    << "    *** has invalid type value ***"
                    << std::endl;
        }
        ptr->imp[currImp].native->request->assets[currAsset].data->type.val = data.type.value();
        if ( data.len )
        {
            if ( data.len->value() <= 0 )
            {
                LOG( TripleLiftBidRequestLogs::trace )
                        << "br.imp["
                        << currImp
                        << "].native.request.assets["
                        << currAsset
                        << "].data.len"
                        << std::endl
                        << "    *** has invalid value, len isn't positive ***"
                        << std::endl;
            }
            else
            {
                ptr->imp[currImp].native->request->assets[currAsset].data->len.val = data.len->value();
            }
        }
    }

    void
    TripleLiftBidRequestParser::
    onNativeAsset( TripleLift::NativeAsset & asset )
    {
        BidRequest * ptr = request.get();
        int currImp = ptr->imp.size() - 1;

        ptr->imp[currImp].native->request->assets.push_back( OpenRTB::NativeAsset() );
        int currAsset = ptr->imp[currImp].native->request->assets.size() - 1;

        ptr->imp[currImp].native->request->assets[currAsset].id.val       = asset.id.value();
        ptr->imp[currImp].native->request->assets[currAsset].required.val = asset.required.val;

        if ( asset.img )
        {
            ptr->imp[currImp].native->request->assets[currAsset].img.reset( new OpenRTB::NativeAssetImage() );
            onNativeAssetImage( *asset.img.get() );
        }
        else
        if ( asset.title )
        {
            ptr->imp[currImp].native->request->assets[currAsset].title.reset( new OpenRTB::NativeAssetTitle() );
            onNativeAssetTitle( *asset.title.get() );
        }
        else
        if ( asset.data )
        {
            ptr->imp[currImp].native->request->assets[currAsset].data.reset( new OpenRTB::NativeAssetData() );
            onNativeAssetData( *asset.data.get() );
        }
        else
        if ( asset.required.val )
        {
            THROW( TripleLiftBidRequestLogs::error )
                    << "br.imp["
                    << currImp
                    << "].native.request.assets["
                    << currAsset
                    << "]"
                    << std::endl
                    << "    *** has not contain any of data, image or title object for requeired asset ***"
                    << std::endl;
        }
        else
        {
            LOG( TripleLiftBidRequestLogs::trace )
                    << "br.imp["
                    << currImp
                    << "].native.request.assets["
                    << currAsset
                    << "]"
                    << std::endl
                    << "    *** has not contain any of data, image or title object ***"
                    << std::endl;
            ptr->imp[currImp].native->request->assets.pop_back();
        }
    }

    void
    TripleLiftBidRequestParser::
    onNativeRequest( TripleLift::NativeRequest & native )
    {
        BidRequest * ptr = request.get();
        int currImp = ptr->imp.size() - 1;

        ptr->imp[currImp].native->request->ver.val = native.ver.value();
        ptr->imp[currImp].native->request->lcmtcnt.val = native.plcmtcnt.value();
        int adunit =  native.adunit.value();
        if ( adunit == -1 )
        {
            LOG( TripleLiftBidRequestLogs::trace )
                    << "br.imp["
                    << currImp
                    << "].native.request"
                    << std::endl
                    << "    *** has UNSPECIFIED adunit value ***"
                    << std::endl;
        }
        else
        if ( adunit < 1 || adunit > 5 )
        {
            THROW( TripleLiftBidRequestLogs::error )
                    << "br.imp["
                    << currImp
                    << "].native.request"
                    << std::endl
                    << "    *** has invalid adunit value ***"
                    << std::endl;
        }
        ptr->imp[currImp].native->request->adunit.val  = adunit;
        int layout = native.layout.value();

        if ( layout == -1 )
        {
            LOG( TripleLiftBidRequestLogs::trace )
                    << "br.imp["
                    << currImp
                    << "].native.request"
                    << std::endl
                    << "    *** has UNSPECIFIED layout value ***"
                    << std::endl;
        }
        else
        if ( layout < 1 || layout > 7 )
        {
            THROW( TripleLiftBidRequestLogs::error )
                    << "br.imp["
                    << currImp
                    << "].native.request"
                    << std::endl
                    << "    *** has invalid layout value ***"
                    << std::endl;
        }
        ptr->imp[currImp].native->request->layout.val  = layout;

        for ( auto & val : native.assets )
        {
            onNativeAsset( val );
        }
    }

    void
    TripleLiftBidRequestParser::
    onNative( TripleLift::Native & native )
    {
        std::ofstream file ( path, std::ios_base::app );
        BidRequest * ptr = request.get();
        int currImp = ptr->imp.size() - 1;

        ptr->imp[currImp].native->ver = std::to_string( native.ver.value() );

        ptr->imp[currImp].native->request.reset( new OpenRTB::NativeRequest() );

        DefaultDescription<TripleLift::NativeRequest> nativeRequestDesc;
        StringJsonParsingContext jsonContext( native.request );
        
        if ( jsonContext.str.length() == 0 )
        {
            THROW( TripleLiftBidRequestLogs::error )
                    << "br.imp["
                    << currImp
                    << "].native"
                    << std::endl
                    << "    *** has empty native request field ***"
                    << std::endl;
        }

        TripleLift::NativeRequest nativeRequest;
        nativeRequestDesc.parseJson( &nativeRequest, jsonContext );

        onNativeRequest( nativeRequest );
    }

    void
    TripleLiftBidRequestParser::
    onNativeFromTripleLift( TripleLift::Banner & banner )
    {
        BidRequest * ptr = request.get();
        int currImp = ptr->imp.size() - 1;

        ptr->imp[currImp].native->request.reset( new OpenRTB::NativeRequest() );

        OpenRTB::NativeRequest * p = ptr->imp[currImp].native->request.get();
        p->ver = 1;
        {
            OpenRTB::NativeAsset asset;
            asset.required = 1;
            asset.title.reset( new OpenRTB::NativeAssetTitle() );
            if ( banner.ext.triplelift.headinglen )
            {
                if ( banner.ext.triplelift.headinglen.get()->val <= 0 )
                {
                    THROW( TripleLiftBidRequestLogs::error )
                            << "br.imp["
                            << currImp
                            << "].banner.ext.triplelift"
                            << std::endl
                            << "    *** has invalid value, headinglen isn't positive ***"
                            << std::endl;
                }
                asset.title->len.val = banner.ext.triplelift.headinglen.get()->val;
            }
            asset.id.val = 1;
            p->assets.push_back( asset );
        }
        {
            OpenRTB::NativeAsset asset;
            asset.required = 1;
            asset.id.val = 4;
            asset.data.reset( new OpenRTB::NativeAssetData() );
            if ( banner.ext.triplelift.captionlen )
            {
                if ( banner.ext.triplelift.captionlen.get()->val <= 0 )
                {
                    THROW( TripleLiftBidRequestLogs::error )
                            << "br.imp["
                            << currImp
                            << "].banner.ext.triplelift"
                            << std::endl
                            << "    *** has invalid value, captionlen isn't positive ***"
                            << std::endl;
                }
                asset.data->len.val = banner.ext.triplelift.captionlen.get()->val;
            }
            asset.data->type.val = 2;
            p->assets.push_back( asset );

            asset.data->len.val = -1;
            asset.id.val = 5;
            asset.data->type.val = 1;
            p->assets.push_back( asset );
        }
        {
            OpenRTB::NativeAsset asset;
            asset.required = 1;
            asset.id.val = 2;
            asset.img.reset( new OpenRTB::NativeAssetImage() );
            if ( banner.ext.triplelift.imgw.value() < 1 )
            {
                THROW( TripleLiftBidRequestLogs::error )
                        << "br.imp["
                        << currImp
                        << "].banner.ext.triplelift"
                        << std::endl
                        << "    *** has invalid value, image width isn't positive ***"
                        << std::endl;
            }
            asset.img->w = banner.ext.triplelift.imgw.value();
            if ( banner.ext.triplelift.imgh.value() < 1 )
            {
                THROW( TripleLiftBidRequestLogs::error )
                        << "br.imp["
                        << currImp
                        << "].banner.ext.triplelift"
                        << std::endl
                        << "    *** has invalid value, image height isn't positive ***"
                        << std::endl;
            }
            asset.img->h = banner.ext.triplelift.imgh.value();
            asset.img->type.val = 3;
            p->assets.push_back( asset );
        }
        std::sort(
            p->assets.begin(),
            p->assets.end(),
            []( OpenRTB::NativeAsset ob1, OpenRTB::NativeAsset ob2 )
            {
                return ob1.id.val < ob2.id.val;
            }
        );
    }

    void
    TripleLiftBidRequestParser::
    onBanner( TripleLift::Banner & banner )
    {
        BidRequest * ptr = request.get();
        int currImp = ptr->imp.size() - 1;

        ptr->imp[currImp].banner->w.push_back( 0 );
        ptr->imp[currImp].banner->h.push_back( 0 );

        for ( auto & val : banner.battr )
        {
            OpenRTB::CreativeAttribute attr;
            attr.val = val.value();

            ptr->imp[currImp].banner->battr.push_back( attr );
        }
        static Datacratic::DefaultDescription<TripleLift::TripleLiftNative> parser;
        std::ostringstream jsonString;
        StreamJsonPrintingContext ext( jsonString );
        parser.printJsonTyped( &banner.ext.triplelift, ext );

        if ( jsonString.str().length() == 0 )
        {
            THROW( TripleLiftBidRequestLogs::error )
                    << "br.imp["
                    << currImp
                    << "].banner.ext.triplelift"
                    << std::endl
                    << "    *** has empty triplelift field ***"
                    << std::endl;
        }
        Json::Value value;
        value["triplelift"] = Json::parse( jsonString.str() );
        ptr->imp[currImp].banner->ext = value;

        ptr->imp[0].native.reset( new OpenRTB::Native() );
        onNativeFromTripleLift( banner );

        ptr->imp[currImp].banner.release();
    }

    void
    TripleLiftBidRequestParser::
    onImpression( TripleLift::Impression & imp )
    {
        BidRequest * ptr = request.get();

        ptr->imp.push_back( AdSpot() );
        int currImp = ptr->imp.size() - 1;

        ptr->imp[currImp].id    = imp.id;
        ptr->imp[currImp].tagid = Datacratic::UnicodeString( imp.tagid );

        if ( imp.banner )
        {
            ptr->imp[currImp].banner.reset( new OpenRTB::Banner() );
            onBanner( *imp.banner.get() );
        }
        if ( imp.native )
        {
            ptr->imp[currImp].native.reset( new OpenRTB::Native() );
            onNative( *imp.native.get() );
        }
    }

    void
    TripleLiftBidRequestParser::
    onPublisher( TripleLift::Publisher & publisher )
    {
        BidRequest * ptr = request.get();
        if ( publisher.id )
        {
            ptr->site->publisher->id = Datacratic::Id( publisher.id->toString() );
        }
        if ( publisher.cat )
        {
            // originaly *(publisher.cat.get())
            for ( auto & val : *publisher.cat.get() )
            {
                ptr->site->publisher->cat.push_back( OpenRTB::ContentCategory( val ) );
            }
        }
    }

    void
    TripleLiftBidRequestParser::
    onSite( TripleLift::Site & site )
    {
        BidRequest * ptr = request.get();

        if ( site.id )
        {
            ptr->site->id = Datacratic::Id( site.id->toString() );
        }

        if ( site.cat )
        {
            for ( auto & val : *( site.cat.get() ) )
            {
                ptr->site->cat.push_back( OpenRTB::ContentCategory( val ) );
            }
        }

        if ( site.domain )
        {
            ptr->site->domain = Datacratic::UnicodeString( site.domain->utf8String() );
        }

        if ( site.page )
        {
            ptr->site->page = Datacratic::Url( site.page->toString() );
        }

        if ( site.publisher )
        {
            ptr->site->publisher.reset( new OpenRTB::Publisher() );
            onPublisher( *site.publisher.get() );
        }
    }

    void
    TripleLiftBidRequestParser::
    onApplication( TripleLift::App & app )
    {
        BidRequest * ptr = request.get();
        if ( app.id )
        {
            ptr->app->id = Datacratic::Id( app.id->toString() );
        }
        if ( app.name )
        {
            ptr->app->name = Datacratic::UnicodeString( *app.name.get() );
        }
        if ( app.publisher )
        {
            ptr->app->publisher.reset( new OpenRTB::Publisher() );
            onPublisher( *app.publisher.get() );
        }
    }

    void
    TripleLiftBidRequestParser::
    onGeo( TripleLift::Geo & geo )
    {
        BidRequest * ptr = request.get();
        if ( geo.lat )
        {
            ptr->device->geo->lat.val = geo.lat->val;
        }
        if ( geo.lon )
        {
            ptr->device->geo->lon.val = geo.lon->val;
        }
        if ( geo.country )
        {
            ptr->location.countryCode = *geo.country.get();
            ptr->device->geo->country = *geo.country.get();
        }
        if ( geo.region )
        {
            ptr->location.regionCode = *geo.region.get();
            ptr->device->geo->region = *geo.region.get();
        }
        if ( geo.city )
        {
            ptr->location.cityName = *geo.city.get();
            ptr->device->geo->city = *geo.city.get();
        }
    }

    void
    TripleLiftBidRequestParser::
    onDevice( TripleLift::Device & device )
    {
        BidRequest * ptr = request.get();
        if ( device.ua )
        {
            ptr->device->ua = Datacratic::UnicodeString( device.ua->utf8String() );
        }
        if ( device.ip )
        {
            ptr->ipAddress  = *device.ip;
            ptr->device->ip = *device.ip;
        }
        if ( device.geo )
        {
            ptr->device->geo.reset( new OpenRTB::Geo() );
            onGeo( *device.geo.get() );
        }
        if ( device.make )
        {
            ptr->device->make = Datacratic::UnicodeString( device.make->utf8String() );
        }
        if ( device.model )
        {
            ptr->device->model = Datacratic::UnicodeString( device.model->utf8String() );
        }
        if ( device.os )
        {
            ptr->device->os = Datacratic::UnicodeString( device.os->utf8String() );
        }
        if ( device.devicetype )
        {
            ptr->device->devicetype.val = device.devicetype->value();
        }
    }

    void
    TripleLiftBidRequestParser::
    onUser( TripleLift::User & user )
    {
        BidRequest * ptr = request.get();
        if ( user.id )
        {
            ptr->user->id = Datacratic::Id( user.id->toString() );
        }
        if ( user.buyeruid )
        {
            ptr->user->buyeruid = Datacratic::Id( user.buyeruid->toString() );
        }
    }

    void
    TripleLiftBidRequestParser::
    onBidRequest( TripleLift::BidRequest & req )
    {
        BidRequest * ptr = request.get();
        ptr->auctionId = req.id;

        for ( auto & imp : req.imp )
        {
            onImpression( imp );
        }

        if ( req.site )
        {
            ptr->site.reset( new OpenRTB::Site() );
            onSite( *req.site.get() );
        }

        if ( req.app )
        {
            ptr->app.reset( new OpenRTB::App() );
            onApplication( *req.app.get() );
        }

        if ( req.device )
        {
            ptr->device.reset( new OpenRTB::Device() );
            onDevice( *req.device.get() );
        }

        ptr->user.reset( new OpenRTB::User() );
        onUser( req.user );

        for ( auto & bcat : req.bcat )
        {
            ptr->blockedCategories.push_back( OpenRTB::ContentCategory( bcat ) );
        }
        ptr->unparseable = req.unparseable;
    }
}
