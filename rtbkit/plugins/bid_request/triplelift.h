#pragma once

#include <string>
#include <memory>
#include <vector>
#include "soa/types/id.h"
#include "soa/types/string.h"
#include "soa/types/url.h"
#include "jml/utils/compact_vector.h"
#include "soa/jsoncpp/value.h"
#include <iostream>
#include "soa/types/value_description.h"
#include "soa/types/basic_value_descriptions.h"

namespace TripleLift
{
    struct NativeLayoutID: public Datacratic::TaggedEnum<NativeLayoutID> {
        enum Vals {
            UNSPECIFIED = -1,

            CONTENT_WALL = 1,
            APP_WALL = 2,
            NEWS_FEED = 3,
            CHAT_LIST = 4,
            CAROUSEL = 5,
            CONTENT_STREAM = 6,
            GRID_ADJOINING_THE_CONTENT = 7
        };
    };

    struct NativeAdUnitID: public Datacratic::TaggedEnum<NativeAdUnitID> {
        enum Vals {
            UNSPECIFIED = -1,

            PAID_SEARCH_UNITS = 1,
            RECOMMENDATION_WIDGETS = 2,
            PROMOTED_LISTINGS = 3,
            IN_AD_NATIVE_ELEMENT_UNITS = 4,
            CUSTOM = 5
        };
    };

    struct NativeBidRequestAssetTypes: public Datacratic::TaggedEnum<NativeBidRequestAssetTypes> {
        enum Vals {
            UNSPECIFIED = -1,

            TITLE_TITLE = 1,
            IMAGE_IMAGE = 2,
            LOGO_IMAGE = 3,
            CAPTION_DATA = 4,
            ADVERTISER_NAME_DATA = 5,
            VIDEO = 6
        };
    };

    struct ImageAssetTypes: public Datacratic::TaggedEnum<ImageAssetTypes> {
        enum Vals {
            UNSPECIFIED = -1,

            LOGO = 2,
            MAIN = 3
        };
    };

    struct DataAssetTypes: public Datacratic::TaggedEnum<DataAssetTypes> {
        enum Vals {
            UNSPECIFIED = -1,

            SPONSORED = 1,
            DESC = 2
        };
    };

    struct DeviceTypeMapping: public Datacratic::TaggedEnum<DeviceTypeMapping> {
        enum Vals {
            UNSPECIFIED = -1,

            MOBILE_TABLET = 1,
            PERSONAL_COMPUTER = 2,
            CONNECTED_TV = 3,
            PHONE = 4,
            TABLET = 5,
            CONNECTED_DEVICE = 6,
            SET_TOP_BOX = 7
        };
    };

    /**
     * BID_REQUEST
     * The following describes the various attributes that are supported in the
     * TripleLift OpenRTB 2.3 implementation. Please note that values marked
     * required" will always be sent. "Optional" means they may or may not be sent
     */

    struct NativeAssetImage {
        ~NativeAssetImage(){}
        ImageAssetTypes type;
        Datacratic::Optional<Datacratic::TaggedInt> w;
        Datacratic::Optional<Datacratic::TaggedInt> h;
        Datacratic::TaggedInt wmin;
        Datacratic::TaggedInt hmin;
    };

    struct NativeAssetTitle {
        ~NativeAssetTitle(){}
        Datacratic::TaggedInt len;
    };

    struct NativeAssetData {
        ~NativeAssetData(){}
        DataAssetTypes type;
        Datacratic::Optional<Datacratic::TaggedInt> len;
    };

    struct NativeAsset {
        ~NativeAsset(){}
        NativeBidRequestAssetTypes id;
        Datacratic::TaggedBool     required;
//      * The asset object will have one of title, img, video, or data.
        Datacratic::Optional<NativeAssetImage> img;
        Datacratic::Optional<NativeAssetTitle> title;
        Datacratic::Optional<NativeAssetData>  data;
    };

    struct NativeRequest {
        ~NativeRequest(){}
        Datacratic::TaggedInt ver;
        NativeLayoutID layout;
        NativeAdUnitID adunit;
        //  Datacratic::TaggedInt seq;
        Datacratic::TaggedInt plcmtcnt;
        Datacratic::List<NativeAsset> assets;
//      *   Unparseable fields get put here
        Json::Value unparseable;
    };

    struct Native {
        ~Native(){}
//      *   "request" is NativeRequest-struct represent as string
        std::string           request;
        Datacratic::TaggedInt ver;
    };

//  *   TripleLiftNative is alternate struct for native-bidrequests for OpenRTB 2.2 protocol
//  *   Needed for backwards compability
    struct TripleLiftNative {
        ~TripleLiftNative(){}
        Datacratic::TaggedInt imgw;
        Datacratic::TaggedInt imgh;
        Datacratic::Optional<Datacratic::TaggedInt> headinglen;
        Datacratic::Optional<Datacratic::TaggedInt> captionlen;
        Datacratic::Optional<Datacratic::TaggedInt> subheadinglen ;
    };

    struct Ext {
        ~Ext(){}
        TripleLiftNative triplelift;
    };

    struct Banner {
        ~Banner(){}
        Datacratic::TaggedInt w;
        Datacratic::TaggedInt h;
        Datacratic::List<Datacratic::TaggedInt> battr;
        Ext ext;
    };

    struct Impression {
        ~Impression(){}
        Datacratic::Id id;
        Datacratic::UnicodeString tagid;
        Datacratic::Optional<Native> native;
        Datacratic::Optional<Banner> banner;
    };

    struct Publisher {
        ~Publisher(){}
        Datacratic::Optional<Datacratic::Id> id;
        Datacratic::Optional<Datacratic::List<std::string>> cat;
    };

    struct Site {
        ~Site(){}
        Datacratic::Optional<Datacratic::Id> id;
        Datacratic::Optional<Datacratic::List<std::string>> cat;
        Datacratic::Optional<Datacratic::UnicodeString> domain;
        Datacratic::Optional<Datacratic::Url> page;
        Datacratic::Optional<Publisher> publisher;
    };

    struct App {
        ~App(){}
        Datacratic::Optional<Datacratic::Id> id;
        Datacratic::Optional<std::string> name;
        Datacratic::Optional<Publisher> publisher;
    };

    struct Geo {
        ~Geo(){}
        Datacratic::Optional<Datacratic::TaggedDouble> lat;
        Datacratic::Optional<Datacratic::TaggedDouble> lon;
        Datacratic::Optional<std::string> country;
        Datacratic::Optional<std::string> region;
        Datacratic::Optional<std::string> city;
    };

    struct Device {
        ~Device(){}
        Datacratic::Optional<Datacratic::UnicodeString> ua;
        Datacratic::Optional<std::string> ip;
        Datacratic::Optional<Geo> geo;
        Datacratic::Optional<Datacratic::UnicodeString> make;
        Datacratic::Optional<Datacratic::UnicodeString> model;
        Datacratic::Optional<Datacratic::UnicodeString> os;
        Datacratic::Optional<DeviceTypeMapping> devicetype;
    };

    struct User {
        ~User(){}
        Datacratic::Optional<Datacratic::Id> id;
        Datacratic::Optional<Datacratic::Id> buyeruid;
    };

    struct BidRequest {
        ~BidRequest(){}
        Datacratic::Id id;
        std::vector<Impression> imp;
        Datacratic::Optional<Site> site;
        Datacratic::Optional<App> app;
        Datacratic::Optional<Device> device;
        User user;
        Datacratic::List<std::string> bcat;
//      *   Unparseable fields get put here
        Json::Value unparseable;
    };

    /**
     * BID_RESPONSE
     * The structure and contents of the Bid Response is the same as in OpenRTB 2.2.
     * To return the specifics of the native ad, use a JSON-encoded string in the
     * adm field.
     * Note:
     *      this is not a JSON object, it is a string of an object. The spec will
     *      describe the JSON structure that determines the string.
     */

    struct ResponseExt {
        ~ResponseExt(){}
        Datacratic::Optional<std::string> viewtracker;
        Datacratic::Optional<Datacratic::TaggedBool> useaadchoices;
        Datacratic::Optional<std::string> adchoicesoverrideurl;
    };

    struct ResponseLink {
        ~ResponseLink(){}
        Datacratic::Url url;
        Datacratic::List<Datacratic::Url> clicktrackers;
    };

    struct ResponseNativeVideo {
        ~ResponseNativeVideo(){}
//      *   TODO wait until it's will be implemented
    };

    struct ResponseNativeData {
        ~ResponseNativeData(){}
        std::string value;
    };

    struct ResponseNativeImage {
        ~ResponseNativeImage(){}
        Datacratic::Url url;
        Datacratic::TaggedInt w;
        Datacratic::TaggedInt h;
    };

    struct ResponseNativeTitle {
        ~ResponseNativeTitle(){}
        std::string text;
    };

    struct ResponseAsset {
        ~ResponseAsset(){}
        Datacratic::Id id;
        Datacratic::Optional<ResponseNativeTitle> title;
        Datacratic::Optional<ResponseNativeImage> img;
        Datacratic::Optional<ResponseNativeData>  data;
        Datacratic::Optional<ResponseNativeVideo> video;
    };

    struct ResponseNative {
        ~ResponseNative(){}
        Datacratic::TaggedInt ver;
        Datacratic::List<ResponseAsset> assets;
        ResponseLink link;
        Datacratic::Optional<Datacratic::List<std::string>> imptrackers;
        Datacratic::Optional<ResponseExt> ext;
    };

    struct ResponseAdm {
         ~ResponseAdm(){}
         ResponseNative native;
    };

    struct ResponseBid_2point3 {
        ~ResponseBid_2point3(){}
        Datacratic::TaggedDouble price;
        Datacratic::Id  id;
        Datacratic::Id  impid;
        Datacratic::Url nurl;
//      *   "adm" is ResponseNative-struct represent as string
        std::string adm;
    };

    struct ResponseSeatBid_2point3 {
        ~ResponseSeatBid_2point3(){}
        Datacratic::List<ResponseBid_2point3> bid;
        Datacratic::Optional<std::string> seat;
    };

    struct BidResponse_2point3 {
        ~BidResponse_2point3(){}
        Datacratic::Id id;
        Datacratic::List<ResponseSeatBid_2point3> seatbid;
    };
}
