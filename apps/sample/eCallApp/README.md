# eCall Reference App Information
The goal in writing this reference application is to create an application that more closely aligns with a wholly functional eCall application (and hopefully achieve box-level eCall certifications in the future). The previous sample eCall was called via an executable, with commands given through command-line arguments. While this works fine for a sample application to demonstrate how the eCall service can be called and used, it doesn't particularly match the actual use case of a real eCall application, especially as the MSD requires fields such as delta location information that would require location information from before the actual triggered crash/call.

## Usage
The eCall reference application is now an application that runs in the background, fetching/storing location information as needed for MSD fields such as recentVehicleLocationN1/N2. As the application is constantly running, it needed a new method for triggering the "crash" or providing the type of eCall being triggered. Currently, the Config Tree is acting as a stand-in for a call button or crash sensors.

To use the app, first start tafMngdPMSvc and then tafECallRefApp. After verifying that the app is currently running, an eCall session can be triggered by editing the Config Tree for tafECallRefApp using the set command.

```config set tafECallRefApp/eCallType 2 int```

The above example triggers an AUTO eCall by setting the eCallType to 2.
* TEST: 1
* AUTO: 2
* MANUAL: 3

## Current Limitations and Issues
The point at which the wake source is acquired was moved from when the eCall is triggered to the initialization of the app, which may not be correct/expected. However this is (currently) necessary, as after starting MPMS, the PM state will change from resume to suspend after 10 seconds. But the PM state needs to be resume in order for GNSS to start, so acquiring the wake source prior to starting GNSS is the current work around (the other being manually setting the PM state to resume after 10 seconds).

Private eCall is currently not implemented.

Certain fields for MSD information are unclear/not implemented properly by eCall regulations.
* Vehicle information for MSD such as vehicle type, propulsion storage type, and VIN number are currently hardcoded.
* Information such as passenger count or location of impact (for AUTO eCall) are also hard coded.
* Fields such as positionCanBeTrusted (isPosTrusted) might not be calculated correctly according to eCall regulations and definitions for the field as well as hAccuracy from locGnss might need to be compared.
* MSD regulations are not always consistent across region. For example ERA-GLONSS and EU-eCall handle missing/invalid location (latitude/longitude) information differently: ERA states that the last reliably evaluated value should be transmitted as a stand-in and then the invalid value as a last case, whereas EU states that an invalid value should be transmitted regardless. Currently the specification is assumed to be for EU-eCall.

Handling for many state changes and timers aren't implemented under the eCall state change handler yet.

The current application is in C++ (converted from the original C app), and should run properly, however may be able to be refactored to take advantage of the additional functionality (namespaces?) provided by C++ that wasn't available when initially written in C.

MT call for audio control is missing from the implementation. 

NG eCall and MS eCall are not supported.