#include "dsp/FIRDecimator.h"
#include <algorithm>

FIRDecimatorC::FIRDecimatorC(const std::vector<float>& taps, uint32_t decim)
  : taps_(taps), delay_(taps.size()), decim_(decim), phase_(0) {}

void FIRDecimatorC::reset() {
  std::fill(delay_.begin(), delay_.end(), std::complex<float>(0,0));
  di_ = 0;
  phase_ = 0;
}

size_t FIRDecimatorC::process(const std::complex<float>* in, size_t n_in, std::complex<float>* out, size_t out_cap) {
  size_t out_n = 0;
  size_t nt = taps_.size();
  for (size_t i = 0; i < n_in; ++i) {
    delay_[di_] = in[i];
    di_ = (di_ + 1) % nt;

    if (phase_ == 0) {
      if (out_n >= out_cap) break;
      std::complex<float> acc(0,0);
      size_t idx = di_;
      for (size_t k = 0; k < nt; ++k) {
        idx = (idx == 0) ? (nt - 1) : (idx - 1);
        acc += delay_[idx] * taps_[k];
      }
      out[out_n++] = acc;
    }
    phase_++;
    if (phase_ >= decim_) phase_ = 0;
  }
  return out_n;
}

FIRDecimatorR::FIRDecimatorR(const std::vector<float>& taps, uint32_t decim)
  : taps_(taps), delay_(taps.size()), decim_(decim), phase_(0) {}

void FIRDecimatorR::reset() {
  std::fill(delay_.begin(), delay_.end(), 0.0f);
  di_ = 0;
  phase_ = 0;
}

size_t FIRDecimatorR::process(const float* in, size_t n_in, float* out, size_t out_cap) {
  size_t out_n = 0;
  size_t nt = taps_.size();
  for (size_t i = 0; i < n_in; ++i) {
    delay_[di_] = in[i];
    di_ = (di_ + 1) % nt;

    if (phase_ == 0) {
      if (out_n >= out_cap) break;
      float acc = 0.0f;
      size_t idx = di_;
      for (size_t k = 0; k < nt; ++k) {
        idx = (idx == 0) ? (nt - 1) : (idx - 1);
        acc += delay_[idx] * taps_[k];
      }
      out[out_n++] = acc;
    }
    phase_++;
    if (phase_ >= decim_) phase_ = 0;
  }
  return out_n;
}
