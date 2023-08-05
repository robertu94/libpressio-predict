stat_gen=function(field_no, dim3)
{
  #############################################################################################################
  #CR and spatial statistics for 2D array
  stat_gen_2D= function(array_2D)
  {
    sample_vx1 <- array_2D
    p=ncol(sample_vx1)
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
      #saveRDS(metrics, paste0("compressor=", compressor,"_", error_bound ,"_P=",Psill,"_N=",Nugget, "_R=", Range, "_p=",p,"metrics.rds"))
      #print(metrics)
      
      #access decompressed data
      decompressed_r <- libpressio::data_to_R(decompressed_lp)
      #saving the image
      #setwd("/home/aganguli/compression/Gaussian_images")
      #pdf(paste0("P=",Psill,"_N=",Nugget, "_R=", Range, "_p=",p,"_uncompressed.pdf"))
      #image.plot(sample_vx1[seq(1, p), seq(1, p)], main='2D-Gaussian sample', col=parula(500))
      #dev.off()
      
      #pdf(paste0("P=",Psill,"_N=",Nugget, "_R=", Range, "_p=",p,"_decompressed.pdf"))
      #image.plot(decompressed_r[seq(1, p), seq(1, p)], main='2D-Gaussian sample', col=parula(500))
      #dev.off()
      #saving the metrics
      CR= metrics$`size:compression_ratio`
      return(CR)
    }
    CR_sz= c(calc_CR("sz",1e-3), calc_CR("sz",1e-4), calc_CR("sz",1e-6))
    CR_zfp= c(calc_CR("zfp", 1e-3), calc_CR("zfp", 1e-4), calc_CR("zfp", 1e-6))
    #CR_tthresh= c(calc_CR("tthresh",1e-3), calc_CR("tthresh",1e-4), calc_CR("tthresh",1e-6))
    CR_sperr= c(calc_CR("sperr",1e-3), calc_CR("sperr",1e-4), calc_CR("sperr",1e-6))
    print("getting CR is done")
    #decompressed_r=  list(calc_CR("sz",1e-3 )[[2]], calc_CR("sz",1e-4 )[[2]], calc_CR("sz",1e-6 )[[2]], 
                          #calc_CR("zfp",1e-3 )[[2]], calc_CR("zfp",1e-4 )[[2]], calc_CR("zfp",1e-6 )[[2]])
    scaled_sample_vx1= scale(sample_vx1, scale=FALSE)
    #defining the intra-block correlation
    # kXk block formation
    k=100
    #n_blocks=floor((p/k)^2)
    mat_split <- function(M, r, c){
      nr <- ceiling(nrow(M)/r)
      nc <- ceiling(ncol(M)/c)
      newM <- matrix(NA, nr*r, nc*c)
      newM[1:nrow(M), 1:ncol(M)] <- M
      
      div_k <- kronecker(matrix(seq_len(nr*nc), nr, byrow = TRUE), matrix(1, r, c))
      matlist <- split(newM, div_k)
      N <- length(matlist)
      #mats <- unlist(matlist)
      #dim(mats)<-c(r, c, N)
      return(matlist)
    }
    
    blocks= mat_split(scaled_sample_vx1,k,k)
    #blocks=blocks[-c(5,10,15,20,21,22,23,24,25)]
    n_blocks= length(blocks)
    #____________________________________________________________________________#_____________________________________________________________________________________________
    #checking the average distance among the blocks
    zero_sd_blocks= which(lapply(blocks, sd)==0)
    if(length(zero_sd_blocks)==n_blocks){print("all zero extries")}
    
    if(length(zero_sd_blocks)<n_blocks)
    {
      valid_blocks_pos=  seq(1:n_blocks)
      if(length(zero_sd_blocks)>0){ valid_blocks_pos= seq(1:n_blocks)[-zero_sd_blocks]} 
      nonzero_blocks=blocks[valid_blocks_pos]
      n_nonzero_blocks= length(nonzero_blocks)
      all_blocks_as_row= matrix(unlist(nonzero_blocks), nrow=n_nonzero_blocks, ncol=k^2, byrow=T)
      dist_matrix= as.matrix(dist(all_blocks_as_row,upper = T,diag = T))
      print("dist_matrix is done")
      #checking the spatial variation via average distance among blocks
      #B=sqrt(n_blocks)
      B=18 #col-blocks
      row_index= function(bl){return(ifelse(bl%%B==0, (bl%/%B), (bl%/%B +1)))}
      col_index= function(bl){return(ifelse(bl%%B==0, B, (bl%%B)))}
      spatial_var_per_block= function(b)
      {
        weights_pos= sapply(seq(1:n_nonzero_blocks), function(x){return(abs(row_index(valid_blocks_pos[b])-row_index(valid_blocks_pos[x]))+abs(col_index(valid_blocks_pos[b])-col_index(valid_blocks_pos[x])))})
        spatial_var_measure= sum(sqrt(weights_pos[-b]*dist_matrix[b,-b]))/sum(sqrt(weights_pos[-b]))
        return(spatial_var_measure)
      }
      spatial_var_all=sapply(1:n_nonzero_blocks, spatial_var_per_block)
      weights_intra= sapply(1:n_nonzero_blocks, function(b){return(sum(sapply(1:(k^2), function(x){return(sum((c(nonzero_blocks[[b]])-c(nonzero_blocks[[b]][x]))^2))}))/(k^2*(k^2-1)))})
      spatial_var_final= -sum(weights_intra*spatial_var_all)*log(1/n_blocks)*(1/n_blocks)
      print("svar is done")
      #checking the spatial variation via covariance among blocks
      cov_calc_per_block= function(b){return(sapply(valid_blocks_pos, function(x){return(cor(c(blocks[[b]]),c(blocks[[x]])))}))}
      var_cov_blocks=t(sapply(valid_blocks_pos, cov_calc_per_block))
      spatial_cor_per_block= function(b)
      {
        weights_pos= sapply(seq(1:n_nonzero_blocks), function(x){return(abs(row_index(valid_blocks_pos[b])-row_index(valid_blocks_pos[x]))+abs(col_index(valid_blocks_pos[b])-col_index(valid_blocks_pos[x])))})
        spatial_cor_measure= sum(sqrt(weights_pos[-b])*(abs(var_cov_blocks[-b,b])))/sum(sqrt(weights_pos[-b]))
        return(spatial_cor_measure)
      }
      spatial_cor_all=sapply(1:length(valid_blocks_pos), spatial_cor_per_block)
      spatial_cor_final= sum(spatial_cor_all/weights_intra)/sum(1/weights_intra)
      
      max_spatial_cor_per_block= sapply(1:length(valid_blocks_pos), function(x){return(max(abs(var_cov_blocks[x,-x])))})
      min_spatial_cor_per_block= sapply(1:length(valid_blocks_pos), function(x){return(min(abs(var_cov_blocks[x,-x])))})
      spatial_cor_minmax=min(max_spatial_cor_per_block)
      spatial_cor_min=min(min_spatial_cor_per_block)
      print("scor is done")

      
      
      
      #________________________________________________________________________________________________________________________________
      #finding coding gain for C_intra
      add <- function(x) Reduce("+", x)
      C_intra= add(lapply(sample(seq(1,n_nonzero_blocks,1),50,replace=F), function(x){return(cbind(nonzero_blocks[[x]])%*%t(cbind(nonzero_blocks[[x]])))}))/50
      #C_intra= add(lapply(1:n_nonzero_blocks, function(x){return(cbind(nonzero_blocks[[x]])%*%t(cbind(nonzero_blocks[[x]])))}))/n_nonzero_blocks
      s_values_C_intra= svd(C_intra,nu=0,nv=0)$d
      print("svd is done")
      gaussian_coding_gain_C_intra= mean(s_values_C_intra)/exp(mean(log(s_values_C_intra)))
      
      coding_gain_C_intra= exp(mean(log(diag(C_intra))))/exp(mean(log(s_values_C_intra)))
      
      var_explained= sapply(1:length(s_values_C_intra),function(x){return(sum(s_values_C_intra[1:x])/sum(s_values_C_intra))})
      svd_trunc=(min(which(var_explained>=.99))/length(s_values_C_intra))*100
      #____________________________________________________________________________________________________________________________________
      #Assumption-free coding gain calculation
      entropy_calculation_ecdf= function(b)
      {
        Delta=(max(b)-min(b))/15
        if(Delta==0){return(0)}
        else
        {
          points= seq(min(b), max(b), Delta)
          #points= c(min(b)-Delta, points, max(b)+Delta)
          ecdf= sapply(points, function(x){mean(b<=x)})
          p=diff(ecdf)
          for(i in 1:length(p)){if(p[i]==0){p[i]=1e-10}}
          return(-sum(p*log2(p)))
        }
      }
      #blocks_input= mat_split(sample_vx1,k,k)[-c(5,10,15,20,21,22,23,24,25)]
      blocks_input= mat_split(sample_vx1,k,k)
      nonzero_input_blocks=blocks_input[valid_blocks_pos]
      input_blockwise_entropy= sapply( nonzero_input_blocks, entropy_calculation_ecdf)
      
      quantized_entropy= function(error_bound)
      {
        quantization= function(b){return(floor(b/error_bound)*error_bound)}
        quant_blocks= lapply(nonzero_input_blocks, quantization)
        return(sapply(quant_blocks, entropy_calculation_ecdf))
      }
      quant_blockwise_entropy_1= quantized_entropy(1e-3)
      quant_blockwise_entropy_2= quantized_entropy(1e-4)
      quant_blockwise_entropy_3= quantized_entropy(1e-6)
      blockwise_distortion_1= sum(sapply(1:n_nonzero_blocks, function(x){return(2^{(-2)*(quant_blockwise_entropy_1[x]-input_blockwise_entropy[x])}/12)}))
      blockwise_distortion_2= sum(sapply(1:n_nonzero_blocks, function(x){return(2^{(-2)*(quant_blockwise_entropy_2[x]-input_blockwise_entropy[x])}/12)}))
      blockwise_distortion_3= sum(sapply(1:n_nonzero_blocks, function(x){return(2^{(-2)*(quant_blockwise_entropy_3[x]-input_blockwise_entropy[x])}/12)}))
      
      
      
      #____________________________________________________________________________________________________________________________________
      #Return statistics
      stats=c(CR_sz, CR_zfp, CR_sperr,mean(sample_vx1), sd(sample_vx1), range(sample_vx1),spatial_var_final,spatial_cor_final,mean(spatial_cor_all), spatial_cor_minmax, spatial_cor_min,
              log10(gaussian_coding_gain_C_intra), log10(coding_gain_C_intra), svd_trunc, blockwise_distortion_1, blockwise_distortion_2, blockwise_distortion_3)
      stats
      

      
      row <- data.frame(t(stats))
      setwd("/home/aganguli/compression/real-datasets/Climate")
      write.table(row, file = 'climate_data_summary_stats_wise_V04.csv', sep = ",", append = TRUE, quote = FALSE, col.names = FALSE, row.names = TRUE)
    }
  }

  #############################################################################################################
  #CR and spatial statistics for 3D array
  stat_gen_3D= function(array_3D)
  {
      array_2D_slice= array_3D[,,dim3]
      stat_gen_2D(array_2D_slice)
  }

  ##############################################################################################################
  library(ncdf4)
  #wd <- "/home/aganguli/compression/real-datasets/qmcpack_h5/" #This will depend on your local environment
  wd <- "/lcrc/group/earthscience/fusion_climate/Parvis/atmos/taylor"
  setwd(wd)
  files=list.files()
  file_index=3 #create a for-loop for all the dataset
  file= files[file_index]
  data<- nc_open(file)
  vars= names(data$var)
  #collect the proper vars
  var_index= rep(1,length(vars))
  for(i in 1:length(vars))
  {
    var_data <- ncvar_get(data, vars[i]) 
    if(length(var_data)<10){var_index[i]=0}else{if(length(dim(var_data))<2){var_index[i]=0 }else{if(ifelse(is.na(sd(var_data)),0,sd(var_data))<.01){var_index[i]=0}}}
  }
  vars_sel= vars[which(var_index==1)]
  print("got vars")
  v=vars_sel[field_no] #create a for-loop for all the fields
  var_data= ncvar_get(data, v) 
  print("job starting")
  if(length(dim(var_data))==2)
  {
       print("2D array is skippped") 
  }
  if(length(dim(var_data))==3)
  {
    stat_gen_3D(var_data)
    id= rep(file_index, dim(var_data)[3])
    var_names= rep(v, dim(var_data)[3])
    print(c(id, v))
    row <- data.frame(t(rbind(id,var_names)))
    setwd("/home/aganguli/compression/real-datasets/Climate")
    write.table(row, file = 'climate_data_id_var_new.csv', sep = ",", append = TRUE, quote = FALSE, col.names = FALSE, row.names = TRUE)
    print(paste0("3D array done for", dim3)) 
  }
}
imp_field= c(5,6,7,28,30,31,36,37,44,52,67,68,69,70,71,72,73)

args <- commandArgs(trailingOnly = TRUE)
stat_gen(imp_field[as.numeric(args[1])], as.numeric(args[2]))


