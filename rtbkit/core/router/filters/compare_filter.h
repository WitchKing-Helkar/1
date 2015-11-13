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
         std::vector<Format>     formatList;
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
               configAssets->image[type].formatList.push_back( Format( asset["img"]["w"].asInt(), asset["img"]["h"].asInt() ) );
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