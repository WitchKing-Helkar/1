#pragma once
#include <iostream>
#include "rtbkit/common/currency.h"
//class BiddingStrategy
//{
//public:
//	BiddingStrategy();
//	~BiddingStrategy();
//};

//typedef int Amount;

using namespace std;
using namespace ML;

namespace RTBKIT {

	struct BiddingStrategy
	{

		Amount m_maxCPI;
		Amount m_currentAmount;
		Amount m_paceBudget;
		Amount m_budget;
		Amount m_totalSpentAmount;
		Amount m_totalSpentPace;

		int m_countBidRequestForPace;
		double m_percSpndAllctn;
		bool m_stopBiddingForPace;
		int m_numWins;
		
		
		BiddingStrategy(void);
		~BiddingStrategy(void);

		virtual Amount useRampingBid(void) = 0;
		virtual void init(void) = 0;
		virtual void calculatePaceResult(void) = 0;
		virtual void printStatObject(void) = 0;
		virtual std::string  strategyNameString(void) =0;

		void incrementCountRequestForPace(void);
		void incrementNumWins(void);
		void setTotalSpentPace(Amount _price);
		Amount getPaceBudget();
	};

	struct BiddingFixPriceStrategy : public BiddingStrategy
	{
		BiddingFixPriceStrategy();
		~BiddingFixPriceStrategy();

		void calculatePaceResult(void);
		void init(void);
		void printStatObject(void);
		virtual std::string  strategyNameString(void);
		Amount useRampingBid();
	};



	struct BiddingLinearStrategy : public BiddingStrategy
	{
		BiddingLinearStrategy();
		~BiddingLinearStrategy();

		void init(void);
		void calculatePaceResult(void);
		Amount useRampingBid();
		void bidding();
		void reset();
		void printStatObject(void);
		virtual std::string  strategyNameString(void);

		Amount m_amountZ;

	private:
		void calculatePriceForPaceNoSpent();
		void calculatePriceForPaceHaveSpent();
	};


	struct BiddingStretchStrategy : public BiddingStrategy
	{
		BiddingStretchStrategy();
		~BiddingStretchStrategy();

		void init(void);
		void calculatePaceResult(void);
		Amount useRampingBid();
		void bidding();
		void reset();
		void printStatObject(void);
		//void setPaceInDay(int);				
		int m_numWinsPace;
		
		int m_paceLeftToSpent;
		int m_noWinForPace;
		int m_pacesLeftInDay;
		int m_count;
		float m_paceRatio;

		virtual std::string  strategyNameString(void);

	private:
		void calculatePriceForPaceNoSpent();
		void calculatePriceForPaceHaveSpent();
	};

	struct BiddingExponentialStrategy : public BiddingStrategy
	{
		BiddingExponentialStrategy();
		~BiddingExponentialStrategy();

		void init();
		void calculatePaceResult(void);
		void bidding();
		void reset();
		Amount useRampingBid();
		bool m_previosPaceRound;
		void printStatObject(void);
		virtual std::string  strategyNameString(void);


		float m_expnFct;
		float m_expnFctPrevious;
		float m_normExpnFct;
		float m_expRampingCoefZ;
		float m_expRampingCoefZPrevious;
		float m_winRate;

		int m_numWinsPace;


	private:
		void calculatePriceForPaceNoSpent();
		void calculatePriceForPaceHaveSpent();
	};

	struct Context
	{
	protected:
		BiddingStrategy* operation;

	public:
		Context(void){}
		~Context(void){}

		virtual Amount useStrategyBidding(void) = 0;
		virtual void setStrategyBidding(BiddingStrategy* v) = 0;
		virtual void calculateResultPace(void) = 0;
		virtual void incrementCountBidRequestPace(void) = 0;
		virtual void setMaxCPI(int _maxcpi) = 0;
		virtual void setPaceBudget(int _paceBudget) = 0;
		virtual void setBudget(int _budget) = 0;
		virtual void setSpentForPace(Amount _secondPrice) = 0;
		virtual void incrementNumWins(void) = 0;
		virtual void init(void) = 0;
		virtual void print(void) = 0;
		virtual std::string getNameSrtategy(void) = 0;
	};

	struct AgentStrategy : public Context
	{
	public:
		AgentStrategy(void){}
		~AgentStrategy(void){}
		
		Amount useStrategyBidding(void);
		void setStrategyBidding(BiddingStrategy* o);
		void calculateResultPace(void);
		void incrementCountBidRequestPace(void);
		void setMaxCPI(int _maxcpi);
		void setPaceBudget(int _paceBudget);
		void setBudget(int _budget);
		Amount getPaceBudget();
		void incrementNumWins(void);
		void setSpentForPace(Amount _secondPrice);
		void init(void);
		void print(void);
		std::string getNameSrtategy(void);
	};

}