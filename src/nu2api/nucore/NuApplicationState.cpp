#include "nu2api/nucore/nucore.hpp"

NuApplicationState::NuApplicationState() : status(NUAPPLICATIONSTATUS_IDLE) {
}

NuApplicationState::~NuApplicationState() {
}

void NuApplicationState::SetStatus(NUAPPLICATIONSTATUS value) {
    status = value;
}

NUAPPLICATIONSTATUS NuApplicationState::GetStatus() const {
    return status;
}
