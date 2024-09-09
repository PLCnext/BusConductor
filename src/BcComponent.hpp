#pragma once
#include "Arp/System/Core/Arp.h"
#if ARP_ABI_VERSION_MAJOR < 2
#include "Arp/System/Acf/ComponentBase.hpp"
#include "Arp/System/Acf/IApplication.hpp"
#else
#include "Arp/Base/Acf/Commons/ComponentBase.hpp"
#endif
#include "Arp/Plc/Commons/Esm/ProgramComponentBase.hpp"
#include "BcComponentProgramProvider.hpp"
#include "Arp/Plc/Commons/Meta/MetaLibraryBase.hpp"
#include "Arp/System/Commons/Logging.h"

#include "Arp/System/Commons/Threading/WorkerThread.hpp"
#include "Arp/System/Rsc/ServiceManager.hpp"
#include "Arp/Io/Axioline/Services/IAxioMasterService.hpp"
#include "Arp/Io/Interbus/Services/IInterbusMasterService.hpp"

namespace BusConductor
{

using namespace Arp;
#if ARP_ABI_VERSION_MAJOR < 2
using namespace Arp::System::Acf;
#else
using namespace Arp::Base::Acf::Commons;
#endif
using namespace Arp::Plc::Commons::Esm;
using namespace Arp::Plc::Commons::Meta;

using namespace Arp::System::Rsc;
using namespace Arp::Io::Axioline::Services;
using namespace Arp::Io::Interbus::Services;
using namespace std;

//#component
class BcComponent : public ComponentBase, public ProgramComponentBase, private Loggable<BcComponent>
{
public: // typedefs

public: // construction/destruction
#if ARP_ABI_VERSION_MAJOR < 2
    BcComponent(IApplication& application, const String& name);
    virtual ~BcComponent() = default;
#else
    BcComponent(ILibrary& library, const String& name);
#endif

public: // IComponent operations
    void Initialize() override;
    void SubscribeServices() override;
    void LoadConfig() override;
    void SetupConfig() override;
    void ResetConfig() override;
    void PowerDown() override;

public: // ProgramComponentBase operations
    void RegisterComponentPorts() override;

private: // methods
#if ARP_ABI_VERSION_MAJOR < 2
    BcComponent(const BcComponent& arg) = delete;
    BcComponent& operator= (const BcComponent& arg) = delete;

public: // static factory operations
    static IComponent::Ptr Create(Arp::System::Acf::IApplication& application, const String& name);
#endif

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

           //#attributes(Hidden)
           struct Ports 
           {
               //#name(NameOfPort)
               //#attributes(Input|Retain|Opc)
               Arp::boolean portField = false;
               // The GDS name is "<componentName>/NameOfPort" if the struct is declared as Hidden
               // otherwise the GDS name is "<componentName>/PORTS.NameOfPort"
			   // If a component port is attributed with "Retain" additional measures need to be implemented. Fur further details refer to chapter "Component ports" in the topic "IComponent and IProgram" of https://www.plcnext.help
           };
           
           //#port
           Ports ports;

           Create one (and only one) instance of this struct.
           Apart from this single struct instance, it is recommended, that there should be no other Component variables 
           declared with the #port comment.
           The only attribute that is allowed on the struct instance is "Hidden", and this is optional. The attribute
           will hide the structure field and simulate that the struct fields are direct ports of the component. In the
           above example that would mean the component has only one port with the name "NameOfPort".
           When there are two struts with the attribute "Hidden" and both structs have a field with the same name, there
           will be an exception in the firmware. That is why only one struct is recommended. If multiple structs need to
           be used the "Hidden" attribute should be omitted.
           The struct can contain as many members as necessary.
           The #name comment can be applied to each member of the struct, and is optional.
           The #name comment defines the GDS name of an individual port element. If omitted, the member variable name is used as the GDS name.
           The members of the struct can be declared with any of the attributes allowed for a Program port.
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

#if ARP_ABI_VERSION_MAJOR < 2
inline IComponent::Ptr BcComponent::Create(Arp::System::Acf::IApplication& application, const String& name)
{
    return IComponent::Ptr(new BcComponent(application, name));
}
#endif
} // end of namespace BusConductor
