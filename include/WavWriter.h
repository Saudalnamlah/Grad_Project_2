#pragma once
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

class WavWriter {
public:
  WavWriter() = default;
  ~WavWriter();

  bool open(const std::string& path, uint32_t sample_rate, uint16_t channels);
  void write_i16(const int16_t* data, size_t samples_per_channel);
  void close();

private:
  void write_header_placeholder();
  void finalize_header();

  std::FILE* f_ = nullptr;
  uint32_t sample_rate_ = 48000;
  uint16_t channels_ = 1;
  uint32_t data_bytes_ = 0;
};
