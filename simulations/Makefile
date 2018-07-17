.PHONY: sim

DOPTIONS=-i --privileged --sysctl net.ipv6.conf.all.disable_ipv6=0 -v `pwd`:/home/user/simulations_host --cap-add=NET_ADMIN

sim:
	docker run --cidfile "$(SIM)/container.id" $(DOPTIONS) ns3cooja ns3cooja $(SIM) $(NS3_SIM) $(NS3_SEED) "$(NS3_ARGS)"
	docker cp `cat $(SIM)/container.id`:/home/user/simulations/results/. $(SIM)
	docker rm `cat $(SIM)/container.id`
	rm $(SIM)/container.id

bash:
	docker run -t --rm $(DOPTIONS) ns3cooja bash

simple:
	docker run --cidfile "container.id" $(DOPTIONS) ns3cooja ns3cooja $(SIM)
	docker cp `cat container.id`:/home/user/simulations/results results
	docker rm `cat container.id`
	rm container.id
