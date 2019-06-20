//
// Copyright (c) Phoenix Contact GmbH & Co. KG. All rights reserved.
// Licensed under the MIT. See LICENSE file in the project root for full license information.
//

#include "BcComponent.hpp"
#include "Arp/Plc/Commons/Esm/ProgramComponentBase.hpp"

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
		configured = ConfigureLocalIo();
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

bool BcComponent::ConfigureLocalIo()
{
    // This sequence is based on PC Worx 6 library "Proficloud_v1_01", POU "AxioBusStartup"

    std::vector<uint16> sendData(100);
    std::vector<uint16> receiveData;
    uint16 response;

    // Reset driver
    sendData[0] = 0x1703;  // code
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

    // Load process data mapping
    sendData[0] = 0x0728;  // code
    sendData[1] = 0x0004;  // parameter count
    sendData[2] = 0x3000;  // address direction: both IN and OUT
    sendData[3] = 0x0001;  // communication relationship: 1st via PD RAM IF
    sendData[4] = 0x0021;  // mode (?)
    sendData[5] = 0x0000;  // reserved

    receiveData.clear();

    this->log.Info("Loading local I/O process data mapping...");

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
        this->log.Error("Load process data mapping failed. Response = {0}", response);
        return false;
    }
    this->log.Info("Done loading process data mapping.");

    // read axioline configuration
    sendData[0] = 0x030B;  // code
    sendData[1] = 0x0001;  // parameter count
    sendData[2] = 0x0007;  // attributes: device type, device id and process data length

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
    return true;
}

bool BcComponent::StartLocalIo()
{
    std::vector<uint16> sendData(100);
    std::vector<uint16> receiveData;
    uint16 response;

    // start data transfer
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
