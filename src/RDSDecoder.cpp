#include "RDSDecoder.h"
#include "dsp/FirDesign.h"
#include "dsp/NCO.h"
#include "Logging.h"
#include <array>
#include <cstring>
#include <algorithm>

static uint16_t rds_crc10(uint16_t data) {
  // RDS CRC10, generator: x^10 + x^8 + x^7 + x^5 + x^4 + x^3 + 1
  // poly bits (excluding x^10): 0x1B9, full 0x5B9 representation is common.
  // Implementation uses 0x1B9 on the 10-bit remainder.
  const uint16_t poly = 0x1B9;
  uint32_t reg = (uint32_t)data << 10;
  for (int i = 0; i < 16; ++i) {
    if (reg & (1u << (25 - i))) {
      reg ^= (uint32_t)poly << (15 - i);
    }
  }
  return uint16_t(reg & 0x3FF);
}

static constexpr uint16_t OFF_A  = 0x0FC;
static constexpr uint16_t OFF_B  = 0x198;
static constexpr uint16_t OFF_C  = 0x168;
static constexpr uint16_t OFF_D  = 0x1B4;
static constexpr uint16_t OFF_Cp = 0x350;

RDSDecoder::RDSDecoder(double fs_mpx)
  : fs_(fs_mpx) {
  // Bandpass around 57 kHz in MPX (fs about 192 kHz)
  int ntaps_bp = 161;
  bp_ = FIRDecimatorR(design_bandpass(float(fs_), 54000.0f, 60000.0f, ntaps_bp), 1);

  // After mixing down, lowpass 3 kHz
  int ntaps_lp = 161;
  lp_ = FIRDecimatorR(design_lowpass(float(fs_), 3000.0f, ntaps_lp), 1);

  nco_.set(fs_, 57000.0);
  reset();
}

void RDSDecoder::reset() {
  bp_.reset();
  lp_.reset();
  chip_phase_ = 0.0;
  chip_inc_ = 2375.0 / fs_; // chips per second
  have_sync_ = false;
  block_idx_ = 0;
  shift_ = 0;
  bits_in_shift_ = 0;
  std::fill(ps_.begin(), ps_.end(), ' ');
  ps_[8] = '\0';
  last_ps_ = ps_;
}

static inline int slicer(float x) { return (x >= 0.0f) ? 1 : 0; }

void RDSDecoder::process(const float* mpx, size_t n) {
  if (!enabled_) return;

  // 1) bandpass 57 kHz
  tmp1_.resize(n);
  bp_.process(mpx, n, tmp1_.data(), n);

  // 2) mix down
  tmpc_.resize(n);
  for (size_t i = 0; i < n; ++i) {
    auto e = nco_.next_conj();
    tmpc_[i] = std::complex<float>(tmp1_[i], 0.0f) * e;
  }

  // 3) take real and lowpass
  tmp2_.resize(n);
  for (size_t i = 0; i < n; ++i) tmp2_[i] = tmpc_[i].real();
  lp_.process(tmp2_.data(), n, tmp2_.data(), n);

  // 4) open-loop chip sampling at 2375 Hz, then Manchester decode
  for (size_t i = 0; i < n; ++i) {
    chip_phase_ += chip_inc_;
    if (chip_phase_ >= 1.0) {
      chip_phase_ -= 1.0;

      int chip = slicer(tmp2_[i]);
      chip_buf_[chip_buf_len_ & 1] = chip;
      chip_buf_len_++;

      if ((chip_buf_len_ & 1) == 0) {
        // Manchester: 01 => 1, 10 => 0, else invalid
        int a = chip_buf_[0], b = chip_buf_[1];
        if (a == 0 && b == 1) push_bit(1);
        else if (a == 1 && b == 0) push_bit(0);
        else {
          // lost timing, reset bit assembly
          have_sync_ = false;
          block_idx_ = 0;
          bits_in_shift_ = 0;
        }
      }
    }
  }
}

void RDSDecoder::push_bit(int bit) {
  // Differential decoding (RDS uses differential at the biphase level)
  int dbit = bit ^ last_bit_;
  last_bit_ = bit;

  shift_ = ((shift_ << 1) | (uint32_t(dbit) & 1u)) & 0x03FFFFFFu;
  if (bits_in_shift_ < 26) bits_in_shift_++;

  if (bits_in_shift_ < 26) return;

  uint16_t data = uint16_t((shift_ >> 10) & 0xFFFFu);
  uint16_t check = uint16_t(shift_ & 0x3FFu);
  uint16_t crc = rds_crc10(data);
  uint16_t syndrome = check ^ crc;

  uint8_t which = 255;
  if (syndrome == OFF_A) which = 0;
  else if (syndrome == OFF_B) which = 1;
  else if (syndrome == OFF_C) which = 2;
  else if (syndrome == OFF_D) which = 3;
  else if (syndrome == OFF_Cp) which = 2;

  if (!have_sync_) {
    if (which == 0) {
      have_sync_ = true;
      block_idx_ = 0;
      blocks_[0] = data;
      block_idx_ = 1;
    }
    return;
  }

  // Expect A,B,C,D in order
  blocks_[block_idx_] = data;
  block_idx_++;

  if (block_idx_ == 4) {
    handle_group(blocks_[0], blocks_[1], blocks_[2], blocks_[3]);
    block_idx_ = 0;
  }
}

void RDSDecoder::handle_group(uint16_t A, uint16_t B, uint16_t C, uint16_t D) {
  (void)A; (void)C;
  uint8_t group_type = uint8_t((B >> 12) & 0xF);
  uint8_t version = uint8_t((B >> 11) & 0x1);

  if (group_type == 0) {
    // Program Service name in block D, segment index in B bits 0..1
    uint8_t seg = uint8_t(B & 0x3);
    char c1 = char((D >> 8) & 0xFF);
    char c2 = char(D & 0xFF);
    if (seg < 4) {
      ps_[seg * 2] = (c1 >= 32 && c1 <= 126) ? c1 : ' ';
      ps_[seg * 2 + 1] = (c2 >= 32 && c2 <= 126) ? c2 : ' ';
      ps_[8] = '\0';
      if (ps_ != last_ps_) {
        last_ps_ = ps_;
        station_ps_ = std::string(ps_.data());
        log_msg(LogLevel::Info, "RDS PS: %s (group 0%c)", station_ps_.c_str(), version ? 'B' : 'A');
      }
    }
  }
}

std::string RDSDecoder::program_service() const { return station_ps_; }
void RDSDecoder::set_enabled(bool en) { enabled_ = en; }
