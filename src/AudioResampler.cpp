#include "AudioResampler.h"
#include "dsp/FirDesign.h"
#include <algorithm>
#include <cmath>

AudioResampler::AudioResampler(double fs_in, uint32_t decim_to_48k, float deemph_tau_s, float audio_cut_hz)
  : fs_in_(fs_in), decim_(decim_to_48k) {

  fs_out_ = fs_in_ / double(decim_);

  // 1st order de-emphasis: y[n] = y[n-1] + a*(x[n] - y[n-1])
  // a = dt / (tau + dt)
  double dt = 1.0 / fs_in_;
  a_ = float(dt / (double(deemph_tau_s) + dt));
  y_ = 0.0f;

  // Audio lowpass and decimator
  int ntaps = 161;
  auto taps = design_lowpass(float(fs_in_), audio_cut_hz, ntaps);
  dec_ = FIRDecimatorR(taps, decim_);
}

void AudioResampler::reset() { y_ = 0.0f; dec_.reset(); }

size_t AudioResampler::process(const float* in_mpx, size_t n_in,
                               int16_t* out_pcm, size_t out_cap) {
  // de-emphasis in place into temp
  tmp_.resize(n_in);
  for (size_t i = 0; i < n_in; ++i) {
    y_ = y_ + a_ * (in_mpx[i] - y_);
    tmp_[i] = y_;
  }

  // lowpass + decimate
  tmp2_.resize(out_cap);
  size_t n_out = dec_.process(tmp_.data(), n_in, tmp2_.data(), out_cap);

  // scale to int16
  for (size_t i = 0; i < n_out; ++i) {
    float v = tmp2_[i] * gain_;
    v = std::max(-1.0f, std::min(1.0f, v));
    out_pcm[i] = int16_t(std::lrintf(v * 32767.0f));
  }
  return n_out;
}

double AudioResampler::fs_out() const { return fs_out_; }
void AudioResampler::set_audio_gain(float g) { gain_ = g; }
