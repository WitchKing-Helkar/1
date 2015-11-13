#include "triplelift_parsing.h"
#include "soa/types/json_parsing.h"

using namespace TripleLift;

namespace Datacratic {

   DefaultDescription<BidRequest>::
   DefaultDescription()
   {
      onUnknownField = [=] (BidRequest * br, JsonParsingContext & context)
      {
         std::function<Json::Value & (int, Json::Value &)> getEntry
         = [&] (int n, Json::Value & curr) -> Json::Value &
         {
             if (n == context.path.size())
                 return curr;
             else if (context.path[n].index != -1)
                 return getEntry(n + 1, curr[context.path[n].index]);
             else return getEntry(n + 1, curr[context.path[n].fieldName()]);
         };

         getEntry(0, br->unparseable)
             = context.expectJson();
      };

      addField("id", &BidRequest::id, "Id of the bid request");
      addField("imp", &BidRequest::imp, "Single impression object");
      addField("site", &BidRequest::site, "An object of type Site");
      addField("app", &BidRequest::app, "An object of type App");
      addField("device", &BidRequest::device, "An object of type Device");
      addField("user", &BidRequest::user, "An object of type User");
      addField("bcat", &BidRequest::bcat, "The array of categories that are blocked by the publisher");
      addField("unparseable", &BidRequest::unparseable, "Unparseable fields are collected here");
   }

   DefaultDescription<Native>::
   DefaultDescription()
   {
       addField("request", &Native::request, "JSON-encoded native request object");
       addField("ver", &Native::ver, "IThis will always be 1");
   }

   DefaultDescription<NativeRequest>::
   DefaultDescription()
   {
       onUnknownField = [=] (NativeRequest * br, JsonParsingContext & context)
       {
          std::function<Json::Value & (int, Json::Value &)> getEntry
          = [&] (int n, Json::Value & curr) -> Json::Value &
          {
              if (n == context.path.size())
                  return curr;
              else if (context.path[n].index != -1)
                  return getEntry(n + 1, curr[context.path[n].index]);
              else return getEntry(n + 1, curr[context.path[n].fieldName()]);
          };

          getEntry(0, br->unparseable)
              = context.expectJson();
       };
       addField("ver", &NativeRequest::ver, "This will always be 1");
       addField("layout", &NativeRequest::layout, "The Layout ID of the native ad unit");
       addField("adunit", &NativeRequest::adunit, "The Ad unit ID of the native ad unit");
       addField("plcmtcnt", &NativeRequest::plcmtcnt, "This will always be 1");
       // addField("seq", &NativeRequest::seq, "Unknown Description");
       addField("assets", &NativeRequest::assets, "Array of AssetObjects");
       addField("unparseable", &NativeRequest::unparseable, "Unparseable fields are collected here");
   }

   DefaultDescription<NativeAsset>::
   DefaultDescription()
   {
       addField("id", &NativeAsset::id, "The Asset ID, as assigned by TripleLift");
       addField("required", &NativeAsset::required, "Set to 1 if required, 0 if not required");
       addField("img", &NativeAsset::img, "Image Object");
       addField("title", &NativeAsset::title, "Title Object");
       addField("data", &NativeAsset::data, "Data Object");
   }

   DefaultDescription<NativeAssetImage>::
   DefaultDescription()
   {
       addField("type", &NativeAssetImage::type, "Type of the image element");
       addField("w", &NativeAssetImage::w, "Width of the image in pixels");
       addField("h", &NativeAssetImage::h, "Height of the image in pixels");
       addField("wmin", &NativeAssetImage::wmin, "Absolute minimum width of image that will be accepted");
       addField("hmin", &NativeAssetImage::hmin, "Absolute minimum height of the image that will be accepted");
   }

   DefaultDescription<NativeAssetTitle>::
   DefaultDescription()
   {
       addField("len", &NativeAssetTitle::len, "The maximum length of the text in the title element");
   }

   DefaultDescription<NativeAssetData>::
   DefaultDescription()
   {
       addField("type", &NativeAssetData::type, "Type ID of the object");
       addField("len", &NativeAssetData::len, "Maximum length of the text in the element's response");
   }

   DefaultDescription<Impression>::
   DefaultDescription()
   {
       addField("id", &Impression::id, "This will always be 1");
       addField("tagid", &Impression::tagid, "TripleLift placement identifier");
       addField("native", &Impression::native, "OpenRTB 2.3 native-object implementation");
       addField("banner", &Impression::banner, "The TripleLift native implementation");
   }

   DefaultDescription<Ext>::
   DefaultDescription()
   {
       addField("triplelift", &Ext::triplelift, "TripleLift native-object");
   }

   DefaultDescription<Banner>::
   DefaultDescription()
   {
       addField("w", &Banner::w, "This will always be 0");
       addField("h", &Banner::h, "This will always be 0");
       addField("battr", &Banner::battr, "Includes the list of attributes that are banned");
       addField("ext", &Banner::ext, "TripleLift Native Extension section");
   }

   DefaultDescription<Site>::
   DefaultDescription()
   {
       addField("id", &Site::id, "ID of the site on TripleLift");
       addField("cat", &Site::cat, "List of IAB content Category codes");
       addField("domain", &Site::domain, "Site domain (may be masked at the publisher's request)");
       addField("page", &Site::page, "URL of the page where the impression will be shown");
       addField("publisher", &Site::publisher, "Publisher object");
   }

   DefaultDescription<App>::
   DefaultDescription()
   {
       addField("id", &App::id, "Application ID on the exchange");
       addField("name", &App::name, "Name of the application");
       addField("publisher", &App::publisher, "Publisher object");
   }

   DefaultDescription<Publisher>::
   DefaultDescription()
   {
       addField("id", &Publisher::id, "Publisher ID on the exchange");
       addField("cat", &Publisher::cat, "List of IAB content Category codes");
   }

   DefaultDescription<Device>::
   DefaultDescription()
   {
       addField("ua", &Device::ua, "Browser user agent string");
       addField("ip", &Device::ip, "Device IP address when available");
       addField("geo", &Device::geo, "Derived geography where possible");
       addField("make", &Device::make, "Device make (\"Apple\")");
       addField("model", &Device::model, "Device model (\"iPhone\")");
       addField("os", &Device::os, "Device OS (\"iOS\")");
       addField("devicetype", &Device::devicetype, "Based on device type mapping below");
   }

   DefaultDescription<Geo>::
   DefaultDescription()
   {
       addField("lat", &Geo::lat, "Latitude of the device");
       addField("lon", &Geo::lon, "Longitude of the device");
       addField("country", &Geo::country, "Country");
       addField("region", &Geo::region, "Region");
       addField("city", &Geo::city, "City");
   }

   DefaultDescription<User>::
   DefaultDescription()
   {
       addField("id", &User::id, "User ID when available");
       addField("buyeruid", &User::buyeruid, "The buyer's external ID that has been sync'd with TripleLift, to the extent that it is available");
   }

   DefaultDescription<TripleLiftNative>::
   DefaultDescription()
   {
       addField("imgw", &TripleLiftNative::imgw, "Width of the image in the placement");
       addField("imgh", &TripleLiftNative::imgh, "Height of the image in the placement");
       addField("headinglen", &TripleLiftNative::headinglen, "Length of the heading field");
       addField("captionlen", &TripleLiftNative::captionlen, "Length of the caption field");
       addField("subheadinglen", &TripleLiftNative::subheadinglen, "DEPRECATED");
   }

   DefaultDescription<BidResponse_2point3>::
   DefaultDescription()
   {
       addField("id", &BidResponse_2point3::id, "Id of the bid request");
       addField("seatbid", &BidResponse_2point3::seatbid, "Array of seatbid objects");
   }

   DefaultDescription<ResponseSeatBid_2point3>::
   DefaultDescription()
   {
       addField("bid", &ResponseSeatBid_2point3::bid, "Array of bid objects");
       addField("seat", &ResponseSeatBid_2point3::seat, "ID of the bidder seat on whose behalf this bid is made");
   }

   DefaultDescription<ResponseBid_2point3>::
   DefaultDescription()
   {
       addField("id", &ResponseBid_2point3::id, "ID for the bid object chosen by the bidder for tracking and debugging purposes");
       addField("impid", &ResponseBid_2point3::impid, "Must be 1");
       addField("price", &ResponseBid_2point3::price, "Bid price in CPM");
       addField("nurl", &ResponseBid_2point3::nurl, "URL for win notifications");
       addField("adm", &ResponseBid_2point3::adm, "Native bid-response");
   }

   DefaultDescription<ResponseAdm>::
   DefaultDescription()
   {
       addField("native", &ResponseAdm::native, "native-response");
   }

   DefaultDescription<ResponseNative>::
   DefaultDescription()
   {
       addField("ver", &ResponseNative::ver, "This must always be 1");
       addField("assets", &ResponseNative::assets, "List of the corresponding Asset objects, matching the specification in the request");
       addField("link", &ResponseNative::link, "Destination link specified by a Link object");
       addField("imptrackers", &ResponseNative::imptrackers, "Array of impression tracking URLs. This must be images or 204s");
       addField("ext", &ResponseNative::ext, "The Ext object contains a viewability pixel");
   }

   DefaultDescription<ResponseAsset>::
   DefaultDescription()
   {
       addField("id", &ResponseAsset::id, "Asset Id, corresponding to the request asset id");
       addField("img", &ResponseAsset::img, "Image object for the image asset(s)");
       addField("data", &ResponseAsset::data, "Data object for the data asset(s)");
       addField("title", &ResponseAsset::title, "Title object for the title asset");
       addField("video", &ResponseAsset::video, "Video object for the video asset, currently not supported");
   }

   DefaultDescription<ResponseNativeImage>::
   DefaultDescription()
   {
       addField("url", &ResponseNativeImage::url, "URL of the image asset");
       addField("w", &ResponseNativeImage::w, "Width of the image");
       addField("h", &ResponseNativeImage::h, "Height of the image");
   }

   DefaultDescription<ResponseNativeVideo>::
   DefaultDescription()
   {
   }

   DefaultDescription<ResponseNativeData>::
   DefaultDescription()
   {
       addField("value", &ResponseNativeData::value, "Data to be displayed");
   }

   DefaultDescription<ResponseNativeTitle>::
   DefaultDescription()
   {
       addField("text", &ResponseNativeTitle::text, "The text associated with the text element");
   }

   DefaultDescription<ResponseLink>::
   DefaultDescription()
   {
       addField("url", &ResponseLink::url, "Landing page of the ad");
       addField("clicktrackers", &ResponseLink::clicktrackers, "List of third-party click trackers");
   }

   DefaultDescription<ResponseExt>::
   DefaultDescription()
   {
       addField("viewtracker", &ResponseExt::viewtracker, "Pixel URL to be fired when the ad is viewable");
       addField("adchoicesoverrideurl", &ResponseExt::adchoicesoverrideurl, "If the useadchoices parameter for your bidder is set to"
                                                                                  " \"when_specified\", then the adchoicesurl that you specified"
                                                                                  " for your bidder will be used to overlay the adchoices icon");
       addField("useaadchoices", &ResponseExt::useaadchoices, "This will override the bidder-level adchoicesurl when provided");
   }
}
