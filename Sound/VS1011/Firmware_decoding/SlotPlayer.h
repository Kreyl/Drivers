/*
 * SlotPlayer.h
 *
 *  Created on: 2 ��� 2018 �.
 *      Author: Kreyl
 */

#pragma once

#include <inttypes.h>

namespace SlotPlayer {
    void Init();
    void Start(uint8_t SlotN, uint16_t Volume, const char* Emo, bool Repeat);
    void SetVolume(uint8_t SlotN, uint16_t Volume);
    void SetVolumeForAll(uint16_t Volume);
    void Stop(uint8_t SlotN);
};
