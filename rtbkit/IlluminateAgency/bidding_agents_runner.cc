/** bidding_agent_runner.cc                                 -*- C++ -*-
    Anisimov, 2 Feb 2015
    
    runner bidding agents.

*/

#include "rtbkit/common/bids.h"
#include "rtbkit/core/banker/slave_banker.h"
#include "rtbkit/core/agent_configuration/agent_config.h"
#include "rtbkit/plugins/bidding_agent/bidding_agent.h"
#include "soa/service/service_utils.h"

#include "illume_bidding_agent.h"
#include "illume_bidding_agent_cluster.h"
#include "agents_rest_manager.h"

#include <boost/program_options/cmdline.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include <iostream>
#include <thread>
#include <chrono>

using namespace std;
using namespace ML;

/******************************************************************************/
/* MAIN                                                                       */
/******************************************************************************/

int main(int argc, char** argv)
{
	using namespace boost::program_options;

	Datacratic::ServiceProxyArguments args;
	RTBKIT::SlaveBankerArguments bankerArgs;

	options_description options = args.makeProgramOptions();
	options.add_options()
		("help,h", "Print this message");
	options.add(bankerArgs.makeProgramOptions());

	variables_map vm;
	store(command_line_parser(argc, argv).options(options).run(), vm);
	notify(vm);

	if (vm.count("help")) {
		cerr << options << endl;
		return 1;
	}

	auto serviceProxies = args.makeServiceProxies();

	// increse socket 
	int socketCount = 4096;
	zmq_ctx_set(static_cast<void*>(*(serviceProxies->zmqContext)), ZMQ_MAX_SOCKETS, socketCount);

	/// setup rest api port
	int listenPort = 4888;
	std::string listenHost = "*";

	RTBKIT::AgentsRestManager agentsRestManager(serviceProxies, "bidding_agents_runner");
	agentsRestManager.init();

	auto add = agentsRestManager.bindTcp(listenPort, listenHost);
	agentsRestManager.start();

	while (true) this_thread::sleep_for(chrono::seconds(10));

	// Won't ever reach this point but this is how you shutdown an agent.
	return 0;
}
