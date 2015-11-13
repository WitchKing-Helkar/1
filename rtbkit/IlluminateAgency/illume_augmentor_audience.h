/** augmentor_ex.h                                 -*- C++ -*-
    Rémi Attab, 22 Feb 2013
    Copyright (c) 2013 Datacratic.  All rights reserved.

    Interface of our Augmentor example for an extremely simple frequency cap
    service.

    Note that this header exists mainly so that it can be integrated into the
    rtbkit_integration_test. Most of the documentation for it is in the cc file.

*/

#pragma once

#include "rtbkit/core/agent_configuration/agent_configuration_listener.h"
#include "rtbkit/plugins/augmentor/augmentor_base.h"
#include "soa/service/zmq_named_pub_sub.h"
#include "soa/service/service_base.h"
#include "soa/jsoncpp/json.h"

#include <string>
#include <memory>
#include <string>

//#include <cassandra.h>

#include <aerospike/aerospike.h>
#include <aerospike/aerospike_key.h>
#include <aerospike/as_error.h>
#include <aerospike/as_record.h>
#include <aerospike/as_status.h>

namespace RTBKIT {

struct AgentConfigEntry;

struct RequestResult;

struct IllumeAugmentorAudience :
    public RTBKIT::SyncAugmentor
{

	IllumeAugmentorAudience(
            std::shared_ptr<Datacratic::ServiceProxies> services,
            const std::string& serviceName,
            const std::string& augmentorName);

    void init();
	void shutdown();

private:
		
    virtual RTBKIT::AugmentationList
    
	onRequest(const RTBKIT::AugmentationRequest& request);
		
	RTBKIT::AgentConfigurationListener agentConfig;
		
		
	/* Setup and connect to cluster */
	//CassCluster* cluster;
	//CassSession* session;

	//CassError connect(const std::string nodes);
	//void closeConnect();
	//CassError printError(CassError error);

	//CassError queryGetExchangeID(const std::string & ifa,
	//							 RequestResult *rr);

	//CassError executeStatement(	const std::string & ifa,
	//							const CassResult** results);
	as_config aerospikeConfig;
	aerospike as;
	void aerospikeConnetct();
	void aerospikeCloseConnect();

	std::string  aerospikeKeyGetAugid(const std::string & strIFA);
	   
};


} // namespace RTBKIT
