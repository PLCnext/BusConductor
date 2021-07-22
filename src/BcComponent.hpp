//
// Copyright (c) 2019 Phoenix Contact GmbH & Co. KG. All rights reserved.
// Licensed under the MIT. See LICENSE file in the project root for full license information.
//

#pragma once
#include "Arp/System/Core/Arp.h"
#include "Arp/System/Acf/ComponentBase.hpp"
#include "Arp/System/Acf/IApplication.hpp"
#include "Arp/Plc/Commons/Esm/ProgramComponentBase.hpp"
#include "BcComponentProgramProvider.hpp"
#include "BusConductorLibrary.hpp"
#include "Arp/Plc/Commons/Meta/MetaLibraryBase.hpp"
#include "Arp/System/Commons/Logging.h"
#include "Arp/System/Commons/Threading/WorkerThread.hpp"
#include "Arp/System/Rsc/ServiceManager.hpp"
#include "Arp/Io/Axioline/Services/IAxioMasterService.hpp"
#include "Arp/Io/Interbus/Services/IInterbusMasterService.hpp"

namespace BusConductor
{

using namespace Arp;
using namespace Arp::System::Acf;
using namespace Arp::Plc::Commons::Esm;
using namespace Arp::Plc::Commons::Meta;
using namespace Arp::System::Rsc;
using namespace Arp::Io::Axioline::Services;
using namespace Arp::Io::Interbus::Services;

//#component
class BcComponent : public ComponentBase, public ProgramComponentBase, private Loggable<BcComponent>
{
public: // typedefs

public: // construction/destruction
    BcComponent(IApplication& application, const String& name);
    virtual ~BcComponent() = default;

public: // IComponent operations
    void Initialize() override;
    void SubscribeServices() override;
    void LoadConfig() override;
    void SetupConfig() override;
    void ResetConfig() override;

public: // ProgramComponentBase operations
    void RegisterComponentPorts() override;

private: // methods
    BcComponent(const BcComponent& arg) = delete;
    BcComponent& operator= (const BcComponent& arg) = delete;

public: // static factory operations
    static IComponent::Ptr Create(Arp::System::Acf::IApplication& application, const String& name);

private: // fields
    BcComponentProgramProvider programProvider;
    System::Commons::Threading::WorkerThread updateThread;
    IAxioMasterService::Ptr pAxioMasterService;
    IInterbusMasterService::Ptr pInterbusMasterService;

    boolean prevConfigReq = false;
    boolean prevStartReq = false;
    boolean interbus = false;
    boolean allSystemsGo = false;

    // Keep a copy of all OUT Port variables, in case the user over-writes these.
    // (this is a work-around for the fast that PLCnext Engineer 2019.3
    //  does not handle Component Ports properly).
    boolean configured = 0;         // Bus is configured
    uint16 num_modules = 0;         // Number of I/O modules detected

private:
    void Update();  // Operation that is executed on each thread loop

public: // custom methods
    bool ConfigureLocalIo(bool validateConfig, string configFile);
    bool StartLocalIo();
    void ReadLocalIoStatus();

public: /* Ports
        =====
        Component ports are defined in the following way:
        //#port
        //#name(NameOfPort)
        boolean portField;

        The name comment defines the name of the port and is optional. Default is the name of the field.
        Attributes which are defined for a component port are IGNORED. If component ports with attributes
        are necessary, define a single structure port where attributes can be defined foreach field of the
        structure.
        */
    //#port
    struct BUS_CONDUCTOR
    {
        boolean CONFIG_REQ = 0;         // Request bus (re)configuration
        boolean CONFIG_MUST_MATCH = 0;	// Don't start the local bus unless it matches user-defined configuration
        boolean START_IO_REQ = 0;       // Request start I/O data exchange
        boolean CONFIGURED = 0;         // Bus is configured
        uint16 NUM_MODULES = 0;         // Number of I/O modules detected
//    	uint16 MODULE_DATA[320] = {0};  // Module data (5 words per module, maximum 64 modules)
    } BusConductor;
};

///////////////////////////////////////////////////////////////////////////////
// inline methods of class BcComponent
inline BcComponent::BcComponent(IApplication& application, const String& name)
: ComponentBase(application, ::BusConductor::BusConductorLibrary::GetInstance(), name, ComponentCategory::Custom, std::numeric_limits< uint32 >::max(), true)
, programProvider(*this)
, ProgramComponentBase(::BusConductor::BusConductorLibrary::GetInstance().GetNamespace(), programProvider)
, updateThread(this, &BusConductor::BcComponent::Update, 1000, "CyclicUpdate")
{
}

inline IComponent::Ptr BcComponent::Create(Arp::System::Acf::IApplication& application, const String& name)
{
    return IComponent::Ptr(new BcComponent(application, name));
}

} // end of namespace BusConductor
