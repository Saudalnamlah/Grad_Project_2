#pragma once
#include <vector>
#include <cmath>

inline std::vector<float> design_lowpass(float fs, float fc, int ntaps) {
  std::vector<float> h(ntaps);
  float norm = fc / fs;
  int M = ntaps - 1;
  for (int n = 0; n < ntaps; ++n) {
    float x = float(n - M / 2.0f);
    float sinc = (x == 0.0f) ? 1.0f : std::sin(2.0f * float(M_PI) * norm * x) / (float(M_PI) * x);
    float w = 0.54f - 0.46f * std::cos(2.0f * float(M_PI) * n / M); // Hamming
    h[n] = 2.0f * norm * sinc * w;
  }
  return h;
}

inline std::vector<float> design_bandpass(float fs, float f1, float f2, int ntaps) {
  // bandpass = lp(f2) - lp(f1)
  auto lp2 = design_lowpass(fs, f2, ntaps);
  auto lp1 = design_lowpass(fs, f1, ntaps);
  std::vector<float> h(ntaps);
  for (int i = 0; i < ntaps; ++i) h[i] = lp2[i] - lp1[i];
  return h;
}
