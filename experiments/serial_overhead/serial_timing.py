import subprocess
import numpy
import re
import csv
import itertools
list1=[]
list1d=[]
list1r=[]
with open('nyxdataset_test2.csv','w',newline='') as csvfile:
    fieldnames=['File','Compressor','Err_Bound_Percentage','Compression_Time','Decompression_Time','Compression_Ratio','Total_Time','Total_Memory']
    writer=csv.DictWriter(csvfile,fieldnames=fieldnames)
    writer.writeheader()
    configs=[
            {
                "name":f"{error_bound*100}%",
                "abs_err_bound":error_bound,
                "compressor id":compressor_id,
                "dataset":dataset
            }
                for compressor_id,error_bound,dataset in 
                itertools.product(
                    ["sz","zfp","libpressio+sz","libpressio+zfp","mgard","libpressio+mgard"],
                    [0.0001, 0.01, 0.02, 0.05, 0.3],
                    ["vx.f32","temperature.dat","V-98x1200x1200.dat"],
                )
            ]
    for config in configs:
        i=0
        if config["dataset"] == "temperature.dat":
            data_range = 4780000
            if config["compressor id"] == "zfp" or config["compressor id"] == "sz":
                dimensions = ["-3","512","512","512"]
            elif config["compressor id"] == "mgard":
                dimensions= ["512","512","512"]
            else:
                dimensions = ["-d","512","-d","512","-d","512"]
        elif config["dataset"] == "vx.f32":
            data_range = 6908.75
            if config["compressor id"] == "libpressio+sz" or config["compressor id"] == "libpressio+zfp":
                dimensions = ["-d","280953867"]
            elif config["compressor id"] == "mgard":
                dimensions = ["280952867"]
            else:
                dimensions = ["-1","280953867"]
        elif config["dataset"] == "V-98x1200x1200.dat":
            data_range = 99.5761
            if config["compressor id"] == "zfp":
                dimensions = ["-3","98","1200","1200"]
            elif config["compressor id"] == "sz":
                dimensions= ["-3","1200","1200","98"]
            elif config["compressor id"] == "mgard":
                dimensions= ["1200","1200","98"]
            else:
                dimensions = ["-d","1200","-d","1200","-d","98"]
        error_num=str(data_range*config["abs_err_bound"])
        if config['compressor id'] == "sz":
            compression_term = 'compression time = (\d.+)'
            decompression_term= 'decompression time = (\d.+) seconds'
            compression_ratio_term = 'compressionRatio=(\d.+)' 
            compression_command=["/usr/bin/time",config["compressor id"],"-z","-i",config["dataset"],"-M","ABS","-A",error_num,"-f",*dimensions]
            decompression_command=["/usr/bin/time",config["compressor id"],"-x","-f","-M","ABS","-A",error_num,*dimensions,"-s",config["dataset"]+".sz","-a","-i",config["dataset"]]
        elif config["compressor id"] == "zfp":
            compression_term = 'Compression Time: (\d.+) seconds'
            decompression_term= 'Decompression Time: (\d.+) seconds'
            compression_ratio_term = 'ratio=(\d.+) '
            compression_command=["/usr/bin/time",config["compressor id"],"-i",config["dataset"],"-z",config["dataset"]+".zfp",*dimensions,"-f","-a",error_num]
            decompression_command=["/usr/bin/time",config["compressor id"],"-z",config["dataset"]+".zfp","-o",config["dataset"]+".dec",*dimensions,"-f","-a",error_num]
        elif config["compressor id"] == "mgard":
            compression_term = 'compression_time (\d+)'
            decompression_term = 'decompression_time (\d+)'
            compression_ratio_term = 'compression_ratio (\d.+)'
            command = ['./mgard_test','float',config["dataset"],*dimensions,error_num]
        elif config["compressor id"] == "libpressio+mgard":
            compression_term = 'time:compress_many <uint32> = (\d+)'
            decompression_term = 'time:decompress_many <uint32> = (\d+)'
            compression_ratio_term = 'size:compression_ratio <double> = (\d+)'
            command = ["pressio","-i",config["dataset"],"-m","time","-m","size","-M","all",*dimensions,"-t","float","mgard","-o","mgard:tolerance="+error_num]
        elif config["compressor id"] == "libpressio+sz":
            compression_term = 'time:compress_many <uint32> = (\d+)'
            decompression_term = 'time:decompress_many <uint32> = (\d+)'
            compression_ratio_term = 'size:compression_ratio <double> = (\d+)' 
            command = ["/usr/bin/time","pressio","-i",config["dataset"],"-m","time","-m","size","-M","all",*dimensions,"-t","float","sz","-o","sz:abs_err_bound="+error_num,"-o","sz:error_bound_mode_str=abs"]
        elif config["compressor id"] == "libpressio+zfp":
            compression_term = 'time:compress_many <uint32> = (\d+)'
            decompression_term = 'time:decompress_many <uint32> = (\d+)'
            compression_ratio_term = 'size:compression_ratio <double> = (\d+)' 
            command = ["/usr/bin/time","pressio","-i",config["dataset"],"-m","time","-m","size","-M","all",*dimensions,"-t","float","zfp","-o","zfp:accuracy="+error_num]
        if (config["compressor id"] == "sz" or config["compressor id"] == "zfp"):
            for i in range(30):
                process=subprocess.run(compression_command,capture_output=True,text=True)
                print(process.stdout)
                print(process.stderr)
                m=re.search(compression_term,process.stdout)
                list1.insert(i,m.group(1))
            print("END "+config["name"]+" COMPRESSION HERE-----------------------------------------------------")
            i=0
            for i in range(30):
                process=subprocess.run(decompression_command,capture_output=True,text=True)
                print(process.stdout)
                print(process.stderr)
                p=re.search(decompression_term,process.stdout)
                if config['compressor id'] == 'sz':
                    q=re.search(compression_ratio_term,process.stdout)
                else:
                    q=re.search(compression_ratio_term,process.stderr)
                list1d.insert(i,p.group(1))
                list1r.insert(i,q.group(1))
            print("END "+ config["name"]+ " DECOMPRESSION HERE----------------------------------------------------")
            x=0;
            while x<30:
                time=float(list1[x])+float(list1d[x])
                writer.writerow({'File':config["dataset"],'Compressor': config["compressor id"],'Err_Bound_Percentage':config["abs_err_bound"],'Compression_Time':list1[x],'Decompression_Time':list1d[x],'Total_Time':time, 'Compression_Ratio':list1r[x]})
                x+=1
        elif (config["compressor id"] == "mgard"):
            i=0;
            if(config["dataset"] == "vx.f32" or config["dataset"] == "temperature.dat"):
                continue
            for i in range(30):
                process = subprocess.run(command,capture_output=True,text=True)
                print(process.stdout)
                print(process.stderr)
                m=re.search(compression_term,process.stdout)
                p=re.search(decompression_term,process.stdout)
                q=re.search(compression_ratio_term,process.stdout)
                time=(float(m.group(1))*1E-3)+(float(p.group(1))*1E-3)
                writer.writerow({'File':config["dataset"],'Compressor':config["compressor id"],'Err_Bound_Percentage':config["abs_err_bound"],'Compression_Time':float(m.group(1))*1E-3,'Decompression_Time':float(p.group(1))*1E-3,'Total_Time':time,'Compression_Ratio':q.group(1)})
            print("END "+config["name"]+" COMPRESSION AND DECOMPRESSION HERE-------------------------------------")
        else:
            i=0;
            if((config["dataset"] == "vx.f32" or config["dataset"] == "temperature.dat") and config["compressor id"] == "libpressio+mgard"):
                continue
            for i in range(30):
                process = subprocess.run(command,capture_output=True,text=True)
                print(process.stdout)
                print(process.stderr)
                m=re.search(compression_term,process.stderr)
                p=re.search(decompression_term,process.stderr)
                q=re.search(compression_ratio_term,process.stderr)
                time=(float(m.group(1))*1E-3)+(float(p.group(1))*1E-3)
                writer.writerow({'File':config["dataset"],'Compressor':config["compressor id"],'Err_Bound_Percentage':config["abs_err_bound"],'Compression_Time':float(m.group(1))*1E-3,'Decompression_Time':float(p.group(1))*1E-3,'Total_Time':time,'Compression_Ratio':q.group(1)})
            print("END "+config["name"]+" COMPRESSION AND DECOMPRESSION HERE-------------------------------------")
