#pragma once

#include "soa/types/value_description.h"
#include "soa/types/basic_value_descriptions.h"
#include "soa/types/json_parsing.h"
#include "triplelift.h"
#include <boost/lexical_cast.hpp>

namespace Datacratic {

    template<>
    struct DefaultDescription<TripleLift::Native>
        : public StructureDescription<TripleLift::Native> {
        DefaultDescription();
    };

    template<>
    struct DefaultDescription<TripleLift::NativeRequest>
        : public StructureDescription<TripleLift::NativeRequest> {
        DefaultDescription();
    };

    template<>
    struct DefaultDescription<TripleLift::NativeAsset>
        : public StructureDescription<TripleLift::NativeAsset> {
        DefaultDescription();
    };

    template<>
    struct DefaultDescription<TripleLift::NativeAssetImage>
        : public StructureDescription<TripleLift::NativeAssetImage> {
        DefaultDescription();
    };

    template<>
    struct DefaultDescription<TripleLift::NativeAssetTitle>
        : public StructureDescription<TripleLift::NativeAssetTitle> {
        DefaultDescription();
    };

    template<>
    struct DefaultDescription<TripleLift::NativeAssetData>
        : public StructureDescription<TripleLift::NativeAssetData> {
        DefaultDescription();
    };

    template<>
    struct DefaultDescription<TripleLift::BidRequest>
        : public StructureDescription<TripleLift::BidRequest> {
        DefaultDescription();
    };

    template<>
    struct DefaultDescription<TripleLift::Impression>
        : public StructureDescription<TripleLift::Impression> {
        DefaultDescription();
    };

    template<>
    struct DefaultDescription<TripleLift::Banner>
        : public StructureDescription<TripleLift::Banner> {
        DefaultDescription();
    };

    template<>
    struct DefaultDescription<TripleLift::Ext>
        : public StructureDescription<TripleLift::Ext> {
        DefaultDescription();
    };

    template<>
    struct DefaultDescription<TripleLift::Site>
        : public StructureDescription<TripleLift::Site> {
        DefaultDescription();
    };

    template<>
    struct DefaultDescription<TripleLift::App>
        : public StructureDescription<TripleLift::App> {
        DefaultDescription();
    };

    template<>
    struct DefaultDescription<TripleLift::Publisher>
        : public StructureDescription<TripleLift::Publisher> {
        DefaultDescription();
    };

    template<>
    struct DefaultDescription<TripleLift::Device>
        : public StructureDescription<TripleLift::Device> {
        DefaultDescription();
    };

    template<>
    struct DefaultDescription<TripleLift::Geo>
        : public StructureDescription<TripleLift::Geo> {
        DefaultDescription();
    };

    template<>
    struct DefaultDescription<TripleLift::User>
        : public StructureDescription<TripleLift::User> {
        DefaultDescription();
    };

    template<>
    struct DefaultDescription<TripleLift::TripleLiftNative>
        : public StructureDescription<TripleLift::TripleLiftNative> {
        DefaultDescription();
    };

    template<>
    struct DefaultDescription<TripleLift::BidResponse_2point3>
        : public StructureDescription<TripleLift::BidResponse_2point3> {
        DefaultDescription();
    };

    template<>
    struct DefaultDescription<TripleLift::ResponseSeatBid_2point3>
        : public StructureDescription<TripleLift::ResponseSeatBid_2point3> {
        DefaultDescription();
    };

    template<>
    struct DefaultDescription<TripleLift::ResponseBid_2point3>
        : public StructureDescription<TripleLift::ResponseBid_2point3> {
        DefaultDescription();
    };

    template<>
    struct DefaultDescription<TripleLift::ResponseNative>
        : public StructureDescription<TripleLift::ResponseNative> {
        DefaultDescription();
    };

    template<>
    struct DefaultDescription<TripleLift::ResponseAsset>
        : public StructureDescription<TripleLift::ResponseAsset> {
        DefaultDescription();
    };

    template<>
    struct DefaultDescription<TripleLift::ResponseNativeImage>
        : public StructureDescription<TripleLift::ResponseNativeImage> {
        DefaultDescription();
    };

    template<>
    struct DefaultDescription<TripleLift::ResponseNativeData>
        : public StructureDescription<TripleLift::ResponseNativeData> {
        DefaultDescription();
    };

    template<>
    struct DefaultDescription<TripleLift::ResponseNativeTitle>
        : public StructureDescription<TripleLift::ResponseNativeTitle> {
        DefaultDescription();
    };

    template<>
    struct DefaultDescription<TripleLift::ResponseNativeVideo>
        : public StructureDescription<TripleLift::ResponseNativeVideo> {
        DefaultDescription();
    };


    template<>
    struct DefaultDescription<TripleLift::ResponseLink>
        : public StructureDescription<TripleLift::ResponseLink> {
        DefaultDescription();
    };

    template<>
    struct DefaultDescription<TripleLift::ResponseExt>
        : public StructureDescription<TripleLift::ResponseExt> {
        DefaultDescription();
    };

    template<>
    struct DefaultDescription<TripleLift::ResponseAdm>
        : public StructureDescription<TripleLift::ResponseAdm> {
        DefaultDescription();
    };

} // namespace Datacratic
