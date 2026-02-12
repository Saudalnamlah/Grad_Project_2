#include "AudioRingBuffer.h"
#include <algorithm>

AudioRingBuffer::AudioRingBuffer(size_t capacity_samples)
  : buf_(capacity_samples), cap_(capacity_samples) {}

void AudioRingBuffer::push(const int16_t* samples, size_t count) {
  std::unique_lock<std::mutex> lock(m_);
  if (stopped_) return;

  for (size_t i = 0; i < count; ++i) {
    if (size_ == cap_) {
      // overwrite oldest
      r_ = (r_ + 1) % cap_;
      --size_;
    }
    buf_[w_] = samples[i];
    w_ = (w_ + 1) % cap_;
    ++size_;
  }
  cv_.notify_one();
}

size_t AudioRingBuffer::pop(int16_t* out, size_t max_count) {
  std::unique_lock<std::mutex> lock(m_);
  cv_.wait(lock, [&]{ return stopped_ || size_ > 0; });
  if (stopped_ && size_ == 0) return 0;

  size_t n = std::min(max_count, size_);
  for (size_t i = 0; i < n; ++i) {
    out[i] = buf_[r_];
    r_ = (r_ + 1) % cap_;
  }
  size_ -= n;
  return n;
}

void AudioRingBuffer::stop() {
  std::lock_guard<std::mutex> lock(m_);
  stopped_ = true;
  cv_.notify_all();
}
