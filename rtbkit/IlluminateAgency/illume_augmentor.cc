

#include "illume_augmentor.h"

#include "rtbkit/core/agent_configuration/agent_configuration_listener.h"
#include "rtbkit/core/agent_configuration/agent_config.h"
#include "rtbkit/plugins/augmentor/augmentor_base.h"
#include "rtbkit/common/bid_request.h"
#include "soa/service/zmq_named_pub_sub.h"
#include "jml/utils/exc_assert.h"

#include <unordered_map>
#include <mutex>
#include <ctime>



using namespace std;

namespace RTBKIT {

/******************************************************************************/
/* FREQUENCY CAP STORAGE                                                      */
/******************************************************************************/
//
//struct RequestResult
//{
//	bool bln;
//	cass_int64_t i64;
//};

/******************************************************************************/
/* FREQUENCY CAP AUGMENTOR                                                    */
/******************************************************************************/

/** Note that the serviceName and augmentorName are distinct because you may
    have multiple instances of the service that provide the same
    augmentation.
*/
IllumeFrequencyCapAugmentor::
IllumeFrequencyCapAugmentor(
        std::shared_ptr<Datacratic::ServiceProxies> services,
        const string& serviceName,
        const string& augmentorName) :
    SyncAugmentor(augmentorName, serviceName, services),
    agentConfig(getZmqContext()),
    palEvents(getZmqContext())
	//cluster (cass_cluster_new()),
	//session (cass_session_new())
{
    recordHit("up");
}


/** Sets up the internal components of the augmentor.

    Note that SyncAugmentorBase is a MessageLoop so we can attach all our
    other service providers to our message loop to cut down on the number of
    polling threads which in turns reduces the number of context switches.
*/
void
IllumeFrequencyCapAugmentor::
init()
{
    SyncAugmentor::init(2 /* numThreads */);

    /* Manages all the communications with the AgentConfigurationService. */
    agentConfig.init(getServices()->config);
    addSource("IllumeFrequencyCapAugmentor::agentConfig", agentConfig);

    palEvents.init(getServices()->config);

    /* This lambda will get called when the post auction loop receives a win
       on an auction.
    */
    palEvents.messageHandler = [&] (const vector<zmq::message_t>& msg)
        {
            RTBKIT::AccountKey account(msg[19].toString());
            RTBKIT::UserIds uids =
                RTBKIT::UserIds::createFromString(msg[15].toString());
			
			//cout << "onWIN:" << endl;
			//queryUpdateExchangeID(account.toString(), uids.exchangeId.toString());
			aerospikeKeyUpdateCount(account.toString(), uids.exchangeId.toString());
            recordHit("wins");
        };

    palEvents.connectAllServiceProviders(
            "rtbPostAuctionService", "logger", {"MATCHEDWIN"});
    addSource("IllumeFrequencyCapAugmentor::palEvents", palEvents);


	//aeroSpike
	as_config_init(&aerospikeConfig);
	aerospikeConfig.hosts[0].addr = "54.77.111.164";
	aerospikeConfig.hosts[0].port = 3000;
	aerospike_init(&as, &aerospikeConfig);
	aerospikeConnetct();

	//cassandra
	/* Add contact points */
	//string nodes = "52.16.174.117";//"cassandra.geode.agency";
	/* Provide the cluster object as configuration to connect the session */
	//if (connect(nodes) != CASS_OK) {
	//	closeConnect();		
	//	throw ML::Exception("connection Cassandra DB ERROR!");
	//}

	// !!! debug !!!!!
	//clearBD();
	//addPeriodic("IllumeFrequencyCapAugmentor::clearBD", 300.0,
	//	[&](uint64_t) { this->clearBD(); });
}

//void
//IllumeFrequencyCapAugmentor::
//clearBD()
//{
//	CassError rc = CASS_OK;
//	CassFuture* result_future = NULL;
//
//	CassString query = cass_string_init("TRUNCATE frequencycap.frequencytable;");
//	CassStatement* statement = cass_statement_new(query, 0);
//
//	result_future = cass_session_execute(session, statement);
//	cass_future_wait(result_future);
//
//	rc = cass_future_error_code(result_future);
//	if (rc != CASS_OK) {
//		printError(rc);
//	}
//	cass_statement_free(statement);
//	cass_future_free(result_future);
//}

void
IllumeFrequencyCapAugmentor::
shutdown()
{
	//closeConnect();
	aerospikeCloseConnect();
	SyncAugmentor::shutdown();
}


RTBKIT::AugmentationList
IllumeFrequencyCapAugmentor::
onRequest(const RTBKIT::AugmentationRequest& request)
{
	//cout << "onRequest:" << endl;
    recordHit("requests");

    RTBKIT::AugmentationList result;

    const RTBKIT::UserIds& uids = request.bidRequest->userIds;

	for (const string& agent : request.agents) {

		RTBKIT::AgentConfigEntry config = agentConfig.getAgentEntry(agent);

		if (!config.valid()) {
			recordHit("unknownConfig");
			continue;
		}

		const RTBKIT::AccountKey& account = config.config->account;
		string strAccount = account.toString();
		string strExchID = uids.exchangeId.toString();

		//get Freq count
		int count = 0;
		for (const auto& augConfig : config.config->augmentations) {
			if (augConfig.name != "frequency-cap-ex") continue;
			count = augConfig.config.asInt();
			break;// augConfig.config.asInt();
		}
		//int count = config.config->augmentations["frequency-cap-ex"].config.asInt();
		
		//RequestResult output;
		//queryGetExchangeID(company, exchID, &output);
		int resultRequset = aerospikeKeyGetCount(strAccount, strExchID);
		//cout << "Request result:" << resultRequset << endl;
		//cout << "count:" << count << endl;
		if (resultRequset != -1) {
			if (count > resultRequset) {
				result[account].tags.insert("pass-frequency-cap-ex");
				recordHit("accounts." + account[0] + ".passed");
				//cout << "passed : count:" << count << ", result:" << resultRequset << endl;
			}
			else {
				recordHit("accounts." + account[0] + ".capped");
				//cout << "capped : count<result:" << count << ", result:" << resultRequset << endl;
			}
				
		}
		else {
			recordHit("accounts." + account[0] + ".capped");
			//cout << "capped : count:" << count << ", result:" << resultRequset << endl;
		}
			
	}
    return result;
}

int
IllumeFrequencyCapAugmentor::
aerospikeKeyGetCount( const std::string & account,
					  const std::string & exchangID)
{
	//cout << "aerospikeKeyGetCount" << endl;
	int _result=-1;
	as_key key;
	as_key_init(&key, "frequency-cap", account.c_str(), exchangID.c_str());
	as_error errAerospike;
	
	//as_record * rec = NULL;
	//cout << "aerospike_key_get" << endl;
	//if (aerospike_key_get(&as, &errAerospike, NULL, &key, &rec) != AEROSPIKE_OK) {
	//	//fprintf(stderr, "error(%d) %s at [%s:%d]", errAerospike.code, errAerospike.message, errAerospike.file, errAerospike.line);
	//	cout << "error:" << errAerospike.code << ":" << errAerospike.message << endl;
	//}
	//else {
	//	cout << "aerospike_key_get:res"<< res << endl;
	//	res = as_record_get_int64(rec, "count", 0);
	//	as_record_destroy(rec);
	//}
	//return res;

	as_operations ops;
	as_operations_inita(&ops, 3);
	
	as_operations_add_write_str(&ops, "exch_id", exchangID.c_str());
	as_operations_add_write_str(&ops, "company", account.c_str());
	as_operations_add_read(&ops, "count");

	as_record _rec;
	as_record *rec = as_record_inita(&_rec, 1);

	if (aerospike_key_operate(&as, &errAerospike, NULL, &key, &ops, &rec) != AEROSPIKE_OK) {
		fprintf(stderr, "err(%d) %s at [%s:%d]\n", errAerospike.code, errAerospike.message, errAerospike.file, errAerospike.line);
	}
	else {
		_result = as_record_get_int64(rec, "count", 0);
	}
	return _result;
}

void
IllumeFrequencyCapAugmentor::
aerospikeKeyUpdateCount(const std::string & account,
						const std::string & exchangID)
{
	//cout << "aerospikeKeyUpdateCount" << endl;
	as_key key;
	as_key_init(&key, "frequency-cap", account.c_str(), exchangID.c_str());
	as_operations ops;
	as_operations_inita(&ops, 4);
	as_operations_add_write_str(&ops, "exch_id", exchangID.c_str());
	as_operations_add_write_str(&ops, "company", account.c_str());
	as_operations_add_incr(&ops, "count", 1);
	as_operations_add_read(&ops, "count");
	
	as_record _rec;
	as_record *rec = as_record_inita(&_rec, 1);
	as_error errAerospike;
	if (aerospike_key_operate(&as, &errAerospike, NULL, &key, &ops, &rec) != AEROSPIKE_OK) {
		//fprintf(stderr, "err(%d) %s at [%s:%d]\n", errAerospike.code, errAerospike.message, errAerospike.file, errAerospike.line);
		cout << "error:" << errAerospike.code << ":" << errAerospike.message << endl;
	}
	//else {
		//cout<< "win current count:" << as_record_get_int64(rec, "count", 0);
	//}
	
}

void
IllumeFrequencyCapAugmentor::
aerospikeConnetct()
{
	as_error errAerospike;

	if (aerospike_connect(&as, &errAerospike) != AEROSPIKE_OK) {
		//fprintf(stderr, "err(%d) %s at [%s:%d]\n", errAerospike.code, errAerospike.message, errAerospike.file, errAerospike.line);
		cout << "frequency-cap aerospike connect error:" << errAerospike.code << ":" << errAerospike.message << endl;
	}
	else {
		cout << "frequency-cap augmentor connecting to aerospike server " << endl;
	}

}

void
IllumeFrequencyCapAugmentor::
aerospikeCloseConnect()
{
	as_error errAerospike;
	if (aerospike_close(&as, &errAerospike) != AEROSPIKE_OK) {
		//fprintf(stderr, "err(%d) %s at [%s:%d]\n", errAerospike.code, errAerospike.message, errAerospike.file, errAerospike.line);
		cout << "error:" << errAerospike.code << ":" << errAerospike.message << endl;;
	}

	aerospike_destroy(&as);
}

//CassError
//IllumeFrequencyCapAugmentor::
//connect(const string nodes)
//{
//	CassError rc = CASS_OK;
//    cout << "FrequencyCapAugmentor connecting to  Cassandra DB:" << nodes << "\n";
//    cluster = cass_cluster_new();
//    session = cass_session_new();
//    CassFuture* connect_future = NULL;
//    
//    cass_cluster_set_contact_points(cluster, nodes.c_str());
//
//    connect_future = cass_session_connect(session, cluster);
//    
//    cass_future_wait(connect_future);
//    rc = cass_future_error_code(connect_future);
//
//    if ( rc == CASS_OK ) {
//        cout << "Connected." << "\n";
//    }
//    else {
//        return printError(rc);
//    }
//    cass_future_free(connect_future);
//    
//	return rc;
//}

//void
//IllumeFrequencyCapAugmentor::
//closeConnect()
//{
//	cout << "Closing down cluster connection." << "\n";
//    cass_session_close(session);
//    cass_cluster_free(cluster);
//}

//CassError
//IllumeFrequencyCapAugmentor::
//printError(CassError error)
//{
//	cout << cass_error_desc(error) << "\n";
//	return error;
//}


//CassError
//IllumeFrequencyCapAugmentor::
//queryGetExchangeID(	const std::string & account,
//					const std::string & exchangID,
//					RequestResult *rr)
//{
//	CassError rc = CASS_OK;
//	const CassResult* results;
//
//	//cout << "queryGetExchangeID" << endl;
//	
//	rc = executeStatement(account, exchangID, &results);
//
//	CassIterator* rows = cass_iterator_from_result(results);
//	if (rc == CASS_OK) {
//		if (cass_iterator_next(rows)) {
//			const CassRow* row = cass_iterator_get_row(rows);
//			cass_value_get_int64 (cass_row_get_column(row, 3), &rr->i64);
//			//cout << "debug : cass_value_get_int64 :" << rr->i64 << endl;
//			
//			rr->bln = true;
//		}
//		else {
//			//cout << "exchange_id: false! " << endl;
//			rr->bln = false;
//		}
//		cass_result_free(results);
//		cass_iterator_free(rows);
//	}
//	
//	return rc;
//}


//CassError 
//IllumeFrequencyCapAugmentor::
//executeStatement(const string & account,
//				 const string & exchangID,
//				 const CassResult** results /* = NULL */)
//{
//	CassError rc = CASS_OK;
//	CassFuture* result_future = NULL;
//
//	CassString query = cass_string_init("SELECT * FROM frequencycap.frequencytable WHERE day = ? AND company = ? AND exchange_id = ?;");
//	CassStatement* statement = cass_statement_new(query, 3);
//	
//	//day
//	/*"YYYY-mm-dd"*/
//	const int MAX = 25;
//	char char_day[MAX + 1];
//	time_t now = time(NULL);
//	strftime(char_day, MAX + 1, "%Y-%m-%d", gmtime(&now));
//	cass_statement_bind_string(statement, 0, cass_string_init(char_day));
//	// company
//	cass_statement_bind_string(statement, 1, cass_string_init(account.c_str()));
//	// exchange_id
//	cass_statement_bind_string(statement, 2, cass_string_init(exchangID.c_str()));
//
//    result_future = cass_session_execute(session, statement);
//    cass_future_wait(result_future);
//    
//    rc = cass_future_error_code(result_future);
//    if (rc == CASS_OK) {
//		//cout << "Statement executed successully." << "\n";
//        if ( results != NULL ) {
//            *results = cass_future_get_result(result_future);
//        }
//    } 
//    else {
//        return printError(rc);
//    }
//    cass_statement_free(statement);
//    cass_future_free(result_future);
//    
//    return rc;
//
//}

//CassError
//IllumeFrequencyCapAugmentor::
//queryUpdateExchangeID(const std::string & account,
//					  const std::string & exchangID)
//{
//	CassError rc = CASS_OK;
//	CassFuture* result_future = NULL;
//	
//	CassString query = cass_string_init("UPDATE frequencycap.frequencytable  SET count = count + 1 WHERE day=? AND company=? AND exchange_id=?;");
//
//	/* Create a statement with 3 parameters */
//	CassStatement* statement = cass_statement_new(query, 3);
//	
//	//day
//	/*"YYYY-mm-dd"*/
//	const int MAX = 25;
//	char char_day[MAX + 1];
//	time_t now = time(NULL);
//	strftime(char_day, MAX + 1, "%Y-%m-%d", gmtime(&now));
//	cass_statement_bind_string(statement, 0, cass_string_init(char_day));
//	// company
//	cass_statement_bind_string(statement, 1, cass_string_init(account.c_str()));
//	// exchange_id
//	cass_statement_bind_string(statement, 2, cass_string_init(exchangID.c_str()));
//	
//	result_future = cass_session_execute(session, statement);
//	cass_future_wait(result_future);
//
//	rc = cass_future_error_code(result_future);
//	if (rc == CASS_OK)	{
//		//cout << "Statement executed successully." << "\n";		
//	}
//	else{
//		return printError(rc);
//	}
//	cass_statement_free(statement);
//	cass_future_free(result_future);
//
//	return rc;
//}

} // namespace RTBKIT

