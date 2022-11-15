#ifndef SOUND_UTILS_H
#define SOUND_UTILS_H

#include <vector>
#include <libraries/math_neon/math_neon.h>

// fade in the sample start and fade out the sample end
float warp_amp_fade_linear(const float index, const float sample_length, const float fade_length = 220.0f) {
  if (index < fade_length) {
    return index / fade_length;
  } else if (index > sample_length - fade_length) {
    return (sample_length - index) / fade_length;
  } else {
    return 1.0f;
  }
}

// integer version
float amp_fade_linear(const unsigned int index, const unsigned int sample_length, const unsigned int fade_length = 220, const float amp_depth = 1.0f) {
  if (index < fade_length) {
    return ((float) index / (float) fade_length * amp_depth) + (1.0f - amp_depth);
  } else if (index > sample_length - fade_length) {
    return ((float) (sample_length - index) / (float) fade_length * amp_depth) + (1.0f - amp_depth);
  } else {
    return 1.0f;
  }
}

// get the floored and ceiled indices of the sample and interpolate between them
float interpolate_sample(std::vector<float> &sample, const float index, const unsigned int sample_length) {
  const int index_floor = floorf_neon(index);
  const int index_ceil = ceilf_neon(index);
  const float frac = index - index_floor;
  const float sample_floor = sample[index_floor];
  const float sample_ceil = sample[index_ceil%sample_length];
  return sample_floor + (sample_ceil - sample_floor) * frac;
}

// Get the sample data for a given index, and adjust for new sample rate
float warp_sample(std::vector<float> &sample, unsigned int &index, const float warp_factor, const unsigned int sample_length) {
  const float fsample_length = (float) sample_length;
  float warp_index = warp_factor * (float)index;
  float warp_index_next = warp_factor * (float)(index + 1);
  float warp_index_prev = warp_factor * (float)(index - 1);
  if (warp_index >= fsample_length) {
    warp_index = fmodf_neon(warp_index, fsample_length);
  }
  if (warp_index_next >= fsample_length) {
    warp_index_next = fmodf_neon(warp_index_next, fsample_length);
  }
  if (warp_index_prev < 0) {
    warp_index_prev = fsample_length + fmodf_neon(warp_index_prev, sample_length);
  }

  const float fade_length = 110.0f * warp_factor;
  

  return (interpolate_sample(sample, warp_index_next, fsample_length) +
    interpolate_sample(sample, warp_index_prev, fsample_length)) /
    2 * warp_amp_fade_linear(warp_index, fsample_length, fade_length);
}

// Get the sample data for a given index, and adjust for new sample rate
float warp_read_sample(std::vector<float> &sample, float &index, const float warp_factor, const unsigned int sample_length, const bool move_read_head = true) {
  const float fsample_length = (float) sample_length;
  float warp_index_next = index + warp_factor;
  // float warp_index_prev = index - warp_factor;
  if (index >= fsample_length) {
    index = fmodf_neon(index, fsample_length);
  }
  if (warp_index_next >= fsample_length) {
    warp_index_next = fmodf_neon(warp_index_next, fsample_length);
  }

  const float fade_length = 110.0f * warp_factor;
  
  const float out = interpolate_sample(sample, index, fsample_length) * warp_amp_fade_linear(index, fsample_length, fade_length);

  if (move_read_head) {
    index = warp_index_next;
  }

  return out;
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
