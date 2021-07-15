import subprocess
import numpy
import re
import csv
import itertools


# This code uses subprocess to run commands for compression and writes the compressor, error bound percentage,
# compression time, decompression time, total time, compression ratio, and number of threads to the csv as
# it runs.

with open('core_speedup.csv','w',newline='')as csvfile:
    fieldnames=['Compressor','Err_Bound_Percentage','Compression_Time','Decompression_Time','Total_Time','Compression_Ratio',"Num_of_Threads"]
    writer=csv.DictWriter(csvfile,fieldnames=fieldnames)
    writer.writeheader()
    configs=[
            {
                "name":f"{error_bound*100}%",
                "abs_err_bound":error_bound,
                "compressor id":compressor_id,
                "core number":num_cores
            }
                for num_cores, error_bound, compressor_id in
                itertools.product(
                    [1,2,4,6,8,10,20,22,24,26,28,30,40],
                    [1e-6,1e-2],
                    ["ZFP_with_ZFP_Threads","ZFP_with_Libpressio_Threads","SZ_with_Libpressio_Threads"],
                )
            ]
    for config in configs:
        #assign commands based on the compressor desired
        if config["compressor id"] == "ZFP_with_ZFP_Threads":
            command=["pressio","-i", "/home/vmalvos/datasets/SCALE_98x1200x1200/V-98x1200x1200.dat", "-t", "float", "-d", "1200", "-d", "1200", "-d", "98", "zfp", "-o", "zfp:accuracy="+str(config["abs_err_bound"]), "-o", "zfp:execution_name=omp", "-o", "zfp:omp_threads="+str(config["core number"]), "-m", "time", "-M", "all","-m", "size"]
        elif config["compressor id"] == "SZ_with_Libpressio_Threads":
            command=["pressio","chunking", "-m", "time", "-m", "size", "-b", "chunking:compressor=many_independent_threaded", "-o", "chunking:size=1200", "-o","chunking:size=1200", "-o","chunking:size=14", "-o"," many_independent_threaded:nthreads="+str(config["core number"]), "-b", "many_independent_threaded:compressor=sz" "-o", "sz:abs_err_bound="+str(config["abs_err_bound"]), "-i","/home/vmalvos/datasets/SCALE_98x1200x1200/V-98x1200x1200.dat", "-t", "float", "-d", "1200", "-d", "1200", "-d", "98", "-M","all"]
        elif config["compressor id"] == "ZFP_with_Libpressio_Threads":
            command=["pressio","chunking", "-m", "time", "-m", "size", "-b", "chunking:compressor=many_independent_threaded", "-o", "chunking:size=1200", "-o","chunking:size=1200", "-o","chunking:size=14", "-o"," many_independent_threaded:nthreads="+str(config["core number"]), "-b", "many_independent_threaded:compressor=zfp" "-o", "zfp:accuracy="+str(config["abs_err_bound"]), "-i","/home/vmalvos/datasets/SCALE_98x1200x1200/V-98x1200x1200.dat", "-t", "float", "-d", "1200", "-d", "1200", "-d", "98", "-M","all"]
        compression_term = 'time:compress_many <uint32> = (\d+)'
        decompression_term = 'time:decompress_many <uint32> = (\d+)'
        compression_ratio_term = 'size:compression_ratio <double> = (\d+)'
        for i in range(30):
            process = subprocess.run(command,capture_output=True,text=True)
            # I have the program print the output so I can check it as it goes, but it isn't necessary.
            #print(process.stdout)
            #print(process.stderr)
            #search for compression time
            m=re.search(compression_term,process.stderr)
            compression_time=float(m.group(1))*1E-3
            #search for decompression time
            p=re.search(decompression_term,process.stderr)
            decompression_time=float(p.group(1))*1E-3
            #search for compression ratio
            q=re.search(compression_ratio_term,process.stderr)
            compression_ratio=float(q.group(1))
            total_time= compression_time + decompression_time
            #write to the csv
            writer.writerow({'Compressor':config["compressor id"],'Err_Bound_Percentage':config["abs_err_bound"],'Compression_Time': compression_time,'Decompression_Time':decompression_time,'Total_Time':total_time,'Compression_Ratio':compression_ratio,'Num_of_Threads':config["core number"]})
        #print("END "+config["name"]+config["compressor id"]+" COMPRESSION HERE--------------------------------------------------")
