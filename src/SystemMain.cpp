//
// Created by szymo on 27.04.2025.
//
// SystemMain.cpp
#include "MPKImpl.h"
#include <iostream>

class MPKServer : public Ice::Application {
public:
    virtual int run(int argc, char* argv[]) override {
        try {
            // Create object adapter
            Ice::ObjectAdapterPtr adapter = communicator()->createObjectAdapterWithEndpoints(
                "MPKAdapter", "default -p 10000");

            // Create MPK implementation
            MPKImpl* mpkImpl = new MPKImpl();
            Ice::ObjectPrx mpkBase = adapter->add(mpkImpl, Ice::stringToIdentity("MPK"));
            SIP::MPKPrx mpk = SIP::MPKPrx::uncheckedCast(mpkBase);

            // Create line factory implementation
            LineFactoryImpl* lineFactoryImpl = new LineFactoryImpl();
            Ice::ObjectPrx lfBase = adapter->add(lineFactoryImpl, Ice::stringToIdentity("LineFactory"));
            SIP::LineFactoryPrx lineFactory = SIP::LineFactoryPrx::uncheckedCast(lfBase);

            // Create stop factory implementation
            StopFactoryImpl* stopFactoryImpl = new StopFactoryImpl();
            Ice::ObjectPrx sfBase = adapter->add(stopFactoryImpl, Ice::stringToIdentity("StopFactory"));
            SIP::StopFactoryPrx stopFactory = SIP::StopFactoryPrx::uncheckedCast(sfBase);

            // Register factories with MPK
            mpk->registerLineFactory(lineFactory);
            mpk->registerStopFactory(stopFactory);

            // Create some sample depos
            DepoImpl* centralDepoImpl = new DepoImpl("Central");
            Ice::ObjectPrx centralDepoBase = adapter->add(centralDepoImpl, Ice::stringToIdentity("CentralDepo"));
            SIP::DepoPrx centralDepo = SIP::DepoPrx::uncheckedCast(centralDepoBase);

            DepoImpl* northDepoImpl = new DepoImpl("North");
            Ice::ObjectPrx northDepoBase = adapter->add(northDepoImpl, Ice::stringToIdentity("NorthDepo"));
            SIP::DepoPrx northDepo = SIP::DepoPrx::uncheckedCast(northDepoBase);

            // Register depos with MPK
            mpk->registerDepo(centralDepo);
            mpk->registerDepo(northDepo);

            // Create some sample lines
            SIP::LinePrx line1 = lineFactory->createLine("1");
            SIP::LinePrx line2 = lineFactory->createLine("2");

            // Create some sample stops for line 1
            SIP::StopList line1Stops;
            std::vector<std::string> line1StopNames = {
                "MainStation", "CityCenter", "Park", "University", "Hospital"
            };

            for (const auto& name : line1StopNames) {
                TramStopImpl* stopImpl = new TramStopImpl(name);
                Ice::ObjectPrx stopBase = adapter->add(stopImpl, Ice::stringToIdentity("Stop_" + name));
                SIP::TramStopPrx stop = SIP::TramStopPrx::uncheckedCast(stopBase);

                SIP::StopInfo stopInfo;
                stopInfo.stop = SIP::StopPrx::uncheckedCast(stop);
                stopInfo.time = {0, 0}; // Default time
                line1Stops.push_back(stopInfo);
            }

            // Set stops for line 1
            line1->setStops(line1Stops);

            // Create some sample stops for line 2
            SIP::StopList line2Stops;
            std::vector<std::string> line2StopNames = {
                "MainStation", "ShoppingMall", "Stadium", "Airport", "BusinessPark"
            };

            for (const auto& name : line2StopNames) {
                // Try to reuse existing stops
                SIP::TramStopPrx stop = mpk->getTramStop(name);

                SIP::StopInfo stopInfo;
                stopInfo.stop = SIP::StopPrx::uncheckedCast(stop);
                stopInfo.time = {0, 0}; // Default time
                line2Stops.push_back(stopInfo);
            }

            // Set stops for line 2
            line2->setStops(line2Stops);

            // Activate adapter
            adapter->activate();

            std::cout << "MPK system is running." << std::endl;
            std::cout << "Available lines: 1, 2" << std::endl;
            std::cout << "Available depos: Central, North" << std::endl;

            // Wait for shutdown
            communicator()->waitForShutdown();

            return 0;
        }
        catch (const Ice::Exception& e) {
            std::cerr << e << std::endl;
            return 1;
        }
    }
};

int main(int argc, char* argv[]) {
    MPKServer app;
    return app.main(argc, argv);
}