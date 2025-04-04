#include <iostream>
#include <map>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <utility>
#include <dlfcn.h>
#include <getopt.h>
#include <std_compat/optional.h>
#include <libpressio_dataset_ext/loader.h>
#include <libpressio_ext/cpp/libpressio.h>
#include <libpressio_ext/cpp/serializable.h>
#include <libpressio_ext/cpp/printers.h>
#include <libdistributed_work_queue.h>

#include "pressio_predict_bench_db.h"
#include "pressio_predict_bench_cli.h"
#include "pressio_predict_bench_kfold.h"
#include "libpressio_predict_ext/cpp/predict.h"
#include <libpressio_predict_ext/cpp/predict_metrics.h>

using namespace libpressio_dataset;
using namespace libpressio::predict;

static pressio_options to_libpressio (std::map<std::string, std::vector<std::string>> const& in_opts) {
    pressio_options out_opts;
    for (auto const& opt : in_opts) {
        if(opt.second.size() > 1) {
            out_opts.set(opt.first, opt.second);
        } else {
            out_opts.set(opt.first, opt.second.front());
        }
    }
    return out_opts;
}
template <class S, class T>
bool contains(S const& set, T const& item) {
    return set.find(item) != set.end();
}

pressio_data extract(std::vector<pressio_options> const& observations, std::vector<std::string> const& labels) {
    pressio_data output = pressio_data::owning(pressio_double_dtype, {labels.size(), observations.size()});
    double* ptr = static_cast<double*>(output.data());
    size_t i = 0;
    for (auto const& observation : observations) {
        for (auto const& label : labels) {
            pressio_option const& obs = observation.get(label);
            if(not (obs.has_value() && obs.type() == pressio_option_double_type)) {
                    std::stringstream err;
                    err << "expected the option to be a double: (" << i << ',' << std::quoted(label)<< ')';
                    throw std::runtime_error(err.str());
            }
            double value = obs.get_value<double>();
            *ptr = value;
            ++ptr;
        }
        ++i;
    }

    return output;
}



int main(int argc, char *argv[])
{
    int rank = 0;
    int size = 0;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    const char* plugins_ptr = getenv("LIBPRESSIO_PLUGINS");
    std::string plugins;
    void* lib = nullptr;
    if(plugins_ptr!=nullptr) {
        plugins = plugins_ptr;
        lib = dlopen(plugins.c_str(), RTLD_NOW|RTLD_LOCAL);
        if(lib) {
            void(*fn)(void) ;
            *(void **)(&fn) = dlsym(lib, "libpressio_register_all");
            fn();
        }
    }

    cli_options opts = parse_options(argc, argv);

    if(rank == 0 && log_level{opts.verbose} >= log_level::warning) {
        std::cerr << opts << std::endl;
    }

    pressio_dataset_loader loader = dataset_loader_plugins().build("pressio");
    if(opts.qualified) loader->set_name("pressio");
    if(loader->cast_options(to_libpressio(opts.loader_early_options), to_libpressio(opts.loader_options))) {
        std::cerr << "configuring loader failed rank(" << rank <<  ") " << loader->error_msg() << std::endl;
    }

    pressio_compressor compressor = compressor_plugins().build("pressio");
    if(opts.qualified) compressor->set_name("pressio");
    if(compressor->cast_options(to_libpressio(opts.compressor_early_options), to_libpressio(opts.compressor_options))) {
        std::cerr << "configuring compressor failed rank(" << rank <<  ") " << compressor->error_msg() << std::endl;
    }

    if(rank == 0) {
        if(contains(opts.actions, Action::Settings)) {
            std::cout << "loader" << std::endl << loader->get_options() << std::endl;
            std::cout << "compressor" << std::endl << compressor->get_options() << std::endl;
        }
    }

    size_t num_datasets = loader->num_datasets();
    if(rank == 0) {
        if(contains(opts.actions, Action::Dataset)) {
            std::cout << num_datasets << std::endl;
            auto metadata = loader->load_all_metadata();
            size_t i=0;
            for (auto const& m : metadata) {
                std::cout << "Dataset " << i << std::endl;
                std::cout << m << std::endl;
                ++i;
            }
        }
    }


    pressio_options experiment_metadata {
        {"experiment:label", opts.experiment_label}
    };
    std::vector<pressio_options> results;

    if(contains(opts.actions, Action::Evaluate)) {
        std::vector<task_t> tasks;


        if(rank == 0) {
            db db_conn("./test.sqlite");
            int64_t experiment_config = db_conn.try_insert_experiment_config(experiment_metadata);
            int64_t compressor_config_h = db_conn.try_insert_compressor_config(compressor->get_options());
            int64_t dataset_config_h = db_conn.try_insert_loader_config(loader->get_options());
            for (uint64_t dataset_id = 0; dataset_id < num_datasets; ++dataset_id) {
                for (uint64_t replicate_id = 0; replicate_id < opts.replicates; ++replicate_id) {
                    task_t t {
                        dataset_config_h,
                        compressor_config_h,
                        experiment_config,
                        replicate_id,
                        dataset_id
                    };

                    if(!db_conn.has_result(t)) {
                        tasks.emplace_back(std::move(t));
                    } else {
                        results.emplace_back(db_conn.load(t));
                    }
                }
            }
        }

        size_t N=tasks.size(), completed=0;
        distributed::queue::work_queue_options<task_t> options;
        distributed::queue::work_queue(options, tasks.begin(), tasks.end(),
                [&opts,&loader,&compressor](task_t const& task) {
                    std::vector<pressio_options> results;
                    pressio_data data = loader->load_data(feild_id(task));
                    for (uint64_t i = 0; i < opts.replicates; ++i) {
                        pressio_data compressed(pressio_data::empty(pressio_byte_dtype, {}));
                        pressio_data output(pressio_data::empty(data.dtype(), data.dimensions()));
                        if(compressor->compress(&data, &compressed) > 0) {
                          std::cerr << "compression failed " << task << " "
                                    << std::quoted(compressor->error_msg()) << std::endl;
                          break;
                        }
                        if(opts.do_decompress){
                            if(compressor->decompress(&compressed, &output) > 0) {
                              std::cerr << "decompression failed " << task << " "
                                        << std::quoted(compressor->error_msg()) << std::endl;
                              break;
                            }
                        }
                        results.emplace_back(compressor->get_metrics_results());
                        results.back().set("task:feild_id", feild_id(task));
                        results.back().set("task:replicate_id", replicate_id(task));
                        results.back().set("task:dataset_id", dataset_id(task));
                        results.back().set("task:compressor_id", compressor_id(task));
                        results.back().set("task:experiment_id", experiment_id(task));
                    }
                    return response_t{task, results};
                },
                [&](response_t const& response){
                    db db_conn("./test.sqlite");
                    auto const& [task, task_results] = response;
                    if(log_level{opts.verbose} >= log_level::warning) {
                        std::cerr << "finished" << ' ' << ++completed << '/' << N << std::endl;
                    } 
                    for (int i = 0; i < task_results.size(); ++i) {
                        if (log_level{opts.verbose} >= log_level::debug) {
                            std::cerr <<  task_results[i] << std::endl;
                        }
                        db_conn.try_insert_result(task, task_results[i]);
                        results.emplace_back(db_conn.load(task));
                    }
                }
        );
    }


    if(rank == 0 && contains(opts.actions, Action::Score)) {
        std::vector<std::string> features_ids;
        std::vector<std::string> label_ids;
        libpressio_predictor_metrics score = predictor_quality_metrics_plugins().build("medape");

        auto folds = kfold(results, opts.k);
        for(auto const& [train, test] : folds) {
            libpressio_predictor predictor = predictor_plugins().build("noop");
            auto train_features = extract(train, features_ids);
            auto test_features = extract(train, features_ids);
            auto train_labels = extract(train, label_ids);
            auto test_labels = extract(train, label_ids);
            pressio_data predicted_labels;

            predictor->fit(train_features, train_labels);
            predictor->predict(test_features, predicted_labels);
            score->score(test_labels, predicted_labels);
        }

        std::cout << score->get_metrics_results() << std::endl;
    }



    MPI_Finalize();
    return 0;
}
