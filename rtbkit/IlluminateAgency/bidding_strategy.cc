#include <stdexcept>
#include <cmath>

#include "bidding_strategy.h"

namespace RTBKIT {

	//BiddingStrategy

	BiddingStrategy::
	BiddingStrategy()
	{
	}


	BiddingStrategy::
	~BiddingStrategy()
	{
	}

	void
	BiddingStrategy::
	incrementCountRequestForPace()
	{
		__sync_fetch_and_add(&m_countBidRequestForPace, 1);
	}
	
	void
	BiddingStrategy::
	incrementNumWins()
	{
		__sync_fetch_and_add(&m_numWins, 1);
	}

	void
	BiddingStrategy::
	setTotalSpentPace(Amount _price)
	{
		m_totalSpentPace = m_totalSpentPace + _price;

		//m_totalSpentAmount
		m_totalSpentAmount = m_totalSpentAmount + _price;
	}
	
	Amount
	BiddingStrategy::
	getPaceBudget() 
	{
		return m_paceBudget;
	}

	//====================== fixed strategy ======================
	BiddingFixPriceStrategy::
	BiddingFixPriceStrategy()
	{}

	BiddingFixPriceStrategy::
	~BiddingFixPriceStrategy()
	{}

	std::string 
	BiddingFixPriceStrategy::
	strategyNameString()
	{
		return "fixed";
	}

	void
	BiddingFixPriceStrategy::
	calculatePaceResult()
	{
	}

	Amount
	BiddingFixPriceStrategy::
	useRampingBid()
	{
		return m_currentAmount;
	}
	
	void
	BiddingFixPriceStrategy::
	init(void)
	{
		m_currentAmount = m_maxCPI;
	}

	void
	BiddingFixPriceStrategy::
	printStatObject(void)
	{
	}


	//====================== BiddingLinearStrategy ======================

	BiddingLinearStrategy::
	BiddingLinearStrategy()
	{
	}

	BiddingLinearStrategy::
	~BiddingLinearStrategy()
	{
	}

	std::string
	BiddingLinearStrategy::
	strategyNameString()
	{
		return "linear";
	}
		
	void
	BiddingLinearStrategy::
	bidding()
	{
		m_currentAmount = m_currentAmount + m_amountZ; //MicroUSD(m_currentPrice.value + m_amountZ.value);
	}

	Amount
	BiddingLinearStrategy::
	useRampingBid()
	{
		/*
		if (m_currentAmount >= m_maxCPI || m_stopBiddingForPace) {
		m_stopBiddingForPace = true;
		m_currentAmount = MicroUSD(0.0);		
		}*/
		if (m_currentAmount >= m_maxCPI) {
			m_currentAmount = m_maxCPI;
		}
		else {
			bidding();
			if (m_currentAmount > m_maxCPI)
				m_currentAmount = m_maxCPI;
		}
		return m_currentAmount;
	}


	void
	BiddingLinearStrategy::
	init(void)
	{
		m_totalSpentPace = MicroUSD(0);
		m_totalSpentAmount = MicroUSD(0);
		m_countBidRequestForPace = 0;
		m_percSpndAllctn = 0;
		m_stopBiddingForPace = false;
		m_numWins = 0;
		m_amountZ = MicroUSD(0);
	}

	void
	BiddingLinearStrategy::
	calculatePriceForPaceNoSpent()
	{
		m_currentAmount = MicroUSD(0.0);

		if (m_countBidRequestForPace == 0)
			throw std::overflow_error("m_numBidRequestsPace: divide by zero exception");
		else
			m_amountZ = MicroUSD((float)m_maxCPI.value / (float)m_countBidRequestForPace);  //m_maxCPI / m_countBidRequestForPace;	

		if (m_amountZ == 0)
			m_amountZ = MicroUSD(1);
	}

	void
	BiddingLinearStrategy::
	calculatePriceForPaceHaveSpent()
	{
		if (m_totalSpentPace == MicroUSD(0)) //m_paceBudget.value
			throw std::overflow_error("m_totalSpentPace.value: divide by zero exception");
		else {
			m_percSpndAllctn = (float)m_paceBudget.value / (float)m_totalSpentPace.value;
		}


		if (m_percSpndAllctn > 0.9) {
			m_percSpndAllctn = 0.9;
			m_currentAmount = MicroUSD(m_percSpndAllctn * (float)m_maxCPI.value);
		}
		else if (m_percSpndAllctn < 0.1) {
			m_percSpndAllctn = 0.1;
			m_currentAmount = MicroUSD( m_percSpndAllctn * (float)m_maxCPI.value);
		}
		else {
			m_currentAmount = MicroUSD((1 - m_percSpndAllctn) * (float)m_maxCPI.value);
		}

		if (m_countBidRequestForPace == 0)
			throw std::overflow_error("m_numBidRequestsPace: divide by zero exception");
		else {
			m_amountZ = MicroUSD((float)m_maxCPI.value / (float)m_countBidRequestForPace);
		}

		if (m_amountZ.value == 0) 
			m_amountZ =  MicroUSD(1);

	}

	void
	BiddingLinearStrategy::
	calculatePaceResult(void)
	{
		m_currentAmount = MicroUSD(0.0);

		// если не было ни одного запроса ставки за период, то расчет не производится
		// 
		if (m_countBidRequestForPace != 0) {

			//если запросы были но не было побед соответсвенно и трат 	
			if (m_totalSpentPace.value == 0) {
				calculatePriceForPaceNoSpent();
			}
			// если были траты на ставках
			else {
				calculatePriceForPaceHaveSpent();
			}
		}
		else {
			m_amountZ = MicroUSD(0);
		}

		printStatObject();

		m_stopBiddingForPace = false;
		m_totalSpentPace = MicroUSD(0);
		m_countBidRequestForPace = 0;
	}

	void
	BiddingLinearStrategy::
	printStatObject(void)
	{
		cout << "Linear Strategy" << endl;
		cout << "m_totalSpentPace: " << m_totalSpentPace << endl;
		cout << "m_countBidRequestForPace: " << m_countBidRequestForPace << endl;
		cout << "m_paceBudget: " << m_paceBudget << endl;
		cout << "m_numWins: " << m_numWins << endl;
		cout << "m_percSpndAllctn: " << m_percSpndAllctn << endl;
		cout << "m_amountZ: " << m_amountZ << endl;
		cout << "m_currentAmount: " << m_currentAmount << endl;
	}

	//====================== stretch logic ======================
	BiddingStretchStrategy::
	BiddingStretchStrategy()
	{
	}

	BiddingStretchStrategy::
	~BiddingStretchStrategy()
	{
	}

	std::string
	BiddingStretchStrategy::
	strategyNameString()
	{
		return "stretch";
	}

	void
	BiddingStretchStrategy::
	init()
	{
		m_totalSpentPace = MicroUSD(0);
		m_totalSpentAmount = MicroUSD(0);
		m_currentAmount = MicroUSD(m_maxCPI.value*0.25);
		m_numWinsPace = 0;
		m_numWins = 0;
		m_count = 0;
		m_countBidRequestForPace = 0;
		if (m_paceBudget.value == 0)
			throw ML::Exception("BiddingStretchStrategy::init()::m_paceBudget == 0");
		m_pacesLeftInDay = m_budget.value / m_paceBudget.value;
		
		if (m_pacesLeftInDay == 0)
			throw ML::Exception("BiddingStretchStrategy::init()::m_pacesLeftInDay == 0");
		
		m_paceLeftToSpent = 0;
		m_noWinForPace = 0;
	}

	void
	BiddingStretchStrategy::
	calculatePaceResult(void)
	{
		cout << "calculatePaceResult:" << endl;
		// call in pace 
		// если не было ни одного запроса ставки за период, то расчет не производится
		// 
		if (m_countBidRequestForPace != 0) {
			//если запросы были но не было побед соответсвенно и трат 	
			if (m_totalSpentPace == 0) {
				calculatePriceForPaceNoSpent();
				m_noWinForPace++;
			}
			// если были траты на ставках
			else {
				calculatePriceForPaceHaveSpent();
			}
		}
		else {
			//reset();
		}
				
		m_totalSpentPace = MicroUSD(0);
		m_countBidRequestForPace = 0;

	}

	void
	BiddingStretchStrategy::
	calculatePriceForPaceNoSpent()
	{
		cout << "calculate Price if pace hadn't spend:" << endl;
		//// увиличиваем цену
		//if (m_noWinForPace == 10) {
		//	m_currentAmount = MicroUSD((float)m_currentAmount.value * 1.25);
		//	m_noWinForPace = 0;
		//}
		cout << "oldCPI:" << m_currentAmount << endl;
		cout << "paces left in day:" << m_pacesLeftInDay << endl;
		cout << "paces left from spend:" << m_paceLeftToSpent << endl;

		if (m_currentAmount < m_maxCPI && m_pacesLeftInDay != 0){

			m_paceLeftToSpent = m_budget.value - m_totalSpentPace.value;

			m_paceRatio = (float)m_pacesLeftInDay / (float)m_paceLeftToSpent;
			
			cout << "pace ratio:" << m_paceRatio << endl;

			float cappedRatio = 0.0, cappedFactor = 0.0;
			//coeff1 = 1 - m_paceRatio;
			//cout << "debug: coeff1:" << coeff1 << endl;
			
			if (m_paceRatio > 1000)
				cappedRatio = 1000;
			else if (m_paceRatio < 0.01)
				cappedRatio = 0.01;
			else
				cappedRatio = 1;
			
			cout << "cappedRatio:" << cappedRatio << endl;

			if (cappedRatio < 1)
				cappedFactor = 2.008 - (1.01*cappedRatio);
			
			if (cappedRatio > 1)
				cappedFactor = 0.6 - (0.005*cappedRatio);
			
			if (cappedRatio == 1)
				cappedFactor = 1;
			/*
			if (coeff1 > 0) {
				coeff2 = 1 + coeff1;
				//cout << "debug: coeff2:" << coeff2 << endl;
			}
			else {
				if (m_paceRatio > 10)
					coeff2 = 0.1;
				else
					coeff2 = 1 + (coeff1 / 10);
				//cout << "debug: coeff2:" << coeff2 << endl;
			}
			*/
			cout << "cappedFactor:" << cappedFactor << endl;
			m_currentAmount = MicroUSD(cappedFactor*(float)m_currentAmount.value);
		}
		
		if (m_currentAmount.value < (m_maxCPI.value / 4))
			m_currentAmount = MicroUSD(m_maxCPI.value / 4);

		cout << "newCPI:" << m_currentAmount << endl;
	}

	void
	BiddingStretchStrategy::
	calculatePriceForPaceHaveSpent()
	{
		cout << "calculatePriceForPaceHaveSpent:" << endl;
		// calculate Paces Left in day
		m_count++;

		if (m_count > 100) {
			Amount tempBudget;			
			tempBudget = MicroUSD(m_budget.value - m_totalSpentAmount.value);
			m_pacesLeftInDay = tempBudget.value / m_paceBudget.value;
			m_count = 0;
			cout << "reculculate count: " << m_count << endl;
		}


		if (m_pacesLeftInDay > 0) {
			m_pacesLeftInDay = m_pacesLeftInDay - 1;
			
			//cout << "debug: m_totalSpentPace.value:" << m_totalSpentPace << endl;
			//cout << "debug: m_totalSpentAmount:" << m_totalSpentAmount << endl;
			m_paceLeftToSpent = (m_budget.value - m_totalSpentAmount.value) / m_totalSpentPace.value;
			//cout << "debug: m_paceLeftToSpent:" << m_paceLeftToSpent  << endl;
			//cout << "debug: m_budget:" << m_budget << endl;
			//cout << "debug: m_totalSpentAmount:" << m_totalSpentAmount << endl;
			
			cout << "Spent in pace:" << m_totalSpentPace << endl;
			cout << "paces left in day:" << m_pacesLeftInDay << endl;
			cout << "paces left from spend:" << m_paceLeftToSpent << endl;
						
						
			if (m_paceLeftToSpent == 0)
				throw ML::Exception("BiddingStretchStrategy::calculatePriceForPaceHaveSpent()::m_paceLeftToSpent == 0");
			else {
				m_paceRatio = (float)m_pacesLeftInDay / (float)m_paceLeftToSpent;
				//cout << "debug: m_paceRatio:" << m_paceRatio << endl;
				cout << "pace ratio:" << m_paceRatio << endl;

				float cappedRatio = 0.0, cappedFactor = 0.0;
				
				if (m_paceRatio > 1000)
					cappedRatio = 1000;
				else if (m_paceRatio < 0.01)
					cappedRatio = 0.01;
				else
					cappedRatio = 1;

				cout << "cappedRatio:" << cappedRatio << endl;

				if (cappedRatio < 1)
					cappedFactor = 2.008 - (1.01*cappedRatio);

				if (cappedRatio > 1)
					cappedFactor = 0.6 - (0.005*cappedRatio);
				
				if (cappedRatio == 1)
					cappedFactor = 1;

				cout << "cappedFactor:" << cappedFactor << endl;
				/*
				float coeff1 = 0.0, coeff2 = 0.0;
				coeff1 = 1 - m_paceRatio;
				//cout << "debug: coeff1:" << coeff1 << endl;
				cout << "cappedRatio:" << coeff1 << endl;

				if (coeff1 > 0) { 
					coeff2 = 1 + coeff1;
					//cout << "debug: coeff2:" << coeff2 << endl;
					cout << "cappedFactor:" << coeff2 << endl;
				}
				else {
					if (m_paceRatio > 10)
						coeff2 = 0.1;
					else {
						coeff2 = 1 + (coeff1 / 10);
						//cout << "debug: coeff2:" << coeff2 << endl;
						cout << "cappedFactor:" << coeff2 << endl;
					}
				}
				*/
				
				m_currentAmount = MicroUSD(cappedFactor *(float)m_currentAmount.value);
					
				if (m_currentAmount > m_maxCPI)
					m_currentAmount = m_maxCPI;

				cout << "newCPI:" << m_currentAmount << endl;
			}			
		}
		else
		{
			m_currentAmount = MicroUSD(0);
			return;
		}

	}

	Amount
	BiddingStretchStrategy::
	useRampingBid()
	{
		// возмжожные проверки
		return m_currentAmount;
	}

	void
	BiddingStretchStrategy::
	bidding()
	{
	}

	void
	BiddingStretchStrategy::
	reset()
	{
		m_totalSpentAmount = MicroUSD(0);
	}

	void
	BiddingStretchStrategy::
	printStatObject()
	{
		cout << "Stretch Strategy" << endl;
		cout << "m_totalSpentPace: " << m_totalSpentPace << endl;
		cout << "m_countBidRequestForPace: " << m_countBidRequestForPace << endl;
		cout << "m_paceBudget: " << m_paceBudget << endl;
		cout << "m_budget: " << m_budget << endl;
		cout << "m_numWinsPace: " << m_numWinsPace << endl;
		cout << "m_numWins: " << m_numWins << endl;
		cout << "m_currentAmount: " << m_currentAmount << endl;
		cout << "m_pacesFeftInDay: " << m_pacesLeftInDay << endl;
	}
	
	

	//====================== exponitial logic ======================
	
	BiddingExponentialStrategy::
	BiddingExponentialStrategy()
	{
	}

	BiddingExponentialStrategy::
	~BiddingExponentialStrategy()
	{
	}

	std::string
	BiddingExponentialStrategy::
	strategyNameString()
	{
		return "exponential";
	}

	void
	BiddingExponentialStrategy::
	init()
	{
		m_totalSpentPace = MicroUSD(0);
		m_countBidRequestForPace = 0;
		m_percSpndAllctn = 0;
		m_stopBiddingForPace = false;
		m_numWins = 0;
		m_expRampingCoefZ = 1;
		m_expnFctPrevious = 0;
	}

	void
	BiddingExponentialStrategy::
	reset()
	{
		m_previosPaceRound = false;
		m_expRampingCoefZPrevious = 1;
		m_expnFctPrevious = 0;
	}

	void
	BiddingExponentialStrategy::
	bidding()
	{
		m_expnFct = m_countBidRequestForPace * m_countBidRequestForPace;
		
		if (m_expnFctPrevious != 0) {
			m_normExpnFct = (m_expnFct / m_expnFctPrevious) * m_expRampingCoefZ;
			m_currentAmount = MicroUSD((float)m_maxCPI.value * m_normExpnFct);
			if (m_currentAmount.value < 0){
				m_currentAmount = MicroUSD(0);
			}

		}
		else
		{
			m_currentAmount = MicroUSD(0);
		}

	}

	Amount
	BiddingExponentialStrategy::
	useRampingBid()
	{
		if (m_currentAmount >= m_maxCPI || m_stopBiddingForPace) {
			m_stopBiddingForPace = true;
			m_currentAmount = MicroUSD(0);		
		}
		else
		{
			bidding();

			if (m_currentAmount > m_maxCPI)
				m_currentAmount = m_maxCPI;
		}
		return m_currentAmount;
	}

	void
	BiddingExponentialStrategy::
	calculatePaceResult(void)
	{
		m_currentAmount = MicroUSD(0);

		// если не было ни одного запроса ставки за период, то расчет не производится
		// 
		if (m_countBidRequestForPace != 0) {
			//если запросы были но не было побед соответсвенно и трат 	
			if (m_totalSpentPace == 0) {
				calculatePriceForPaceNoSpent();
			}
			// если были траты на ставках
			else {
				calculatePriceForPaceHaveSpent();
			}
		}
		else {
			reset();
		}
		printStatObject();

		m_stopBiddingForPace = false;
		m_totalSpentPace = MicroUSD(0);
		m_countBidRequestForPace = 0;
	}

	void
	BiddingExponentialStrategy::
	calculatePriceForPaceNoSpent()
	{
		m_expnFctPrevious = m_countBidRequestForPace * m_countBidRequestForPace;
		m_previosPaceRound = true;
	}

	void
	BiddingExponentialStrategy::
	calculatePriceForPaceHaveSpent()
	{

		m_expRampingCoefZPrevious = m_expRampingCoefZ;
		m_expnFctPrevious = m_countBidRequestForPace * m_countBidRequestForPace;

		m_percSpndAllctn = (float)m_totalSpentPace.value / (float)m_paceBudget.value;
		m_winRate = (float)m_numWinsPace / (float)m_countBidRequestForPace;

		if (m_percSpndAllctn > 1) {

			if (m_percSpndAllctn > 1.9)
				m_percSpndAllctn = 1.9;

			m_expRampingCoefZ = abs(m_expRampingCoefZPrevious * (1 - (m_percSpndAllctn - 1)));

		}
		else {
			m_expRampingCoefZ = m_expRampingCoefZPrevious * (1 + (1 - m_winRate));
		}
	}

	void
	BiddingExponentialStrategy::
	printStatObject(void)
	{
		cout << "Exponential Strategy: " << endl;
		cout << "m_totalSpentPace: " << m_totalSpentPace << endl;  
		cout << "m_countBidRequestForPace: " << m_countBidRequestForPace << endl;
		cout << "m_percSpndAllctn: " << m_percSpndAllctn << endl;
		cout << "m_stopBiddingForPace: " << m_stopBiddingForPace << endl;
		cout << "m_numWins: " << m_numWins << endl;
		cout << "m_expRampingCoefZ: " << m_expRampingCoefZ << endl;
		cout << "m_expnFctPrevious: " << m_expnFctPrevious << endl;
		cout << "m_paceBudget: " << m_paceBudget << endl;
		
	}

	// AgentStrategy

	Amount
	AgentStrategy::
	useStrategyBidding(void)
	{
		return (operation->useRampingBid()); // use();
	}

	void
	AgentStrategy::
	setStrategyBidding(BiddingStrategy* o)
	{
		operation = o;
	}

	void
	AgentStrategy::
	calculateResultPace(void)
	{
		operation->calculatePaceResult();
	}

	void
	AgentStrategy::
	incrementCountBidRequestPace()
	{
		
		operation->incrementCountRequestForPace();
	}

	void
	AgentStrategy::
	setMaxCPI(int _maxcpi)
	{
		operation->m_maxCPI = MicroUSD(_maxcpi);
	}
	
	void
	AgentStrategy::
	setPaceBudget(int _paceBudget)
	{
		operation->m_paceBudget = MicroUSD(_paceBudget);
	}

	void
	AgentStrategy::
	setBudget(int _budget)
	{
		operation->m_budget = USD(_budget);
	}

	Amount
	AgentStrategy::
	getPaceBudget()
	{
		return (operation->getPaceBudget());
	}


	void
	AgentStrategy::
	incrementNumWins()
	{
		operation->incrementNumWins();
	}

	void
	AgentStrategy::
	setSpentForPace(Amount _secondPrice)
	{
		operation->setTotalSpentPace(_secondPrice);
	}

	void
	AgentStrategy::
	init()
	{
		operation->init();
	}

	void
	AgentStrategy::
	print()
	{
		operation->printStatObject();
	}

	std::string
	AgentStrategy::
	getNameSrtategy()
	{
		return operation->strategyNameString();
	}


}