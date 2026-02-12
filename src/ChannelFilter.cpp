#include "ChannelFilter.h"
#include "dsp/FirDesign.h"
#include "Logging.h"

ChannelFilter::ChannelFilter(double fs_in, uint32_t decim, float cut_hz)
  : fs_in_(fs_in), decim_(decim) {
  fs_out_ = fs_in_ / double(decim_);
  int ntaps = 161;
  auto taps = design_lowpass(float(fs_in_), cut_hz, ntaps);
  decim_ = decim;
  dec_ = FIRDecimatorC(taps, decim_);
  log_msg(LogLevel::Info, "ChannelFilter: fs_in=%.0f Hz, decim=%u, fs_out=%.0f Hz", fs_in_, decim_, fs_out_);
}

size_t ChannelFilter::process(const std::complex<float>* in, size_t n_in,
                              std::complex<float>* out, size_t out_cap) {
  return dec_.process(in, n_in, out, out_cap);
}

double ChannelFilter::fs_out() const { return fs_out_; }
