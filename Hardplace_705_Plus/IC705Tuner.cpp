#include "IC705Tuner.h"

const unsigned CIC_705Tuner::m_uTunePwr(   (255 * 25) /  100); // 2.5 Watts (need to keep it between 5 and 1 watt)
const unsigned CIC_705Tuner::m_uTunePwrMax((255 * 45) /  100); // Don't tune with greater than 4.5 Watts
const unsigned CIC_705Tuner::m_uTunePwrMin((255 * 10) /  100); // Can't tune with less than 1 Watt
const unsigned CIC_705Tuner::m_RTTY(4);                        // RTTY, CW is 3
const unsigned CIC_705Tuner::m_uFilterWidthNormal(2);
