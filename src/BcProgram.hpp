//
// Copyright (c) Phoenix Contact GmbH & Co. KG. All rights reserved.
// Licensed under the MIT. See LICENSE file in the project root for full license information.
//

#pragma once
#include "Arp/System/Core/Arp.h"
#include "Arp/Plc/Commons/Esm/ProgramBase.hpp"
#include "Arp/System/Commons/Logging.h"
#include "BcComponent.hpp"

namespace BusConductor
{

using namespace Arp;
using namespace Arp::System::Commons::Diagnostics::Logging;
using namespace Arp::Plc::Commons::Esm;

//#program
//#component(BusConductor::BcComponent)
class BcProgram : public ProgramBase, private Loggable<BcProgram>
{
public: // typedefs

public: // construction/destruction
    BcProgram(BusConductor::BcComponent& bcComponentArg, const String& name);
    BcProgram(const BcProgram& arg) = delete;
    virtual ~BcProgram() = default;

public: // operators
    BcProgram&  operator=(const BcProgram& arg) = delete;

public: // properties

public: // operations
    void    Execute() override;

public: /* Ports
           =====
           Ports are defined in the following way:
           //#port
           //#attributes(Input|Retain)
           //#name(NameOfPort)
           boolean portField;

           The attributes comment define the port attributes and is optional.
           The name comment defines the name of the port and is optional. Default is the name of the field.
        */

private: // fields
    BusConductor::BcComponent& bcComponent;
};

///////////////////////////////////////////////////////////////////////////////
// inline methods of class ProgramBase
inline BcProgram::BcProgram(BusConductor::BcComponent& bcComponentArg, const String& name)
: ProgramBase(name)
, bcComponent(bcComponentArg)
{
}

} // end of namespace BusConductor
