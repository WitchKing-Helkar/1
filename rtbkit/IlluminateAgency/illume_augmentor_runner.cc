/** augmentor_ex_runner.cc                                 -*- C++ -*-
    Rémi Attab, 22 Feb 2013
    Copyright (c) 2013 Datacratic.  All rights reserved.

    Runner for our augmentor example.

*/

#include "illume_augmentor.h"
#include "illume_augmentor_audience.h"

#include "soa/service/service_utils.h"

#include <boost/program_options/cmdline.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include <iostream>
#include <thread>
#include <chrono>


using namespace std;

/******************************************************************************/
/* MAIN                                                                       */
/******************************************************************************/

int main(int argc, char** argv)
{
    using namespace boost::program_options;

    Datacratic::ServiceProxyArguments args;

    options_description options = args.makeProgramOptions();
    options.add_options() ("help,h", "Print this message");

    variables_map vm;
    store(command_line_parser(argc, argv).options(options).run(), vm);
    notify(vm);

    if (vm.count("help")) {
        cerr << options << endl;
        return 1;
    }

    auto serviceProxies = args.makeServiceProxies();
	RTBKIT::IllumeFrequencyCapAugmentor augmentorFrequency(serviceProxies, "frequency-cap-ex", "frequency-cap-ex");
	//RTBKIT::IllumeAugmentorAudience augmentorAudience(serviceProxies, "audience", "audience");
	
	augmentorFrequency.init();
	augmentorFrequency.start();

	//augmentorAudience.init();
	//augmentorAudience.start();

    while (true) this_thread::sleep_for(chrono::seconds(10));

    // Won't ever reach this point but this is how you shutdown an augmentor.
	augmentorFrequency.shutdown();
	//augmentorAudience.shutdown();

    return 0;
}
