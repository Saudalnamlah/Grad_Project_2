#include "FMReceiver.h"
#include "Logging.h"
#include <algorithm>
#include <cmath>

FMReceiver::FMReceiver(const ReceiverConfig& cfg, AudioRingBuffer& audio_out)
  : cfg_(cfg),
    audio_out_(audio_out),
    chan_(cfg.sample_rate_hz, cfg.rf_decim, cfg.channel_cut_hz),
    audio_(chan_.fs_out(), cfg.audio_decim, cfg.deemph_tau_s, cfg.audio_cut_hz),
    rds_(chan_.fs_out()) {

  q_.resize(size_t(cfg_.sample_rate_hz / 10) * 2); // about 0.1s of IQ bytes
}

FMReceiver::~FMReceiver() { stop(); }

bool FMReceiver::start() {
  if (running_) return true;
  if (!dev_.open()) return false;

  if (!dev_.configure(cfg_.rf_freq_hz, cfg_.sample_rate_hz, cfg_.lna_gain_db, cfg_.vga_gain_db)) return false;

  rds_.set_enabled(cfg_.enable_rds);

  q_stop_ = false;
  q_r_ = q_w_ = q_size_ = 0;

  running_ = true;
  th_ = std::thread(&FMReceiver::worker, this);

  bool ok = dev_.start_rx([this](const uint8_t* iq, size_t bytes){ on_hackrf_iq(iq, bytes); });
  if (!ok) {
    running_ = false;
    q_stop_ = true;
    cv_.notify_all();
    if (th_.joinable()) th_.join();
    return false;
  }

  log_msg(LogLevel::Info, "RX started at %.3f MHz", cfg_.rf_freq_hz / 1e6);
  return true;
}

void FMReceiver::stop() {
  if (!running_) return;
  dev_.stop_rx();
  {
    std::lock_guard<std::mutex> lock(m_);
    q_stop_ = true;
  }
  cv_.notify_all();
  if (th_.joinable()) th_.join();
  dev_.close();
  running_ = false;
}

bool FMReceiver::set_frequency_mhz(double mhz) {
  double hz = mhz * 1e6;
  if (mhz < 87.5 || mhz > 108.0) return false;
  cfg_.rf_freq_hz = hz;
  return dev_.set_frequency(hz);
}

bool FMReceiver::set_lna_gain_db(uint32_t db) { cfg_.lna_gain_db = db; return dev_.set_lna_gain(db); }
bool FMReceiver::set_vga_gain_db(uint32_t db) { cfg_.vga_gain_db = db; return dev_.set_vga_gain(db); }
void FMReceiver::set_audio_gain(float g) { audio_.set_audio_gain(g); }

std::string FMReceiver::program_service() const { return rds_.program_service(); }

void FMReceiver::on_hackrf_iq(const uint8_t* iq, size_t bytes) {
  q_push(iq, bytes);
}

void FMReceiver::q_push(const uint8_t* p, size_t n) {
  std::unique_lock<std::mutex> lock(m_);
  if (q_stop_) return;

  for (size_t i = 0; i < n; ++i) {
    if (q_size_ == q_.size()) {
      q_r_ = (q_r_ + 1) % q_.size();
      --q_size_;
    }
    q_[q_w_] = p[i];
    q_w_ = (q_w_ + 1) % q_.size();
    ++q_size_;
  }
  cv_.notify_one();
}

size_t FMReceiver::q_pop(uint8_t* out, size_t nmax) {
  std::unique_lock<std::mutex> lock(m_);
  cv_.wait(lock, [&]{ return q_stop_ || q_size_ >= nmax; });
  if (q_stop_) return 0;

  size_t n = nmax;
  for (size_t i = 0; i < n; ++i) {
    out[i] = q_[q_r_];
    q_r_ = (q_r_ + 1) % q_.size();
  }
  q_size_ -= n;
  return n;
}

void FMReceiver::worker() {
  // Work in chunks aligned to HackRF USB transfers.
  const size_t bytes_per_chunk = 262144; // 256 KB
  std::vector<uint8_t> raw(bytes_per_chunk);

  // Convert to complex float
  std::vector<std::complex<float>> iqf(bytes_per_chunk / 2);

  // After RF decim, size reduces by rf_decim
  size_t max_decim_out = iqf.size() / cfg_.rf_decim + 8;
  std::vector<std::complex<float>> iqc(max_decim_out);

  std::vector<float> mpx(max_decim_out);
  std::vector<int16_t> pcm(max_decim_out);

  while (true) {
    size_t got = q_pop(raw.data(), bytes_per_chunk);
    if (got == 0) break;

    size_t n_iq = got / 2;
    for (size_t i = 0; i < n_iq; ++i) {
      float I = (float(raw[2*i]) - 127.5f) / 128.0f;
      float Q = (float(raw[2*i + 1]) - 127.5f) / 128.0f;
      iqf[i] = {I, Q};
    }

    size_t n_c = chan_.process(iqf.data(), n_iq, iqc.data(), iqc.size());
    size_t n_m = demod_.process(iqc.data(), n_c, mpx.data(), mpx.size());

    if (cfg_.enable_rds) rds_.process(mpx.data(), n_m);

    size_t n_pcm = audio_.process(mpx.data(), n_m, pcm.data(), pcm.size());
    if (n_pcm > 0) audio_out_.push(pcm.data(), n_pcm);
  }
}
