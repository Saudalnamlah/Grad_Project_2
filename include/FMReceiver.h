#pragma once
#include "Config.h"
#include "HackRFDevice.h"
#include "AudioRingBuffer.h"
#include "ChannelFilter.h"
#include "FMDemodulator.h"
#include "AudioResampler.h"
#include "RDSDecoder.h"
#include <complex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <string>

class FMReceiver {
public:
  FMReceiver(const ReceiverConfig& cfg, AudioRingBuffer& audio_out);
  ~FMReceiver();

  bool start();
  void stop();

  bool set_frequency_mhz(double mhz);
  bool set_lna_gain_db(uint32_t db);
  bool set_vga_gain_db(uint32_t db);
  void set_audio_gain(float g);

  std::string program_service() const;

private:
  void on_hackrf_iq(const uint8_t* iq, size_t bytes);
  void worker();

  ReceiverConfig cfg_;
  AudioRingBuffer& audio_out_;

  HackRFDevice dev_;

  ChannelFilter chan_;
  FMDemodulator demod_;
  AudioResampler audio_;
  RDSDecoder rds_;

  std::atomic<bool> running_{false};
  std::thread th_;

  // IQ queue (byte ring)
  std::mutex m_;
  std::condition_variable cv_;
  std::vector<uint8_t> q_;
  size_t q_r_ = 0;
  size_t q_w_ = 0;
  size_t q_size_ = 0;
  bool q_stop_ = false;

  void q_push(const uint8_t* p, size_t n);
  size_t q_pop(uint8_t* out, size_t nmax);
};
