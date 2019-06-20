# PLCnext Technology - Dynamic Bus Configuration

[![Feature Requests](https://img.shields.io/github/issues/PLCnext/MqttGdsConnector/feature-request.svg)](https://github.com/PLCnext/MqttGdsConnector/issues?q=is%3Aopen+is%3Aissue+label%3Afeature-request+sort%3Areactions-%2B1-desc)
[![Bugs](https://img.shields.io/github/issues/PLCnext/MqttGdsConnector/bug.svg)](https://github.com/PLCnext/MqttGdsConnector/issues?utf8=✓&q=is%3Aissue+is%3Aopen+label%3Abug)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Web](https://img.shields.io/badge/PLCnext-Website-blue.svg)](https://www.phoenixcontact.com/plcnext)
[![Community](https://img.shields.io/badge/PLCnext-Community-blue.svg)](https://www.plcnext-community.net)

| Date       | Version | Authors                     |
|------------|---------|-----------------------------|
| 24.05.2019 | 1.0.0   | Martin Boers                |


## Description

This app can be used to configure a local PLCnext Control I/O bus, containing either Axioline or Inline I/O modules, without the need to hard-code the specific I/O configuration either directly in .tic files, or indirectly using PLCnext Engineer.

This app can be used in any application where the precise I/O configuration is not known at design-time, or must be changed dynamically at run-time. Examples include:

1. A machine builder who uses a single PLCnext Engineer project for multiple machine configurations, where the PLC program dynamically adapts itself based on the arrangement of local I/O modules detected.

1. A PLCnext Control used as a general-purpose RTU, data logger, or multiplexer, where the I/O arrangement must be flexible.

1. A custom PLCnext Control run-time written in any language (e.g. C/C++, rust, java, python) which requires flexible local I/O, but which does not want to manipulate .tic files. This can be used by applications like [Sample Runtime](https://github.com/PLCnext/SampleRuntime) so that PLCnext Engineer is no longer required for I/O configuration.

The source code for this app is freely available. Software developers who want to build this type of functionality into their own projects are free to use this app as a reference.

Note: The terms "Inline" and "Interbus" are used interchangeably in this document, since the Inline I/O system uses the Interbus communication protocol.

## Installation

Download the app from the PLCnext Store and install the PLCnext Engineer library called "BusConductor" on the host machine.

## Operation

The BusConductor library contains one component, called BcComponent. One instance of this component must be created by the user. The component includes one GDS port named "BusConductor", which is a structure containing the following fields:

| Field name   | C++ Type | Direction | Description                                |
|--------------|----------|-----------|--------------------------------------------|
| CONFIG_REQ   | boolean  | input     | Requst to (re-)configure the local bus     |
| START_IO_REQ | boolean  | input     | Request to start local I/O exchange        |
| CONFIGURED   | boolean  | output    | Local bus has been (re-)configured         |
| RUNNING      | boolean  | output    | Local bus is exchanging I/O                |
| NUM_MODULES  | uint16   | output    | Total number of local I/O modules detected |

The two REQ commands are processed on a rising edge. 

Once the local bus is running, process data can be exchanged with all local I/O modules using the GDS ports "Arp.Io.AxlC/0.DI4096" (inputs) and "Arp.Io.AxlC/0.DO4096" (outputs).

Output variables are latched, and are only reset on a rising edge of the CONFIG_REQ input. Once running, the real-time status of the local I/O bus should be monitored using the diagnostics variables provided by the PLCnext Control (refer to the "Local bus diagnostics" section below).

Note that this app starts all I/O modules using the configuration that is stored in the module. Any changes to module-specific configuration (e.g. analog output channel configuration) must be done separately, using (for example) the `PDI_WRITE` function block in the PLCnext Controller library, or the Acyclic Communication RSC service, or the [Startup+](http://www.phoenixcontact.net/product/2700636) configuration tool.

## Quick start

This app is packaged as a PLCnext Engineer Library, but can be used without PLCnext Engineer. This quick-start covers both cases. Use of this app without PLCnext Engineer requires advanced knowledge of file-based configuration and operation of PLCnext Control components.

### Quick Start using PLCnext Engineer

- [Create a new PLCnext Engineer project](https://youtu.be/I-FeT3p6cGA).
- Keep the default local bus type (Axioline), or [switch  the local bus type to InLine](https://www.youtube.com/watch?v=5JoVf4q7Hzg&t=166s).
- Create a generic I/O configuration. In the PLCnext Engineer Components Search box, enter "AXL F physcial" (sic) for Axioline, or "IB IL 256" for Inline. Drag the generic I/O module from the Component library to the Axioline or Inline controller in the Project tree.
- Create the following user-defined data types:
   ```
   TYPE
     ARR_AX_B_1_512 : ARRAY [1..512] OF BYTE;
   END_TYPE

   TYPE
     BUS_CONDUCTOR : STRUCT
       CONFIG_REQ   : BOOL;
       START_IO_REQ : BOOL;
       CONFIGURED   : BOOL;
       RUNNING      : BOOL;
       NUM_MODULES  : UINT;
     END_STRUCT
   END_TYPE
   ```
- Create a local IEC program in any language. In the variable table for that program, declare the following variables:

   | Name              | Type           | Usage    |
   |-------------------|----------------|----------|
   | Field_Inputs      | ARR_AX_B_1_512 | IN Port  |
   | Field_Outputs     | ARR_AX_B_1_512 | OUT Port |
   | BusConfig_Inputs  | BUS_CONDUCTOR  | IN Port  |
   | BusConfig_Outputs | BUS_CONDUCTOR  | OUT Port |

- In a Code sheet for the local IEC program, add the following logic:
   ```
   BusConfig_Outputs.CONFIGURED := BusConfig_Inputs.CONFIGURED;
   BusConfig_Outputs.RUNNING := BusConfig_Inputs.RUNNING;
   BusConfig_Outputs.NUM_MODULES := BusConfig_Inputs.NUM_MODULES;
   ```
   (This is a work-around for a current limitiation on Component Ports in PLCnext Engineer)
- Create one instance of the local IEC program in a PLCnext Task.
- In the PLCnext Port List, connect the `Field_Inputs` and `Field_Outputs` to the `DO4096` and `DI4096` ports on the generic I/O module.
- Add the BusConductor user library to the PLCnext Engineer project.
- Create one instance of the `BcProgram` program (from the BusConductor library) in a PLCnext Task.
- In the PLCnext Port List, connect the ports `BusConfig_Inputs` and `BusConfig_Outputs` to the `BusConductor` port on `BcComponent1`.
- Download the PLCnext Engineer project to the PLC and enter Debug mode.
- Set the variable `BusConfig_Outputs.CONFIG_REQ` to TRUE.
- Check that the variable `BusConfig_Inputs.CONFIGURED` goes TRUE, and `BusConfig_Inputs.NUM_MODULES` is set to the total number of I/O modules on the local bus. The local bus is now configured and ready to exchange I/O data.
- Set the variable `BusConfig_Outputs.START_IO_REQ` to TRUE.
- Check that the variable `BusConfig_Inputs.RUNNING` goes TRUE. The local bus is now exchanging I/O data with the `Field_Inputs` and `Field_Outputs` variables.

Process Data from each I/O module is now mapped to the `Field_Inputs` and `Field_Outputs` byte arrays. Check the documentation for each I/O module to see how many bytes of process data are provided by that module.

### Quick Start without PLCnext Engineer

This example describes the use of Axioline I/O. A similar procedure is used to configure Inline I/O.
- Using PuTTY on the host machine, log in to the PLC as admin.
- On the PLC, create a new directory for this project, e.g.
   ```
   mkdir /opt/plcnext/projects/BusConductor
   ```
- On the host machine, locate the file `BusConductor.pcwlx` that was installed with the app.
- Unzip `BusConductor.pcwlx` using a tool like 7-Zip.
- Locate the file `libBusConductor.so` in the sub-directory `PROJECT\Logical%20Elements\AXCF2152_19.3.0`. This is the shared object library containing the PLCnext Component that must be instantiated by the Automation Component Framework (ACF).
- Using WinCSP, copy the shared object library `libBusConductor.so` from the host to the PLC directory `/opt/plcnext/projects/BusConductor`
- On the PLC, download the ACF configuration file from the Github repository:
   ```
   cd /opt/plcnext/projects/BusConductor
   wget http://github.... 
   ```
   The acf.config file tells the ACF to load the shared object library and create one instance of the BusConductor component.
- Check that the contents of the acf.config file are correct. For example, the path to the shared object library may need to be updated for your installation. 
- Download the generic Axioline I/O configuration from Github.
   ```
   cd /opt/plcnext/projects/BusConductor
   wget --recursive --no-parent http://example.com/generic_axioline/
   ```
- Change the 'current' symlink to point to the new project directory.
   ```
   cd /opt/plcnext/projects
   rm current 
   ln -s BusConductor current
   ```
    This tells the PLCnext Control to use this as the default project, which will load the acf.config file and generic I/O configuration from this directory.

- Restart the PLCnext runtime to create the BusConductor component.
- If using the acf.config file in this example, the name of the BusConductor port in the Global Data Space will be "BcComponent1/BusConductor"

You must now send commands to the BcComponent instance from your own application via the Global Data Space. How this is done will be specific to each user application. For example, you can use a gds.config file to connect the BusConductor port to a GDS port in your own C++ project, or you can read and write GDS data by subscribing to the Data Access RSC Service in the PLC.

Once the local bus is running, I/O data can be exchanged with a user application in a number of ways. Entries in a gds.config file can be used to connect I/O data to GDS ports in the user application, or the ANSI-C interface can be used to read and write I/O data.

An example of how to use the Data Access RSC Service (for GDS access) and the ANSI-C libraries (for I/O access) are given in the [Sample Runtime](https://github.com/PLCnext/SampleRuntime) example. This sample could be modified to use the BusConductor component to start the Axioline bus, which would eliminate the need to use PLCnext Engineer to generate a different I/O configuration for each specific arrangement of I/O modules.

## Local bus diagnostics

Once the local I/O bus is running, diagnostics are available from the following standard GDS OUT port variables. These dignostics can be used in decisions to reset the local I/O bus, if and when required.

### Axioline
| Port Name                                | IEC Type   |
|------------------------------------------|------------|
| Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_HI      | Bitstring8 |
| Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_LOW     | Bitstring8 |
| Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_PF      | BOOL       |
| Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_BUS     | BOOL       |
| Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_RUN     | BOOL       |
| Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_ACT     | BOOL       |
| Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_RDY     | BOOL       |
| Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_SYSFAIL | BOOL       |
| Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_PW      | BOOL       |
| Arp.Io.AxlC/AXIO_DIAG_PARAM_REG_HI       | Bitstring8 |
| Arp.Io.AxlC/AXIO_DIAG_PARAM_REG_LOW      | Bitstring8 |
| Arp.Io.AxlC/AXIO_DIAG_PARAM_2_REG_HI     | Bitstring8 |
| Arp.Io.AxlC/AXIO_DIAG_PARAM_2_REG_LOW    | Bitstring8 |

A description of each of these variables is given in the document "UM EN AXL F SYS DIAG", available for download from the Phoenix Contact website.

### Inline
| Port Name                                | IEC Type   |
|------------------------------------------|------------|
| Arp.Io.IbM/IB_DIAG_STATUS_REG_HI         | Bitstring8 |
| Arp.Io.IbM/IB_DIAG_STATUS_REG_LOW        | Bitstring8 |
| Arp.Io.IbM/IB_DIAG_STATUS_REG_PF         | BOOL       |
| Arp.Io.IbM/IB_DIAG_STATUS_REG_BUS        | BOOL       |
| Arp.Io.IbM/IB_DIAG_STATUS_REG_RUN        | BOOL       |
| Arp.Io.IbM/IB_DIAG_STATUS_REG_ACT        | BOOL       |
| Arp.Io.IbM/IB_DIAG_STATUS_REG_RDY        | BOOL       |
| Arp.Io.IbM/IB_DIAG_STATUS_REG_SYSFAIL    | BOOL       |
| Arp.Io.IbM/IB_DIAG_STATUS_REG_WARN       | BOOL       |
| Arp.Io.IbM/IB_DIAG_STATUS_REG_QUAL       | BOOL       |
| Arp.Io.IbM/IB_DIAG_STATUS_REG_USER       | BOOL       |
| Arp.Io.IbM/IB_DIAG_STATUS_REG_CTRL       | BOOL       |
| Arp.Io.IbM/IB_DIAG_STATUS_REG_DTC        | BOOL       |
| Arp.Io.IbM/IB_DIAG_PARAM_REG_HI          | Bitstring8 |
| Arp.Io.IbM/IB_DIAG_PARAM_REG_LOW         | Bitstring8 |
| Arp.Io.IbM/IB_DIAG_PARAM_2_REG_HI        | Bitstring8 |
| Arp.Io.IbM/IB_DIAG_PARAM_2_REG_LOW       | Bitstring8 |

A description of each of these variables is given in the document "IBS SYS DIAG DSC UM E", available for download from the Phoenix Contact website.

## Changing the local I/O update rate

By default, I/O data is exhanged with local I/O modules every 50 ms. This can be changed, if required, using either PLCnext Engineer or (for applications that do not use PLCnext Engineer) by editing configuration files directly on the PLC:

### Using PLCnext Engineer

- In PLCnext Engineer, double-click on the local I/O bus controller (e.g. "Axioline F") in the Project pane, click on the Settings tab, and select the "Update task" menu item.
- Select a task from the drop-down list. Data will be exchanged with local I/O modules every time this task is executed.
- If no task is selected, the I/O update rate defaults to 50 ms.

### Without PLCnext Engineer

- In the esm.config file for your project, define a task that will be used to trigger local I/O updates.
- On the PLC, open the following file in an editor: `/opt/plcnext/projects/Default/Plc/Esm/Axio.esm.config`
- In this file, change the `taskName` in the `TaskEvent` entries from "Globals" to the name of your I/O update task.

## Troubleshooting

If the BusConductor component appears not to be working, check the contents of the file `/opt/plcnext/logs/Output.log`, and look for entries from the component `BusConductor.BcComponent`. This can be done with the following command:
```
cat /opt/plcnext/logs/Output.log | grep BusConductor.BcComponent
```

## How to get support
This app is supported in the forum of the [PLCnext Community](https://www.plcnext-community.net/index.php?option=com_easydiscuss&view=categories&Itemid=221&lang=en).
Please raise an issue with a detailed error description and always provide a copy of the Output.log file.

## Design notes

- This project was developed entirely in Eclipse, using the PLCnext CLI project template and the configuration/build process provided by the PLCnext Add-In for Eclipse. The current version of the PLCnext CLI (2019.0) provides limited support for the Component Ports feature of the PLCnext Control firmware, and more Component Port features could be utilised by moving away from the PLCnext CLI project template and build process.

- This app is designed to be used in PLCnext Engineer, but also be useable by other C++ components. Note that the current version of PLCnext Engineer (2019.3) provides limited support for the "Component Ports" feature of the PLCnext Control firmware, and no support for component-only libraries. This project includes some features that work around these limitiations - for example, the only purpose of the program `BcProgram` in the BusConductor library is to force PLCnext Engineer to instantiate `BcComponent`, which would otherwise not be instantiated by PLCnext Engineer.

- For software developers who would like to reproduce the functions of this app in their own IEC 61131 (PLCnext Engineer) project, use the `AX_CONTROL` and/or `IB_CONTROL_NEXT` function blocks to access the Axioline Master and Interbus Master RSC services respectively. These FBs are included in the PLCnext Controller library that comes with PLCnext Engineer.

-----------

Copyright © 2019 Phoenix Contact Electronics GmbH

All rights reserved. This program and the accompanying materials are made available under the terms of the [MIT License](http://opensource.org/licenses/MIT) which accompanies this distribution.
