/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "ApiHms.hpp"

#include "EcallEventBus.hpp"

namespace ecall
{

ModemEvtType MapToModemEvtType(taf_hms_ModemEvtType_t tafType)
{
    switch (tafType)
    {
        case TAF_HMS_MODEM_EVENT_TYPE_CONTINUE_REBOOT:
            return ModemEvtType::ContinueReboot;
                case TAF_HMS_MODEM_EVENT_TYPE_CONNECTION_LOST:
            return ModemEvtType::ConnectionLost;
        case TAF_HMS_MODEM_EVENT_TYPE_CONNECTION_AVAIL:
            return ModemEvtType::ConnectionAvail;
    }

    LE_ERROR("Unknown TAF modem event type received: %d", static_cast<int>(tafType));
    return ModemEvtType::ContinueReboot;
}

ModemEvtSeverity MapToModemEvtSeverity(taf_hms_ModemEvtSeverity_t tafSeverity)
{
    switch (tafSeverity)
    {
        case TAF_HMS_MODEM_EVENT_SEVERITY_LOW:
            return ModemEvtSeverity::Low;
        case TAF_HMS_MODEM_EVENT_SEVERITY_MEDIUM:
            return ModemEvtSeverity::Medium;
        case TAF_HMS_MODEM_EVENT_SEVERITY_HIGH:
            return ModemEvtSeverity::High;
    }

    LE_ERROR("Unknown TAF modem event severity received: %d", static_cast<int>(tafSeverity));
    return ModemEvtSeverity::Low;
}

// static
ApiHms& ApiHms::GetInstance()
{
    static ApiHms inst;
    return inst;
}

ApiHms::ApiHms()
{
    // Connect to HMS service and register the modem event handler.
    taf_hms_ConnectService();

    const taf_hms_ModemEvtBitmask_t reqEventBits =
        TAF_HMS_MODEM_EVENT_TYPE_CONTINUE_REBOOT |
        TAF_HMS_MODEM_EVENT_TYPE_CONNECTION_LOST |
        TAF_HMS_MODEM_EVENT_TYPE_CONNECTION_AVAIL;

    taf_hms_AddModemEvtHandler(reqEventBits,
                               &ApiHms::ModemEvtCallback,
                               this);

    LE_INFO("[ApiHms] HMS service initialized");
}

// static
void ApiHms::ModemEvtCallback(taf_hms_ModemEvtType_t eventType,
                        taf_hms_ModemEvtSeverity_t eventSeverity,
                        taf_hms_ModemEventRef_t eventRef,
                        void* contextPtr)
{
    auto* self = static_cast<ApiHms*>(contextPtr);
    if (!self)
    {
        return;
    }

    self->HandleModemEvt(eventType, eventSeverity);
}

void ApiHms::HandleModemEvt(taf_hms_ModemEvtType_t eventType,
                       taf_hms_ModemEvtSeverity_t eventSeverity)
{
	 const ModemHmsEvt hmsEvt{
		 MapToModemEvtType(eventType),
		 MapToModemEvtSeverity(eventSeverity)
	 };

    auto& bus = EcallEventBus::GetInstance();

    bus.PublishHms(hmsEvt);

    LE_DEBUG("[ApiHms] Modem event received: type=%d severity=%d",
             static_cast<int>(eventType),
             static_cast<int>(eventSeverity));

}

} // namespace ecall
