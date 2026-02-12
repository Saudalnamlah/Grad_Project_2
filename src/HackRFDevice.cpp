#include "HackRFDevice.h"
#include "Logging.h"

HackRFDevice::~HackRFDevice() { close(); }

bool HackRFDevice::open() {
  if (dev_) return true;

  int r = hackrf_init();
  if (r != HACKRF_SUCCESS) {
    log_msg(LogLevel::Error, "hackrf_init failed: %s", hackrf_error_name((hackrf_error)r));
    return false;
  }

  r = hackrf_open(&dev_);
  if (r != HACKRF_SUCCESS || !dev_) {
    log_msg(LogLevel::Error, "hackrf_open failed: %s", hackrf_error_name((hackrf_error)r));
    hackrf_exit();
    dev_ = nullptr;
    return false;
  }
  log_msg(LogLevel::Info, "HackRF opened");
  return true;
}

void HackRFDevice::close() {
  stop_rx();
  if (dev_) {
    hackrf_close(dev_);
    dev_ = nullptr;
  }
  hackrf_exit();
}

bool HackRFDevice::configure(double freq_hz, double sample_rate_hz, uint32_t lna_gain_db, uint32_t vga_gain_db) {
  if (!dev_) return false;

  int r = hackrf_set_sample_rate(dev_, sample_rate_hz);
  if (r != HACKRF_SUCCESS) {
    log_msg(LogLevel::Error, "set_sample_rate failed: %s", hackrf_error_name((hackrf_error)r));
    return false;
  }

  r = hackrf_set_freq(dev_, uint64_t(freq_hz));
  if (r != HACKRF_SUCCESS) {
    log_msg(LogLevel::Error, "set_freq failed: %s", hackrf_error_name((hackrf_error)r));
    return false;
  }

  if (!set_lna_gain(lna_gain_db)) return false;
  if (!set_vga_gain(vga_gain_db)) return false;

  // baseband filter bandwidth, pick a safe value near 1.75 MHz for FM capture bandwidth margin
  r = hackrf_set_baseband_filter_bandwidth(dev_, 1750000);
  if (r != HACKRF_SUCCESS) {
    log_msg(LogLevel::Warn, "set_baseband_filter_bandwidth failed: %s", hackrf_error_name((hackrf_error)r));
  }
  return true;
}

bool HackRFDevice::start_rx(RxCallback cb) {
  if (!dev_ || running_) return false;
  cb_ = std::move(cb);
  int r = hackrf_start_rx(dev_, &HackRFDevice::rx_callback_static, this);
  if (r != HACKRF_SUCCESS) {
    log_msg(LogLevel::Error, "start_rx failed: %s", hackrf_error_name((hackrf_error)r));
    return false;
  }
  running_ = true;
  return true;
}

void HackRFDevice::stop_rx() {
  if (!dev_ || !running_) return;
  hackrf_stop_rx(dev_);
  running_ = false;
}

bool HackRFDevice::set_frequency(double freq_hz) {
  if (!dev_) return false;
  int r = hackrf_set_freq(dev_, uint64_t(freq_hz));
  if (r != HACKRF_SUCCESS) {
    log_msg(LogLevel::Error, "set_freq failed: %s", hackrf_error_name((hackrf_error)r));
    return false;
  }
  return true;
}

bool HackRFDevice::set_lna_gain(uint32_t db) {
  if (!dev_) return false;
  int r = hackrf_set_lna_gain(dev_, db);
  if (r != HACKRF_SUCCESS) {
    log_msg(LogLevel::Error, "set_lna_gain failed: %s", hackrf_error_name((hackrf_error)r));
    return false;
  }
  return true;
}

bool HackRFDevice::set_vga_gain(uint32_t db) {
  if (!dev_) return false;
  int r = hackrf_set_vga_gain(dev_, db);
  if (r != HACKRF_SUCCESS) {
    log_msg(LogLevel::Error, "set_vga_gain failed: %s", hackrf_error_name((hackrf_error)r));
    return false;
  }
  return true;
}

int HackRFDevice::rx_callback_static(hackrf_transfer* transfer) {
  auto* self = reinterpret_cast<HackRFDevice*>(transfer->rx_ctx);
  return self->rx_callback(transfer);
}

int HackRFDevice::rx_callback(hackrf_transfer* transfer) {
  if (cb_ && transfer && transfer->buffer && transfer->valid_length > 0) {
    cb_(transfer->buffer, size_t(transfer->valid_length));
  }
  return 0;
}
