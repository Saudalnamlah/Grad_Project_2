#pragma once
#include <cstdint>
#include <functional>
#include <mutex>
#include <vector>
#include <atomic>
#include <hackrf.h>

class HackRFDevice {
public:
  using RxCallback = std::function<void(const uint8_t* iq_u8, size_t bytes)>;

  HackRFDevice() = default;
  ~HackRFDevice();

  bool open();
  void close();

  bool configure(double freq_hz, double sample_rate_hz, uint32_t lna_gain_db, uint32_t vga_gain_db);
  bool start_rx(RxCallback cb);
  void stop_rx();

  bool set_frequency(double freq_hz);
  bool set_lna_gain(uint32_t db);
  bool set_vga_gain(uint32_t db);

private:
  static int rx_callback_static(hackrf_transfer* transfer);
  int rx_callback(hackrf_transfer* transfer);

  hackrf_device* dev_ = nullptr;
  std::atomic<bool> running_{false};
  RxCallback cb_;
};
