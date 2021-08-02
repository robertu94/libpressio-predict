import seaborn as sb
import pandas as pd
#import matplotlib as mpl
import matplotlib.pyplot as plt
import re
import csv
import numpy


dataset=pd.read_csv("nyxdataset_test2.csv",usecols=["Err_Bound_Percentage","File","Compressor","Total_Time"])
sorted_dataset=dataset.sort_values(by=["File","Compressor"],ascending=True)
sorted_dataset.index=range(2100)
sz_list=sorted_dataset[sorted_dataset.Compressor == "sz"]
sz_list.index=range(450)
libpressio_sz_list=sorted_dataset[sorted_dataset.Compressor == "libpressio+sz"]
libpressio_sz_list.index=range(450)
#sz_overhead=(libpressio_sz_list.Total_Time.astype(float)/sz_list.Total_Time.astype(float))*100
#sz_overhead=((libpressio_sz_list.Total_Time.astype(float) / sz_list.Total_Time.astype(float))-1)*100
sz_overhead=libpressio_sz_list.Total_Time.astype(float)-sz_list.Total_Time.astype(float)
Sz_total=pd.concat([sz_list,sz_overhead],axis=1)
Sz_total=Sz_total.replace(to_replace="sz",value="libpressio_sz")
Sz_total.columns=["File","Compressor","Err_Bound_Percentage","Total_Time","Time_Overhead"]

zfp_list=sorted_dataset[sorted_dataset.Compressor == "zfp"]
zfp_list.index=range(450)
libpressio_zfp_list=sorted_dataset[sorted_dataset.Compressor == "libpressio+zfp"]
libpressio_zfp_list.index=range(450)
#zfp_overhead=(libpressio_zfp_list.Total_Time.astype(float)/zfp_list.Total_Time.astype(float))*100
#zfp_overhead=((libpressio_zfp_list.Total_Time.astype(float) / zfp_list.Total_Time.astype(float))-1)*100
zfp_overhead=libpressio_zfp_list.Total_Time.astype(float)-zfp_list.Total_Time.astype(float)
Zfp_total=pd.concat([zfp_list,zfp_overhead],axis=1)
Zfp_total=Zfp_total.replace(to_replace="zfp",value="libpressio_zfp")
Zfp_total.columns=["File","Compressor","Err_Bound_Percentage","Total_Time","Time_Overhead"]

mgard_list=sorted_dataset[sorted_dataset.Compressor == "mgard"]
mgard_list.index=range(150)
libpressio_mgard_list=sorted_dataset[sorted_dataset.Compressor == "libpressio+mgard"]
libpressio_mgard_list.index=range(150)
#mgard_overhead = (libpressio_mgard_list.Total_Time.astype(float) / mgard_list.Total_Time.astype(float))*100
#mgard_overhead = ((libpressio_mgard_list.Total_Time.astype(float) / mgard_list.Total_Time.astype(float))-1)*100
mgard_overhead=libpressio_mgard_list.Total_Time.astype(float)-mgard_list.Total_Time.astype(float)
Mgard_total=pd.concat([mgard_list,mgard_overhead],axis=1)
Mgard_total=Mgard_total.replace(to_replace="mgard",value="libpressio_mgard")
Mgard_total.columns=["File","Compressor","Err_Bound_Percentage","Total_Time","Time_Overhead"]
Total_Overhead=pd.concat([Sz_total,Zfp_total,Mgard_total],axis=0)
Total_Overhead.index=range(1050)
pd.set_option("display.max_rows", None, "display.max_columns", None)
print(Total_Overhead)
plot1=sb.catplot(data=Total_Overhead,kind="bar",col="File",x="Compressor",y="Time_Overhead",hue="Err_Bound_Percentage",ci="sd")
plot1.savefig("TEST_IMAGE3.png")
