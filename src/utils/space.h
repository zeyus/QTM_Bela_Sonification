#ifndef SPACE_UTILS_H
#define SPACE_UTILS_H

#include <cmath>
#include <array>
#include <libraries/math_neon/math_neon.h>

float pos_to_freq(float pos, float min, float max, float min_freq, float max_freq) {
  return min_freq + (pos - min) * (max_freq - min_freq) / (max - min);
}

// calculates the distance between two x positions along a given axis segment
// and returns an amplitude modulated by the distance.
// if it's beyond the threshold percentage, it returns 0.
float sync_to_amp(float x1, float x2, float xmin, float xmax, float threshold = 0.1f) {
  float dist = fabs(x1 - x2);
  float max_dist = (xmax - xmin) * threshold;
  if (dist > max_dist) return 0;
  return 1 - (dist / max_dist);
}


float calc_center_freq(float f1, float f2) {
  if (f1/f2 < 1.1) {
    return sqrtf_neon(f1 * f2);
  } else {
    return (f1 + f2) / 2;
  }
}

// returns two frequencies that deviate from a center frequency
// if x1 and x2 are equal, both f1 and f2 will be the center frequency
// if x1 is greater than x2, f1 will be higher than f2
// if x1 is less than x2, f1 will be lower than f2
std::array<float, 2> sync_to_freq(float x1, float x2, float xmin, float xmax, float min_freq, float max_freq, float threshold = 0.3f) {
  const float center_freq = calc_center_freq(min_freq, max_freq);
  // float f1 = center_freq;
  // float f2 = center_freq;

  const float dist = x1 - x2;
  const float max_dist = (xmax - xmin) * threshold;
  if (fabs(dist) >= max_dist) {
    if (x1 > x2) return {{max_freq, min_freq}};
    else return {{min_freq, max_freq}};
  }

  // calculate delta from center frequency
  const float delta = (max_freq - min_freq) * (dist / max_dist / 2);

  // calculate f1 and f2
  const float f1 = center_freq + delta;
  const float f2 = center_freq - delta;

  // return the frequencies
  return {{f1, f2}};
}

#endif
