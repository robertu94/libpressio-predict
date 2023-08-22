#ifndef PRESSIO_PREDICT_BENCH_CLI_CC_VMJW17BA
#define PRESSIO_PREDICT_BENCH_CLI_CC_VMJW17BA
#include <iosfwd>
#include <map>
#include <set>
#include <vector>
#include <string>

namespace libpressio { namespace predict {

enum class Action {
    Settings,
    Evaluate,
    Dataset,
    Flush,
    Score,
};

enum class log_level: int {
    silent=0,
    error=10,
    warning=20,
    debug=30
};


struct cli_options {
    std::map<std::string, std::vector<std::string>> loader_early_options;
    std::map<std::string, std::vector<std::string>> loader_options;
    std::map<std::string, std::vector<std::string>> compressor_early_options;
    std::map<std::string, std::vector<std::string>> compressor_options;
    std::string experiment_label;
    size_t replicates = 1;
    size_t k = 10;
    std::set<Action> actions;
    bool qualified = false;
    bool do_decompress = false;
    int verbose = 0;
};

std::ostream& operator<<(std::ostream& out, Action action);
std::ostream& operator<<(std::ostream& out, cli_options const& opts);

cli_options parse_options(int argc, char* argv[]);

}}

#endif /* end of include guard: PRESSIO_PREDICT_BENCH_CLI_CC_VMJW17BA */
