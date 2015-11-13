/*
    rest_manager_agents.h -*- C++ -*-
    Anisimov, 2 Feb 2015
    This class implements cluster management agents through REST API
*/

#ifndef REST_MANAGER_AGENTS_H
#define REST_MANAGER_AGENTS_H

#include <boost/program_options/cmdline.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/thread/thread.hpp>
#include <boost/make_shared.hpp>
#include <string>

#include "soa/types/value_description.h"
#include "soa/types/basic_value_descriptions.h"
#include "soa/service/rest_service_endpoint.h"
#include "soa/service/rest_request_router.h"
#include "soa/service/zmq_named_pub_sub.h"
#include "soa/service/s3.h"
#include "soa/service/sqs.h"
#include "soa/service/rest_request_binding.h"
#include "soa/service/service_utils.h"

//#include "soa/service/service_base.h"
#include "illume_bidding_agent_cluster.h"

#include "jml/utils/pair_utils.h"
#include "jml/arch/timers.h"
#include "jml/arch/futex.h"

#include "soa/service/s3.h"
#include "soa/jsoncpp/json.h"

using namespace std;
using namespace ML;

namespace Datacratic
{

	using Datacratic::jsonDecode;
	using Datacratic::jsonEncode;

	using namespace Datacratic;

	struct AgentsRestManager
		: public ServiceBase,
		  public RestServiceEndpoint
	{
		AgentsRestManager(std::shared_ptr<ServiceProxies> proxies,
						   const std::string & serviceName = "AgentsRestManager");

		virtual ~AgentsRestManager();

		void init();

		void start();

		void stop();

		void shutdown();

		std::string bindTcp(const PortRange & portRange = PortRange(),
							const std::string & host = "localhost");

		Json::Value handleAgentCreate(const std::string & agentName,
									  const Json::Value & config);

		Json::Value handleAgentDelete(const std::string & agentName);

		//Json::Value handleAgentStop(const std::string & agentName);

		Json::Value handleAgentReconfigure(const std::string & agentName,
										   const Json::Value & jsonAgentNewConfig);
	
		bool handleAgentStatus(const std::string & agentName);

		void agentStatus();

		//bool clusterStart(const int & a_value);

		//bool clusterStop(const int & a_value);

		//bool addAgentToCluster( , std::string agentName);

		bool deleteAgentToCluster(std::string agentName);

		RestRequestRouter router;

		RTBKIT::IllumeBiddingAgentCluster agentsCluster;

		std::shared_ptr<ServiceProxies> spProxies;
	};
}

#endif /* rest_manager_agents_h_ */
