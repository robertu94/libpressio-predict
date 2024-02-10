#ifndef PRESSIO_PREDICT_BENCH_KFOLD_H_EHJCYGDT
#define PRESSIO_PREDICT_BENCH_KFOLD_H_EHJCYGDT
#include <cstddef>
#include <libpressio_ext/cpp/options.h>

namespace libpressio { namespace predict {

struct fold_t {
    size_t k;
    std::vector<pressio_options> data;
    struct fold_iterator_t {
        using value_type = std::pair<
            std::vector<pressio_options>,
            std::vector<pressio_options>
            >;
        using reference_type = value_type&;

        std::vector<pressio_options> const& train() {
            return m_data.first;
        }
        std::vector<pressio_options> const& test() {
            return m_data.second;
        }

        fold_iterator_t& operator++() {
            i++;
            start_idx = end_idx;
            if(remainder > 0) {
                remainder--;
            }
            end_idx += (ptr->data.size() / ptr->k) + ((remainder > 0) ? 1 : 0);
            update();
            return *this;
        }

        reference_type operator*() {
            if(m_data.first.empty() && m_data.second.empty()) {
                update();
            }
            return m_data;
        }

        bool operator==(fold_iterator_t& it) const {
            return i == it.i;
        }
        bool operator!=(fold_iterator_t& it) const {
            return i != it.i;
        }

        void update() {
            m_data.first.clear();
            m_data.second.clear();
            if(i < ptr->k) {
                size_t entries = ptr->data.size();
                for (int j = 0; j < entries; j++) {
                    if(start_idx <= j && j < end_idx) {
                        m_data.second.emplace_back(ptr->data[j]);
                    } else {
                        m_data.first.emplace_back(ptr->data[j]);
                    }
                }
            }
        }

        fold_iterator_t(size_t i, size_t start_idx, size_t end_idx, size_t remainder, fold_t* ptr):
            i(i), start_idx(start_idx), end_idx(end_idx), remainder(remainder),
            ptr(ptr), m_data() {
        }

        size_t i=0;
        size_t start_idx=0,end_idx=0,remainder=0;
        fold_t* ptr;
        value_type m_data{};
    };
    fold_iterator_t begin() {
        size_t data_size = data.size();
        size_t fold_size = data_size/k;
        size_t start_idx = 0;
        size_t remainder = data_size % k;
        size_t end_idx = fold_size + (remainder > 0 ? 1 : 0);
        return fold_iterator_t(0, start_idx, end_idx, remainder, this);
    }
    fold_iterator_t end() {
        return fold_iterator_t(k, this->data.size(), 0, this->data.size(), this);
    }
};

fold_t kfold(std::vector<pressio_options> inputs, size_t k);

} }

#endif /* end of include guard: PRESSIO_PREDICT_BENCH_KFOLD_H_EHJCYGDT */
