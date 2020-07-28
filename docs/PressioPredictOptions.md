# Configuration Options {#predictoptions}

This page summarizes the options used by the predict meta compressor.

Here is a list of the current prediction algorithms:


+ Nearest Mahalanoblis Neighbor (mahalanoblis)

## Common Options

| Option Name                      | Type          | Description                                                                 |
|----------------------------------|---------------|-----------------------------------------------------------------------------|
| `predict:training_feature_names` | string[]      | the names of the features used for prediction (i.e. `sz:rel_error_bound`    |
| `predict:training_target_names`  | string[]      | the names of the outputs used for prediction (i.e. `size:compression_ratio` |
| `predict:training_features`      | pressio\_data | the inputs that are to be used for training                                 |
| `predict:training_targets`       | pressio\_data | the outputs that are to be used for training                                |

## Nearest Mahalanoblis Neighbor (mahalanoblis)

| Option Name                      | Type          | Description                                                                 |
|----------------------------------|---------------|-----------------------------------------------------------------------------|
| `mahalanoblis:k`                 | unsigned int  | the number  of nearest Neighbors to use in the prediction                   |
