#pragma once
#include <complex>
#include <cstddef>

class FMDemodulator {
public:
  FMDemodulator();
  void reset();

  size_t process(const std::complex<float>* in, size_t n_in,
                 float* out, size_t out_cap);

private:
  std::complex<float> prev_{1.0f, 0.0f};
  bool have_prev_ = false;
};
