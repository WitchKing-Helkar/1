/** augmentor_ex.cc                                 -*- C++ -*-
    Rémi Attab, 14 Feb 2013
    Copyright (c) 2013 Datacratic.  All rights reserved.

    Augmentor example that can be used to do extremely simplistic frequency
    capping.

*/

#include "illume_augmentor_audience.h"

#include "rtbkit/core/agent_configuration/agent_configuration_listener.h"
#include "rtbkit/core/agent_configuration/agent_config.h"
#include "rtbkit/plugins/augmentor/augmentor_base.h"
#include "rtbkit/common/bid_request.h"
#include "soa/service/zmq_named_pub_sub.h"
#include "jml/utils/exc_assert.h"

#include <unordered_map>
#include <mutex>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <openssl/engine.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <ctime>
#include <cstring>
#include <cstdio>
#include <curl/curl.h>



using namespace std;

namespace RTBKIT {

//struct RequestResult
//{
//	bool bln;
//	string str;
//};


/******************************************************************************/
/* AUGMENTOR Audience                                                   */ 
/******************************************************************************/

/** Note that the serviceName and augmentorName are distinct because you may
    have multiple instances of the service that provide the same
    augmentation.
*/
IllumeAugmentorAudience::
IllumeAugmentorAudience(
        std::shared_ptr<Datacratic::ServiceProxies> services,
        const string& serviceName,
        const string& augmentorName) :
    SyncAugmentor(augmentorName, serviceName, services),
    agentConfig(getZmqContext())
	//cluster(cass_cluster_new()),
	//session(cass_session_new())	
{	
    recordHit("up");
}


/** Sets up the internal components of the augmentor.

    Note that SyncAugmentorBase is a MessageLoop so we can attach all our
    other service providers to our message loop to cut down on the number of
    polling threads which in turns reduces the number of context switches.
*/
void
IllumeAugmentorAudience::
init()
{
    SyncAugmentor::init(2 /* numThreads */);

    /* Manages all the communications with the AgentConfigurationService. */
    agentConfig.init(getServices()->config);
    addSource("IllumeAugmentorAudience::agentConfig", agentConfig);

    //palEvents.init(getServices()->config);

    /* This lambda will get called when the post auction loop receives a win
       on an auction.
    */
    //palEvents.messageHandler = [&] (const vector<zmq::message_t>& msg)
    //    {
    //        RTBKIT::AccountKey account(msg[19].toString());
    //        RTBKIT::UserIds uids =
    //            RTBKIT::UserIds::createFromString(msg[15].toString());
			            
    //    };

    //palEvents.connectAllServiceProviders(
    //        "rtbPostAuctionService", "logger", {"MATCHEDWIN"});
    //addSource("IllumeAugmentorAudience::palEvents", palEvents);

	//cassandra
	/* Add contact points */
	//string nodes = "52.16.174.117";//"cassandra.geode.agency";
	/* Provide the cluster object as configuration to connect the session */
	//if (connect(nodes) != CASS_OK) {
	//	closeConnect();
	//	throw ML::Exception("connection Cassandra DB ERROR!");
	//}

	//aeroSpike
	as_config_init(&aerospikeConfig);
	aerospikeConfig.hosts[0].addr = "52.17.41.144";
	aerospikeConfig.hosts[0].port = 3000;
	aerospike_init(&as, &aerospikeConfig);
	aerospikeConnetct();
}

void
IllumeAugmentorAudience::
shutdown()
{
	aerospikeCloseConnect();
	SyncAugmentor::shutdown();
}

void
IllumeAugmentorAudience::
aerospikeConnetct()
{
	as_error errAerospike;

	if (aerospike_connect(&as, &errAerospike) != AEROSPIKE_OK) {
		//fprintf(stderr, "err(%d) %s at [%s:%d]\n", errAerospike.code, errAerospike.message, errAerospike.file, errAerospike.line);
		cout << "audience aerospike connect error:" << errAerospike.code << ":" << errAerospike.message << endl;
	}
	else {
		cout << "audience augmentor connecting to aerospike server " << endl;
	}
}

void
IllumeAugmentorAudience::
aerospikeCloseConnect()
{
	as_error errAerospike;
	if (aerospike_close(&as, &errAerospike) != AEROSPIKE_OK) {
		//fprintf(stderr, "err(%d) %s at [%s:%d]\n", errAerospike.code, errAerospike.message, errAerospike.file, errAerospike.line);
		cout << "error:" << errAerospike.code << ":" << errAerospike.message << endl;
	}

	aerospike_destroy(&as);
}

string
IllumeAugmentorAudience::
aerospikeKeyGetAugid(const string &  strIFA)
{
	//cout << "aerospikeKeyGetCount" << endl;
	string _result;
	as_key key;
	as_key_init(&key, "audience", "augid-set", /*"23100-123123-12300"*/strIFA.c_str());
	as_error errAerospike;

	as_record * rec = NULL;
	//cout << "aerospike_key_get" << endl;
	if (aerospike_key_get(&as, &errAerospike, NULL, &key, &rec) != AEROSPIKE_OK) {
		//fprintf(stderr, "error(%d) %s at [%s:%d]", errAerospike.code, errAerospike.message, errAerospike.file, errAerospike.line);
		cout << "error:" << errAerospike.code << ":" << errAerospike.message << endl;
	}
	else {
		//cout << "aerospike_key_get:res" << _result << endl;
		_result = as_record_get_str(rec, "augid");
		as_record_destroy(rec);
	}
	return _result;
		
}

RTBKIT::AugmentationList
IllumeAugmentorAudience::
onRequest(const RTBKIT::AugmentationRequest& request)
{
	recordHit("requests");

	RTBKIT::AugmentationList result;

	for (const string& agent : request.agents) {

		RTBKIT::AgentConfigEntry config = agentConfig.getAgentEntry(agent);

		if (!config.valid()) {
			recordHit("unknownConfig");
			continue;
		}

		const RTBKIT::AccountKey& account = config.config->account;

		//int audience = 0;
		//cout << "ifa:" << request.bidRequest->device->ifa << endl;
		
		//int audience = 0;
		string strAugid;
		int auigid = 0;
		//get AUGID for audience
		for (const auto& augConfig : config.config->augmentations) {
			if (augConfig.name != "audience") continue;
			strAugid = augConfig.filters.include[auigid];
			break;
		}


		//string strAUGID = config.config->augmentations[audience].filters.include[auigid];
		string strIFA = request.bidRequest->device->ifa;
		
		//RequestResult output;
		//queryGetExchangeID(strIFA, &output);
		string strRequestAugid = aerospikeKeyGetAugid(strIFA);
		//cout << "augid request: " << strRequestAugid << endl;
		//cout << "augid account: " << strAugid << endl;

		if (strRequestAugid == strAugid) {
			result[account].tags.insert(strAugid);
			recordHit("accounts." + account[0] + ".passed");
		}
		else {
			recordHit("accounts." + account[0] + ".capped");
		}		

	}

	return result;
}

//CassError
//IllumeAugmentorAudience::
//printError(CassError error)
//{
//	cout << cass_error_desc(error) << "\n";
//	return error;
//}

//void
//IllumeAugmentorAudience::
//closeConnect()
//{
//	cout << "Closing down cluster connection." << "\n";
//	cass_session_close(session);
//	cass_cluster_free(cluster);
//}

//CassError
//IllumeAugmentorAudience::
//queryGetExchangeID(const string & ifa,
//				   RequestResult *rr)
//{
//	CassError rc = CASS_OK;
//	const CassResult* results;
//
//	rc = executeStatement(ifa, &results);
//
//	CassIterator* rows = cass_iterator_from_result(results);
//	if (rc == CASS_OK) {
//		if (cass_iterator_next(rows)) {
//			const CassRow* row = cass_iterator_get_row(rows);
//			CassString cass_ifa;
//			cass_value_get_string(cass_row_get_column(row, 1), &cass_ifa);
//			rr->bln = true;
//			rr->str = cass_ifa.data;
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
//IllumeAugmentorAudience::
//executeStatement(const string & ifa,
//				 const CassResult** results /* = NULL */)
//{
//	CassError rc = CASS_OK;
//	CassFuture* result_future = NULL;
//
//	CassString query = cass_string_init("SELECT * FROM audiencecap.audeince WHERE ifa = ?;");
//	CassStatement* statement = cass_statement_new(query, 1);
//	
//	//ifa
//	cass_statement_bind_string(statement, 0, cass_string_init(ifa.c_str()));
//	
//	result_future = cass_session_execute(session, statement);
//	cass_future_wait(result_future);
//
//	rc = cass_future_error_code(result_future);
//	if (rc == CASS_OK) {
//		//cout << "Statement executed successully." << "\n";
//		if (results != NULL) {
//			*results = cass_future_get_result(result_future);
//		}
//	}
//	else {
//		return printError(rc);
//	}
//	cass_statement_free(statement);
//	cass_future_free(result_future);
//
//	return rc;
//
//}

/** Augments the bid request with our frequency cap information.

    This function has a 5ms window to respond (including network latency).
    Note that the augmentation is in charge of ensuring that the time
    constraints are respected and any late responses will be ignored.
*/


//CassError
//IllumeAugmentorAudience::
//connect(const string nodes)
//{
//	CassError rc = CASS_OK;
//	cout << "AugmentorAudience connecting to  Cassandra DB:" << nodes << "\n";
//	cluster = cass_cluster_new();
//	session = cass_session_new();
//	CassFuture* connect_future = NULL;
//
//	cass_cluster_set_contact_points(cluster, nodes.c_str());
//
//	connect_future = cass_session_connect(session, cluster);
//
//	cass_future_wait(connect_future);
//	rc = cass_future_error_code(connect_future);
//
//	if (rc == CASS_OK) {
//		cout << "Connected." << "\n";
//	}
//	else {
//		return printError(rc);
//	}
//	cass_future_free(connect_future);
//
//	return rc;
//}

} // namespace RTBKIT

