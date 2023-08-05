setwd("/Users/arkaprabhaganguli/Documents/OneDrive - Michigan State University/Documents/NSF MSGI 2022/experiment codes/")
data_all= read.csv("qmcpack_summary_statistics.csv", header=FALSE)[,-c(1,2,3)]
colnames(data_all)= c("CR_SZ_1e.3" , "CR_SZ_1e.4" , 
                      "CR_SZ_1e.6", "CR_ZFP_1e.3", "CR_ZFP_1e.4"  , "CR_ZFP_1e.6", "CR_sperr_1e.3",  "CR_sperr_1e.4", 
                      "CR_sperr_1e.6","mean" ,"sd",  "min",  "max" , "spatial_var" , 
                      "spatial_cor_weighted" , "spatial_cor_mean",  "spatial_cor_minimax",  
                      "spatial_cor_min", "log_10.homoscadastic_coding_gain_intra.", "log_10.heteroscadastic_coding_gain_intra." ,
                      "svd_trunc_intra" , "Distortion_1e.3"  ,"Distortion_1e.4"  ,  
                      "Distortion_1e.6" )
train_index= c(1:20, 40:60, 80:100, 120:140, 160:180, 200:220, 240:280)
data_train_o= data_all[unlist(lapply(train_index, function(x){return(c(((x-1)*115+1):(x*115)))})),]
data_train_o= data_train_o[data_train_o$CR_SZ_1e.3<150,]
data_train=data_train_o
data_train[,1:9]=log(data_train[,1:9])
colnames(data_train)[20]="CodingGain"
n=nrow(data_train)
#scaling the data
library(caret)
normalizing_param= preProcess(data_train[,-c(1:9)])
data_train[,-c(1:9)]= predict(normalizing_param, data_train[,-c(1:9)])

data_test=list(NULL)
test_index= c(21:39, 61:79, 101:119, 141:159,181:199,221:239)
for(i in 1:length(test_index))
{
  data_test[[i]]= data_all[((test_index[i]-1)*115+1):(test_index[i]*115),]
  data_test[[i]]= data_test[[i]][data_test[[i]]$CR_SZ_1e.3<150, ]
  data_test[[i]][,1:9]=log(data_test[[i]][,1:9])
  colnames(data_test[[i]])[20]="CodingGain"
  data_test[[i]][,-c(1:9)]= predict(normalizing_param, data_test[[i]][,-c(1:9)])
}

wss= sapply(1:10, function(x){kmeans(data_train[,c(1,14,16,20,21,22)], x, algorithm="Lloyd",iter.max = 1000, nstart = 2000)$tot.withinss})
plot(c(1:10),wss, main="within sum of squares vs # clusters", xlab="#clusters", ylab="wss" )

kmm= kmeans(data_train[,c(1,14,16,20,21,22)], 4, algorithm="Lloyd", iter.max = 1000, nstart = 2000)

fviz_cluster(kmm, data = data_train[,c(1,14,16,20,21,22)],
             geom = "point", ellipse = FALSE,
             ggtheme = theme_bw()
)

diamond_color_colors <-setNames(c("darkred", "blue", "green", "darkmagenta", "tan1"), levels(as.factor(kmm$cluster)))
p=ggplot(data_train, aes(x =spatial_var, fill = as.factor(kmm$cluster))) +                       # Draw overlaying histogram
  geom_histogram(position = "identity", alpha = 0.2, bins = 50)+scale_fill_manual(values = diamond_color_colors)


data=read.csv("qmcpack_SZ_1e-3.csv",header=T)[,-1]
# Dimension reduction using PCA
res.pca <- prcomp(data[, c(1,14,16,20,21,22)],  scale = TRUE)
# Coordinates of individuals
ind.coord <- as.data.frame(get_pca_ind(res.pca)$coord)
# Add clusters obtained using the K-means algorithm
ind.coord$cluster <- factor(data[,25])
# Data inspection
head(ind.coord)

# Percentage of variance explained by dimensions
eigenvalue <- round(get_eigenvalue(res.pca), 1)
variance.percent <- eigenvalue$variance.percent
head(eigenvalue)

ggscatter(
  ind.coord, x = "Dim.1", y = "Dim.2", 
  color = "cluster", palette = "npg", ellipse = FALSE,
  size = 1.5,  legend = "right", ggtheme = theme_bw(),
  xlab = paste0("Dim 1 (", variance.percent[1], "% )" ),
  ylab = paste0("Dim 2 (", variance.percent[2], "% )" )
) +
  stat_mean(aes(color = cluster), size = 4)

diamond_color_colors <-setNames(c("darkred", "blue", "green", "darkmagenta", "tan1"), levels(as.factor(data$cl_alloc)))
p=ggplot(data, aes(x =CR_SZ_1e.3, fill = as.factor(cl_alloc))) +                       # Draw overlaying histogram
  geom_histogram(position = "identity", alpha = 0.2, bins = 50)+scale_fill_manual(values = diamond_color_colors)
print(p)




setwd("/Users/arkaprabhaganguli/Documents/OneDrive - Michigan State University/Documents/NSF MSGI 2022/experiment codes/")
data_1= as.matrix(read.csv("miranda_velocityx_svalues.csv",header=F)[,-1])
colnames(data_1)= sapply(1:ncol(data_1), function(i){return(paste0("P_",i))})
matplot(t(data_1[,1:20]), type = "l", ylab= "value", main= "miranda_velocityx")


setwd("/Users/arkaprabhaganguli/Documents/OneDrive - Michigan State University/Documents/NSF MSGI 2022/experiment codes/")
data_2= as.matrix(read.csv("miranda_velocityy_svalues.csv",header=F)[,-1])
colnames(data_2)= sapply(1:ncol(data_2), function(i){return(paste0("P_",i))})
matplot(t(data_2[,1:20]), type = "l", ylab= "value", main= "miranda_velocityy")


setwd("/Users/arkaprabhaganguli/Documents/OneDrive - Michigan State University/Documents/NSF MSGI 2022/experiment codes/")
data_3= as.matrix(read.csv("miranda_velocityz_svalues.csv",header=F)[,-1])
colnames(data_3)= sapply(1:ncol(data_3), function(i){return(paste0("P_",i))})
matplot(t(data_3[,1:20]), type = "l", ylab= "value", main= "miranda_velocityz")

setwd("/Users/arkaprabhaganguli/Documents/OneDrive - Michigan State University/Documents/NSF MSGI 2022/experiment codes/")
data_4= as.matrix(read.csv("miranda_diffusivity_svalues.csv",header=F)[,-1])
colnames(data_4)= sapply(1:ncol(data_4), function(i){return(paste0("P_",i))})
matplot(t(data_4[,1:20]), type = "l", ylab= "value", main= "miranda_diffusivity")

setwd("/Users/arkaprabhaganguli/Documents/OneDrive - Michigan State University/Documents/NSF MSGI 2022/experiment codes/")
data_5= as.matrix(read.csv("miranda_density_svalues.csv",header=F)[,-1])
colnames(data_5)= sapply(1:ncol(data_5), function(i){return(paste0("P_",i))})
matplot(t(data_5[,1:20]), type = "l", ylab= "value", main= "miranda_density")

setwd("/Users/arkaprabhaganguli/Documents/OneDrive - Michigan State University/Documents/NSF MSGI 2022/experiment codes/")
data_6= as.matrix(read.csv("miranda_pressure_svalues.csv",header=F)[,-1])
colnames(data_6)= sapply(1:ncol(data_6), function(i){return(paste0("P_",i))})
matplot(t(data_6[,1:20]), type = "l", ylab= "value", main= "miranda_pressure")

setwd("/Users/arkaprabhaganguli/Documents/OneDrive - Michigan State University/Documents/NSF MSGI 2022/experiment codes/")
data_7= as.matrix(read.csv("miranda_viscocity_svalues.csv",header=F)[,-1])
colnames(data_7)= sapply(1:ncol(data_7), function(i){return(paste0("P_",i))})
matplot(t(data_7[,1:20]), type = "l",ylab= "value", main= "miranda_viscocity")






###########################################################################################



setwd("/Users/arkaprabhaganguli/Documents/OneDrive - Michigan State University/Documents/NSF MSGI 2022/experiment codes/")
data_1= as.matrix(read.csv("precip_svalues.csv",header=F)[,-1])
colnames(data_1)= sapply(1:ncol(data_1), function(i){return(paste0("P_",i))})
matplot(t(data_1[,1:20]), type = "l", ylab= "value", main= "Hurricane_Precip_log10")

data_2= as.matrix(read.csv("cloud_svalues.csv",header=F)[,-1])
colnames(data_2)= sapply(1:ncol(data_2), function(i){return(paste0("P_",i))})
matplot(t(data_2[,1:20]), type = "l", ylab= "value", main= "Hurricane_Cloud_log10")

data_3= as.matrix(read.csv("qcloud_svalues.csv",header=F)[,-1])
colnames(data_3)= sapply(1:ncol(data_3), function(i){return(paste0("P_",i))})
matplot(t(data_3[,1:20]), type = "l", ylab= "value", main= "Hurricane_QCloud_log10")

data_4= as.matrix(read.csv("qgraup_svalues.csv",header=F)[,-1])
colnames(data_4)= sapply(1:ncol(data_4), function(i){return(paste0("P_",i))})
matplot(t(data_4[,1:20]), type = "l", ylab= "value", main= "Hurricane_QGRAUP_log10")


data_5= as.matrix(read.csv("qrain_svalues.csv",header=F)[,-1])
colnames(data_5)= sapply(1:ncol(data_5), function(i){return(paste0("P_",i))})
matplot(t(data_5[,1:20]), type = "l", ylab= "value", main= "Hurricane_QRain_log10")


data_6= as.matrix(read.csv("qice_svalues.csv",header=F)[,-1])
colnames(data_6)= sapply(1:ncol(data_6), function(i){return(paste0("P_",i))})
matplot(t(data_6[,1:20]), type = "l", ylab= "value", main= "Hurricane_QIce_log10")

data_7= as.matrix(read.csv("qsnow_svalues.csv",header=F)[,-1])
colnames(data_7)= sapply(1:ncol(data_7), function(i){return(paste0("P_",i))})
matplot(t(data_7[,1:20]), type = "l", ylab= "value", main= "Hurricane_QSnow_log10")


##################################################################################################

#First run the code "getting_s_values.R" for all the fields and same with corresponding field's name. Then run the following code to get the distance metric. 

setwd("/Users/arkaprabhaganguli/Documents/OneDrive - Michigan State University/Documents/NSF MSGI 2022/experiment codes/")
data=list(NULL)
data_1= as.matrix(read.csv("miranda_velocityx_svalues.csv",header=F)[,-1])
data[[1]]= scale(data_1[,1:10])
data_2= as.matrix(read.csv("miranda_velocityy_svalues.csv",header=F)[,-1])
data[[2]]= scale(data_2[,1:10])
data_3= as.matrix(read.csv("miranda_velocityz_svalues.csv",header=F)[,-1])
data[[3]]= scale(data_3[,1:10])
data_4= as.matrix(read.csv("miranda_diffusivity_svalues.csv",header=F)[,-1])
data[[4]]= scale(data_4[,1:10])
data_5= as.matrix(read.csv("miranda_density_svalues.csv",header=F)[,-1])
data[[5]]= scale(data_5[,1:10])
data_6= as.matrix(read.csv("miranda_pressure_svalues.csv",header=F)[,-1])
data[[6]]= scale(data_6[,1:10])
data_7= as.matrix(read.csv("miranda_viscocity_svalues.csv",header=F)[,-1])
data[[7]]= scale(data_7[,1:10])

m=matrix(rep(0,7*7), ncol=7)
for(i in 1:7)
{
  for(j in 1:7)
  {
    m[i,j]=(median(mahalanobis(data[[i]], apply(data[[j]], 2,mean), cov(data[[j]])))*sd(mahalanobis(data[[i]], apply(data[[j]], 2,mean), cov(data[[j]])))+
                median(mahalanobis(data[[j]], apply(data[[i]], 2,mean), cov(data[[i]])))*sd(mahalanobis(data[[j]], apply(data[[i]], 2,mean), cov(data[[i]]))))/2
  }
}
colnames(m)= c("Velocity_X", "Velocity_Y", "Velocity_Z", "Diffusivity", "Density", "Pressure", "Viscocity")
rownames(m)= c("Velocity_X", "Velocity_Y", "Velocity_Z", "Diffusivity", "Density", "Pressure", "Viscocity")

##################################################################################################

data_1= as.matrix(read.csv("miranda_velocityx_svalues.csv",header=F)[,-1])
data[[1]]= -t(apply(data_1, 1, function(x){return(diff(x[1:10])/x[1:9])}))
data_2= as.matrix(read.csv("miranda_velocityy_svalues.csv",header=F)[,-1])
data[[2]]= -t(apply(data_2, 1, function(x){return(diff(x[1:10])/x[1:9])}))
data_3= as.matrix(read.csv("miranda_velocityz_svalues.csv",header=F)[,-1])
data[[3]]= -t(apply(data_3, 1, function(x){return(diff(x[1:10])/x[1:9])}))
data_4= as.matrix(read.csv("miranda_diffusivity_svalues.csv",header=F)[,-1])
data[[4]]= -t(apply(data_4, 1, function(x){return(diff(x[1:10])/x[1:9])}))
data_5= as.matrix(read.csv("miranda_density_svalues.csv",header=F)[,-1])
data[[5]]= -t(apply(data_5, 1, function(x){return(diff(x[1:10])/x[1:9])}))
data_6= as.matrix(read.csv("miranda_pressure_svalues.csv",header=F)[,-1])
data[[6]]= -t(apply(data_6, 1, function(x){return(diff(x[1:10])/x[1:9])}))
data_7= as.matrix(read.csv("miranda_viscocity_svalues.csv",header=F)[,-1])
data[[7]]= -t(apply(data_7, 1, function(x){return(diff(x[1:10])/x[1:9])}))


m=matrix(rep(0,7*7), ncol=7)
for(i in 1:7)
{
  for(j in 1:7)
  {
    m[i,j]=(median(mahalanobis(data[[i]], apply(data[[j]], 2,mean), cov(data[[j]])))*sd(mahalanobis(data[[i]], apply(data[[j]], 2,mean), cov(data[[j]])))+
              median(mahalanobis(data[[j]], apply(data[[i]], 2,mean), cov(data[[i]])))*sd(mahalanobis(data[[j]], apply(data[[i]], 2,mean), cov(data[[i]]))))/2
  }
}
colnames(m)= c("Velocity_X", "Velocity_Y", "Velocity_Z", "Diffusivity", "Density", "Pressure", "Viscocity")
rownames(m)= c("Velocity_X", "Velocity_Y", "Velocity_Z", "Diffusivity", "Density", "Pressure", "Viscocity")

m=apply(m,1,function(x){return(x/sum(x))})


m=matrix(rep(0,7*7), ncol=7)
for(i in 1:7)
{
  for(j in 1:7)
  {
    m[i,j]=(mean(mahalanobis(data[[i]], apply(data[[j]], 2,mean), cov(data[[j]])))+
              mean(mahalanobis(data[[j]], apply(data[[i]], 2,mean), cov(data[[i]]))))/2
  }
}
colnames(m)= c("Velocity_X", "Velocity_Y", "Velocity_Z", "Diffusivity", "Density", "Pressure", "Viscocity")
rownames(m)= c("Velocity_X", "Velocity_Y", "Velocity_Z", "Diffusivity", "Density", "Pressure", "Viscocity")

distance_metric=m
cor_dataset = distance_metric
tmp = c(1:7)
dataset2 = expand.grid(tmp,tmp)
dataset2$cor_v = sapply(1:nrow(dataset2),function(i) cor_dataset[dataset2[i,1],dataset2[i,2]])
colnames(dataset2) = c("t1","t2","Distance")
p2 = ggplot(dataset2,aes(t1,t2))+ geom_tile(aes(fill = Distance),show.legend = TRUE) +
  scale_fill_gradient(low="yellow", high="red") + labs(title="Quantifying Disparity in Spatial Smoothness through Mahalanobis Distance on Singular Value Decay", x="Fields", y="Fields")+
  theme(plot.title = element_text(size = 8,hjust = 0.5),axis.text=element_text(size=6,face = "bold"),
        axis.title=element_text(size=8))+scale_x_discrete(limit = c("Velocity_X", "Velocity_Y", "Velocity_Z", "Diffusivity", "Density", "Pressure", "Viscocity"))+
  scale_y_discrete(limit = c("Velocity_X", "Velocity_Y", "Velocity_Z", "Diffusivity", "Density", "Pressure", "Viscocity"))
plot(p2)


###############################################################################################################################################################################

setwd("/Users/arkaprabhaganguli/Documents/OneDrive - Michigan State University/Documents/NSF MSGI 2022/experiment codes/")
data=list(NULL)
data_1= as.matrix(read.csv("1cloud_svalues.csv",header=F)[,-1])
data[[1]]= -t(apply(data_1, 1, function(x){return(diff(x[1:10])/x[1:9])}))
data_2= matrix(as.numeric(as.matrix(read.csv("1qcloud_svalues.csv",header=F)[,-1])),ncol=625)
data[[2]]= -t(apply(data_2, 1, function(x){return(diff(x[1:10])/x[1:9])}))
data_3= as.matrix(read.csv("precip_svalues.csv",header=F)[,-1])
data[[3]]= -t(apply(data_3, 1, function(x){return(diff(x[1:10])/x[1:9])}))
data_4= as.matrix(read.csv("1qgraup_svalues.csv",header=F)[,-1])
data[[4]]= -t(apply(data_4, 1, function(x){return(diff(x[1:10])/x[1:9])}))
data_5= as.matrix(read.csv("qrain_svalues.csv",header=F)[,-1])
data[[5]]= -t(apply(data_5, 1, function(x){return(diff(x[1:10])/x[1:9])}))
data_6= as.matrix(read.csv("qsnow_svalues.csv",header=F)[,-1])
data[[6]]= -t(apply(data_6, 1, function(x){return(diff(x[1:10])/x[1:9])}))
data_7= as.matrix(read.csv("qice_svalues.csv",header=F)[,-1])
data[[7]]= -t(apply(data_7, 1, function(x){return(diff(x[1:10])/x[1:9])}))
data_8= as.matrix(read.csv("tc_svalues.csv",header=F)[,-1])
data[[8]]= -t(apply(data_8, 1, function(x){return(diff(x[1:10])/x[1:9])}))
data_9= as.matrix(read.csv("1u_svalues.csv",header=F)[,-1])
data[[9]]= -t(apply(data_9, 1, function(x){return(diff(x[1:10])/x[1:9])}))
data_10= as.matrix(read.csv("1v_svalues.csv",header=F)[,-1])
data[[10]]= -t(apply(data_10, 1, function(x){return(diff(x[1:10])/x[1:9])}))
data_11= as.matrix(read.csv("1w_svalues.csv",header=F)[,-1])
data[[11]]= -t(apply(data_11, 1, function(x){return(diff(x[1:10])/x[1:9])}))
data_12= as.matrix(read.csv("1p_svalues.csv",header=F)[,-1])
data[[12]]= -t(apply(data_12, 1, function(x){return(diff(x[1:10])/x[1:9])}))
data_13= as.matrix(read.csv("qvapor_svalues.csv",header=F)[,-1])
data[[13]]= -t(apply(data_13, 1, function(x){return(diff(x[1:10])/x[1:9])}))



m=matrix(rep(0,13*13), ncol=13)
for(i in 1:13)
{
  for(j in 1:13)
  {
    m[i,j]=(mean(mahalanobis(data[[i]], apply(data[[j]], 2,mean), cov(data[[j]])))+
              mean(mahalanobis(data[[j]], apply(data[[i]], 2,mean), cov(data[[i]]))))/2
  }
}
colnames(m)= c("Cloud", "QCloud", "Precip", "QGraup", "QRain", "QSnow", "QIce", "TC", "U", "V", "W","P", "QVapor")
rownames(m)= c("Cloud", "QCloud", "Precip", "QGraup", "QRain", "QSnow", "QIce", "TC", "U", "V", "W","P", "QVapor")

distance_metric=m[-c(13,14), -c(13,14)]
cor_dataset = distance_metric
tmp = c(1:11)
dataset2 = expand.grid(tmp,tmp)
dataset2$cor_v = sapply(1:nrow(dataset2),function(i) cor_dataset[dataset2[i,1],dataset2[i,2]])
colnames(dataset2) = c("t1","t2","Distance")
p2 = ggplot(dataset2,aes(t1,t2))+ geom_tile(aes(fill = Distance),show.legend = TRUE) +
  scale_fill_gradientn(colours = terrain.colors(5))  + labs(title="Quantifying Disparity in Spatial Smoothness through Mahalanobis Distance on Singular Value Decay", x="Fields", y="Fields")+
  theme(plot.title = element_text(size = 8,hjust = 0.5),axis.text=element_text(size=6,face = "bold"),
        axis.title=element_text(size=8))+scale_x_discrete(limit = c("Cloud", "QCloud", "Precip", "QGraup", "QRain", "QSnow", "QIce", "TC", "U", "V", "W"))+
  scale_y_discrete(limit = c("Cloud", "QCloud", "Precip", "QGraup", "QRain", "QSnow", "QIce", "TC", "U", "V", "W"))
plot(p2)


FA_values=read.csv("FA_values_all.csv")[-1]
library(ggplot2)

i=3 #tract_no
cor_dataset = abs(cor(FA_values[,((i-1)*98+2):((i-1)*98+99)]))
tmp = c(1:13)
dataset1 = expand.grid(tmp,tmp)
dataset1$cor_v = sapply(1:nrow(dataset1),function(i) m[dataset1[i,1],dataset1[i,2]])
colnames(dataset1) = c("t1","t2","Cor")
p1 = ggplot(dataset1,aes(t1,t2))+ geom_tile(aes(fill = Cor),show.legend = FALSE) +
  scale_fill_gradient(low="yellow", high="red") + labs(title="ATR_left",x = "vertex number", y = "vertex number")+
  theme(plot.title = element_text(size = 8,hjust = 0.5),axis.text=element_text(size=6,face = "bold"),
        axis.title=element_text(size=8)) 

