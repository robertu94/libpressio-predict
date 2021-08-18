import subprocess
import numpy
import re
import csv
import itertools
from mpi4py import MPI
import mpi4py.futures as fut

def execution_code(config):
    if config["dataset"] == "temperature.dat":
        data_range = 4780000
        if config["compressor id"] == "zfp" or config["compressor id"] == "sz":
            dimensions = ["-3","512","512","512"]
        elif config["compressor id"] == "mgard":
            dimensions= ["512","512","512"]
        else:
            dimensions = ["-d","512","-d","512","-d","512"]
        data_type = "float"
    elif config["dataset"] == "vx.f32":
        data_range = 6908.75
        if config["compressor id"] == "libpressio+sz" or config["compressor id"] == "libpressio+zfp":
            dimensions = ["-d","280953867"]
        elif config["compressor id"] == "mgard":
            dimensions = ["280952867"]
        else:
            dimensions = ["-1","280953867"]
        data_type = "float"
    elif config["dataset"] == "V-98x1200x1200.dat":
        data_range = 99.5761
        if config["compressor id"] == "sz" or config["compressor id"] == "zfp":
            dimensions= ["-3","1200","1200","98"]
        elif config["compressor id"] == "mgard":
            dimensions= ["1200","1200","98"]
        else:
            dimensions = ["-d","1200","-d","1200","-d","98"]
        data_type = "float"
    elif config["dataset"] == "CLOUDf44.bin":
        data_range = 0.0227631
        if config["compressor id"] == "sz" or config["compressor id"] == "zfp":
            dimensions= ["-3","500","500","100"]
        elif config["compressor id"] == "mgard":
            dimensions= ["500","500","100"]
        else:
            dimensions = ["-d","500","-d","500","-d","100"]
        data_type = "float"
    elif config["dataset"] == "exalt-24-0.dat":
        data_range = 1.00108
        if config["compressor id"] == "libpressio+sz" or config["compressor id"] == "libpressio+zfp":
            dimensions = ["-d", "1077290"]
        elif config["compressor id"] == "mgard":
            dimensions = ["1077290"]
        else:
            dimensions = ["-1","1077290"]
        data_type = "float"
    elif config["dataset"] == "pressure.d64":
        data_range = 4.41428
        if config["compressor id"] == "sz" or config["compressor id"] == "zfp":
            dimensions= ["-3","256","384","384"]
        elif config["compressor id"] == "mgard":
            dimensions= ["256","384","384"]
        else:
            dimensions = ["-d","256","-d","384","-d","384"]
        data_type = "double"
    print(config["compressor id"])
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
        command = ['./mgard_test',data_type,config["dataset"],*dimensions,error_num]
    elif config["compressor id"] == "libpressio+mgard":
        compression_term = 'time:compress_many <uint32> = (\d+)'
        decompression_term = 'time:decompress_many <uint32> = (\d+)'
        compression_ratio_term = 'size:compression_ratio <double> = (\d+)'
        command = ["pressio","-i",config["dataset"],"-m","time","-m","size","-M","all",*dimensions,"-t",data_type,"mgard","-o","mgard:tolerance="+error_num]
    elif config["compressor id"] == "libpressio+sz":
        compression_term = 'time:compress_many <uint32> = (\d+)'
        decompression_term = 'time:decompress_many <uint32> = (\d+)'
        compression_ratio_term = 'size:compression_ratio <double> = (\d+)' 
        command = ["/usr/bin/time","pressio","-i",config["dataset"],"-m","time","-m","size","-M","all",*dimensions,"-t",data_type,"sz","-o","sz:abs_err_bound="+error_num,"-o","sz:error_bound_mode_str=abs"]
    elif config["compressor id"] == "libpressio+zfp":
        compression_term = 'time:compress_many <uint32> = (\d+)'
        decompression_term = 'time:decompress_many <uint32> = (\d+)'
        compression_ratio_term = 'size:compression_ratio <double> = (\d+)' 
        command = ["/usr/bin/time","pressio","-i",config["dataset"],"-m","time","-m","size","-M","all",*dimensions,"-t",data_type,"zfp","-o","zfp:accuracy="+error_num]
    if (config["compressor id"] == "sz" or config["compressor id"] == "zfp"):
        process=subprocess.run(compression_command,capture_output=True,text=True)
        m=re.search(compression_term,process.stdout)
        process=subprocess.run(decompression_command,capture_output=True,text=True)
        p=re.search(decompression_term,process.stdout)
        if config['compressor id'] == 'sz':
            q=re.search(compression_ratio_term,process.stdout)
        else:
            q=re.search(compression_ratio_term,process.stderr)
        time=float(m.group(1))+float(p.group(1))
        sz_zfp_specs={'File':config["dataset"],'Compressor': config["compressor id"],'Err_Bound_Percentage':config["abs_err_bound"],'Compression_Time':m.group(1),'Decompression_Time':p.group(1),'Total_Time':time, 'Compression_Ratio':q.group(1)}
        return sz_zfp_specs
    elif (config["compressor id"] == "mgard"):
        if(config["dataset"] == "vx.f32" or config["dataset"] == "temperature.dat" or config["dataset"] == "exalt-24-0.dat"):
            next(config)
        process = subprocess.run(command,capture_output=True,text=True)
        m=re.search(compression_term,process.stdout)
        p=re.search(decompression_term,process.stdout)
        q=re.search(compression_ratio_term,process.stdout)
        time=(float(m.group(1))*1E-3)+(float(p.group(1))*1E-3)
        mgard_specs={'File':config["dataset"],'Compressor':config["compressor id"],'Err_Bound_Percentage':config["abs_err_bound"],'Compression_Time':float(m.group(1))*1E-3,'Decompression_Time':float(p.group(1))*1E-3,'Total_Time':time,'Compression_Ratio':q.group(1)}
        return mgard_specs
    else:
        if((config["dataset"] == "vx.f32" or config["dataset"] == "temperature.dat" or config["dataset"] == "exalt-24-0.dat") and config["compressor id"] == "libpressio+mgard"):
            next(config)
        process = subprocess.run(command,capture_output=True,text=True)
        m=re.search(compression_term,process.stderr)
        p=re.search(decompression_term,process.stderr)
        q=re.search(compression_ratio_term,process.stderr)
        time=(float(m.group(1))*1E-3)+(float(p.group(1))*1E-3)
        libpressio_specs={'File':config["dataset"],'Compressor':config["compressor id"],'Err_Bound_Percentage':config["abs_err_bound"],'Compression_Time':float(m.group(1))*1E-3,'Decompression_Time':float(p.group(1))*1E-3,'Total_Time':time,'Compression_Ratio':q.group(1)}
        return libpressio_specs

with open('parallelized_data.csv','w',newline='') as csvfile:
    fieldnames=['File','Compressor','Err_Bound_Percentage','Compression_Time','Decompression_Time','Compression_Ratio','Total_Time','Total_Memory']
    writer=csv.DictWriter(csvfile,fieldnames=fieldnames)
    writer.writeheader()
    compressors = ["zfp","sz","libpressio+sz","libpressio+zfp","mgard","libpressio+mgard"]
    err_percentages= [0.0001, 0.01, 0.02, 0.05]
    datasets = ["V-98x1200x1200.dat","vx.f32","temperature.dat","pressure.d64","exalt-24-0.dat","CLOUDf44.bin"]
    configs=[
            {
                "name":f"{err_percent*100}%",
                "abs_err_bound":err_percent,
                "compressor id":compressor_id,
                "dataset":dataset
            }
              for reps,compressor_id,err_percent, dataset in list(
                  itertools.product(range(30),compressors, err_percentages,datasets)
              )
            ]
    node_comm = MPI.COMM_WORLD.Split_type(MPI.COMM_TYPE_SHARED)
    each_node = MPI.COMM_WORLD.Split(node_comm.rank)
    if node_comm.rank == 0:
        with fut.MPICommExecutor(each_node) as pool:
            return_val=list(pool.map(execution_code, configs, unordered=True))
            writer.writerow(return_val)
