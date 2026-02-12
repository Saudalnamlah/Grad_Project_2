#pragma once
#include <complex>
#include <cmath>

class NCO {
public:
  void set(double fs, double f) {
    fs_ = fs; f_ = f;
    inc_ = 2.0 * M_PI * f_ / fs_;
  }

  std::complex<float> next_conj() {
    float c = std::cos(phase_);
    float s = std::sin(phase_);
    phase_ += inc_;
    if (phase_ > 2.0 * M_PI) phase_ -= 2.0 * M_PI;
    return {c, -s}; // conjugate
  }

private:
  double fs_ = 0;
  double f_ = 0;
  double phase_ = 0;
  double inc_ = 0;
};
