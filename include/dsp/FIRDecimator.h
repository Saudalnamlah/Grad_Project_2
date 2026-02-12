#pragma once
#include <vector>
#include <complex>
#include <cstddef>
#include <cstdint>

class FIRDecimatorC {
public:
  FIRDecimatorC() = default;
  FIRDecimatorC(const std::vector<float>& taps, uint32_t decim);

  void reset();
  // in: complex float samples
  // out: writes decimated samples
  size_t process(const std::complex<float>* in, size_t n_in, std::complex<float>* out, size_t out_cap);

private:
  std::vector<float> taps_;
  std::vector<std::complex<float>> delay_;
  size_t di_ = 0;
  uint32_t decim_ = 1;
  uint32_t phase_ = 0;
};

class FIRDecimatorR {
public:
  FIRDecimatorR() = default;
  FIRDecimatorR(const std::vector<float>& taps, uint32_t decim);

  void reset();
  size_t process(const float* in, size_t n_in, float* out, size_t out_cap);

private:
  std::vector<float> taps_;
  std::vector<float> delay_;
  size_t di_ = 0;
  uint32_t decim_ = 1;
  uint32_t phase_ = 0;
};
