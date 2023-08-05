setwd("/Users/arkaprabhaganguli/Documents/OneDrive - Michigan State University/Documents/NSF MSGI 2022/experiment codes/")
data= read.csv("velocityx_summary_statistics.csv", header=FALSE)[,-c(1)]
data=data[which(data[,1]<100), ]
colnames(data)= c("CR_SZ_1e.3" , "CR_SZ_1e.4" , 
                  "CR_SZ_1e.6", "CR_ZFP_1e.3", "CR_ZFP_1e.4"  , "CR_ZFP_1e.6", "CR_sperr_1e.3",  "CR_sperr_1e.4", 
                  "CR_sperr_1e.6","mean" ,"sd",  "min",  "max" , "spatial_var" , 
                  "spatial_cor_weighted" , "spatial_cor_mean",  "spatial_cor_minimax",  
                  "spatial_cor_min", "log_10.homoscadastic_coding_gain_intra.", "log_10.heteroscadastic_coding_gain_intra." ,
                  "svd_trunc_intra" , "Distortion_1e.3"  ,"Distortion_1e.4"  ,  
                  "Distortion_1e.6" )

line2user <- function(line, side) {
  lh <- par('cin')[2] * par('cex') * par('lheight')
  x_off <- diff(grconvertX(0:1, 'inches', 'user'))
  y_off <- diff(grconvertY(0:1, 'inches', 'user'))
  switch(side,
         `1` = par('usr')[3] - line * y_off * lh,
         `2` = par('usr')[1] - line * x_off * lh,
         `3` = par('usr')[4] + line * y_off * lh,
         `4` = par('usr')[2] + line * x_off * lh,
         stop("side must be 1, 2, 3, or 4", call.=FALSE))
}
par(mfrow = c(3, 2))
plot(data$spatial_var, data$CR_SZ_1e.3, xlab="Spatial Variability", ylab= "CR", col="blue", cex.lab=1.4)
plot(data$spatial_cor_mean, data$CR_SZ_1e.3, xlab="Spatial Correlation", ylab= "CR", col="blue", cex.lab=1.4)
plot(data$log_10.heteroscadastic_coding_gain_intra., data$CR_SZ_1e.3, xlab="Coding Gain", ylab= "CR", col="green", cex.lab=1.4)
plot(data$Distortion_1e.3, data$CR_SZ_1e.3, xlab="Distortion", ylab= "CR", , col="orange", cex.lab=1.4)
plot(data$svd_trunc_intra, data$CR_SZ_1e.3, xlab="SVD-trunc", ylab= "CR", col="darkblue", cex.lab=1.4)
