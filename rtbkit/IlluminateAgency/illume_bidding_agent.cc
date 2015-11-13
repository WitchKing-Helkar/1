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

	IllumeBiddingAgent::
		IllumeBiddingAgent(std::shared_ptr<Datacratic::ServiceProxies> services,
		const string& serviceName) :
		BiddingAgent(services, serviceName),
		m_accountSetup(false),
		m_nameAgent(serviceName)
		//m_templateNurlSetup(false)
	{
		setupCallbacks();
		clearModelParams();
	}

	void
	IllumeBiddingAgent::
	init(/*const std::string &bankerUri*/)
	{
		// We only want to specify a subset of the callbacks so turn the
		// annoying safety belt off.
		strictMode(false);

		// This component is used to speak with the master banker and pace the
		// rate at which we spend our budget.
		m_budgetController.setApplicationLayer(make_application_layer<ZmqLayer>(getServices()));
		m_budgetController.start();

		// Update our pacer every 10 seconds. Note that since this interacts
		// with the budgetController which is only synced up with the router
		// every few seconds, the wait period shouldn't be set too low.
		addPeriodic("IllumeBiddingAgent::pace",
			10, //m_paceBlock = 10sec
			[&](uint64_t) { this->pace(); });

		BiddingAgent::init();
	}

	void
	IllumeBiddingAgent::
	start()
	{
		BiddingAgent::start();
		//applyConfig();
	}

	void
	IllumeBiddingAgent::
	shutdown()
	{

		BiddingAgent::shutdown();
		m_budgetController.shutdown();
	}


	void
	IllumeBiddingAgent::
	setNewConfig(const Json::Value & jsonNewConfig)
	{
		cout << "setNewConfig: " << endl;
		//for AgentConfig
		m_config.fromJson(jsonNewConfig["AgentConfig"]);

		cout << "setNewConfig:m_config " << m_config.toJson() << endl;

		//for config model 
		configModelParams(jsonNewConfig["AgentConfigEx"]);

		// Tell the world about our config. We can change the configuration of
		// an agent at any time by calling this function.
		//doConfig(m_config);
	}

	void
	IllumeBiddingAgent::
	applyConfig()
	{

		// Tell the world about our config. We can change the configuration of
		// an agent at any time by calling this function.
		doConfig(m_config);

	}

	void
	IllumeBiddingAgent::
	bid(double timestamp,
		const Id & id,
		std::shared_ptr<RTBKIT::BidRequest> br,
		Bids bids,
		double timeLeftMs,
		const Json::Value & augmentations,
		WinCostModel const & wcm)
	{
		//cout << "bid reqeust: " << br->toJson() << endl;

		m_agntStrg.incrementCountBidRequestPace();

		Amount price = m_agntStrg.useStrategyBidding();
		//Amount price = MicroUSD(100);

		cout << m_nameAgent 
			<< " " 
			<< m_agntStrg.getNameSrtategy() 
			<< ": bid price: " 
			<< price << endl;

		//m_agntStrg.print();
		
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
			(void)br->imp[bid.spotIndex];
			(void)m_config.creatives[availableCreative];

			// Create a 0.0001$  bid with our available creative.
			// Note that by default, the bid price is set to 0 which indicates
			// that we don't wish to bid on the given spot.

			bid.bid(availableCreative, MicroUSD(price));
			//bid.bid(availableCreative, m_currentPrice);
			//cout << "Test:" << bid.bid  << endl;
		}

		// A value that will be passed back to us when we receive the result of
		// our bid.
		Json::Value metadata = 42;

		//cerr<< nameAgent << ": id " << id << endl;
		// Send our bid back to the agent.
		doBid(id, bids, metadata, wcm);
	}



	void
	IllumeBiddingAgent::
	pace()
	{
		cout << "pace result:" << endl;
		cout << m_nameAgent
			<< " "
			<< m_agntStrg.getNameSrtategy()
			<< endl;

		// We need to register our account once with the banker service.
		if (!m_accountSetup) {
			m_accountSetup = true;
			m_budgetController.addAccountSync(m_config.account);
		}

		// Make sure we have 1$ to spend for the next period.
		//m_budgetController.topupTransferSync(m_config.account, USD(1));
		m_budgetController.topupTransferSync(m_config.account, m_agntStrg.getPaceBudget());


		//cout << "Total spent: " << m_totalSpentPace << endl;
		//m_totalSpentPace = MicroUSD(0);
		// 
		m_agntStrg.calculateResultPace();
		//m_agntStrg.print();
	}


	void
	IllumeBiddingAgent::
	setupCallbacks()
	{
		cout << "setupCallbacks:" << endl;
		//cerr << nameAgent << ": ===> setupCallbacks " << endl;

		onBidRequest
			= boost::bind(&IllumeBiddingAgent::bid, this, _1, _2, _3, _4, _5, _6, _7);

		onError
			= boost::bind(&IllumeBiddingAgent::bidError, this, _1, _2, _3);

		onWin
			= boost::bind(&IllumeBiddingAgent::bidWin, this, _1);

	}

	void
	IllumeBiddingAgent::
	clearModelParams()
	{
		cout << "clearModelParams:" << endl;
		/*m_numBidRequests = m_numBidRequestsPace = m_numBidRequestPrevious = m_numErrors = m_numGotConfig = 0;*/
		/*m_numWins = m_numWinsPace = m_numWinsPreviousPace = 0;*/
		/*m_expRampingCoefZ = m_expRampingCoefZPrevious = 0;*/

		/*m_totalSpent = m_totalSpentPace = m_totalSpentPreviousPace = MicroUSD(0);*/
		//numBidsOutstanding = 0;
		//m_numPace = 0;

		/*m_normExpnFct = m_expnFct = m_expnFctPrevious = 0;*/

		//m_amountZ = MicroUSD(0.0);
		//cout << "m_amountZ: " << m_amountZ << endl;
		//m_currentPrice = MicroUSD(0.0);
		//m_totalSpent = MicroUSD(0.0);

		//m_stopBiddingForPace = false;
	}

	void
	IllumeBiddingAgent::
	configModelParams(const Json::Value & jsonNewConfig)
	{
		cout << "configModelParams:" << endl;

		string modelRamping;
		int budget = 0;
		int maxCPI = 0;
		int maxPaceBudget = 0;

		for (auto it = jsonNewConfig.begin(), end = jsonNewConfig.end(); it != end; ++it) {
			
			if (it.memberName() == "ModelRamping"){
				modelRamping = it->asString();				
			}
			else if (it.memberName() == "Budget") {
				budget = it->asInt();
			}
			else if (it.memberName() == "MaxCPI") {
				maxCPI = it->asInt();
			}
			else if (it.memberName() == "MaxPacesBudget") {
				maxPaceBudget = it->asInt();
			}

		}

		// init agent stategy obj

		if (modelRamping == "linear")
			m_agntStrg.setStrategyBidding(&m_bls);
		if (modelRamping == "exponential")
			m_agntStrg.setStrategyBidding(&m_bes);
		if (modelRamping == "fixed")
			m_agntStrg.setStrategyBidding(&m_bfps);
		if (modelRamping == "stretch")
			m_agntStrg.setStrategyBidding(&m_bss);
		

		if (budget > 0 && maxPaceBudget > 0) {
			//double result = (budget * 1000000) / (24/*houre*/ * 60 /*min*/ * 60 /*sec*/);
			//int paceBudget = (int)result * 10; //pace 10 sec
			m_agntStrg.setPaceBudget(maxPaceBudget);
			m_agntStrg.setBudget(budget);					
		}
		else
		{
			m_agntStrg.setPaceBudget(0);
			m_agntStrg.setBudget(0);
		}
			

		if (maxCPI > 0)
			m_agntStrg.setMaxCPI(maxCPI);
		//m_maxCPI = MicroUSD(iMaxCPI);
		else
			m_agntStrg.setMaxCPI(0);
		//m_maxCPI = MicroUSD(0.0);		

		//if (paceInDay > 0)
		//	m_agntStrg.setPaceInDay(paceInDay);


		m_agntStrg.init();
		m_agntStrg.print();
	}

	void
	IllumeBiddingAgent::
	bidError(double timestamp,
	const std::string & error,
	const std::vector<std::string> & message)
	{
		cout << m_nameAgent << " got error: " << error << " from message: " << message << endl;
		__sync_fetch_and_add(&m_numErrors, 1);
	}

	void
	IllumeBiddingAgent::
	bidWin(const RTBKIT::BidResult & args)
	{
		cout << m_nameAgent
			<< m_agntStrg.getNameSrtategy()
			<<": agent got win: "
			<< args.secondPrice << endl;
		//m_totalSpentPace = m_totalSpentPace + args.secondPrice;
		m_agntStrg.incrementNumWins();
		// tatal spent for pace:
		m_agntStrg.setSpentForPace(args.secondPrice);
	}
}



