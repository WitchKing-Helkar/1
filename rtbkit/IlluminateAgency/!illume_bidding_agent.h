/*
	agent  -*- C++ -*-
	Anisimov, 2 Feb 2015

	This class implements logic for bidding game
*/


#ifndef ILLUME_BIDDING_AGENT_H
#define ILLUME_BIDDING_AGENT_H

#include "rtbkit/common/auction.h"
#include "rtbkit/common/bids.h"
#include "rtbkit/common/auction_events.h"
#include "rtbkit/common/win_cost_model.h"

#include "rtbkit/core/banker/slave_banker.h"
#include "rtbkit/core/agent_configuration/agent_config.h"
#include "rtbkit/plugins/bidding_agent/bidding_agent.h"

#include "soa/service/service_utils.h"



using namespace std;
using namespace ML;

namespace RTBKIT {

    struct IllumeBiddingAgent : public BiddingAgent
    {
		IllumeBiddingAgent(std::shared_ptr<Datacratic::ServiceProxies> services,
                         const string& serviceName);

        void init(/*const std::string &bankerUri*/);

        void start();

        void shutdown();

        void config();

        void bid(double timestamp,
                 const Id & id,
                 std::shared_ptr<RTBKIT::BidRequest> br,
                 Bids bids,
                 double timeLeftMs,
                 const Json::Value & augmentations,
                 WinCostModel const & wcm);

        void pace();

        void setupCallbacks();

        void clear();

        void bidError(double timestamp, const std::string & error,
                      const std::vector<std::string> & message);

        void bidWin(const RTBKIT::BidResult & args);

        void bidLoss(const RTBKIT::BidResult & args);

        void acountNoBudget(const RTBKIT::BidResult & args);

        void bidTooLate(const RTBKIT::BidResult & args);

        void visit(const DeliveryEvent & args);

        void click(const DeliveryEvent & args);

        void impression(const DeliveryEvent & args);

        AgentConfig mConfig;

        bool accountSetup;

        SlaveBudgetController budgetController;

        int numHeartbeats;
        int numBidRequests;
        int numErrors;
        int numGotConfig;
        int numWins;
        int numLosses;
        int numNoBudgets;
        int numTooLates;
        int numBidsOutstanding;
        int numImpression;
        int numClick;
        int numVisit;
        string nameAgent;

    };

}

#endif
