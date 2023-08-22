#include "pressio_predict_bench_cli.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <unistd.h>


namespace libpressio { namespace predict {
        
std::ostream& operator<<(std::ostream& out, Action action) {
    switch(action) {
        case Action::Settings:
            out << "settings";
            break;
        case Action::Dataset:
            out << "dataset";
            break;
        case Action::Evaluate:
            out << "evaluate";
            break;
        case Action::Flush:
            out << "flush";
            break;
        case Action::Score:
            out << "score";
            break;
    }
    return out;
}

struct format_dict {
    std::map<std::string, std::vector<std::string>> const& map;
};
std::ostream& operator<<(std::ostream& out, format_dict const& opts) {
    out << "{\n";
    for (auto const& [key, value] : opts.map) {
        out << '\t' << key << '=';
        out << '[';
        for (auto const& j : value) {
            out << j << ',' << ' ';
        }
        out << "]\n";
    }
    out << "}\n";

    return out;
}
template <class T>
struct format_set {
    std::set<T> const& s;
};
template <class T>
std::ostream& operator<<(std::ostream& out, format_set<T> const& s) {
    out << "{";
    for (auto const& i : s.s) {
        out << i << ", ";
    }
    out << "}\n";
    return out;
}


std::ostream& operator<<(std::ostream& out, cli_options const& opts) {
    out << "loader_early" << format_dict{opts.loader_early_options} << std::endl;
    out << "loader" << format_dict{opts.loader_options} << std::endl;
    out << "compressor_early" << format_dict{opts.compressor_early_options} << std::endl;
    out << "compressor" << format_dict{opts.compressor_options} << std::endl;
    out << "replicas " << opts.replicates << std::endl;
    out << "qualified " << std::boolalpha << opts.qualified << std::endl;
    out << "do_decompress " << std::boolalpha << opts.do_decompress << std::endl;
    out << "verbose " << std::boolalpha << opts.verbose << std::endl;
    out << "actions " << std::boolalpha << format_set<Action>{opts.actions} << std::endl;
    out << "experiment label " << std::quoted(opts.experiment_label) << std::endl;

    return out;
}

const std::map<std::string, Action> actions {
    {"settings", Action::Settings},
    {"dataset", Action::Dataset},
    {"run", Action::Evaluate},
    {"score", Action::Score},
    {"flush", Action::Flush}
};
static std::pair<std::string, std::string> split(std::string const& s) {
    size_t pos = s.find("=");
    if(pos == std::string::npos) {
        std::stringstream ss;
        ss << "failed to parse option :" << std::quoted(s);
        throw std::runtime_error(ss.str());
    }
    return std::make_pair(s.substr(0, pos), s.substr(pos+1));
}

cli_options parse_options(int argc, char* argv[]) {
    cli_options opts;
    int opt;
    while ((opt = getopt(argc, argv, "a:k:o:b:l:vL:QDR:")) != -1) {
        switch (opt) {
            case 'v':
                opts.verbose += 10;
                break;
            case 'k':
                opts.k = atoi(optarg);
                break;
            case 'o':
                {
                    auto arg = split(optarg);
                    opts.compressor_options[std::move(arg.first)].emplace_back(std::move(arg.second));
                }
                break;
            case 'b':
                {
                    auto arg = split(optarg);
                    opts.compressor_early_options[std::move(arg.first)].emplace_back(std::move(arg.second));
                }
                break;
            case 'L':
                {
                    auto arg = split(optarg);
                    opts.loader_early_options[std::move(arg.first)].emplace_back(std::move(arg.second));
                }
                break;
            case 'l':
                {
                    auto arg = split(optarg);
                    opts.loader_options[std::move(arg.first)].emplace_back(std::move(arg.second));
                }
                break;
            case 'a':
                {
                    try {
                    Action action = actions.at(optarg);
                    opts.actions.emplace(action);
                    } catch(std::out_of_range const& ex) {
                        std::stringstream ss;
                        ss << "failed to find key " << std::quoted(optarg);
                        throw std::runtime_error(ss.str());
                    }
                }
                break;
            case 'Q':
                opts.qualified = true;
                break;
            case 'D':
                opts.do_decompress = true;
                break;
            case 'R':
                opts.replicates = atoi(optarg);
                break;
        }
    }

    if(opts.actions.empty()) {
        opts.actions.emplace(Action::Evaluate);
    }

    return opts;
}

    } /* predict */ 
} /* libpressio  */ 
