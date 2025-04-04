#include <libpressio_predict_ext/cpp/predict.h>
#include <libpressio_predict_ext/cpp/predict_metrics.h>
#include <libpressio_ext/cpp/printers.h>
#include <iostream>

using namespace libpressio::predict;
using namespace std::string_literals;

int main(int argc, char *argv[])
{
   libpressio_predictor predictor = predictor_plugins().build("python"); 
   assert(predictor && "could not find predict method");
   libpressio_predictor_metrics score = predictor_quality_metrics_plugins().build("medape"); 
   assert(score && "could not find score methods");
   std::string program = R"(
import numpy as np

def predict(features, state):
    features = np.array(features)
    print("predict", names, features, state)
    return features + state["diff"]

def fit(features, labels):
    features = np.array(features)
    labels = np.array(labels)
    print("fit", names, features, labels)
    return {
        "diff": (labels - features).max()
    }
   )";
   std::vector<std::string> names {"a"};
   pressio_data train_features {
       1.0,
       2.0,
       3.0,
   };
   pressio_data train_labels {
       2.0,
       3.0,
       4.0,
   };
   pressio_data test_features {
       2.0,
       3.0,
       4.0,
   };
   pressio_data test_labels {
       3.0,
       4.0,
       5.0,
   };
   pressio_data predicted_labels{};
   predictor->set_options({
           {"python:program", program},
           {"python:feature_names", names},
   });
   std::cout << predictor->get_options() << std::endl;
   if(predictor->fit(train_features, train_labels)) {
       std::cout << predictor->error_msg() << std::endl;
       return 1;
   }
   if(predictor->predict(test_features, predicted_labels)) {
       std::cout << predictor->error_msg() << std::endl;
       return 1;
   }
   if(score->score(test_labels, predicted_labels)) {
       std::cout << score->error_msg() << std::endl;
       return 1;
   }
   std::cout << score->get_metrics_results() << std::endl;

    
    
    return 0;
}
