#include "gamelib_util_types.h"

void NetSmallStats::Draw(float, float, float, float, NetSmallStats::eInfo) const {
}

void NetSample::Reset() {
    values[0] = 0;
    values[1] = 0;
    values[2] = 0;
    values[3] = 0;
}

void NetSample::operator+=(NetSample const &other) {
    values[0] += other.values[0];
    values[1] += other.values[1];
    values[2] += other.values[2];
    values[3] += other.values[3];
}

void NetSample::operator-=(NetSample const &other) {
    values[0] -= other.values[0];
    values[1] -= other.values[1];
    values[2] -= other.values[2];
    values[3] -= other.values[3];
}

void NetSample::Max(NetSample const &other) {
    values[0] = values[0] < other.values[0] ? other.values[0] : values[0];
    values[1] = values[1] < other.values[1] ? other.values[1] : values[1];
    values[2] = values[2] < other.values[2] ? other.values[2] : values[2];
    values[3] = values[3] < other.values[3] ? other.values[3] : values[3];
}

void NetStats::Draw(float, float, float, float, NetSmallStats::eInfo) const {
}

void NetStats::Update() {
}
