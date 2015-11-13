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
         int type = r["data"]["type"].asInt();
         if ( r["required"].asBool() ) // если поле обезательное, то оно уже проверено в фильтре
         {
            currAsset["data"]["value"] = assets.data[type].value;
            currAsset["id"] = r["id"].asInt();                
            response.append( currAsset );
         }
         else  // нужна проверка на соответствие
         {
            if ( assets.data.find( type ) != assets.data.end() ) // проверка есть ли такой обьект в конфигах
            {
               int len = r["data"]["len"].asInt();
               if ( assets.data[type].value.length() <= len || len == -1 )  // а если есть, то проходит ли по длинне
               {
                  currAsset["data"]["value"] = assets.data[type].value;
                  currAsset["id"] = r["id"].asInt();                
                  response.append( currAsset );
               }
            }
         }
         continue;
      }
      if ( r.isMember( "img" ) )
      {
         int type = r["img"]["type"].asInt();
         if ( r["required"].asBool() ) // если поле обезательное, то оно уже проверено в фильтре
         {
            currAsset["img"]["url"] = assets.image[type].link;
            currAsset["img"]["w"]   = assets.image[type].w;
            currAsset["img"]["h"]   = assets.image[type].h;
            currAsset["id"] = r["id"].asInt();
            
            response.append( currAsset );
         }
         else  // нужна проверка на соответствие
         {
            if ( assets.image.find( type ) != assets.image.end() ) // проверка есть ли такой обьект в конфигах
            {
               if ( r["img"].isMember( "h" ) && r["img"].isMember( "w" ) ) // у нас есть четкие требуемые размеры
               {
                  int h = r["img"]["h"].asInt(),
                      w = r["img"]["w"].asInt();
                  if ( assets.image[type].w == w && assets.image[type].h == h )  // сравниваем с размерами из конфига
                  {
                     currAsset["img"]["url"] = assets.image[type].link;
                     currAsset["img"]["w"]   = assets.image[type].w;
                     currAsset["img"]["h"]   = assets.image[type].h;
                     currAsset["id"] = r["id"].asInt();
                     
                     response.append( currAsset );
                  }
               }
               else  // есть только минимальные размеры 
               {
                  int hmin = r["img"]["hmin"].asInt(),
                      wmin = r["img"]["wmin"].asInt();
                  if ( hmin <= assets.image[type].h && wmin <= assets.image[type].w )  // размеры конфига должны быть не меньше минимальных
                  {
                     float requestRatio = wmin / hmin,
                           configRatio  = assets.image[type].w / assets.image[type].h;
                     if ( ( 0.8 * requestRatio ) <= configRatio && ( 1.2 * requestRatio ) >= configRatio )  // проходит ли по соотношению сторон
                     {
                        currAsset["img"]["url"] = assets.image[type].link;
                        currAsset["img"]["w"]   = assets.image[type].w;
                        currAsset["img"]["h"]   = assets.image[type].h;
                        currAsset["id"] = r["id"].asInt();
                        
                        response.append( currAsset );
                     }
                  }
               }
            }
         }
         continue;
      }
   }

   return response;
}
