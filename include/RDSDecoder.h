#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <complex>
#include "dsp/FIRDecimator.h"
#include "dsp/NCO.h"

class RDSDecoder {
public:
  explicit RDSDecoder(double fs_mpx);

  void reset();
  void process(const float* mpx, size_t n);

  std::string program_service() const;
  void set_enabled(bool en);

private:
  void push_bit(int bit);
  void handle_group(uint16_t A, uint16_t B, uint16_t C, uint16_t D);

  double fs_ = 0;
  bool enabled_ = true;

  FIRDecimatorR bp_;
  FIRDecimatorR lp_;
  NCO nco_;

  std::vector<float> tmp1_;
  std::vector<float> tmp2_;
  std::vector<std::complex<float>> tmpc_;

  double chip_phase_ = 0.0;
  double chip_inc_ = 0.0;

  int chip_buf_[2] = {0,0};
  uint32_t chip_buf_len_ = 0;

  int last_bit_ = 0;

  bool have_sync_ = false;
  uint16_t blocks_[4] = {0,0,0,0};
  uint8_t block_idx_ = 0;

  uint32_t shift_ = 0;
  uint8_t bits_in_shift_ = 0;

  std::array<char, 9> ps_{};
  std::array<char, 9> last_ps_{};
  std::string station_ps_;
};
