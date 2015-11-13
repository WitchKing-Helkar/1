#------------------------------------------------------------------------------#
# IlluminateAgency.mk
# Anisimov, 2 Feb 2015
# 
# Makefile for  RTBkit Illuminate Agency agent, rest api, augmentor. 
#------------------------------------------------------------------------------#

$(eval $(call library,illume_augmentor,illume_augmentor.cc,augmentor_base rtb bid_request agent_configuration))
$(eval $(call library,illume_augmentor_audience,illume_augmentor_audience.cc,augmentor_base rtb bid_request agent_configuration))

$(eval $(call program,illume_augmentor_runner,illume_augmentor illume_augmentor_audience boost_program_options,illume_augmentor_runner.cc,illlume_augmentor.cc,illume_augmentor_audience.cc))


$(eval $(call program,bidding_agents_runner,bidding_agent rtb_router boost_program_options services,bidding_agents_runner.cc agents_rest_manager.cc illume_bidding_agent.cc bidding_strategy.cc))


