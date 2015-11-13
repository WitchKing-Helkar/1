/** triplelift_agent_ex.cc                                 -*- C++ -*-
    Sviatoslav Borysiuk, 27 Aug 2015
    Copyright (c) 2013 Datacratic.  All rights reserved.

    Example of a simple fixed-price bidding agent that confgurated for triplelift exchange.

*/

#include "rtbkit/common/bids.h"
#include "rtbkit/core/banker/slave_banker.h"
#include "rtbkit/core/agent_configuration/agent_config.h"
#include "rtbkit/plugins/bidding_agent/bidding_agent.h"

#include "soa/service/service_utils.h"

#include <boost/program_options/cmdline.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include <iostream>
#include <thread>
#include <chrono>
#include <ctime>

using namespace std;
using namespace ML;

namespace RTBKIT {

/******************************************************************************/
/* TripleLift BIDDING AGENT                                                   */
/******************************************************************************/

/** Simple bidding agent for triplelift exhange
 */
struct TripleLiftBiddingAgent :
        public BiddingAgent
{
    std::string configFile;
    std::string logFile;
    
    AgentConfig config;
    bool accountSetup;
    SlaveBudgetController budgetController;
    
    TripleLiftBiddingAgent(
            std::shared_ptr<Datacratic::ServiceProxies> services,
            const string& serviceName) :
        BiddingAgent(services, serviceName),
        accountSetup(false)
    {
        configFile = "/home/ubuntu/rtbkit/rtbkit/examples/triplelift_agent_configuration.json";
        logFile    = "/home/ubuntu/rtbkit/logs/triplelift/rtbkit_log.txt";
    }


    void init(const std::shared_ptr<ApplicationLayer> appLayer)
    {
        // We only want to specify a subset of the callbacks so turn the
        // annoying safety belt off.
        strictMode(false);

        onBidRequest = bind(
                &TripleLiftBiddingAgent::bid, this, _1, _2, _3, _4, _5, _6);

        // This component is used to speak with the master banker and pace the
        // rate at which we spend our budget.
        budgetController.setApplicationLayer(appLayer);
        budgetController.start();

        // Update our pacer every 10 seconds. Note that since this interacts
        // with the budgetController which is only synced up with the router
        // every few seconds, the wait period shouldn't be set too low.
        addPeriodic("TripleLiftBiddingAgent::pace", 10.0,
                [&] (uint64_t) { this->pace(); });

        BiddingAgent::init();
    }

    void start()
    {
        BiddingAgent::start();
        // Build our configuration and tell the world about it.
        setConfig();
    }

    void shutdown()
    {
        BiddingAgent::shutdown();

        budgetController.shutdown();
    }


    /** Sets up an agent configuration for our example. */
    void setConfig()
    {
        config = AgentConfig();
        
        ML::filter_istream stream( configFile );
        std::string result;

        while (stream) 
        {
           std::string line;
           getline(stream, line);
           result += line + "\n";
        }
        config.fromJson( Json::parse( result ) );
        doConfig(config);
    }

    /** Simple fixed price bidding strategy. Note that the router is in charge
        of making sure we stay within budget and don't go bankrupt.
    */
    void bid(
            double timestamp,
            const Id & id,
            std::shared_ptr<RTBKIT::BidRequest> br,
            Bids bids,
            double timeLeftMs,
            const Json::Value & augmentations)
    {
        std::ofstream file( logFile, std::ios_base::app );
        file << "    THERE IS SOME BIDS COME TO AGENT( "
             << config.account.toString()
             << " ) : "
             << bids.size()
             << std::endl;
        for (Bid& bid : bids) {
            // In our example, all our creatives are of the different sizes so
            // there should only ever be one biddable creative. Note that that
            // the router won't ask for bids on imp that don't have any
            // biddable creatives.
            ExcAssertEqual(bid.availableCreatives.size(), 1);
            int availableCreative = bid.availableCreatives.front();

            // We don't really need it here but this is how you can get the
            // AdSpot and Creative object from the indexes.
            (void) br->imp[bid.spotIndex];
            (void) config.creatives[availableCreative];

            // Create a 0.0001$ CPM bid with our available creative.
            // Note that by default, the bid price is set to 0 which indicates
            // that we don't wish to bid on the given spot.

            srand( time( NULL ) );
            unsigned int price  = 0;
            if ( rand() / 2 ) { price = 500 + rand() % 200; } else { price = 500 - rand() % 200 ; }

            bid.bid( availableCreative, MicroUSD( price ) );
        }

        // A value that will be passed back to us when we receive the result of
        // our bid.
        Json::Value metadata = 42;

        file << "    SEND BID WITH PRICE : " << bids[0].price.toString() << std::endl;
        file << "------------------------------------------------------" << std::endl;
        file.close();
        // Send our bid back to the agent.
        doBid(id, bids, metadata);
    }


    /** Simple pacing scheme which allocates 1$ to spend every period. */
    void pace()
    {
        // We need to register our account once with the banker service.
        if (!accountSetup) {
            accountSetup = true;
            budgetController.addAccountSync(config.account);
        }

        // Make sure we have 1$ to spend for the next period.
        budgetController.topupTransferSync(config.account, USD(1));
    }
};

} // namepsace RTBKIT


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
    RTBKIT::TripleLiftBiddingAgent agent(serviceProxies, "triplelift-agent-ex");
    agent.init(bankerArgs.makeApplicationLayer(serviceProxies));
    agent.start();

    while (true) this_thread::sleep_for(chrono::seconds(10));

    // Won't ever reach this point but this is how you shutdown an agent.
    agent.shutdown();

    return 0;
}

