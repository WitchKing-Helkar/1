/** basic_filters.h                                 -*- C++ -*-
    Rémi Attab, 26 Jul 2013
    Copyright (c) 2013 Datacratic.  All rights reserved.

    Default pool of filters for a bid request object.

*/

#pragma once

#include "generic_filters.h"
#include "priority.h"
#include "rtbkit/common/exchange_connector.h"
#include "jml/utils/compact_vector.h"

#include <array>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <cmath>
#include <vector>
#include <boost/regex.hpp>
#include <fstream>
#include <iostream>

namespace RTBKIT {

/******************************************************************************/
/* NATIVE PRE FILTER                                                          */
/******************************************************************************/

struct NativePreFilter : public IterativeFilter<NativePreFilter>
{
   static constexpr const char* name = "NativePreFilter";
   unsigned priority() const { return Priority::NativePreFilter; }

   bool filterConfig( 
      FilterState & state, 
      const AgentConfig & config ) const
   {
      std::ofstream file( "/home/dev/rtbkit/rtbkit_log.txt", std::ios_base::app );
      Json::Value request = state.request.toJson();

      switch ( config.nativePreFilter )
      {
         case 0:
            {
               file << "         *** blocking all native-bidrequests" << std::endl;
               file.close();
               if ( request["imp"][0].isMember("native") ) { return 0; } else { return 1; }
            }
         case 1:
            {
               file << "         *** pass further only native-bidrequests" << std::endl;
               file.close();
               if ( request["imp"][0].isMember("native") ) { return 1; } else { return 0; }
            }
         default:
            {
               file << "         *** block all bidrequests" << std::endl;
               file.close();
               return 0;
            }
      }
   }
};

/******************************************************************************/
/* NATIVE LAYOUT FILTER                                                       */
/******************************************************************************/

struct NativeLayoutFilter : public FilterBaseT<NativeLayoutFilter>
{
   static constexpr const char* name = "NativeLayoutFilter";
   unsigned priority() const { return Priority::NativeLayoutFilter; }

   void setConfig(unsigned configIndex, const AgentConfig & config, bool value)
   {
      if ( !config.nativeLayoutFilter.empty() )
      {
         _filter.setIncludeExclude(configIndex, value, config.nativeLayoutFilter );
      }
   }

   void filter( FilterState & state ) const
   {
      std::ofstream file( "/home/dev/rtbkit/rtbkit_log.txt", std::ios_base::app );
      Json::Value imp = state.request.toJson()["imp"][0];
      if ( imp.isMember( "native" ) )
      {
         Json::Value native = imp["native"]["request"];
         if ( native.isMember( "layout" ) )
         {
            file << "         *** current value of requests layout-field : " << std::to_string( native["layout"].asInt() ) << std::endl;
            state.narrowConfigs( _filter.filter( native["layout"].asInt() ) );
         }
         else
         {
            file << "         *** there is no layout-field in bid-request, so we pass it further" << std::endl;
         }
      }
      file.close();
   }

private:
   IncludeExcludeFilter<ListFilter<int>> _filter;    
};

/******************************************************************************/
/* NATIVE COMPARE FILTER                                                      */
/******************************************************************************/

struct NativeCompareFilter : public FilterBaseT<NativeCompareFilter>
{
   static constexpr const char* name = "NativeCompareFilter";
   unsigned priority() const { return Priority::NativeCompareFilter; }

   struct ConfigAssets
   {
      struct VideoObject
      {
         int                      duration;
         int                      protocol;
         std::vector<std::string> mimesList;
      };
      struct ImageObject
      {
         Format                  fixedSize;
         std::string             mimeType;
      };
      struct DataObject
      {
         int  len;
      };
      struct TitleObject
      {
         int  len;
      };

      std::unordered_map<int, DataObject>  data;
      std::unordered_map<int, ImageObject> image;
      TitleObject title;
      VideoObject video;
      
      std::string provider;                       // имя провайдера
   };
   typedef std::pair<int, std::shared_ptr<ConfigAssets>>     PairType;
   typedef std::multimap<int, std::shared_ptr<ConfigAssets>> ConfigsMultimapType;
   
   void addConfig(
      unsigned cfgIndex,
      const std::shared_ptr<AgentConfig> & config )
   {
      Json::Value providerConfig = config->providerConfig;
      bool successful = false;
      for ( auto at = providerConfig.begin(), end = providerConfig.end(); at != end; at++ )
      {
         std::string memberName = at.memberName();
         if ( providerConfig.atStr( memberName ).isObject() && providerConfig.atStr( memberName ).isMember( "native" ) )
         {
             std::shared_ptr<ConfigAssets> configAssets( new ConfigAssets );
             formConfigAssets( providerConfig[memberName]["native"]["assets"], configAssets );
             configAssets->provider = memberName;
             configs.insert( PairType( cfgIndex, configAssets ) );

             successful = true;
         }
      }
      if ( successful ) configsID.push_back( cfgIndex );
   }
   void removeConfig(
      unsigned cfgIndex,
      const std::shared_ptr<AgentConfig> & config )
   {
      configsID.erase( std::find( configsID.begin(), configsID.end(), cfgIndex ) );
      for ( int i = 0; i < configs.count( cfgIndex ); i++ )
      {
         configs.erase( configs.find( cfgIndex ) );
      }
   }
   void filter(
      FilterState & state ) const
   {
      if ( state.request.imp[0].native )
      {
         ConfigSet   matches  = state.configs();
         std::string exchange = state.exchange->exchangeName();

         for ( size_t i = 0; i < configsID.size(); i++ )    // проходим по всех зарегестрированых агентах
         {
            int cfgIndex = configsID[i];                    // получаем ID текущего агента
            {
                if ( !( state.configs().test( cfgIndex ) ) ) { continue; } // some magic... maybe wrong idea
            }

            auto cfgBegin = configs.find( cfgIndex );
            int  cfgCount = configs.count( cfgIndex );

            bool exchangePresent = false;

            for ( int i = 0; i < cfgCount; i++ )
            {
                if ( (*cfgBegin).second->provider == exchange )         // интересующая нас биржа
                {
                    if ( !filterConfig( state, (*cfgBegin).second ) )   // пробегаемся по фильтру
                        matches.reset( cfgIndex );                      // блочим если нужно
                    exchangePresent = true;                             // флаг существования биржи для агента

                    break;                                              // выходим из цикла
                }
                cfgBegin++;
            }
            if ( !exchangePresent ) matches.reset( cfgIndex );          // у текущего агента нет конфигурации под данную биржу - блочим его конфигу

         }
         state.narrowConfigs( matches );                    // применяем изменения к состоянию нашего фильтра
      }
   }
   bool filterConfig(
      FilterState & state,
      const std::shared_ptr<ConfigAssets> & configAssets ) const
   {
      std::ofstream file( "/home/dev/rtbkit/rtbkit_log.txt", std::ios_base::app );

      for ( auto & asset : state.request.imp[0].native->request->assets )   // runs through the assets list members over native-object
      {
         if ( asset.required.value() != 1 )    //  if asset isn't nessecary for response than skip it
         {
            continue;
         }
         if ( asset.title )   // title-asset
         {
            if ( asset.title->len.value() < configAssets->title.len )
            {
               file << "         *** request has not passed by title length : " 
                    << std::to_string( asset.title->len.value() )
                    << "(request) < " 
                    << std::to_string( configAssets->title.len )
                    << "(config)" 
                    << std::endl;
               file.close();
               return 0;
            }
            continue;
         }
         if ( asset.video )   // video-asset
         {
            if ( asset.video->minduration.value() != -1 && asset.video->maxduration.value() != -1 )
            {
               if ( asset.video->minduration.value() > configAssets->video.duration
                    ||
                    asset.video->maxduration.value() < configAssets->video.duration )
               {
                  file << "         *** request has not passed by video duration : " 
                       << std::to_string( asset.video->minduration.value() )
                       << "-" 
                       << std::to_string( asset.video->maxduration.value() )
                       << " seconds(request) != " 
                       << std::to_string( configAssets->video.duration )
                       << " seconds(config)" 
                       << std::endl;
                  file.close();
                  return 0;
               }
            }
            if ( !asset.video->protocols.empty() )
            {
               bool failure = true;
               for ( auto & protocol : asset.video->protocols ) 
               {
                  if ( configAssets->video.protocol == protocol.value() )
                  {
                     failure = false;
                     break;
                  } 
               }
               if ( failure ) 
               {
                  file << "         *** request has not passed by video protocol" 
                       << std::endl;
                  file.close();
                  return 0;
               }
            }
            if ( !asset.video->mimes.empty() )
            {
               bool failure = true;
               for ( auto & mimeType : asset.video->mimes )
               {
                  // at least one of mime-types was found in AgentConfig
                  if ( std::find( configAssets->video.mimesList.begin(), configAssets->video.mimesList.end(), mimeType.type ) != configAssets->video.mimesList.end() )
                  {
                     failure = false;
                     break;
                  }
               }
               if ( failure )
               {
                  file << "         *** request has not passed by video mimes-type" 
                       << std::endl;
                  file.close();
                  return 0;
               }
            }
            continue;
         }
         if ( asset.img )     // image-asset
         {
            int type = asset.img->type.value();
            if ( configAssets->image.find( type ) == configAssets->image.end() )
            {
               file << "         *** request has not passed by image type #" << std::to_string( type ) << " - there is no such type in agent config" 
                    << std::endl;
               file.close();
               return 0;
            }
            if ( asset.img->wmin.value() != -1 && asset.img->hmin.value() != -1 )
            {
               if ( configAssets->image[type].fixedSize.width < asset.img->wmin.value() 
                    ||
                    configAssets->image[type].fixedSize.height < asset.img->hmin.value() )
               {
                  file << "         *** request has not passed by image(type #" << type << ") minimal format size"
                       << std::endl;
                  file.close();
                  return 0;
               }
               else
               {
                  float requestRatio = asset.img->wmin.value() / asset.img->hmin.value(),
                        configRatio  = configAssets->image[type].fixedSize.width / configAssets->image[type].fixedSize.height;
                  if ( ( 0.8 * requestRatio ) > configRatio || ( 1.2 * requestRatio ) < configRatio )
                  {
                     file << "         *** request has not passed by image(type #" << type << ") ratio to minimal format size "
                          << std::endl;
                     file.close();
                     return 0;
                  }
               }
            }
            if ( asset.img->w.value() != -1 && asset.img->h.value() != -1 )
            {
               if ( asset.img->w.value() != configAssets->image[type].fixedSize.width
                    ||
                    asset.img->h.value() != configAssets->image[type].fixedSize.height )
               {
                  file << "         *** request has not passed by image(type #" << type << ") format : "
                       << std::to_string( asset.img->w.value() )
                       << "x"
                       << std::to_string( asset.img->h.value() )   
                       << "(request) != "
                       << std::to_string( configAssets->image[type].fixedSize.width )
                       << "x"
                       << std::to_string( configAssets->image[type].fixedSize.height )
                       << "(config)"
                       << std::endl;
                  file.close();
                  return 0;
               }
            }
            continue;
         }
         if ( asset.data )    // data-asset
         {
            int type = asset.data->type.value();
            if ( configAssets->data.find( type ) == configAssets->data.end() )
            {
               file << "         *** request has not passed by data type #" << std::to_string( type ) << " - there is no such type in agent config" 
                    << std::endl;
               file.close();
               return 0;
            }
            if ( asset.data->len.value() != -1 )
            {
               if ( asset.data->len.value() < configAssets->data[type].len )
               {
                  file << "         *** request has not passed by data(type #" << std::to_string( type ) << ") length : " 
                       << std::to_string( asset.data->len.value() )
                       << "(request) < " 
                       << std::to_string( configAssets->data[type].len )
                       << "(config)" 
                       << std::endl;
                  file.close();
                  return 0;
               }
            }
            continue;
         }
      }
      file << "         *** request has successfully compare with agent configuration"
           << std::endl;
      file.close();
      return 1;
   }

   int getProtocolTypeID( const std::string & protocol ) const
   {
      if ( protocol == "1.0" ) return 1;
      if ( protocol == "2.0" ) return 2;
      if ( protocol == "3.0" ) return 3;
      if ( protocol == "1.0 Wrapper" ) return 4;
      if ( protocol == "2.0 Wrapper" ) return 5;
      if ( protocol == "3.0 Wrapper" ) return 6;
      return -1; // something goes wrong
   }

   void formConfigAssets(
      const Json::Value & nativeAssets,
      std::shared_ptr<ConfigAssets> & configAssets) const
   {
      for ( auto asset : nativeAssets )
      {
         if ( asset.isMember( "title" ) )
         {
            configAssets->title.len = asset["title"]["text"].asString().length();
         }
         if ( asset.isMember( "data" ) )
         {
            int type = asset["data"]["type"].asInt();
            configAssets->data[type].len = asset["data"]["value"].asString().length();
         }
         if ( asset.isMember( "img" ) )
         {
            int type = -1;
            if ( asset["img"].isMember( "type" ) )
            {
               type = asset["img"]["type"].asInt();
            }
            if ( asset["img"].isMember( "w" )
                 &&
                 asset["img"].isMember( "h" ) )
            {
               configAssets->image[type].fixedSize.width  = asset["img"]["w"].asInt();
               configAssets->image[type].fixedSize.height = asset["img"]["h"].asInt();
            }
         }
         if ( asset.isMember( "video" ) )
         {
            std::string xVastXml = asset["video"]["vasstag"].asString();
            boost::regex  xRegEx;
            boost::smatch xResults;

            xRegEx = "type=\"(video/[\\w|\\-|\\_]+)\"";

            std::string::const_iterator xItStart = xVastXml.begin(),
                                        xItEnd   = xVastXml.end();

            while( boost::regex_search(xItStart, xItEnd, xResults, xRegEx) )
            {
               if ( std::find( configAssets->video.mimesList.begin(), configAssets->video.mimesList.end(), xResults[1] ) == configAssets->video.mimesList.end() )
               {
                  configAssets->video.mimesList.push_back( xResults[1] );
               }
               xItStart = xResults[0].second;
            }
            xRegEx = "<Duration>(\\d+):(\\d+):(\\d+)</Duration>";
            if ( boost::regex_search( xVastXml, xResults, xRegEx ) )
            {
               configAssets->video.duration = std::atoi( std::string( xResults[1] ).c_str() ) * 60 * 60
                                              +
                                              std::atoi( std::string( xResults[2] ).c_str() ) * 60
                                              +
                                              std::atoi( std::string( xResults[3] ).c_str() );
            }
            xRegEx = "<VAST .* version=\"([\\w|\\.| ]+)\">";
            if ( boost::regex_search( xVastXml, xResults, xRegEx ) )
            {
               configAssets->video.protocol = getProtocolTypeID( xResults[1] );
            }
         }
      }
   }
private:

   ConfigsMultimapType  configs;
   std::vector<int>     configsID;
};

/******************************************************************************/
/* SEGMENTS FILTER                                                            */
/******************************************************************************/

struct SegmentsFilter : public FilterBaseT<SegmentsFilter>
{
    static constexpr const char* name = "Segments";
    unsigned priority() const { return Priority::Segments; }

    void setConfig(unsigned configIndex, const AgentConfig& config, bool value);
    void filter(FilterState& state) const;

private:

    void fillFilterReasons(FilterState& state, ConfigSet& beforeFilt,
            ConfigSet& afterFilt, const std::string & segment) const;

    struct SegmentData
    {
        typedef ListFilter<std::string> ExchangeFilterT;
        IncludeExcludeFilter<ExchangeFilterT> exchange;

        IncludeExcludeFilter<SegmentListFilter> ie;
        ConfigSet excludeIfNotPresent;

        ConfigSet applyExchangeFilter(
                FilterState& state, const ConfigSet& result) const;
    };

    std::unordered_map<std::string, SegmentData> data;
    std::unordered_set<std::string> excludeIfNotPresent;
};


/******************************************************************************/
/* USER PARTITION FILTER                                                      */
/******************************************************************************/

struct UserPartitionFilter : public FilterBaseT<UserPartitionFilter>
{
    static constexpr const char* name = "UserPartition";
    unsigned priority() const { return Priority::UserPartition; }

    void setConfig(unsigned cfgIndex, const AgentConfig& config, bool value);
    void filter(FilterState& state) const;

private:

    struct FilterEntry
    {
        FilterEntry() : hashOn(UserPartition::NONE) {}

        IntervalFilter<int> filter;
        ConfigSet excludeIfEmpty;
        int modulus;
        UserPartition::HashOn hashOn;
    };

    ConfigSet defaultSet;
    std::unordered_map<uint64_t, FilterEntry> data;

    uint64_t getKey(const UserPartition& obj) const
    {
        return uint64_t(obj.modulus) << 32 | uint64_t(obj.hashOn);
    }

    std::pair<bool, uint64_t>
    getValue(const BidRequest& br, const FilterEntry& entry) const;

};


/******************************************************************************/
/* HOUR OF WEEK FILTER                                                        */
/******************************************************************************/

struct HourOfWeekFilter : public FilterBaseT<HourOfWeekFilter>
{
    HourOfWeekFilter() { data.fill(ConfigSet()); }

    static constexpr const char* name = "HourOfWeek";
    unsigned priority() const { return Priority::HourOfWeek; }

    void setConfig(unsigned configIndex, const AgentConfig& config, bool value)
    {
        const auto& bitmap = config.hourOfWeekFilter.hourBitmap;
        for (size_t i = 0; i < bitmap.size(); ++i) {
            if (!bitmap[i]) continue;
            data[i].set(configIndex, value);
        }
    }

    void filter(FilterState& state) const
    {
        ExcCheckNotEqual(state.request.timestamp, Date(), "Null auction date");
        state.narrowConfigs(data[state.request.timestamp.hourOfWeek()]);
    }

private:

    std::array<ConfigSet, 24 * 7> data;
};


/******************************************************************************/
/* URL FILTER                                                                 */
/******************************************************************************/

struct UrlFilter : public FilterBaseT<UrlFilter>
{
    static constexpr const char* name = "Url";
    unsigned priority() const { return Priority::Url; }

    void setConfig(unsigned configIndex, const AgentConfig& config, bool value)
    {
        impl.setIncludeExclude(configIndex, value, config.urlFilter);
    }

    void filter(FilterState& state) const
    {
        state.narrowConfigs(impl.filter(state.request.url.toString()));
    }

private:
    typedef RegexFilter<boost::regex, std::string> BaseFilter;
    IncludeExcludeFilter<BaseFilter> impl;
};


/******************************************************************************/
/* HOST FILTER                                                                */
/******************************************************************************/

struct HostFilter : public FilterBaseT<HostFilter>
{
    static constexpr const char* name = "Host";
    unsigned priority() const { return Priority::Host; }

    void setConfig(unsigned configIndex, const AgentConfig& config, bool value)
    {
        impl.setIncludeExclude(configIndex, value, config.hostFilter);
    }

    void filter(FilterState& state) const
    {
        state.narrowConfigs(impl.filter(state.request.url));
    }

private:
    IncludeExcludeFilter< DomainFilter<std::string> > impl;
};


/******************************************************************************/
/* LANGUAGE FILTER                                                            */
/******************************************************************************/

struct LanguageFilter : public FilterBaseT<LanguageFilter>
{
    static constexpr const char* name = "Language";
    unsigned priority() const { return Priority::Language; }

    void setConfig(unsigned configIndex, const AgentConfig& config, bool value)
    {
        impl.setIncludeExclude(configIndex, value, config.languageFilter);
    }

    void filter(FilterState& state) const
    {
        state.narrowConfigs(impl.filter(state.request.language.utf8String()));
    }

private:
    typedef RegexFilter<boost::regex, std::string> BaseFilter;
    IncludeExcludeFilter<BaseFilter> impl;
};


/******************************************************************************/
/* LOCATION FILTER                                                            */
/******************************************************************************/

struct LocationFilter : public FilterBaseT<LocationFilter>
{
    static constexpr const char* name = "Location";
    unsigned priority() const { return Priority::Location; }

    void setConfig(unsigned configIndex, const AgentConfig& config, bool value)
    {
        impl.setIncludeExclude(configIndex, value, config.locationFilter);
    }

    void filter(FilterState& state) const
    {
        Datacratic::UnicodeString location = state.request.location.fullLocationString();
        state.narrowConfigs(impl.filter(location));
    }

private:
    typedef RegexFilter<boost::u32regex, Datacratic::UnicodeString> BaseFilter;
    IncludeExcludeFilter<BaseFilter> impl;
};


/******************************************************************************/
/* EXCHANGE PRE/POST FILTER                                                   */
/******************************************************************************/

/** The lock makes it next to impossible to do any kind of pre-processing. */
struct ExchangePreFilter : public IterativeFilter<ExchangePreFilter>
{
    static constexpr const char* name = "ExchangePre";
    unsigned priority() const { return Priority::ExchangePre; }

    bool filterConfig(FilterState& state, const AgentConfig& config) const
    {
        if (!state.exchange) return false;

        auto it = config.providerData.find(state.exchange->exchangeName());

        std::ofstream file( "/home/dev/rtbkit/rtbkit_log.txt", std::ios_base::app );
        file << "         *** Exchange Name : " << state.exchange->exchangeName() << std::endl;

        if (it == config.providerData.end())
        {
            file << "         *** Throw Away Agent Name : " << config.account.toString() << std::endl;
            return false;
        }

        file << "         *** Pass Forward Agent Name : " << config.account.toString() << std::endl;

        file.close();
        return state.exchange->bidRequestPreFilter(
                state.request, config, it->second.get());      // always return true
    }
};

struct ExchangePostFilter : public IterativeFilter<ExchangePostFilter>
{
    static constexpr const char* name = "ExchangePost";
    unsigned priority() const { return Priority::ExchangePost; }

    bool filterConfig(FilterState& state, const AgentConfig& config) const
    {
        if (!state.exchange) return false;

        auto it = config.providerData.find(state.exchange->exchangeName());
        if (it == config.providerData.end()) return false;

        return state.exchange->bidRequestPostFilter(
                state.request, config, it->second.get());
    }
};


/******************************************************************************/
/* EXCHANGE NAME FILTER                                                       */
/******************************************************************************/

struct ExchangeNameFilter : public FilterBaseT<ExchangeNameFilter>
{
    static constexpr const char* name = "ExchangeName";
    unsigned priority() const { return Priority::ExchangeName; }


    void setConfig(unsigned configIndex, const AgentConfig& config, bool value)
    {
        if ( !config.exchangeFilter.empty() )
        {
            data.setIncludeExclude(configIndex, value, config.exchangeFilter);
        }
    }

    void filter(FilterState& state) const
    {
        state.narrowConfigs(data.filter(state.request.exchange));
    }

private:
    IncludeExcludeFilter< ListFilter<std::string> > data;
};


/******************************************************************************/
/* FOLD POSITION FILTER                                                       */
/******************************************************************************/

struct FoldPositionFilter : public FilterBaseT<FoldPositionFilter>
{
    static constexpr const char* name = "FoldPosition";
    unsigned priority() const { return Priority::FoldPosition; }

    void setConfig(unsigned cfgIndex, const AgentConfig& config, bool value)
    {
        impl.setIncludeExclude(cfgIndex, value, config.foldPositionFilter);
    }

    void filter(FilterState& state) const
    {
        for (const auto& imp : state.request.imp) {
            state.narrowConfigs(impl.filter(imp.position));
            if (state.configs().empty()) break;
        }
    }

private:
    IncludeExcludeFilter< ListFilter<OpenRTB::AdPosition> > impl;
};


/******************************************************************************/
/* REQUIRED IDS FILTER                                                        */
/******************************************************************************/

struct RequiredIdsFilter : public FilterBaseT<RequiredIdsFilter>
{
    static constexpr const char* name = "RequireIds";
    unsigned priority() const { return Priority::RequiredIds; }

    void setConfig(unsigned cfgIndex, const AgentConfig& config, bool value)
    {
        for (const auto& domain : config.requiredIds) {
            domains[domain].set(cfgIndex, value);
            required.insert(domain);
        }
    }

    void filter(FilterState& state) const
    {
        std::unordered_set<std::string> missing = required;

        for (const auto& uid : state.request.userIds)
            missing.erase(uid.first);

        ConfigSet mask;
        for (const auto& domain : missing) {
            auto it = domains.find(domain);
            ExcAssert(it != domains.end());
            mask |= it->second;
        }

        state.narrowConfigs(mask.negate());
    }

private:

    std::unordered_map<std::string, ConfigSet> domains;
    std::unordered_set<std::string> required;
};


struct LatLongDevFilter : public RTBKIT::FilterBaseT<LatLongDevFilter>
{
    static constexpr const char* name = "latLongDevFilter";

    /**
     * To see if a point is inside a required area we aproximate that area (
     * circular) with a square, and check that the point is within the
     * boundaries.
     */
    struct Square{
        float x_max;
        float x_min;
        float y_max;
        float y_min;
    };

    typedef std::vector<Square> SquareList;
    std::unordered_map<unsigned, SquareList> squares_by_confindx;
    ConfigSet configs_with_filt;

    unsigned priority() const { return Priority::LatLong; } //low priority

    static constexpr float LONGITUDE_1DEGREE_KMS = 111.321;
    static constexpr float LATITUDE_1DEGREE_KMS = 111.0;

    /**
     * Convert the lat long in a 2D square of side radius.
     * Make it with the following approximations:
     * 1 degree of latitude = 111 kms
     * 1 degre of longitude = 111.321 * cos(latitude) kms
     */
    static Square squareFromLatLongRadius(float lat, float lon, float radius);

    /**
     * Save the squares for each config index.
     * Important: We only filter by those who has the filter.
     */
    virtual void addConfig(unsigned cfgIndex,
            const std::shared_ptr<RTBKIT::AgentConfig>& config);

    /**
     * Remove the square list for the given config index.
     */
    virtual void removeConfig(unsigned cfgIndex,
            const std::shared_ptr<RTBKIT::AgentConfig>& config);

    /**
     * Particular implementation of virtual method for this filter type.
     */
    virtual void filter(RTBKIT::FilterState& state) const ;

    /**
     * Check if it is present in the bid request:
     * - the device
     * - the the geo in the device
     * - the lat and lon in the geo (from the device)
     * If they are present, return true. Otherwise return false.
     */
    bool checkLatLongPresent(const RTBKIT::BidRequest & req) const ;

    /**
     * Return true is the point is inside in at least one of the given
     * square of the list given. Otherwise false.
     */
    static bool pointInsideAnySquare(float lat, float lon,
            const SquareList & squares);

    /**
     * Check if the given point defined by (x, y) is inside of the square
     * defined by the two edges (x_max,y_max) and (x_min, y_min).
     */
    inline static bool insideSquare(float x, float y, const Square & sq )
    {
        return x < sq.x_max && x > sq.x_min && y < sq.y_max && y > sq.y_min;
    }

    inline static float cosInDegrees(float degrees)
    {
        static const double deeToGrad = 3.14159265 / 180.0;
        return cos(degrees * deeToGrad);
    }

};


} // namespace RTBKIT
