/*
 * Effects.h
 *
 *  Created on: 5 ???. 2019 ?.
 *      Author: Kreyl
 */

#pragma once

#include "color.h"

namespace Eff {
void Init();

//const
void StartFlaming();

void SetColor(Color_t AClr);

void FadeIn();
void FadeOut();
}
