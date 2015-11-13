#include "agents_rest_manager.h"

#include <memory>
#include <string>

#include "soa/jsoncpp/value.h"
#include "soa/types/value_description.h"
#include "soa/types/basic_value_descriptions.h"
#include "soa/service/rest_service_endpoint.h"
#include "soa/service/rest_request_router.h"
#include "soa/service/zmq_named_pub_sub.h"
#include "soa/service/s3.h"
#include "soa/service/sqs.h"
#include "soa/service/rest_request_binding.h"
#include "soa/service/service_utils.h"

#include "illume_bidding_agent.h"
//#include "rtbkit/testing/test_agent.h"

#include <boost/algorithm/string.hpp>
#include <jml/arch/futex.h>


namespace Datacratic
{
using Datacratic::jsonDecode;
using Datacratic::jsonEncode;
using namespace Datacratic;

AgentsRestManager::
AgentsRestManager(std::shared_ptr<ServiceProxies> proxies,
                  const std::string & serviceName)
					: ServiceBase(serviceName, proxies),
                      RestServiceEndpoint(getZmqContext()),
                      agentsCluster(proxies, serviceName + "Cluster"),
                      spProxies(proxies)
{
}

AgentsRestManager::
~AgentsRestManager()
{
}

void 
AgentsRestManager::
init()
{
    registerServiceProvider(serviceName(), { "AgentsRestManager" });
    RestServiceEndpoint::init(getServices()->config, serviceName());
    router.description = "REST API Manager Bidding Agents RTBKIT";
    onHandleRequest = router.requestHandler();

    // defines the help route
    // The return value of the help route is automatically built using the information from the other routes of the system
    //
    router.addHelpRoute("/", "GET");

    auto & versionNode = router.addSubRouter("/v1", 
											 "version 1 of API");

    auto & agentsNode = versionNode.addSubRouter("/agents",
                                                 "Operations on agents");

    auto & agent = agentsNode.addSubRouter(Rx("/([^/]*)", 
		                                   "/<agentName>"),
                                           "operations on an individual agent");

    // Illustrates using the Router directly instead of the helper functions to add a route
    //
    RestRequestRouter::OnProcessRequest pingRoute
        = [] (const RestServiceEndpoint::ConnectionId & connection,
              const RestRequest & request,
              const RestRequestParsingContext & context)
    {
        connection.sendResponse(200, "1");
        return RestRequestRouter::MR_YES;
    };

    router.addRoute("/ping", 
		            "GET", "Ping the availability of the endpoint",
                    pingRoute,
                    Json::Value());
	
    // using sub routes
    //auto & agentStartNode = router.addSubRouter("/agent/start", "start agent");
	//auto & agentStopNode  = router.addSubRouter("/agent/stop", "stop agent");

    RequestParam<std::string> agentKeyParam(-2, "<agentName>", "agent to operate on");

    addRouteSyncReturn(	agent,
						"/create",
						{"POST"},
						"create and start agent in cluster",
						"return value JSON",
						[] (Json::Value v) { return v;},
						&AgentsRestManager::handleAgentCreate,
						this,
						agentKeyParam,       //RestParam<std::string>("value", "a_value start agent"),
						JsonParam<Json::Value>("", "Configuration block for agent"));

	addRouteSyncReturn(	agent,
						"/delete",
						{"POST"},
						"removal in cluster",
						"return value",
						[] (Json::Value v) { return v;},
						&AgentsRestManager::handleAgentDelete,
						this,
						agentKeyParam);

	addRouteSyncReturn(	agent,
						"/reconfigure",
						{ "POST","PUT" },
						"reconfigure agents in cluster",
						"return value",
						[](Json::Value v) { return v; },
						&AgentsRestManager::handleAgentReconfigure,
						this,
						agentKeyParam,
						JsonParam<Json::Value>("", "Configuration block for agent"));

    addRouteSyncReturn(	agent,
						"/status",
						{"GET"},
						"status agent",
						"return value",
                        [] (const bool v) { return jsonEncode(v); },
						&AgentsRestManager::handleAgentStatus,
						this,
						agentKeyParam);

	/*addRouteSyncReturn(	agent,
						"/agents",
						{"GET"},
						"name agents",
						"return value",
						[](Json::Value v) { return v; },
						&AgentsRestManager::handlGetNameAgents,
						this,
						agentKeyParam);*/
}

void 
AgentsRestManager::
start()
{
    agentsCluster.start();
    httpEndpoint.spinup(8,false);
}

void 
AgentsRestManager::
stop()
{

}

void 
AgentsRestManager::
shutdown()
{

}

std::string 
AgentsRestManager::
bindTcp( const PortRange & portRange,
		 const std::string & host)
{
    return httpEndpoint.bindTcp(portRange, host);
}

/*
bool CAgentsRestManager::clusterStop(const int & a_value)
{
    std::cerr <<  "cluster stop :" << "a_value= " << (a_value > 0)  << std::endl;
    return true;
}
*/

Json::Value
AgentsRestManager::
handleAgentCreate(const std::string & agentName,
				  const Json::Value & jsonAgentConfig)
{
    Json::Value result;
	 
	//std::cerr << "agent start name: " << agentName << std::endl;
	//std::cerr << "agent JSON: " << jsonAgentConfig << std::endl;;

	if (jsonAgentConfig.empty())
	{
		result["result"] = "json for agent configurate is empty";
		return result;
	}

    if(handleAgentStatus(agentName))
    {
        result["result"] = "agent exist";
        return result;
    }

	AccountKey account = AccountKey::fromJson(jsonAgentConfig["AgentConfig"]["account"]);

	if (account.size() < 2){
		result["the_result"] = "Error account size < 2 words!";
		return result;
	}

	auto spAgent = make_shared<RTBKIT::IllumeBiddingAgent>(spProxies, agentName);
	
	spAgent->setNewConfig(jsonAgentConfig);
	
    if( agentsCluster.join(spAgent, agentName))
    {
        agentsCluster[agentName]->init();
				
		agentsCluster[agentName]->applyConfig();
		
        result["result"] = "agent join to cluster successfully";
    }
    else
    {
        result["result"] = "agent not add to cluster";
        std::cerr << "agent not add to cluster" << std::endl;
    }

    return result;

}

Json::Value
AgentsRestManager::
handleAgentDelete(const std::string & agentName)
{

    Json::Value result;

    if(!handleAgentStatus(agentName))
    {
        result["result"] = "agent not exist";
        return result;
    }

    std::cerr << "agent stop:" << agentName << std::endl;

    if(!deleteAgentToCluster(agentName))
    {
        result["result"] = "agent not leave to cluster";
        std::cerr << "agent not delete to cluster" << std::endl;
    }
    else
        result["result"] = "agent leave to cluster successfully";

    return result;
}

Json::Value
AgentsRestManager::
handleAgentReconfigure(const std::string & agentName,
					   const Json::Value & jsonAgentNewConfig)
{
	Json::Value result;
	
	if (jsonAgentNewConfig.empty())	{
		result["result"] = "json for agent configurate is empty";
		return result;
	}

	if (!handleAgentStatus(agentName)) {
		result["result"] = "agent not exist in cluster";
		return result;
	}

	AccountKey account = AccountKey::fromJson(jsonAgentNewConfig["AgentConfig"]["account"]);

	if (account.size() < 2) {
		result["the_result"] = "Error account size < 2 words!";
		return result;
	}

	agentsCluster[agentName]->clearModelParams();
	agentsCluster[agentName]->setNewConfig(jsonAgentNewConfig);
	agentsCluster[agentName]->applyConfig();
		
	result["result"] = "agent reconfigurate";
	
	return result;
}

bool
AgentsRestManager::
handleAgentStatus(const std::string & agentName)
{
    return agentsCluster.is_member(agentName);
}

//Json::Value
//AgentsRestManager::
//handlGetNameAgents()
//{
//	Json::Value result;
//	agentsCluster.get_names();
//	return result;
//}

/*
bool CAgentsRestManager::addAgentToCluster(std::string agentName, )
{
    return agentsCluster.join (spAgent, agentName);
}
*/

bool 
AgentsRestManager::
deleteAgentToCluster(std::string agentName)
{
    return agentsCluster.leave(agentName);
}

}


