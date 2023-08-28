#include "pressio_data.h"
#include "pressio_compressor.h"
#include "pressio_options.h"
#include "libpressio_ext/cpp/metrics.h"
#include "libpressio_ext/cpp/pressio.h"
#include "libpressio_ext/cpp/options.h"
#include "std_compat/memory.h"
#include <cmath>

namespace libpressio { namespace rahman2023_metrics_ns {

class rahman2023_plugin : public libpressio_metrics_plugin {
  public:
    int begin_compress_impl(struct pressio_data const* input, pressio_data const*) override {
      if (!input->has_data()){
        return set_error(1, "Data has not loaded successfully!");
      }
      else {
        // check the dimensions and hook appropriate function
        // Expecting data dimensions in reverse order, for example with Hurricane dataset 500x500x100
        // make sure data has either of only two types
        if (input->dtype() != pressio_float_dtype && input->dtype() != pressio_double_dtype) {
          return set_error(1, "Data type must be single or double precision floating point for FXRZ");
        }
        switch (input->num_dimensions()) 
        {
        case 1:
          runFXRZwith1DData(input); 
          return 0;
        case 2:
          runFXRZwith2DData(input);
          return 0;
        case 3:
          runFXRZwith3DData(input);
          return 0;
        default:
          set_error(1, "Not valid number of dimenstions. Number of dimensions must be among 1, 2 or 3.");
          break;
        }
      }
      return 0;
    }
  
    struct pressio_options get_configuration_impl() const override {
      pressio_options opts;
      set(opts, "pressio:stability", "stable");
      set(opts, "pressio:thread_safe", pressio_thread_safety_multiple);
      return opts;
    }

    struct pressio_options get_documentation_impl() const override {
      pressio_options opt;
      set(opt, "pressio:description", "");
      return opt;
    }

    pressio_options get_metrics_results(pressio_options const &) override {
      pressio_options opt;
      set(opt, "rahman2023:optimization:non_constant_block_percentage", nonConstantBlockPercentage);
      set(opt, "rahman2023:feature:value_range", valueRange);
      set(opt, "rahman2023:feature:mean_value", meanValue);
      set(opt, "rahman2023:feature:mean_neighbor_difference", meanNeighborDifference);
      set(opt, "rahman2023:feature:mean_lorenzo_difference", meanLorenzoDifference);
      set(opt, "rahman2023:feature:mean_spline_difference", meanSplineDifference);
      return opt;
    }

    std::unique_ptr<libpressio_metrics_plugin> clone() override {
      return compat::make_unique<rahman2023_plugin>(*this);
    }
    const char* prefix() const override {
      return "rahman2023";
    }

  private:
    // macros
    const size_t BLOCK_SIZE = 4 ;
    const size_t SAMPLING_OFFSET = 4;
    const double NUM_CELL_THRESHOLD = 0.75;

    // additional helper variables
    std::vector<double>sampled_data_values;
  
    // features
    double valueRange = 0.0;
    double meanValue = 0.0;
    double meanNeighborDifference = 0.0;
    double meanLorenzoDifference = 0.0;
    double meanSplineDifference = 0.0;

    // optimization module variables
    double nonConstantBlockPercentage = 0.0;

    // Utilities start
    double getSplinePrediction(double value1, double value2, double value3, double value4)
    {
            return -(1.0 / 16.0) * value1 + (9.0 / 16.0) * value2 + (9.0 / 16.0) * value3 - (1.0 / 16.0) * value4;
    }

    double changeScale(double old_val, double old_min, double old_max, double new_min, double new_max) {
      double old_range = old_max - old_min;
      double new_range = new_max - new_min;
      double ratio = (old_val - old_min) / old_range;
      return (ratio * new_range + new_min);
    }
    // Utilities end


    /** For 1D dataset start */ 



    // Feature extraction start
    std::vector <double> runFeatureExtractionwith1DData(const void *data, const enum pressio_dtype dtype, const int num_entries, 
                                              const size_t num_dims, 
                                              std::vector<size_t> dims, double &data_min, 
                                              double &data_max) {

      
      // Expecting data dimensions in reverse order, for example with Hurricane dataset 500x500x100
	    size_t X = dims[0];

      int num_sampled_entries = 0;
      int num_sampled_lorenzo_entries = 0;
      double avg_value  = 0.0;
      double min_val = (dtype == pressio_float_dtype) ? (double)((float*)data)[0] : (double)((double*)data)[0];
      double max_val = (dtype == pressio_float_dtype) ? (double)((float*)data)[0] : (double)((double*)data)[0];	
      
      // neighbor
      double sum_dif_neighbors = 0.0;
      int num_sampled_neighbors = 0;


      // sum dif spline
      double sum_dif_splines = 0.0;	
      int num_valid_splines = 0;

      // mean dif lorenzo
      double sum_dif_lorenzos = 0.0;
      int num_valid_lorenzos = 0;

      for (size_t i = 0; i < dims[0]; i += SAMPLING_OFFSET) {
            size_t I = i;
            double cur_data_val = (dtype == pressio_float_dtype) ? (double)((float*)data)[I] : (double)((double*)data)[I];
            sampled_data_values.push_back(cur_data_val);

            // Mean value feature
            avg_value += cur_data_val;
				    num_sampled_entries++;

            // Value range feature
				    if(min_val > cur_data_val) min_val = cur_data_val;
				    if(max_val < cur_data_val) max_val = cur_data_val;


            // Mean neighbor difference
            double sum_neighbors = 0.0;
            int num_adj_neighbors = 0;
            
            // Mean lorenzon difference
            int cur_lorenzo_neighbors = 0;	
            double cur_lorenzo_sum = 0.0;

            if (I-1 >= 0 && I-1 < X) {
              sum_neighbors += (dtype == pressio_float_dtype) ? (double)((float*)data)[I-1] : (double)((double*)data)[I-1]; 
              num_adj_neighbors++; 
              cur_lorenzo_sum += (dtype == pressio_float_dtype) ? (double)((float*)data)[I-1] : (double)((double*)data)[I-1]; 
              cur_lorenzo_neighbors++;
            }
            if (I+1 < X) {
              sum_neighbors += (dtype == pressio_float_dtype) ? (double)((float*)data)[I+1] : (double)((double*)data)[I+1]; 
              num_adj_neighbors++;
            }	

            // Mean spline difference
            if (I+3 < X && I-3 >= 0 && I-3 < X) {
              double spline_value1 = getSplinePrediction((dtype == pressio_float_dtype) ? (double)((float*)data)[I-3] : (double)((double*)data)[I-3], 
                                      (dtype == pressio_float_dtype) ? (double)((float*)data)[I-1] : (double)((double*)data)[I-1], 
                                      (dtype == pressio_float_dtype) ? (double)((float*)data)[I+1] : (double)((double*)data)[I+1], 
                                      (dtype == pressio_float_dtype) ? (double)((float*)data)[I+3] : (double)((double*)data)[I+3]);
              sum_dif_splines += fabs(cur_data_val - spline_value1);
                                            num_valid_splines ++;	
            }

            
            
            // Final mean lorenzo difference calculation
            if (cur_lorenzo_neighbors == (1<<num_dims) - 1) {
              cur_lorenzo_sum /= ((1<<num_dims) - 1);	
              sum_dif_lorenzos += fabs( cur_lorenzo_sum - cur_data_val);
              num_valid_lorenzos++;	 	
            }
          

            

            // Final mean neighbor difference calculation
            if(num_adj_neighbors > 0) sum_neighbors /= num_adj_neighbors; 
            if(num_adj_neighbors > 0) { sum_dif_neighbors += fabs(sum_neighbors - cur_data_val); num_sampled_neighbors++; }


      }

      avg_value /= num_sampled_entries;
		
      std::vector<double>features;
      features.push_back(max_val - min_val); 
      features.push_back(avg_value);
      features.push_back(sum_dif_neighbors / (1.0 * num_sampled_neighbors));
      features.push_back(sum_dif_lorenzos / (1.0 * num_valid_lorenzos));	
      features.push_back(sum_dif_splines / ( 1.0 * num_valid_splines) );	

      
      data_min = min_val;
      data_max = max_val; 

      
      return features;

    }
    // Feature extraction end

    // Start run optimization: adjusting compression ratio
    int determineCurrent1DBlock(const void *data, const enum pressio_dtype dtype, 
                                              const size_t* block_starts,
                                              const int num_entries, 
                                              const size_t num_dims, 
                                              std::vector<size_t> dims,
                                              double closeness_threshold)
    {
      size_t min_cell_req = (int)(NUM_CELL_THRESHOLD * (BLOCK_SIZE ));
	    size_t num_cells = 0; 

	    size_t X = dims[0];

      double min_val, max_val;
      int is_first = 1;

      for (size_t i = block_starts[0]; i < dims[0] && i < block_starts[0] + BLOCK_SIZE; i++) {
            size_t I = i;
            double curVal = (dtype == pressio_float_dtype) ? (double)((float*)data)[I] : (double)((double*)data)[I];
            if (is_first) {min_val = curVal; max_val = curVal; is_first = 0;}
            if(min_val > curVal) min_val = curVal;
            if(max_val < curVal) max_val = curVal;
            num_cells++;
      }

      if((num_cells < min_cell_req) || (max_val - min_val > closeness_threshold)){
        return 1;
      }

      return 0;

    }

    double runAdjustingCompressionRatioOptimizationwith1DData(const void *data, const enum pressio_dtype dtype, const int num_entries, 
                                              const size_t num_dims, 
                                              std::vector<size_t> dims,
                                              double closeness_threshold) {
      size_t* starts = (size_t* ) malloc(sizeof(size_t) * num_dims);
      int num_constant_blocks = 0;
	    int num_non_constant_blocks = 0;
	    int num_blocks = 0;

      for (size_t i = 0; i < dims[0]; i += SAMPLING_OFFSET * BLOCK_SIZE) {
            starts[0] = i;
				    num_blocks++;
            int status = determineCurrent2DBlock(data, dtype, starts, num_entries, num_dims, dims, closeness_threshold);
				    if (status == 0) num_constant_blocks++;
				    if (status == 1) num_non_constant_blocks++;
      }

      free(starts);
	    return (1.0 * num_non_constant_blocks) / num_blocks;

    }
    // End run optimization: adjusting compression ratio

    void runFXRZwith1DData(struct pressio_data const* input) {
      std::vector<size_t> dims = input->dimensions();
      const size_t num_dims = input->num_dimensions();
      const int num_entries = input->num_elements();

      double data_min = 0.0, data_max = 0.0;
      sampled_data_values.clear();

      // feature extraction
      std::vector<double> features = runFeatureExtractionwith1DData(input->data(), input->dtype(), num_entries, 
                                                                  num_dims, dims, data_min, data_max);


      // perform scaling to scale up the sampled data in positive range
      //  to obtain the actual mean value
      double val_range = data_max - data_min;
	    double new_min = 0.0, new_max = val_range ;	
	    size_t sampled_entries = 0;
        double value_sum = 0;
      for (double val: sampled_data_values) {
        double new_val = changeScale(val, data_min, data_max, new_min, new_max);
        value_sum += new_val;
        sampled_entries++;
      }
      double avg_val = value_sum / (1.0 * sampled_entries); 	
	    features[1] = avg_val;


      // perform optimization: adjusting compression ratio
      double closeness_threshold = avg_val * 0.15;
      double non_constant_block_percentage = runAdjustingCompressionRatioOptimizationwith1DData(input->data(), 
                                              input->dtype(), num_entries, 
                                              num_dims, 
                                              dims,
                                              closeness_threshold);

      nonConstantBlockPercentage = changeScale(non_constant_block_percentage, 0.0, 1.0, 0.5, 1.0 );
      valueRange = features[0];
      meanValue = features[1];
      meanNeighborDifference = features[2];
      meanLorenzoDifference = features[3];
      meanSplineDifference = features[4];

    }
    /** For 1D dataset end */ 


    /** For 2D dataset start */ 

    // Feature extraction start
    std::vector <double> runFeatureExtractionwith2DData(const void *data, const enum pressio_dtype dtype, const int num_entries, 
                                              const size_t num_dims, 
                                              std::vector<size_t> dims, double &data_min, 
                                              double &data_max) {

      
      // Expecting data dimensions in reverse order, for example with Hurricane dataset 500x500x100
	    size_t XY = dims[0]*dims[1];
	    size_t X = dims[1];

      int num_sampled_entries = 0;
      int num_sampled_lorenzo_entries = 0;
      double avg_value  = 0.0;
      double min_val = (dtype == pressio_float_dtype) ? (double)((float*)data)[0] : (double)((double*)data)[0];
      double max_val = (dtype == pressio_float_dtype) ? (double)((float*)data)[0] : (double)((double*)data)[0];	
      
      // neighbor
      double sum_dif_neighbors = 0.0;
      int num_sampled_neighbors = 0;


      // sum dif spline
      double sum_dif_splines = 0.0;	
      int num_valid_splines = 0;

      // mean dif lorenzo
      double sum_dif_lorenzos = 0.0;
      int num_valid_lorenzos = 0;

      for (size_t i = 0; i < dims[0]; i += SAMPLING_OFFSET) {
		    for (size_t j = 0; j < dims[1]; j += SAMPLING_OFFSET ) {
            size_t I = i*X+j;
            double cur_data_val = (dtype == pressio_float_dtype) ? (double)((float*)data)[I] : (double)((double*)data)[I];
            sampled_data_values.push_back(cur_data_val);

            // Mean value feature
            avg_value += cur_data_val;
				    num_sampled_entries++;

            // Value range feature
				    if(min_val > cur_data_val) min_val = cur_data_val;
				    if(max_val < cur_data_val) max_val = cur_data_val;


            // Mean neighbor difference
            double sum_neighbors = 0.0;
            int num_adj_neighbors = 0;
            
            // Mean lorenzon difference
            int cur_lorenzo_neighbors = 0;	
            double cur_lorenzo_sum = 0.0;

            if (I-1 >= 0 && I-1 < XY) {
              sum_neighbors += (dtype == pressio_float_dtype) ? (double)((float*)data)[I-1] : (double)((double*)data)[I-1]; 
              num_adj_neighbors++; 
              cur_lorenzo_sum += (dtype == pressio_float_dtype) ? (double)((float*)data)[I-1] : (double)((double*)data)[I-1]; 
              cur_lorenzo_neighbors++;
            }
            if (I-X >= 0 && I-X < XY) {
              sum_neighbors += (dtype == pressio_float_dtype) ? (double)((float*)data)[I-X] : (double)((double*)data)[I-X]; 
              num_adj_neighbors++; 
              cur_lorenzo_sum += (dtype == pressio_float_dtype) ? (double)((float*)data)[I-X] : (double)((double*)data)[I-X]; 
              cur_lorenzo_neighbors++;
            }
            if (I+1 < XY) {
              sum_neighbors += (dtype == pressio_float_dtype) ? (double)((float*)data)[I+1] : (double)((double*)data)[I+1]; 
              num_adj_neighbors++;
            }	
            if (I+X < XY) {
              sum_neighbors += (dtype == pressio_float_dtype) ? (double)((float*)data)[I+X] : (double)((double*)data)[I+X];
              num_adj_neighbors++;
            }
            if (I-X-1 >= 0 && I-X-1 < XY) {
              cur_lorenzo_sum -= (dtype == pressio_float_dtype) ? (double)((float*)data)[I-X-1] : (double)((double*)data)[I-X-1]; 
              cur_lorenzo_neighbors++;
            }

            // Mean spline difference
            if (I+3 < XY && I-3 >= 0 && I-3 < XY) {
              double spline_value1 = getSplinePrediction((dtype == pressio_float_dtype) ? (double)((float*)data)[I-3] : (double)((double*)data)[I-3], 
                                      (dtype == pressio_float_dtype) ? (double)((float*)data)[I-1] : (double)((double*)data)[I-1], 
                                      (dtype == pressio_float_dtype) ? (double)((float*)data)[I+1] : (double)((double*)data)[I+1], 
                                      (dtype == pressio_float_dtype) ? (double)((float*)data)[I+3] : (double)((double*)data)[I+3]);
              sum_dif_splines += fabs(cur_data_val - spline_value1);
                                            num_valid_splines ++;	
            }

            if (I+3*X < XY && I-3*X >= 0 && I-3*X < XY) {
              double spline_value2 = getSplinePrediction((dtype == pressio_float_dtype) ? (double)((float*)data)[I-3*X] : (double)((double*)data)[I-3*X], 
                                      (dtype == pressio_float_dtype) ? (double)((float*)data)[I-X] : (double)((double*)data)[I-X], 
                                      (dtype == pressio_float_dtype) ? (double)((float*)data)[I+X] : (double)((double*)data)[I+X], 
                                      (dtype == pressio_float_dtype) ? (double)((float*)data)[I+3*X] : (double)((double*)data)[I+3*X]);
              sum_dif_splines += fabs(cur_data_val - spline_value2);
                                            num_valid_splines ++;

            }

            
            
            // Final mean lorenzo difference calculation
            if (cur_lorenzo_neighbors == (1<<num_dims) - 1) {
              cur_lorenzo_sum /= ((1<<num_dims) - 1);	
              sum_dif_lorenzos += fabs( cur_lorenzo_sum - cur_data_val);
              num_valid_lorenzos++;	 	
            }
          

            

            // Final mean neighbor difference calculation
            if(num_adj_neighbors > 0) sum_neighbors /= num_adj_neighbors; 
            if(num_adj_neighbors > 0) { sum_dif_neighbors += fabs(sum_neighbors - cur_data_val); num_sampled_neighbors++; }


        }
      }

      avg_value /= num_sampled_entries;
		
      std::vector<double>features;
      features.push_back(max_val - min_val); 
      features.push_back(avg_value);
      features.push_back(sum_dif_neighbors / (1.0 * num_sampled_neighbors));
      features.push_back(sum_dif_lorenzos / (1.0 * num_valid_lorenzos));	
      features.push_back(sum_dif_splines / ( 1.0 * num_valid_splines) );	

      
      data_min = min_val;
      data_max = max_val; 

      
      return features;

    }
    // Feature extraction end

    // Start run optimization: adjusting compression ratio
    int determineCurrent2DBlock(const void *data, const enum pressio_dtype dtype, 
                                              const size_t* block_starts,
                                              const int num_entries, 
                                              const size_t num_dims, 
                                              std::vector<size_t> dims,
                                              double closeness_threshold)
    {
      size_t min_cell_req = (int)(NUM_CELL_THRESHOLD * (BLOCK_SIZE * BLOCK_SIZE ));
	    size_t num_cells = 0; 

	    size_t XY = dims[0]*dims[1];
	    size_t X = dims[1];

      double min_val, max_val;
      int is_first = 1;

      for (size_t i = block_starts[0]; i < dims[0] && i < block_starts[0] + BLOCK_SIZE; i++) {
		    for (size_t j = block_starts[1]; j < dims[1] && j < block_starts[1] + BLOCK_SIZE; j++) {			
            size_t I = i*X+j;
            double curVal = (dtype == pressio_float_dtype) ? (double)((float*)data)[I] : (double)((double*)data)[I];
            if (is_first) {min_val = curVal; max_val = curVal; is_first = 0;}
            if(min_val > curVal) min_val = curVal;
            if(max_val < curVal) max_val = curVal;
            num_cells++;
          }
        }

      if((num_cells < min_cell_req) || (max_val - min_val > closeness_threshold)){
        return 1;
      }

      return 0;

    }

    double runAdjustingCompressionRatioOptimizationwith2DData(const void *data, const enum pressio_dtype dtype, const int num_entries, 
                                              const size_t num_dims, 
                                              std::vector<size_t> dims,
                                              double closeness_threshold) {
      size_t* starts = (size_t* ) malloc(sizeof(size_t) * num_dims);
      int num_constant_blocks = 0;
	    int num_non_constant_blocks = 0;
	    int num_blocks = 0;

      for (size_t i = 0; i < dims[0]; i += SAMPLING_OFFSET * BLOCK_SIZE) {
		    for (size_t j = 0; j < dims[1]; j += SAMPLING_OFFSET * BLOCK_SIZE) {
            starts[0] = i, starts[1] = j;
				    num_blocks++;
            int status = determineCurrent2DBlock(data, dtype, starts, num_entries, num_dims, dims, closeness_threshold);
				    if (status == 0) num_constant_blocks++;
				    if (status == 1) num_non_constant_blocks++;
        }
      }

      free(starts);
	    return (1.0 * num_non_constant_blocks) / num_blocks;

    }
    // End run optimization: adjusting compression ratio

    void runFXRZwith2DData(struct pressio_data const* input) {
      std::vector<size_t> dims = input->dimensions();
      const size_t num_dims = input->num_dimensions();
      const int num_entries = input->num_elements();

      double data_min = 0.0, data_max = 0.0;
      sampled_data_values.clear();

      // feature extraction
      std::vector<double> features = runFeatureExtractionwith2DData(input->data(), input->dtype(), num_entries, 
                                                                  num_dims, dims, data_min, data_max);


      // perform scaling to scale up the sampled data in positive range
      //  to obtain the actual mean value
      double val_range = data_max - data_min;
	    double new_min = 0.0, new_max = val_range ;	
	    size_t sampled_entries = 0;
        double value_sum = 0;
      for (double val: sampled_data_values) {
        double new_val = changeScale(val, data_min, data_max, new_min, new_max);
        value_sum += new_val;
        sampled_entries++;
      }
      double avg_val = value_sum / (1.0 * sampled_entries); 	
	    features[1] = avg_val;


      // perform optimization: adjusting compression ratio
      double closeness_threshold = avg_val * 0.15;
      double non_constant_block_percentage = runAdjustingCompressionRatioOptimizationwith2DData(input->data(), 
                                              input->dtype(), num_entries, 
                                              num_dims, 
                                              dims,
                                              closeness_threshold);

      nonConstantBlockPercentage = changeScale(non_constant_block_percentage, 0.0, 1.0, 0.5, 1.0 );
      valueRange = features[0];
      meanValue = features[1];
      meanNeighborDifference = features[2];
      meanLorenzoDifference = features[3];
      meanSplineDifference = features[4];

    }
    /** For 2D dataset end */ 




    /** For 3D dataset start */ 
    // Feature extraction start
    std::vector <double> runFeatureExtractionwith3DData(const void *data, const enum pressio_dtype dtype, const int num_entries, 
                                              const size_t num_dims, 
                                              std::vector<size_t> dims, double &data_min, 
                                              double &data_max) {

      
      // Expecting data dimensions in reverse order, for example with Hurricane dataset 500x500x100
      size_t XYZ = dims[0]*dims[1]*dims[2];
	    size_t XY = dims[1]*dims[2];
	    size_t X = dims[2];

      int num_sampled_entries = 0;
      int num_sampled_lorenzo_entries = 0;
      double avg_value  = 0.0;
      double min_val = (dtype == pressio_float_dtype) ? (double)((float*)data)[0] : (double)((double*)data)[0];
      double max_val = (dtype == pressio_float_dtype) ? (double)((float*)data)[0] : (double)((double*)data)[0];	
      
      // neighbor
      double sum_dif_neighbors = 0.0;
      int num_sampled_neighbors = 0;


      // sum dif spline
      double sum_dif_splines = 0.0;	
      int num_valid_splines = 0;

      // mean dif lorenzo
      double sum_dif_lorenzos = 0.0;
      int num_valid_lorenzos = 0;

      for (size_t i = 0; i < dims[0]; i += SAMPLING_OFFSET) {
		    for (size_t j = 0; j < dims[1]; j += SAMPLING_OFFSET ) {
			    for (size_t k = 0; k < dims[2]; k += SAMPLING_OFFSET ) {
            size_t I = i*XY+j*X+k;
            double cur_data_val = (dtype == pressio_float_dtype) ? (double)((float*)data)[I] : (double)((double*)data)[I];
            sampled_data_values.push_back(cur_data_val);

            // Mean value feature
            avg_value += cur_data_val;
				    num_sampled_entries++;

            // Value range feature
				    if(min_val > cur_data_val) min_val = cur_data_val;
				    if(max_val < cur_data_val) max_val = cur_data_val;


            // Mean neighbor difference
            double sum_neighbors = 0.0;
            int num_adj_neighbors = 0;
            
            // Mean lorenzon difference
            int cur_lorenzo_neighbors = 0;	
            double cur_lorenzo_sum = 0.0;

            if (I-1 >= 0 && I-1 < XYZ) {
              sum_neighbors += (dtype == pressio_float_dtype) ? (double)((float*)data)[I-1] : (double)((double*)data)[I-1]; 
              num_adj_neighbors++; 
              cur_lorenzo_sum += (dtype == pressio_float_dtype) ? (double)((float*)data)[I-1] : (double)((double*)data)[I-1]; 
              cur_lorenzo_neighbors++;
            }
            if (I-X >= 0 && I-X < XYZ) {
              sum_neighbors += (dtype == pressio_float_dtype) ? (double)((float*)data)[I-X] : (double)((double*)data)[I-X]; 
              num_adj_neighbors++; 
              cur_lorenzo_sum += (dtype == pressio_float_dtype) ? (double)((float*)data)[I-X] : (double)((double*)data)[I-X]; 
              cur_lorenzo_neighbors++;
            }
            if (I-XY >= 0 && I-XY < XYZ) {
              sum_neighbors += (dtype == pressio_float_dtype) ? (double)((float*)data)[I-XY] : (double)((double*)data)[I-XY];
              num_adj_neighbors++; 
              cur_lorenzo_sum += (dtype == pressio_float_dtype) ? (double)((float*)data)[I-XY] : (double)((double*)data)[I-XY]; 
              cur_lorenzo_neighbors++;
            }
            if (I+1 < XYZ) {
              sum_neighbors += (dtype == pressio_float_dtype) ? (double)((float*)data)[I+1] : (double)((double*)data)[I+1]; 
              num_adj_neighbors++;
            }	
            if (I+X < XYZ) {
              sum_neighbors += (dtype == pressio_float_dtype) ? (double)((float*)data)[I+X] : (double)((double*)data)[I+X];
              num_adj_neighbors++;
            }
            if (I+XY < XYZ) {
              sum_neighbors += (dtype == pressio_float_dtype) ? (double)((float*)data)[I+XY] : (double)((double*)data)[I+XY]; 
              num_adj_neighbors++;
            }
            if (I-XY-1 >= 0 && I-XY-1 < XYZ) {
              cur_lorenzo_sum -= (dtype == pressio_float_dtype) ? (double)((float*)data)[I-XY-1] : (double)((double*)data)[I-XY-1];
              cur_lorenzo_neighbors++;
            }
            if (I-X-1 >= 0 && I-X-1 < XYZ) {
              cur_lorenzo_sum -= (dtype == pressio_float_dtype) ? (double)((float*)data)[I-X-1] : (double)((double*)data)[I-X-1]; 
              cur_lorenzo_neighbors++;
            }
            if (I-XY-X >= 0 && I-XY-X < XYZ) {
              cur_lorenzo_sum -= (dtype == pressio_float_dtype) ? (double)((float*)data)[I-XY-X] : (double)((double*)data)[I-XY-X]; 
              cur_lorenzo_neighbors++;
            }
            if (I-XY-X-1 >= 0 && I-XY-X-1 < XYZ) {
              cur_lorenzo_sum += (dtype == pressio_float_dtype) ? (double)((float*)data)[I-XY-X-1] : (double)((double*)data)[I-XY-X-1];
              cur_lorenzo_neighbors++;
            }

            // Mean spline difference
            if (I+3 < XYZ && I-3 >= 0 && I-3 < XYZ) {
              double spline_value1 = getSplinePrediction((dtype == pressio_float_dtype) ? (double)((float*)data)[I-3] : (double)((double*)data)[I-3], 
                                      (dtype == pressio_float_dtype) ? (double)((float*)data)[I-1] : (double)((double*)data)[I-1], 
                                      (dtype == pressio_float_dtype) ? (double)((float*)data)[I+1] : (double)((double*)data)[I+1], 
                                      (dtype == pressio_float_dtype) ? (double)((float*)data)[I+3] : (double)((double*)data)[I+3]);
              sum_dif_splines += fabs(cur_data_val - spline_value1);
                                            num_valid_splines ++;	
            }

            if (I+3*X < XYZ && I-3*X >= 0 && I-3*X < XYZ) {
              double spline_value2 = getSplinePrediction((dtype == pressio_float_dtype) ? (double)((float*)data)[I-3*X] : (double)((double*)data)[I-3*X], 
                                      (dtype == pressio_float_dtype) ? (double)((float*)data)[I-X] : (double)((double*)data)[I-X], 
                                      (dtype == pressio_float_dtype) ? (double)((float*)data)[I+X] : (double)((double*)data)[I+X], 
                                      (dtype == pressio_float_dtype) ? (double)((float*)data)[I+3*X] : (double)((double*)data)[I+3*X]);
              sum_dif_splines += fabs(cur_data_val - spline_value2);
                                            num_valid_splines ++;

            }

            if (I+3*XY < XYZ && I-3*XY >= 0 && I-3*XY < XYZ) {
              double spline_value3 = getSplinePrediction((dtype == pressio_float_dtype) ? (double)((float*)data)[I-3*XY] : (double)((double*)data)[I-3*XY], 
                                      (dtype == pressio_float_dtype) ? (double)((float*)data)[I-XY] : (double)((double*)data)[I-XY], 
                                      (dtype == pressio_float_dtype) ? (double)((float*)data)[I+XY] : (double)((double*)data)[I+XY], 
                                      (dtype == pressio_float_dtype) ? (double)((float*)data)[I+3*XY] : (double)((double*)data)[I+3*XY]);
              sum_dif_splines += fabs(cur_data_val - spline_value3);
              num_valid_splines ++;
            }
            
            
            // Final mean lorenzo difference calculation
            if (cur_lorenzo_neighbors == (1<<num_dims) - 1) {
              cur_lorenzo_sum /= ((1<<num_dims) - 1);	
              sum_dif_lorenzos += fabs( cur_lorenzo_sum - cur_data_val);
              num_valid_lorenzos++;	 	
            }
          

            

            // Final mean neighbor difference calculation
            if(num_adj_neighbors > 0) sum_neighbors /= num_adj_neighbors; 
            if(num_adj_neighbors > 0) { sum_dif_neighbors += fabs(sum_neighbors - cur_data_val); num_sampled_neighbors++; }


          }
        }
      }

      avg_value /= num_sampled_entries;
		
      std::vector<double>features;
      features.push_back(max_val - min_val); 
      features.push_back(avg_value);
      features.push_back(sum_dif_neighbors / (1.0 * num_sampled_neighbors));
      features.push_back(sum_dif_lorenzos / (1.0 * num_valid_lorenzos));	
      features.push_back(sum_dif_splines / ( 1.0 * num_valid_splines) );	

      
      data_min = min_val;
      data_max = max_val; 

      
      return features;

    }
    // Feature extraction end

    // Start run optimization: adjusting compression ratio
    int determineCurrent3DBlock(const void *data, const enum pressio_dtype dtype, 
                                              const size_t* block_starts,
                                              const int num_entries, 
                                              const size_t num_dims, 
                                              std::vector<size_t> dims,
                                              double closeness_threshold)
    {
      size_t min_cell_req = (int)(NUM_CELL_THRESHOLD * (BLOCK_SIZE * BLOCK_SIZE * BLOCK_SIZE));
	    size_t num_cells = 0; 

      size_t XYZ = dims[0]*dims[1]*dims[2];
	    size_t XY = dims[1]*dims[2];
	    size_t X = dims[2];

      double min_val, max_val;
      int is_first = 1;

      for (size_t i = block_starts[0]; i < dims[0] && i < block_starts[0] + BLOCK_SIZE; i++) {
		    for (size_t j = block_starts[1]; j < dims[1] && j < block_starts[1] + BLOCK_SIZE; j++) {			
			    for (size_t k = block_starts[2]; k < dims[2] && k < block_starts[2] + BLOCK_SIZE; k++) {
            size_t I = i*XY+j*X+k;
            double curVal = (dtype == pressio_float_dtype) ? (double)((float*)data)[I] : (double)((double*)data)[I];
            if (is_first) {min_val = curVal; max_val = curVal; is_first = 0;}
            if(min_val > curVal) min_val = curVal;
            if(max_val < curVal) max_val = curVal;
            num_cells++;
          }
        }
      }

      if((num_cells < min_cell_req) || (max_val - min_val > closeness_threshold)){
        return 1;
      }

      return 0;

    }

    double runAdjustingCompressionRatioOptimizationwith3DData(const void *data, const enum pressio_dtype dtype, const int num_entries, 
                                              const size_t num_dims, 
                                              std::vector<size_t> dims,
                                              double closeness_threshold) {
      size_t* starts = (size_t* ) malloc(sizeof(size_t) * num_dims);
      int num_constant_blocks = 0;
	    int num_non_constant_blocks = 0;
	    int num_blocks = 0;

      for (size_t i = 0; i < dims[0]; i += SAMPLING_OFFSET * BLOCK_SIZE) {
		    for (size_t j = 0; j < dims[1]; j += SAMPLING_OFFSET * BLOCK_SIZE) {
			    for (size_t k = 0; k < dims[2]; k += SAMPLING_OFFSET * BLOCK_SIZE) {
            starts[0] = i, starts[1] = j, starts[2] = k;
				    num_blocks++;
            int status = determineCurrent3DBlock(data, dtype, starts, num_entries, num_dims, dims, closeness_threshold);
				    if (status == 0) num_constant_blocks++;
				    if (status == 1) num_non_constant_blocks++;
          }
        }
      }

      free(starts);
	    return (1.0 * num_non_constant_blocks) / num_blocks;

    }
    // End run optimization: adjusting compression ratio

    void runFXRZwith3DData(struct pressio_data const* input) {
      std::vector<size_t> dims = input->dimensions();
      const size_t num_dims = input->num_dimensions();
      const int num_entries = input->num_elements();

      double data_min = 0.0, data_max = 0.0;
      sampled_data_values.clear();

      // feature extraction
      std::vector<double> features = runFeatureExtractionwith3DData(input->data(), input->dtype(), num_entries, 
                                                                  num_dims, dims, data_min, data_max);


      // perform scaling to scale up the sampled data in positive range
      //  to obtain the actual mean value
      double val_range = data_max - data_min;
	    double new_min = 0.0, new_max = val_range ;	
	    size_t sampled_entries = 0;
        double value_sum;
      for (double val: sampled_data_values) {
        double new_val = changeScale(val, data_min, data_max, new_min, new_max);
        value_sum += new_val;
        sampled_entries++;
      }
      double avg_val = value_sum / (1.0 * sampled_entries); 	
	    features[1] = avg_val;


      // perform optimization: adjusting compression ratio
      double closeness_threshold = avg_val * 0.15;
      double non_constant_block_percentage = runAdjustingCompressionRatioOptimizationwith3DData(input->data(), 
                                              input->dtype(), num_entries, 
                                              num_dims, 
                                              dims,
                                              closeness_threshold);

      nonConstantBlockPercentage = changeScale(non_constant_block_percentage, 0.0, 1.0, 0.5, 1.0 );
      valueRange = features[0];
      meanValue = features[1];
      meanNeighborDifference = features[2];
      meanLorenzoDifference = features[3];
      meanSplineDifference = features[4];

    }
    /** For 3D dataset end */ 
      
};

static pressio_register metrics_rahman2023_plugin(metrics_plugins(), "rahman2023", [](){ return compat::make_unique<rahman2023_plugin>(); });
}}
