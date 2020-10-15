/*
 * Effects.h
 *
 *  Created on: 5 ???. 2019 ?.
 *      Author: Kreyl
 */

#pragma once

#include "color.h"

enum State_t { staOff, staFire, staFlash, staWhite };
extern State_t State;

namespace Eff {
void Init();

void EnterOff();
void StartFlaming();
void DoFlash();

void FadeIn();
void FadeOut();
}
