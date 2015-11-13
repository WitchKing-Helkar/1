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

#include "bidding_strategy.h"

#include "soa/service/service_utils.h"



using namespace std;
using namespace ML;

namespace RTBKIT {

    struct IllumeBiddingAgent : public BiddingAgent
    {
		IllumeBiddingAgent(std::shared_ptr<Datacratic::ServiceProxies> services,
						   const string& serviceName);

        void init(/*const std::string &bankerUri*/); // x

        void start();

        void shutdown();

		void applyConfig();
		
		void setNewConfig(const Json::Value & jsonNewConfig);

        void bid(double timestamp,
                 const Id & id,
                 std::shared_ptr<RTBKIT::BidRequest> br,
                 Bids bids,
                 double timeLeftMs,
                 const Json::Value & augmentations,
                 WinCostModel const & wcm);

        void pace();

		void configModelParams(const Json::Value & jsonNewConfig);
		
		void clearModelParams();

        void setupCallbacks(); 

        void bidError(double timestamp, const std::string & error,
                      const std::vector<std::string> & message);

        void bidWin(const RTBKIT::BidResult & args);
		       		
		void formatNurlForBidswitch(const string &  auctionIdStr);

		void replace(std::string &s,
			   		 std::string toReplace,
					 std::string replaceWith);

        AgentConfig m_config; 
		bool m_accountSetup;   
		SlaveBudgetController m_budgetController;

	

	//private:
		
		//Amount m_maxCPI;
		//Amount m_averageCPI;
		//Amount m_amountZ;
		//Amount m_paceBudget;
		
		//Amount m_currentPrice;
		
		//Amount m_totalSpent;
		//Amount m_totalSpentPace;
		//Amount m_totalSpentPreviousPace;

		//int m_totalWinsOfPaceBlock;
		//bool m_stopBiddingForPace;
		
		//double m_perSecBidRequest;
		//double m_percSpndAllctn;
		

		//variable exponentioa ramping
		//double m_expnFct;
		//double m_expnFctPrevious;
		//double m_normExpnFct;
		//double m_expRampingCoefZ;
		//double m_expRampingCoefZPrevious;
		//double m_winRate;
		//double m_bidCapping;

		
		
		//int m_numBidRequestsPace;
		//int m_numBidRequestPrevious;
		//int m_numPace;
		//int m_numWinsPreviousPace;
		//int m_paceBlock;

		
		
        //int m_numBidRequests;
        int m_numErrors;
        //int m_numGotConfig;
        //int m_numWins;
		//int m_numWinsPace;
        int m_numBidsOutstanding;
                
        
        string m_nameAgent;
		
		BiddingExponentialStrategy	m_bes;
		BiddingLinearStrategy		m_bls;
		BiddingFixPriceStrategy		m_bfps;
		BiddingStretchStrategy		m_bss;
		AgentStrategy				m_agntStrg; 

		//string templateNurlStr;
		//bool m_templateNurlSetup;
				

    };

}

#endif
