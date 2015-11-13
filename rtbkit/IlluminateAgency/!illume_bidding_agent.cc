#include "illume_bidding_agent.h"

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

	IllumeBiddingAgent::IllumeBiddingAgent(std::shared_ptr<Datacratic::ServiceProxies> services,
									   const string& serviceName):
												BiddingAgent(services, serviceName),
												accountSetup(false),
												nameAgent(serviceName)
	{
		//cerr << nameAgent << ": ===> constructor " << endl;
		setupCallbacks();
		clear();
	}

	void IllumeBiddingAgent::init(/*const std::string &bankerUri*/)
	{
		// We only want to specify a subset of the callbacks so turn the
		// annoying safety belt off.
		strictMode(false);

		// This component is used to speak with the master banker and pace the
		// rate at which we spend our budget.
		budgetController.setApplicationLayer(make_application_layer<ZmqLayer>(getServices()));
		budgetController.start();

		// Update our pacer every 10 seconds. Note that since this interacts
		// with the budgetController which is only synced up with the router
		// every few seconds, the wait period shouldn't be set too low.
		addPeriodic("IllumeBiddingAgent::pace",
					10.0,
					[&] (uint64_t) { this->pace(); });

		BiddingAgent::init();
	}

	void IllumeBiddingAgent::start()
	{
		//cerr << nameAgent << ": ===> start " << endl;
		BiddingAgent::start();

		// Build our configuration and tell the world about it.
		config();
	}

	void IllumeBiddingAgent::shutdown()
	{
		//cerr << nameAgent << ": ===> shutdown " << endl;

		BiddingAgent::shutdown();

		budgetController.shutdown();
	}

	
	void IllumeBiddingAgent::config()
	{
		//cerr << nameAgent << ": ===> config " << endl;
		// Tell the world about our config. We can change the configuration of
		// an agent at any time by calling this function.
		doConfig(mConfig);
	}

	void IllumeBiddingAgent::bid(double timestamp,
							     const Id & id,
							     std::shared_ptr<RTBKIT::BidRequest> br,
							     Bids bids,
							     double timeLeftMs,
							     const Json::Value & augmentations,
							     WinCostModel const & wcm)
	{
		__sync_fetch_and_add(&numBidRequests, 1);

		for (Bid& bid : bids)
		{

			// In our example, all our creatives are of the different sizes so
			// there should only ever be one biddable creative. Note that that
			// the router won't ask for bids on imp that don't have any
			// biddable creatives.
			ExcAssertEqual(bid.availableCreatives.size(), 1);
			int availableCreative = bid.availableCreatives.front();

			// We don't really need it here but this is how you can get the
			// AdSpot and Creative object from the indexes.
			(void) br->imp[bid.spotIndex];
			(void) mConfig.creatives[availableCreative];

			// Create a 0.0001$  bid with our available creative.
			// Note that by default, the bid price is set to 0 which indicates
			// that we don't wish to bid on the given spot.
			bid.bid(availableCreative, MicroUSD(100));
			//cerr << nameAgent << ": bid.bid " << endl;
		}

		// A value that will be passed back to us when we receive the result of
		// our bid.
		Json::Value metadata = 42;

		//cerr<< nameAgent << ": id " << id << endl;

		// Send our bid back to the agent.
		doBid(id, bids, metadata, wcm);
	}

	void IllumeBiddingAgent::pace()
	{
		cerr << "[ REPORT: " << nameAgent                << "] \n";
		cerr << "| agent iab-categories : " << mConfig.segments["iab-categories"].toJson()["include"];
		cerr << "| agent got bidRequest : " << numBidRequests << "\n";
		cerr << "| agent got wins       : " << numWins   << "\n";
		cerr << "| agent got error      : " << numErrors << "\n";
		cerr << "| agent got bidLoss    : " << numLosses    << "\n";
		cerr << "| agent got noBudgets  : " << numNoBudgets << "\n";
		cerr << "| agent got bidToolate : " << numTooLates << "\n";
		cerr << "| agent got visits     : " << numVisit << "\n";
		cerr << "| agent got clicks     : " << numClick << "\n";
		cerr << "| agent go impressions : " << numImpression << "\n";

		//cerr << nameAgent << ": pace " << endl;
		//We need to register our account once with the banker service.
		if (!accountSetup) {
			accountSetup = true;
			budgetController.addAccountSync(mConfig.account);
		}
		cerr << "[ mConfig.account: " << mConfig.account               << "] \n";
		// Make sure we have 1$ to spend for the next period.
		budgetController.topupTransferSync(mConfig.account, USD(1));
	}

	void IllumeBiddingAgent::setupCallbacks()
	{
		//cerr << nameAgent << ": ===> setupCallbacks " << endl;

		onBidRequest
			= boost::bind(&IllumeBiddingAgent::bid, this, _1, _2, _3, _4, _5, _6, _7);

		onError
			= boost::bind(&IllumeBiddingAgent::bidError, this, _1, _2, _3);

		onWin
			= boost::bind(&IllumeBiddingAgent::bidWin, this, _1);

		onLoss
			= boost::bind(&IllumeBiddingAgent::bidLoss, this, _1);

		onNoBudget
			= boost::bind(&IllumeBiddingAgent::acountNoBudget, this, _1);

		onTooLate
			= boost::bind(&IllumeBiddingAgent::bidTooLate, this, _1);

		onVisit
			= boost::bind(&IllumeBiddingAgent::visit, this, _1);

		onImpression
			= boost::bind(&IllumeBiddingAgent::impression, this, _1);

		//onClick
		//    = boost::bind(&IllumeBiddingAgent::click, this, _1);
	}

	void IllumeBiddingAgent::clear()
	{
		numHeartbeats = numBidRequests = numErrors = numGotConfig = 0;
		numWins = numLosses = numNoBudgets = numTooLates = 0;
		numBidsOutstanding = 0;
	}

	void IllumeBiddingAgent::bidError(double timestamp,
									  const std::string & error,
									  const std::vector<std::string> & message)
	{
		cerr << nameAgent <<" got error: " << error << " from message: "
			 << message << endl;
		__sync_fetch_and_add(&numErrors, 1);
	}

	void IllumeBiddingAgent::bidWin(const RTBKIT::BidResult & args)
	{
		//cerr << nameAgent << ": agent got win " << endl;
		__sync_fetch_and_add(&numWins, 1);
	}

	void IllumeBiddingAgent::bidLoss(const RTBKIT::BidResult & args)
	{
		//cerr << nameAgent << ": ===> bidLoss " << endl;
		__sync_fetch_and_add(&numLosses, 1);
	}

	void IllumeBiddingAgent::acountNoBudget(const RTBKIT::BidResult & args)
	{
		//cerr << nameAgent << ": ===> noBudget " << endl;
		__sync_fetch_and_add(&numNoBudgets, 1);
	}

	void IllumeBiddingAgent::bidTooLate(const RTBKIT::BidResult & args)
	{
		//cerr << nameAgent << ": ===> bidToolate " << endl;
		__sync_fetch_and_add(&numTooLates, 1);
	}

	void IllumeBiddingAgent::visit(const DeliveryEvent & args)
	{
		//cerr << nameAgent << ": ===> visit " << endl;
		__sync_fetch_and_add(&numVisit, 1);
	}

	void IllumeBiddingAgent::click(const DeliveryEvent & args)
	{
		//cerr << nameAgent << ": ===> click " << endl;
		__sync_fetch_and_add(&numClick, 1);
	}
	void IllumeBiddingAgent::impression(const DeliveryEvent & args)
	{
		//cerr << nameAgent << ": ===> impression " << endl;
		__sync_fetch_and_add(&numImpression, 1);
	}

}