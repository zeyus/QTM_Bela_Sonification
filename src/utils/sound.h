#ifndef SOUND_UTILS_H
#define SOUND_UTILS_H

#include <vector>
#include <libraries/math_neon/math_neon.h>

// Get the sample data for a given index, and adjust for new sample rate
float warp_sample(std::vector<float> &sample, unsigned int &index, const float base_sr, const float warp_sr, const unsigned int sample_length) {
  float warp_index = (warp_sr / base_sr) * (float)index;
  while (warp_index > (float) sample_length) warp_index -= sample_length;
  int index_prev = floor(warp_index);
  int index_next = ceil(warp_index);
  float frac = warp_index - (float) index_prev;
  float sample_prev = sample[index_prev];
  float sample_next = sample[index_next];

  return sample_prev + frac * (sample_next - sample_prev);
}


// this will probably change, but a simple function
// to get a value for a given phase and frequency
// for a sin wave
float sin_freq(float &phase, float freq, float inv_sr) {
  const float out = 0.8f * sinf_neon(phase);
  phase += 2.0f * (float)M_PI * freq * inv_sr;
  if (phase > M_PI) phase -= 2.0f * (float)M_PI;
  return out;
}



#endif