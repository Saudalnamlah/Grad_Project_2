#include "Config.h"
#include "Logging.h"
#include "FMReceiver.h"
#include "AudioRingBuffer.h"
#include "WavWriter.h"
#include <chrono>
#include <thread>
#include <cstring>

static void print_usage() {
  std::fprintf(stderr,
    "Usage: fm_relay --freq <MHz> [--sr <Hz>] [--lna <dB>] [--vga <dB>] [--wav <path>] [--seconds <N>]\n"
    "Defaults: freq=99.9, sr=9600000, lna=16, vga=20, wav=out.wav, seconds=20\n");
}

int main(int argc, char** argv) {
  ReceiverConfig cfg;
  int seconds = 20;

  for (int i = 1; i < argc; ++i) {
    if (!std::strcmp(argv[i], "--freq") && i + 1 < argc) cfg.rf_freq_hz = std::atof(argv[++i]) * 1e6;
    else if (!std::strcmp(argv[i], "--sr") && i + 1 < argc) cfg.sample_rate_hz = std::atof(argv[++i]);
    else if (!std::strcmp(argv[i], "--lna") && i + 1 < argc) cfg.lna_gain_db = std::atoi(argv[++i]);
    else if (!std::strcmp(argv[i], "--vga") && i + 1 < argc) cfg.vga_gain_db = std::atoi(argv[++i]);
    else if (!std::strcmp(argv[i], "--wav") && i + 1 < argc) cfg.wav_path = argv[++i];
    else if (!std::strcmp(argv[i], "--seconds") && i + 1 < argc) seconds = std::atoi(argv[++i]);
    else if (!std::strcmp(argv[i], "--no-rds")) cfg.enable_rds = false;
    else { print_usage(); return 1; }
  }

  if (cfg.rf_freq_hz < 87.5e6 || cfg.rf_freq_hz > 108.0e6) {
    log_msg(LogLevel::Error, "Frequency out of FM band");
    return 1;
  }

  AudioRingBuffer audio_rb(48000 * 10);
  FMReceiver rx(cfg, audio_rb);

  if (!rx.start()) {
    log_msg(LogLevel::Error, "Receiver start failed");
    return 1;
  }

  WavWriter wav;
  if (cfg.write_wav) {
    if (!wav.open(cfg.wav_path, 48000, 1)) {
      log_msg(LogLevel::Error, "WAV open failed");
      rx.stop();
      return 1;
    }
  }

  auto t0 = std::chrono::steady_clock::now();
  std::vector<int16_t> out(48000 / 2);

  while (true) {
    auto now = std::chrono::steady_clock::now();
    int elapsed = int(std::chrono::duration_cast<std::chrono::seconds>(now - t0).count());
    if (elapsed >= seconds) break;

    size_t n = audio_rb.pop(out.data(), out.size());
    if (n > 0 && cfg.write_wav) wav.write_i16(out.data(), n);

    if ((elapsed % 5) == 0) {
      auto ps = rx.program_service();
      if (!ps.empty()) log_msg(LogLevel::Info, "Status: %.3f MHz, PS=%s", cfg.rf_freq_hz / 1e6, ps.c_str());
    }
  }

  rx.stop();
  audio_rb.stop();
  wav.close();

  log_msg(LogLevel::Info, "Done. Wrote %s", cfg.wav_path.c_str());
  return 0;
}
