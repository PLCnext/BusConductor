//
// Copyright (c) 2019 Phoenix Contact GmbH & Co. KG. All rights reserved.
// Licensed under the MIT. See LICENSE file in the project root for full license information.
//

#include "BcComponent.hpp"
#include "Arp/Plc/Commons/Esm/ProgramComponentBase.hpp"
#include <fstream>

namespace BusConductor
{

void BcComponent::Initialize()
{
    // never remove next line
    ProgramComponentBase::Initialize();

    // subscribe events from the event system (Nm) here
}

void BcComponent::SubscribeServices()
{
    // load project config here
    if (ServiceManager::TryGetService<IAxioMasterService>("Arp", this->pAxioMasterService))
    {
        this->log.Info("Subscribed to Axioline Master Service.");
        interbus = false;  // Axioline
        allSystemsGo = true;
    }
    else if (ServiceManager::TryGetService<IInterbusMasterService>("Arp", this->pInterbusMasterService))
    {
        this->log.Info("Subscribed to Interbus Master Service.");
        interbus = true;
        allSystemsGo = true;
    }
    else
    {
        this->log.Critical("Cannot subscribe to either Axioline or Interbus service.");
        allSystemsGo = false;
    }
}

void BcComponent::LoadConfig()
{
    // load project config here
}

void BcComponent::SetupConfig()
{
    // never remove next line
    ProgramComponentBase::SetupConfig();

    // setup project config here

    // Start the worker thread
    if (allSystemsGo) this->updateThread.Start();
}

void BcComponent::ResetConfig()
{
    // never remove next line
    ProgramComponentBase::ResetConfig();

    // implement this inverse to SetupConfig() and LoadConfig()

	this->updateThread.Stop();

}

void BcComponent::Update()
{
	// Check for configuration request
	boolean ConfigReq = this->BusConductor.CONFIG_REQ;
	if (ConfigReq && !prevConfigReq)
	{
		// Set all status information
		running = false;
		num_modules = 0;
		configured = ConfigureLocalIo(this->BusConductor.CONFIG_MUST_MATCH, "/opt/plcnext/projects/BusConductor/config.txt");
	}
	prevConfigReq = ConfigReq;

	// Check for start request
	boolean StartReq = this->BusConductor.START_IO_REQ;
	if (StartReq && !prevStartReq)
	{
		// Check current status
		if (!configured)
		{
		    this->log.Warning("Please configure local I/O before attempting to start the bus.");
		}
		else
		{
			running = StartLocalIo();
		}
	}
	prevStartReq = StartReq;

	// Copy all status variables to the Port struct
	this->BusConductor.CONFIGURED = configured;
	this->BusConductor.RUNNING = running;
	this->BusConductor.NUM_MODULES = num_modules;
}

bool BcComponent::ConfigureLocalIo(bool validateConfig, string configFile)
{
    // This sequence is based on PC Worx 6 library "Proficloud_v1_01", POU "AxioBusStartup"

    std::vector<uint16> sendData(100);
    std::vector<uint16> receiveData;
    uint16 response;

    // Reset driver
    sendData[0] = (interbus ? 0x1303 : 0x1703);  // code
    sendData[1] = 0x0000;  // parameter count

    receiveData.clear();

    this->log.Info("Resetting local I/O driver...");

    if (interbus)
    {
        response = this->pInterbusMasterService->InterbusControl(sendData, receiveData);
    }
    else
    {
        response = this->pAxioMasterService->AxioControl(sendData, receiveData);
    }

    if (response != 0)
    {
        this->log.Error("Reset driver failed. Response = {0}", response);
        return false;
    }
    this->log.Info("Done resetting driver.");

    // Create configuration
    // NOTE that this can also be achieved using the helper method "CreateConfiguration" on
    // the Axio/Interbus MasterService
    sendData[0] = 0x0710;  // code
    sendData[1] = 0x0001;  // parameter count
    sendData[2] = 0x0001;  // frame reference

    receiveData.clear();

    this->log.Info("Creating local I/O configuration...");

    if (interbus)
    {
        response = this->pInterbusMasterService->InterbusControl(sendData, receiveData);
    }
    else
    {
        response = this->pAxioMasterService->AxioControl(sendData, receiveData);
    }

    if (response != 0)
    {
        this->log.Error("Create configuration failed. Response = {0}", response);
        return false;
    }
    this->log.Info("Done creating configuration.");

    // read local I/O configuration
    // NOTE that this can also be achieved using the helper method "ReadConfiguration" on
    // the Axio/Interbus MasterService
    sendData[0] = 0x030B;  // code
    sendData[1] = 0x0001;  // parameter count
    sendData[2] = 0x0003;  // attributes: device type and device id

    receiveData.clear();

    this->log.Info("Reading local I/O configuration...");

    if (interbus)
    {
        response = this->pInterbusMasterService->InterbusControl(sendData, receiveData);
    }
    else
    {
        response = this->pAxioMasterService->AxioControl(sendData, receiveData);
    }

    if (response != 0)
    {
        this->log.Error("Read local I/O configuration failed.");
        return false;
    }
    this->log.Info("Done reading local I/O configuration.");

    // Get the number of I/O modules detected
    // TODO: Check the "More Follows" word (receiveData[3]), and handle the situation where
    // there are more I/O modules than can fit in one message.
    if (receiveData.size() < 8)
	{
        this->log.Error("Read local I/O configuration failed. Response message too short.");
		return false;
	}

    num_modules =  receiveData[7];

    // Now read the user configuration file, if necessary:
    if (validateConfig)
    {
    	string line;
    	ifstream f (configFile);
    	if (!f.is_open())
    	{
            this->log.Error("Cannot open file {0}.", configFile);
            return false;
    	}
    	int index = 7;  // index of the first element of receiveData that we are interested in
    	int expected_lines = num_modules * 4 + 1;  // Expect 4 words per module, plus the length data
    	int actual_lines = 0;
    	while(getline(f, line) && actual_lines < expected_lines)
    	{
    		line.erase(remove_if(line.begin(), line.end(), ::isspace), line.end());
			actual_lines++;
    		if (line.empty())
    		{
	            this->log.Error("User configuration file must not contain empty lines, but line {0} is empty.", actual_lines);
	    		return false;
    		}
    		else
    		{
    			if (line.compare(to_string(receiveData[index])) != 0)
				{
    	            this->log.Error("Line {0} in user configuration file ({1}) does not match actual configuration ({2}).", actual_lines, line, receiveData[index]);
    	    		return false;
				}
				index++;
    		}
		}
    	if (f.bad())
    	{
            this->log.Error("Error while reading file {0}.", configFile);
    		return false;
    	}

    	// Check that we processed the expected number of lines in the config file
    	if (actual_lines != expected_lines)
    	{
            this->log.Error("Expected {0} entries in user configuration file; found {1}", expected_lines, actual_lines);
    		return false;
    	}
    }

    // If we reach here, all must be OK
    return true;
}

bool BcComponent::StartLocalIo()
{
    std::vector<uint16> sendData(100);
    std::vector<uint16> receiveData;
    uint16 response;

    if (!interbus)
    {
        // Load process data mapping
        sendData[0] = 0x0728;  // code
        sendData[1] = 0x0004;  // parameter count
        sendData[2] = 0x3000;  // address direction: both IN and OUT
        sendData[3] = 0x0001;  // communication relationship: 1st via PD RAM IF
        sendData[4] = 0x0021;  // mode (?)
        sendData[5] = 0x0000;  // reserved

        receiveData.clear();
        this->log.Info("Loading local I/O process data mapping...");
		response = this->pAxioMasterService->AxioControl(sendData, receiveData);
        if (response != 0)
        {
            this->log.Error("Load process data mapping failed. Response = {0}", response);
            return false;
        }
        this->log.Info("Done loading process data mapping.");
    }

    // start data transfer
    // NOTE that this can also be achieved using the helper method "StartDataTransfer" on
    // the Interbus MasterService ... but there is no equivalent on the Axio Master Service ...
    sendData[0] = 0x0701;  // code
    sendData[1] = 0x0001;  // parameter count
    sendData[2] = 0x0001;  // IO Data CR

    receiveData.clear();

    this->log.Info("Enabling I/O data output...");

    if (interbus)
    {
        response = this->pInterbusMasterService->InterbusControl(sendData, receiveData);
    }
    else
    {
        response = this->pAxioMasterService->AxioControl(sendData, receiveData);
    }

    if (response != 0)
    {
        this->log.Error("Enable data output failed. Response = {0}", response);
        return false;
    }
    this->log.Info("Done enabling data output.");

    // TODO: Read local I/O status and only return true if the RUN flag is TRUE.
    return true;
}

void BcComponent::ReadLocalIoStatus()
{
        std::vector<uint16> sendData(100);
        std::vector<uint16> receiveData;
        uint16 response;

        sendData[0] = 0x0351;  // code
        sendData[1] = 0x0002;  // parameter count
        sendData[2] = 0x0001;  // variable count
        sendData[3] = 0x0104;  // variable ID: Diagnostic status

        receiveData.clear();

        this->log.Info("Reading Axioline status...");

        if (interbus)
        {
            response = this->pInterbusMasterService->InterbusControl(sendData, receiveData);
        }
        else
        {
            response = this->pAxioMasterService->AxioControl(sendData, receiveData);
        }

        this->log.Info("AxioControl Response:  {0}", response);
        this->log.Info("Size of received data:  {0}", receiveData.size());

        if (response != 0)
        {
            this->log.Error("Read Axioline status failed.");
            return;
        }
        this->log.Info("Done reading Axioline status.");

        this->log.Info("xPF = {0}", (receiveData[5] & 0x0002) > 0);
        this->log.Info("xBus = {0}", (receiveData[5] & 0x0004) > 0);
        this->log.Info("xCtrl = {0}", (receiveData[5] & 0x0008) > 0);

        this->log.Info("xRun = {0}", (receiveData[5] & 0x0020) > 0);
        this->log.Info("xActive = {0}", (receiveData[5] & 0x0040) > 0);
        this->log.Info("xReady = {0}", (receiveData[5] & 0x0080) > 0);

        this->log.Info("xBD = {0}", (receiveData[5] & 0x0100) > 0);
        this->log.Info("xForce = {0}", (receiveData[5] & 0x0400) > 0);
        this->log.Info("==========================");
}

} // end of namespace BusConductor
