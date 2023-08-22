#ifndef PRESSIO_PREDICT_BENCH_DB_H_DPSGVYBM
#define PRESSIO_PREDICT_BENCH_DB_H_DPSGVYBM
#include <string>
#include <tuple>
#include <iosfwd>
#include <sqlite3.h>
#include <libpressio_ext/cpp/options.h>

namespace libpressio { namespace predict {

using task_t = std::tuple<
    int64_t /*dataset_id*/,
    int64_t /*compressor_id*/,
    int64_t /*experiment_id*/,
    int64_t /*replicate_id*/,
    int64_t /*field_id*/
>;
int64_t feild_id(task_t const& t);
int64_t replicate_id(task_t const& t);
int64_t dataset_id(task_t const& t);
int64_t compressor_id(task_t const& t);
int64_t experiment_id(task_t const& t);
std::ostream& operator<<(std::ostream&, task_t const&);

using response_t = 
    std::pair<
        task_t,
        std::vector<pressio_options>
    >;

class db{
    public:
    db();
    db(std::string const& db_path);
    ~db();

    db(db&)=delete;
    db(db&&)=delete;
    db& operator=(db&)=delete;
    db& operator=(db&&)=delete;

    bool has_result(task_t const& t);
    size_t try_insert_result(task_t const& t, pressio_options const& result);
    int64_t try_insert_compressor_config(pressio_options const& config);
    int64_t try_insert_loader_config(pressio_options const& config);
    int64_t try_insert_experiment_config(pressio_options const& config);

    pressio_options load(task_t const& task);
    private:
    sqlite3* dbptr;
};


}
}

#endif /* end of include guard: PRESSIO_PREDICT_BENCH_DB_H_DPSGVYBM */
