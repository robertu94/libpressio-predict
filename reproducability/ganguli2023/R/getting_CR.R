#Creating the image
library('fields')
library('pals')
library('gstat')
library('sp') 
p=512
Field <- expand.grid(1:p/(p/10), 1:p/(p/10))  # `distance' on the grid goes between 0 and 10
names(Field) <- c('x','y')
Psill <- 5 #    ## Partial sill = Magnitude of variation
Nugget <- 1e-8  ## Small-scale variations
Beta <- 0       ## mean yieldin the field

### Parameter to be varied: 
Range <- 5  ## Maximal distance of autocorrelation, range is varied between c('1', '3', '5', '7', '10', '13')
nsim <- 3    ## Number of generated samples

RDT_modelling <- gstat(formula=z~1, ## We assume that there is a constant trend in the data
                       locations=~x+y,
                       dummy=T,    ## Logical value to set to True for unconditional simulation
                       beta=Beta,  ## Necessity to set the average value over the field
                       model=vgm(psill=Psill, range=Range, nugget=Nugget,
                                 model='Gau'), ## Spherical semi-variogram model
                       nmax=40) ## number of nearest observations used for each new prediction

## Simulate the yield spatial structure within the field
RDT_gaussian_field <- predict(RDT_modelling, newdata=Field, nsim=nsim) 

sample_vx1 <- matrix(RDT_gaussian_field[[3]], p, p)
calc_CR= function(compressor,error_bound)
{
  input_lp <- libpressio::data_from_R_typed(sample_vx1, libpressio::DType.float)
  compressed_lp <- libpressio::data_new_empty(libpressio::DType.byte, numeric(0))
  decompressed_lp <- libpressio::data_new_clone(input_lp)
  
  #load the compressor
  lib <- libpressio::get_instance()
  pressio <- libpressio::get_compressor(lib, "pressio")
  
  #configure the meta-compressor
  pressio_opts <- libpressio::compressor_get_options(pressio)
  pressio_opts_R <- libpressio::options_to_R(pressio_opts)
  pressio_opts_R$`pressio:compressor` <- compressor
  pressio_opts_R$`pressio:metric` <- "composite"
  pressio_opts <- libpressio::options_from_R_typed(pressio_opts_R, pressio_opts)
  libpressio::compressor_set_options(pressio, pressio_opts)
  
  #configure the compressor
  pressio_opts <- libpressio::compressor_get_options(pressio)
  pressio_opts_R <- list()
  pressio_opts_R$`pressio:abs` <- error_bound
  pressio_opts_R$`composite:plugins` <- c("error_stat","size", "time","data_gap")
  pressio_opts <- libpressio::options_from_R_typed(pressio_opts_R, pressio_opts)
  libpressio::compressor_set_options(pressio, pressio_opts)

  pressio_opts <- libpressio::compressor_get_options(pressio)
  pressio_opts_R <- libpressio::options_to_R(pressio_opts)
  #print(pressio_opts_R)
  
  #run the compressor
  libpressio::compressor_compress(pressio, input_lp, compressed_lp)
  libpressio::compressor_decompress(pressio, compressed_lp, decompressed_lp)
  
  #get metrics
  metrics <- libpressio::options_to_R(libpressio::compressor_get_metrics_results(pressio))
  saveRDS(metrics, paste0("compressor=", compressor,"_", error_bound ,"_P=",Psill,"_N=",Nugget, "_R=", Range, "_p=",p,"metrics.rds"))
  #print(metrics)
  
  #access decompressed data
  decompressed_r <- libpressio::data_to_R(decompressed_lp)
  #saving the image
  setwd("/tmp/test_r")
  pdf(paste0("P=",Psill,"_N=",Nugget, "_R=", Range, "_p=",p,"_uncompressed.pdf"))
  image.plot(sample_vx1[seq(1, p), seq(1, p)], main='2D-Gaussian sample', col=parula(500))
  dev.off()
  
  pdf(paste0("P=",Psill,"_N=",Nugget, "_R=", Range, "_p=",p,"_decompressed.pdf"))
  image.plot(decompressed_r[seq(1, p), seq(1, p)], main='2D-Gaussian sample', col=parula(500))
  dev.off()
  #saving the metrics
  CR= metrics$`size:compression_ratio`
  return(CR)
}

calc_CR('sz', 1e-4)

