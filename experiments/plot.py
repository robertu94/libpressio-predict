import seaborn as sb
import pandas as pd
import matplotlib.pyplot as plt
import re
import csv
import numpy

dataset=pd.read_csv("core_speedup.csv",usecols=["Err_Bound","Compressor","Compression_Ratio","Num_of_Threads","Compression_Time","Decompression_Time"])
'''
plot1=sb.catplot(data=dataset,kind="bar",x="Num_of_Threads",y="Compression_Ratio",hue="Compressor",col="Err_Bound",ci="sd")
plot1.savefig("Speedup_ratio_timing.png")
'''



# This code is for calculation the ratio of median times for N threads/median time for 1 thread. 
# I haven't been able to get it working properly yet, but I will be working on it tomorrow to 
# get this working.
#grouped_data=dataset.groupby(['Compressor','Err_Bound','Num_of_Threads']).median(['Compression_Time']).rename_axis(['Compressor','Err_Bound','Num_of_Threads'])

#print(grouped_data)
#print(one_thread)

#plot2=sb.catplot(data=dataset,kind="bar",x="Num_of_Threads",
