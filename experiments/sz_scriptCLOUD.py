import subprocess
import numpy
import re
import csv
list1=[]
list1d=[]
list1m=[]
list1m2=[]
list2=[]
list2d=[]
list2m=[]
list2m2=[]
list3=[]
list3d=[]
list3m=[]
list3m2=[]
list4=[]
list4d=[]
list4m=[]
list4m2=[]
list5=[]
list5d=[]
list5m=[]
list5m2=[]
with open('sz_testCLOUDf44.csv','w',newline='') as csvfile:
    fieldnames=['File','Compressor','0.01% Compression Time','0.01% Decompression Time','0.01% Compression Memory','0.01% Decompression Memory','1% Compression Time','1% Decompression Time','1% Compression Memory','1% Decompression Memory','2% Compression Time','2% Decompression Time','2% Compression Memory','2% Decompression Memory','5% Compression Time', '5% Decompression Time','5% Compression Memory','5% Decompression Memory', '30% Compression Time', '30% Decompression Time','30% Compression Memory','30% Decompression Memory']
    writer=csv.DictWriter(csvfile,fieldnames=fieldnames)
    writer.writeheader()
    i=0
    dimension=numpy.fromfile("/home/vmalvos/datasets/hurricane_data/hurricane/CLOUDf44.bin",dtype=numpy.float32).shape
    print(dimension)
    i=0
    while i<30:
        process=subprocess.run(["/usr/bin/time","sz","-z","-i","CLOUDf44.bin","-M","ABS","-A","0.000000227631","-f","-3","500","500","100"],capture_output=True,text=True)
        print(process.stdout)
        print(process.stderr)
        m=re.search('compression time = (\d.+)',process.stdout)
        o=re.search('(\d+)maxresident',process.stderr)
        list1.insert(i,m.group(1))
        list1m2.insert(i,o.group(1))
        i+=1
    print("END (0.01%) COMPRESSION HERE-----------------------------------------------------")
    i=0
    while i<30:
        process=subprocess.run(["/usr/bin/time","sz","-x","-f","-M","ABS","-A","0.000000227631","-3","500","500","100","-s","CLOUDf44.bin.sz"],capture_output=True,text=True)
        print(process.stdout)
        print(process.stderr)
        p=re.search('decompression time = (\d.+) seconds',process.stdout)
        q=re.search('(\d+)maxresident',process.stderr)
        list1d.insert(i,p.group(1))
        list1m.insert(i,q.group(1))
        i+=1
    print("END (0.01%) DECOMPRESSION HERE----------------------------------------------------")
    i=0
    while i<30:
        process=subprocess.run(["/usr/bin/time","sz","-z","-i","CLOUDf44.bin","-M","ABS","-A","0.0000227631","-f","-3","500","500","100"],capture_output=True,text=True)
        print(process.stdout)
        print(process.stderr)
        m=re.search('compression time = (\d.+)',process.stdout)
        o=re.search('(\d+)maxresident',process.stderr)
        list2.insert(i,m.group(1))
        list2m2.insert(i,o.group(1))
        i+=1
    print("END (1%) COMPRESSION HERE-----------------------------------------------------")
    i=0
    while i<30:
        process=subprocess.run(["/usr/bin/time","sz","-x","-f","-M","ABS","-A","0.0000227631","-3","500","500","100","-s","CLOUDf44.bin.sz"],capture_output=True,text=True)
        print(process.stdout)
        print(process.stderr)
        p=re.search('decompression time = (\d.+) seconds',process.stdout)
        q=re.search('(\d+)maxresident',process.stderr)
        list2d.insert(i,p.group(1))
        list2m.insert(i,q.group(1))
        i+=1
    print("END (1%) DECOMPRESSION HERE----------------------------------------------------")
    i=0
    while i<30:
        process=subprocess.run(["/usr/bin/time","sz","-z","-i","CLOUDf44.bin","-M","ABS","-A","0.00004553","-f","-3","500","500","100"],capture_output=True,text=True)
        print(process.stdout)
        print(process.stderr)
        m=re.search('compression time = (\d.+)',process.stdout)
        o=re.search('(\d+)maxresident',process.stderr)
        list3.insert(i,m.group(1))
        list3m2.insert(i,o.group(1))
        i+=1
    print("END (2%) COMPRESSION HERE-----------------------------------------------------")
    i=0
    while i<30:
        process=subprocess.run(["/usr/bin/time","sz","-x","-f","-M","ABS","-A","0.00004553","-3","500","500","100","-s","CLOUDf44.bin.sz"],capture_output=True,text=True)
        print(process.stdout)
        print(process.stderr)
        p=re.search('decompression time = (\d.+) seconds',process.stdout)
        q=re.search('(\d+)maxresident',process.stderr)
        list3d.insert(i,p.group(1))
        list3m.insert(i,q.group(1))
        i+=1
    print("END (2%) DECOMPRESSION HERE----------------------------------------------------")
    i=0
    while i<30:
        process=subprocess.run(["/usr/bin/time","sz","-z","-i","CLOUDf44.bin","-M","ABS","-A","0.00011382","-f","-3","500","500","100"],capture_output=True,text=True)
        print(process.stdout)
        print(process.stderr)
        m=re.search('compression time = (\d.+)',process.stdout)
        o=re.search('(\d+)maxresident',process.stderr)
        list4.insert(i,m.group(1))
        list4m2.insert(i,o.group(1))
        i+=1
    print("END (5%) COMPRESSION HERE-----------------------------------------------------")
    i=0
    while i<30:
        process=subprocess.run(["/usr/bin/time","sz","-x","-f","-M","ABS","-A","0.00011382","-3","500","500","100","-s","CLOUDf44.bin.sz"],capture_output=True,text=True)
        print(process.stdout)
        print(process.stderr)
        p=re.search('decompression time = (\d.+) seconds',process.stdout)
        q=re.search('(\d+)maxresident',process.stderr)
        list4d.insert(i,p.group(1))
        list4m.insert(i,q.group(1))
        i+=1
    print("END (5%) DECOMPRESSION HERE----------------------------------------------------")
    i=0
    while i<30:
        process=subprocess.run(["/usr/bin/time","sz","-z","-i","CLOUDf44.bin","-M","ABS","-A","0.00068289","-f","-3","500","500","100"],capture_output=True,text=True)
        print(process.stdout)
        print(process.stderr)
        m=re.search('compression time = (\d.+)',process.stdout)
        o=re.search('(\d+)maxresident',process.stderr)
        list5.insert(i,m.group(1))
        list5m2.insert(i,o.group(1))
        i+=1
    print("END (30%) COMPRESSION HERE-----------------------------------------------------")
    i=0
    while i<30:
        process=subprocess.run(["/usr/bin/time","sz","-x","-f","-M","ABS","-A","0.00068289","-3","500","500","100","-s","CLOUDf44.bin.sz"],capture_output=True,text=True)
        print(process.stdout)
        print(process.stderr)
        p=re.search('decompression time = (\d.+) seconds',process.stdout)
        q=re.search('(\d+)maxresident',process.stderr)
        list5d.insert(i,p.group(1))
        list5m.insert(i,q.group(1))
        i+=1
    print("END (30%) DECOMPRESSION HERE----------------------------------------------------")
    x=0;
    while x<30:
        writer.writerow({'File':'CLOUDf44.bin','Compressor':'SZ','0.01% Compression Time':list1[x],'0.01% Decompression Time':list1d[x],'0.01% Compression Memory':list1m[x],'0.01% Decompression Memory':list1m2[x],'1% Compression Time':list2[x],'1% Decompression Time':list2d[x],'1% Compression Memory':list2m[x],'1% Decompression Memory':list2m2[x],'2% Compression Time':list3[x],'2% Decompression Time':list3d[x],'2% Compression Memory':list3m[x],'2% Decompression Memory':list3m2[x],'5% Compression Time':list4[x],'5% Decompression Time':list4d[x],'5% Compression Memory':list4m[x],'5% Decompression Memory':list4m2[x],'30% Compression Time':list5[x],'30% Decompression Time':list5d[x],'30% Compression Memory':list5m[x],'30% Decompression Memory':list5m2[x]})
        x+=1
