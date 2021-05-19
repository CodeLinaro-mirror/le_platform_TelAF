/*
 *  Copyright (c) 2021 The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "legato.h"
#include "interfaces.h"
#include <telux/tel/PhoneFactory.hpp>
#include "telux/common/CommonDefines.hpp"
#include "tafSvcIF.hpp"

using namespace telux::tel;
using namespace telux::common;

namespace telux {
    namespace tafsvc {

        typedef struct
        {
            taf_sim_Id_t      simId;   ///< SIM identififier
            taf_sim_States_t  state;   ///< SIM state
        }
        sim_event_t;

        class tafCardListener : public telux::tel::ICardListener {
            public:
                void onCardInfoChanged(int slotId) override;
        };

        class taf_sim :public ITafSvc {
            public:
                void Init(void);
                static taf_sim &GetInstance();
                taf_sim() {};
                ~taf_sim() {};

                std::shared_ptr<telux::tel::ICardManager> cardManager;
                std::shared_ptr<telux::tel::ICardListener> cardListener;
                std::vector<std::shared_ptr<telux::tel::ICard>> cards;


                int slot = DEFAULT_SLOT_ID;
                le_event_Id_t NewStateEventId;

                void RemoveStateHandler(taf_sim_NewStateHandlerRef_t handlerRef);
                taf_sim_States_t getState(taf_sim_Id_t simId);
                const char* cardStateToString(CardState state);
                taf_sim_States_t cardStateToTafSimStates(CardState state);
                static void FirstLayerNewSimStateHandler(void* reportPtr, void* secondLayerHandlerFunc);
                taf_sim_NewStateHandlerRef_t AddStateHandler(taf_sim_NewStateHandlerFunc_t handlerPtr,
                        void* contextPtr);
        };
    }
}
