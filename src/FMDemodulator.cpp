#include "FMDemodulator.h"
#include <cmath>

FMDemodulator::FMDemodulator() { reset(); }

void FMDemodulator::reset() {
  prev_ = {1.0f, 0.0f};
  have_prev_ = false;
}

size_t FMDemodulator::process(const std::complex<float>* in, size_t n_in,
                              float* out, size_t out_cap) {
  size_t n = (n_in < out_cap) ? n_in : out_cap;
  for (size_t i = 0; i < n; ++i) {
    std::complex<float> x = in[i];
    if (!have_prev_) {
      prev_ = x;
      have_prev_ = true;
      out[i] = 0.0f;
      continue;
    }
    // Quadrature discriminator: angle(conj(prev)*x)
    float re = (prev_.real() * x.real()) + (prev_.imag() * x.imag());
    float im = (prev_.real() * x.imag()) - (prev_.imag() * x.real());
    out[i] = std::atan2(im, re);
    prev_ = x;
  }
  return n;
}
