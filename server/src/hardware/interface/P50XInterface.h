//
// Created by micha on 17.11.2023.
//

#ifndef TRAINTASTIC_SERVER_P50XINTERFACE_H
#define TRAINTASTIC_SERVER_P50XINTERFACE_H


#include "interface.hpp"
#include "../decoder/decodercontroller.hpp"
#include "../input/inputcontroller.hpp"
#include "../output/outputcontroller.hpp"

namespace p50x {

    class P50XInterface final : public Interface, public DecoderController, public InputController, public OutputController
    {
    CLASS_ID("interface.p50x")
    DEFAULT_ID("p50x")
    CREATE_DEF(P50XInterface)

    };
}

#endif //TRAINTASTIC_SERVER_P50XINTERFACE_H
