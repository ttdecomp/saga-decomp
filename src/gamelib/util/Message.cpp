#include "gamelib_util_types.h"

NetMessage::MessageData NetMessage::sm_poolMessageData[512];

void NetMessage::RaiseError() {
    theSession->error = 0xa0001000;
}

void NetMessage::DebugPrint() const {
}
