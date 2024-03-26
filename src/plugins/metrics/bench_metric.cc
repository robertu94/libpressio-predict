
#include <cmath>
#include <chrono>
#include "pressio_data.h"
#include "pressio_compressor.h"
#include "pressio_options.h"
#include "libpressio_ext/cpp/metrics.h"
#include "libpressio_ext/cpp/pressio.h"
#include "libpressio_ext/cpp/options.h"
#include "std_compat/memory.h"

namespace libpressio { namespace bench_metric_metrics_ns {

class bench_metric_plugin : public libpressio_metrics_plugin {
  public:
    int begin_compress_impl(struct pressio_data const* input, pressio_data const* output) override {
      return track(begin_compress_t, [=,this](){ return this->metric->begin_compress(input, output);});
    }
    int begin_decompress_impl(struct pressio_data const* input, pressio_data const* output) override {
      return track(begin_decompress_t, [=,this](){ return this->metric->begin_decompress(input, output);});
    }
    int end_compress_impl(struct pressio_data const* input, pressio_data const* output, int rc) override {
      return track(end_compress_t, [=,this](){ return this->metric->end_compress(input, output, rc);});
    }

    int end_decompress_impl(struct pressio_data const* input, pressio_data const* output, int rc) override {
      return track(end_decompress_t, [=,this](){ return this->metric->end_decompress(input, output, rc);});
    }

  
  int set_options(pressio_options const& opts) override {
      get_meta(opts, "bench_metric:metric", metrics_plugins(), metric_id, metric);
      get(opts, "bench_metric:replicates", &replicates);
      get(opts, "bench_metric:warmups", &warmup);
      return 0;
  }
  struct pressio_options get_options() const override {
      pressio_options opts;
      set_meta(opts, "bench_metric:metric", metric_id, metric);
      set(opts, "bench_metric:replicates", replicates);
      set(opts, "bench_metric:warmups", warmup);
      return opts;
  }

  struct pressio_options get_configuration_impl() const override {
    pressio_options opts;
    set_meta_configuration(opts, "bench_metric:metric", metrics_plugins(), metric);
    set(opts, "pressio:stability", "stable");
    set(opts, "pressio:thread_safe", pressio_thread_safety_multiple);
    return opts;
  }

  struct pressio_options get_documentation_impl() const override {
    pressio_options opt;
    set_meta_docs(opt, "bench_metric:metric", "metric to benchmark", metric);
    set(opt, "pressio:description", "runs a metric multiple times recording its timing");
    set(opt, "bench_metric:warmups", "skip this many executions before starting to record");
    set(opt, "bench_metric:replicates", "execute this many times (excluding warmups)");
    auto doc_trackers = [this,&opt](std::string const& event_name){
        set(opt, "bench_metric:" + event_name + ":mean", "mean time for " + event_name);
        set(opt, "bench_metric:" + event_name + ":std", "standard deviation for " + event_name);
        set(opt, "bench_metric:" + event_name + ":min", "min for " + event_name);
        set(opt, "bench_metric:" + event_name + ":max", "max for " + event_name);
    };
    doc_trackers("begin_compress");
    doc_trackers("end_compress");
    doc_trackers("begin_decompress");
    doc_trackers("end_decompress");

    return opt;
  }

  pressio_options get_metrics_results(pressio_options const & m) override {
    pressio_options opt;
    auto set_trackers = [this,&opt](std::string const& event_name, tracker const& t){
        double mean=0, std=0;
        size_t N=0;
        double min,max;
        if(!t.times_ms.empty()) {
            min = t.times_ms.front();
            max = t.times_ms.front();
        }
        for (auto i : t.times_ms) {
            mean += i;
            min = std::min<double>(min, i);
            max = std::max<double>(max, i);
            N++;
        }
        mean /= (double)N;
        for (auto i : t.times_ms) {
            std += std::pow((i - mean),2);
        }
        std = std::sqrt(1.0/(N-1.0)* std);
        set(opt, "bench_metric:" + event_name + ":mean", mean);
        set(opt, "bench_metric:" + event_name + ":std", std);
        set(opt, "bench_metric:" + event_name + ":min", min);
        set(opt, "bench_metric:" + event_name + ":max", max);
    };
    opt.copy_from(metric->get_metrics_results(m));
    set_trackers("begin_compress", begin_compress_t);
    set_trackers("end_compress", end_compress_t);
    set_trackers("begin_decompress", begin_decompress_t);
    set_trackers("end_decompress", end_decompress_t);
    return opt;
  }

  void set_name_impl(std::string const& s) override {
      metric->set_name(s + "/" + metric->prefix());
  }

  std::unique_ptr<libpressio_metrics_plugin> clone() override {
    return compat::make_unique<bench_metric_plugin>(*this);
  }
  const char* prefix() const override {
    return "bench_metric";
  }

  private:
  std::string metric_id = "noop";
  pressio_metrics metric = metrics_plugins().build(metric_id);
  uint64_t replicates = 30;
  uint64_t warmup = 2;

  class tracker {
      public:
      tracker() {}
      tracker(size_t n, size_t warmup) {
            times_ms.reserve(n);
            begin = std::chrono::steady_clock::now();
      }
      void record() {
        auto end = std::chrono::steady_clock::now();
        if(warmup) {
            warmup--;
        } else {
            times_ms.emplace_back(std::chrono::duration_cast<std::chrono::milliseconds>(end-begin).count());
        }
        begin = end;
      }

      std::chrono::steady_clock::time_point begin;
      std::vector<uint64_t> times_ms;
      size_t warmup = 0;
  };

  tracker begin_compress_t, end_compress_t, begin_decompress_t, end_decompress_t;
  template <class Func>
  int track(tracker& t, Func&& func) {
      int rc = 0;
      t = tracker(replicates, warmup);
      for (size_t i = 0; i < replicates+warmup; ++i) {
          if((rc = func())) {
              return  rc;
          }
          t.record();
      }
      return rc;
  }
};

static pressio_register metrics_bench_metric_plugin(metrics_plugins(), "bench_metric", [](){ return compat::make_unique<bench_metric_plugin>(); });
}}

