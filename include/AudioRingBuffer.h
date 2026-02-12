#pragma once
#include <cstdint>
#include <condition_variable>
#include <mutex>
#include <vector>

class AudioRingBuffer {
public:
  explicit AudioRingBuffer(size_t capacity_samples = 48000 * 5);

  void push(const int16_t* samples, size_t count);
  size_t pop(int16_t* out, size_t max_count);
  void stop();

private:
  std::vector<int16_t> buf_;
  size_t cap_ = 0;
  size_t r_ = 0;
  size_t w_ = 0;
  size_t size_ = 0;

  bool stopped_ = false;
  std::mutex m_;
  std::condition_variable cv_;
};
