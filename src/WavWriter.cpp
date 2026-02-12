#include "WavWriter.h"
#include <cstring>

static void write_u32le(std::FILE* f, uint32_t v) {
  uint8_t b[4] = { uint8_t(v & 0xFF), uint8_t((v >> 8) & 0xFF), uint8_t((v >> 16) & 0xFF), uint8_t((v >> 24) & 0xFF) };
  std::fwrite(b, 1, 4, f);
}
static void write_u16le(std::FILE* f, uint16_t v) {
  uint8_t b[2] = { uint8_t(v & 0xFF), uint8_t((v >> 8) & 0xFF) };
  std::fwrite(b, 1, 2, f);
}

WavWriter::~WavWriter() { close(); }

bool WavWriter::open(const std::string& path, uint32_t sample_rate, uint16_t channels) {
  close();
  f_ = std::fopen(path.c_str(), "wb");
  if (!f_) return false;
  sample_rate_ = sample_rate;
  channels_ = channels;
  data_bytes_ = 0;
  write_header_placeholder();
  return true;
}

void WavWriter::write_header_placeholder() {
  // RIFF header with placeholders
  std::fwrite("RIFF", 1, 4, f_);
  write_u32le(f_, 0); // size placeholder
  std::fwrite("WAVE", 1, 4, f_);

  std::fwrite("fmt ", 1, 4, f_);
  write_u32le(f_, 16);             // PCM fmt chunk size
  write_u16le(f_, 1);              // PCM
  write_u16le(f_, channels_);
  write_u32le(f_, sample_rate_);
  uint32_t byte_rate = sample_rate_ * channels_ * 2;
  write_u32le(f_, byte_rate);
  uint16_t block_align = channels_ * 2;
  write_u16le(f_, block_align);
  write_u16le(f_, 16);             // bits

  std::fwrite("data", 1, 4, f_);
  write_u32le(f_, 0); // data size placeholder
}

void WavWriter::write_i16(const int16_t* data, size_t samples_per_channel) {
  if (!f_) return;
  size_t total = samples_per_channel * channels_;
  std::fwrite(data, sizeof(int16_t), total, f_);
  data_bytes_ += uint32_t(total * sizeof(int16_t));
}

void WavWriter::finalize_header() {
  if (!f_) return;
  long end = std::ftell(f_);
  std::fseek(f_, 4, SEEK_SET);
  write_u32le(f_, 36 + data_bytes_);
  std::fseek(f_, 40, SEEK_SET);
  write_u32le(f_, data_bytes_);
  std::fseek(f_, end, SEEK_SET);
}

void WavWriter::close() {
  if (!f_) return;
  finalize_header();
  std::fclose(f_);
  f_ = nullptr;
}
