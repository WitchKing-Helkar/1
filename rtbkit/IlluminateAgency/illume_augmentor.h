
#pragma once

#include "rtbkit/core/agent_configuration/agent_configuration_listener.h"
#include "rtbkit/plugins/augmentor/augmentor_base.h"
#include "soa/service/zmq_named_pub_sub.h"
#include "soa/service/service_base.h"

#include <string>
#include <memory>

//#include <cassandra.h>

#include <aerospike/aerospike.h>
#include <aerospike/aerospike_key.h>
#include <aerospike/as_error.h>
#include <aerospike/as_record.h>
#include <aerospike/as_status.h>

namespace RTBKIT {

struct AgentConfigEntry;

struct RequestResult;

/******************************************************************************/
/* FREQUENCY CAP AUGMENTOR                                                    */
/******************************************************************************/

/** A Simple frequency cap augmentor which limits the number of times an ad can
    be shown to a specific user. It's multithreaded and connects to the
    following services:

    - The augmentation loop for its bid request stream.
    - The post auction loop for its win notification
    - The agent configuration listener to retrieve agent configuration for the
      augmentor.
    - FrequencyCapStorage for its simplistic data repository.

 */
struct IllumeFrequencyCapAugmentor :
    public RTBKIT::SyncAugmentor
{

	IllumeFrequencyCapAugmentor( std::shared_ptr<Datacratic::ServiceProxies> services,
								 const std::string& serviceName,
								 const std::string& augmentorName);
	void init();
	void shutdown();

private:

    virtual RTBKIT::AugmentationList
    onRequest(const RTBKIT::AugmentationRequest& request);
				
   	RTBKIT::AgentConfigurationListener agentConfig;
    Datacratic::ZmqNamedMultipleSubscriber palEvents;

	/* Setup and connect to cluster */
	//CassCluster* cluster;
	//CassSession* session;

	//CassError connect(const std::string nodes);
	//void closeConnect();
	//CassError printError(CassError error);
	
	//CassError queryGetExchangeID(const std::string & account, 
	//							 const std::string & exchangID,
	//							 RequestResult *rr);
	
	//CassError executeStatement(	const std::string & account,
	//							const std::string & exchangID,
	//							const CassResult** results);

	//CassError queryUpdateExchangeID(const std::string & account,
	//								const std::string & exchangID);

	//void clearBD();
	int  aerospikeKeyGetCount(const std::string & account,
							  const std::string & exchangID);
	void aerospikeKeyUpdateCount(const std::string & account,
								const std::string & exchangID);
	as_config aerospikeConfig;
	aerospike as;
	void aerospikeConnetct();
	void aerospikeCloseConnect();
};


} // namespace RTBKIT
