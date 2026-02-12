#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include "dsp/FIRDecimator.h"

class AudioResampler {
public:
  AudioResampler(double fs_in, uint32_t decim_to_48k, float deemph_tau_s, float audio_cut_hz);
  void reset();

  size_t process(const float* in_mpx, size_t n_in,
                 int16_t* out_pcm, size_t out_cap);

  double fs_out() const;
  void set_audio_gain(float g);

private:
  double fs_in_ = 0;
  double fs_out_ = 0;
  uint32_t decim_ = 1;

  float a_ = 0.0f;
  float y_ = 0.0f;

  float gain_ = 0.8f;

  FIRDecimatorR dec_;
  std::vector<float> tmp_;
  std::vector<float> tmp2_;
};
