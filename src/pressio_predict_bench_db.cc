#include "pressio_predict_bench_db.h"

#include <sqlite3.h>
#include <libpressio_ext/cpp/libpressio.h>
#include <libpressio_ext/json/pressio_options_json.h>
#include <libpressio_ext/hash/libpressio_hash.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <mpi.h>

namespace libpressio { namespace predict {

namespace sql {
namespace impl {
void bind_helper(sqlite3_stmt* stmt, int64_t val, size_t i) {
    if(sqlite3_bind_int64(stmt, i, val) != SQLITE_OK) {
        std::stringstream err;
        err << "failed to bind parameter " << i << " with value " << val;
        throw std::runtime_error(err.str());
    }
}
void bind_helper(sqlite3_stmt* stmt, std::string const& val, size_t i) {
    if(sqlite3_bind_text(stmt, i, val.c_str(), val.size(), SQLITE_STATIC) != SQLITE_OK) {
        std::stringstream err;
        err << "failed to bind parameter " << i << " with value " << std::quoted(val);
        throw std::runtime_error(err.str());
    }
}
void bind_helper(sqlite3_stmt* stmt, double val, size_t i) {
    if(sqlite3_bind_double(stmt, i, val) != SQLITE_OK) {
        std::stringstream err;
        err << "failed to bind parameter " << i << " with value " << val;
        throw std::runtime_error(err.str());
    }
}
void retrieve_helper(sqlite3_stmt* stmt, int64_t& val, size_t i) {
    int type;
    if((type =sqlite3_column_type(stmt, i)) != SQLITE_INTEGER) {
        std::stringstream err;
        err << "failed to get column " << i <<  " unexpected type " << type << " expected " << SQLITE_TEXT;
        throw std::runtime_error(err.str());
    }
    val = sqlite3_column_int64(stmt, i);
}
void retrieve_helper(sqlite3_stmt* stmt, std::string& val, size_t i) {
    int type;
    if((type =sqlite3_column_type(stmt, i)) != SQLITE_TEXT) {
        std::stringstream err;
        err << "failed to get column " << i <<  " unexpected type " << type << " expected " << SQLITE_TEXT;
        throw std::runtime_error(err.str());
    }
    val = (char*)sqlite3_column_text(stmt, i);
}
void retrieve_helper(sqlite3_stmt* stmt, double& val, size_t i) {
    int type;
    if((type =sqlite3_column_type(stmt, i)) != SQLITE_FLOAT) {
        std::stringstream err;
        err << "failed to get column " << i <<  " unexpected type " << type << " expected " << SQLITE_TEXT;
        throw std::runtime_error(err.str());
    }
    val = sqlite3_column_double(stmt, i);
}
template <size_t... Is, class... Args>
void bind_impl(sqlite3_stmt* stmt, std::index_sequence<Is...>, std::tuple<Args const&...> args) {
    return (bind_helper(stmt, std::get<Is>(args), Is + 1),...);
}
template <size_t... Is, class... Args>
void retrieve_impl(sqlite3_stmt* stmt, std::index_sequence<Is...>, std::tuple<Args...>& args) {
    (retrieve_helper(stmt, std::get<Is>(args), Is),...);
}
}


template <class... ReturnType, class... Args>
std::vector<std::tuple<ReturnType...>> query(sqlite3* sqlite, std::string const& sql, Args const&... args) {
    std::vector<std::tuple<ReturnType...>> ret;
    sqlite3_stmt* stmt;
    if(sqlite3_prepare_v3(sqlite, sql.c_str(), sql.size(), 0, &stmt, nullptr) != SQLITE_OK) {
        std::stringstream err;
        err << "failed to prepare sql statement " << std::quoted(sql);
        throw std::runtime_error(err.str());
    }
    impl::bind_impl(stmt, std::index_sequence_for<Args...>{}, std::forward_as_tuple(args...));

    while(1) {
        int rc = sqlite3_step(stmt);
        if(rc != SQLITE_ROW) {
            break;
        }

        ret.emplace_back();
        impl::retrieve_impl(stmt, std::index_sequence_for<ReturnType...>{}, ret.back());
    }
    sqlite3_finalize(stmt);
    return ret;
}

template <class... ReturnType, class... Args>
std::tuple<ReturnType...> query_one(sqlite3* sqlite, std::string const& sql, Args const&... args) {
    auto result = query<ReturnType...>(sqlite, sql, args...);
    if (result.size() == 1) {
        return result.front();
    } else if (result.size() > 1) {
        throw std::runtime_error("expected 1 result, but got multiple results");
    } else {
        throw std::runtime_error("expected 1 result, but got no results");
    }
}


}

std::string hash_options(pressio_options const& options) {
    size_t output_size = 0;
    pressio library;
    uint8_t* bytes =  libpressio_options_hashentries(&library, &options, &output_size);

    std::stringstream ss;
    for (int i = 0; i < output_size; ++i) {
        ss << std::hex << int{bytes[i]};
    }
    free(bytes);
    return ss.str();
}
std::string json_options(pressio_options const& options) {
    pressio library;
    char* ptr = pressio_options_to_json(&library, &options);
    std::string result = ptr;
    free(ptr);
    return result;
}

int64_t dataset_id(task_t const& t) {
    return std::get<0>(t);
}
int64_t compressor_id(task_t const& t) {
    return std::get<1>(t);
}
int64_t experiment_id(task_t const& t) {
    return std::get<2>(t);
}
int64_t replicate_id(task_t const& t) {
    return std::get<3>(t);
}
int64_t feild_id(task_t const& t) {
    return std::get<4>(t);
}

std::ostream& operator<<(std::ostream& out, task_t const& t) {
    return out << '{' <<
        ".label=" << experiment_id(t) << ", " <<
        ".dataset=" << dataset_id(t) << ", " <<
        ".compressor=" << compressor_id(t) << ", " <<
        ".dataset_id=" << feild_id(t) << ", " <<
        ".replicate=" << replicate_id(t) << "}";
}

using namespace std::string_literals;

db::db(): db(":memory:") {}
db::~db() {
    sqlite3_close(dbptr);
}
db::db(std::string const& db_path) {
    sqlite3_open(db_path.c_str(), &dbptr);
    char* errmsg;
    int rc = sqlite3_exec(dbptr,
    R"sql(

        PRAGMA foreign_keys;
        CREATE TABLE IF NOT EXISTS Experiments(
            id INTEGER PRIMARY KEY,
            hash TEXT UNIQUE ON CONFLICT IGNORE NOT NULL ON CONFLICT ABORT,
            config TEXT CHECK (json_valid(config)) UNIQUE ON CONFLICT IGNORE NOT NULL ON CONFLICT ABORT
        );
        CREATE TABLE IF NOT EXISTS Datasets(
            id INTEGER PRIMARY KEY,
            hash TEXT UNIQUE ON CONFLICT IGNORE NOT NULL ON CONFLICT ABORT,
            config TEXT CHECK (json_valid(config)) UNIQUE ON CONFLICT IGNORE NOT NULL ON CONFLICT ABORT
        );
        CREATE TABLE IF NOT EXISTS Compressors(
            id INTEGER PRIMARY KEY,
            hash TEXT UNIQUE ON CONFLICT IGNORE NOT NULL ON CONFLICT ABORT,
            config TEXT CHECK (json_valid(config)) UNIQUE ON CONFLICT IGNORE NOT NULL ON CONFLICT ABORT
        );
        CREATE TABLE IF NOT EXISTS Metrics(
            id INTEGER PRIMARY KEY,
            value TEXT UNIQUE ON CONFLICT IGNORE NOT NULL ON CONFLICT ABORT
        );
        CREATE TABLE IF NOT EXISTS Results(
            id INTEGER PRIMARY KEY,
            experiment INTEGER REFERENCES Experiments(id),
            compressor INTEGER REFERENCES Compressors(id),
            dataset INTEGER REFERENCES Datasets(id),
            field INTEGER CHECK(field >= 0),
            replicate INTEGER CHECK(replicate >= 0),
            UNIQUE (experiment, compressor, dataset, field, replicate) ON CONFLICT IGNORE
        );
        CREATE TABLE IF NOT EXISTS Entries(
            result REFERENCES Results(id),
            metric REFERENCES Metrics(id),
            value REAL NOT NULL
        );

    )sql",
    nullptr, nullptr, &errmsg);
    if(rc) {
        sqlite3_close(dbptr);
        throw std::runtime_error("sqlite error during table construction:  "s + errmsg);
    }
}
bool db::has_result(task_t const& t) { 

    std::string const sql_cmd = R"sql(

    SELECT EXISTS(
        SELECT 1
        FROM Results
        WHERE
            experiment = ? AND
            compressor = ? AND
            dataset = ? AND
            replicate = ? AND
            field = ?
    );

    )sql";
    int64_t id = std::get<0>(sql::query_one<int64_t>(dbptr, sql_cmd,
            experiment_id(t),
            compressor_id(t),
            dataset_id(t),
            replicate_id(t),
            feild_id(t)));

    
    return id == 1;
}

size_t db::try_insert_result(task_t const& t, pressio_options const& result) {
    std::string results_insert_cmd = R"sql(
        INSERT INTO Results(experiment, compressor, dataset, field, replicate)
        VALUES (?,?,?,?,?);
    )sql";
    sql::query<>(dbptr, results_insert_cmd, 
            experiment_id(t),
            compressor_id(t),
            dataset_id(t),
            feild_id(t),
            replicate_id(t)
            );


    std::string results_id_cmd = R"sql(

        SELECT id from Results
        WHERE
            experiment = ? AND
            compressor = ? AND
            dataset = ? AND
            replicate = ? AND
            field = ?;

    )sql";
    auto result_id = std::get<0>(sql::query_one<int64_t>(dbptr, results_id_cmd, 
            experiment_id(t),
            compressor_id(t),
            dataset_id(t),
            replicate_id(t),
            feild_id(t)));


    std::string metrics_insert_cmd = R"sql(
        INSERT INTO Metrics(value)
        VALUES (?);
    )sql";
    std::string metrics_id_cmd = R"sql(
        SELECT id from Metrics
        WHERE value = ?;
    )sql";
    std::string entries_insert_cmd = R"sql(
        INSERT INTO Entries(result, metric, value)
        VALUES (?,?,?);
    )sql";

    for (auto const& [key,value] : result) {
        auto converted = value.as(pressio_option_double_type, pressio_conversion_special);
        if (converted.has_value()) {
            sql::query<>(dbptr, metrics_insert_cmd, key);
            auto metric_id = std::get<0>(sql::query_one<int64_t>(dbptr, metrics_id_cmd, key));
            switch(converted.type()) {
                case pressio_option_double_type:
                sql::query<>(dbptr, entries_insert_cmd, result_id, metric_id, converted.get_value<double>());
                default:
                    break;
            }
        }
    }

    return 0;
}

int64_t try_insert_impl(pressio_options const& config, std::string const& table, sqlite3* dbptr) {
    std::string hash = hash_options(config);
    std::string json = json_options(config);

    std::stringstream sql_cmd_ss;
    sql_cmd_ss << "INSERT INTO " << table << "(config, hash) VALUES (?, ?);";
    std::string sql_cmd = sql_cmd_ss.str();
    sql::query<>(dbptr, sql_cmd_ss.str(), json, hash);

    std::stringstream sql_get_id;
    sql_get_id << "SELECT id FROM " <<  table << " WHERE hash = ?;";
    return std::get<0>(sql::query_one<int64_t>(dbptr, sql_get_id.str(), hash));
}

int64_t db::try_insert_compressor_config(pressio_options const& config) {
    return try_insert_impl(config, "Compressors", dbptr);
}

int64_t db::try_insert_loader_config(pressio_options const& config) {
    return try_insert_impl(config, "Datasets", dbptr);
}

int64_t db::try_insert_experiment_config(pressio_options const& config) {
    return try_insert_impl(config, "Experiments", dbptr);
}
pressio_options db::load(task_t const& t) {
    pressio_options options;

    std::string sql_cmd = R"sql(

        SELECT Metrics.value, Entries.value FROM Entries
        LEFT JOIN Metrics ON Metrics.id == Entries.metric
        LEFT JOIN Results ON Results.id == Entries.result
        WHERE Results.id == (
            SELECT id FROM Results
            WHERE
                experiment = ? AND
                compressor = ? AND
                dataset = ? AND
                replicate = ? AND
                field = ?
        );

    )sql";

    auto entries = sql::query<std::string, double>(dbptr, sql_cmd, 
            experiment_id(t),
            compressor_id(t),
            dataset_id(t),
            replicate_id(t),
            feild_id(t));

    for (auto const& [key,value] : entries) {
        options.set(key, value);
    }

    return options;
}


} }
