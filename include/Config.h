#pragma once
#include <cstdint>
#include <string>

struct ReceiverConfig {
  double rf_freq_hz = 99.9e6;
  double sample_rate_hz = 9.6e6;

  // HackRF gains
  // hackrf_set_lna_gain: 0..40 step 8
  // hackrf_set_vga_gain: 0..62 step 2
  uint32_t lna_gain_db = 16;
  uint32_t vga_gain_db = 20;

  // DSP
  uint32_t rf_decim = 50;          // 9.6e6 / 50 = 192000
  uint32_t audio_decim = 4;        // 192000 / 4 = 48000
  float channel_cut_hz = 100000.0f;
  float deemph_tau_s = 50e-6f;     // Riyadh typically follows ITU Region 1 (50 us)
  float audio_cut_hz = 16000.0f;

  // Output
  std::string wav_path = "out.wav";
  bool write_wav = true;

  // RDS
  bool enable_rds = true;
};
