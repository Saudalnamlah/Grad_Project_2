#pragma once
#include <complex>
#include <cstddef>
#include <cstdint>
#include "dsp/FIRDecimator.h"

class ChannelFilter {
public:
  ChannelFilter(double fs_in, uint32_t decim, float cut_hz);

  size_t process(const std::complex<float>* in, size_t n_in,
                 std::complex<float>* out, size_t out_cap);

  double fs_out() const;

private:
  double fs_in_ = 0;
  double fs_out_ = 0;
  uint32_t decim_ = 1;
  FIRDecimatorC dec_;
};
