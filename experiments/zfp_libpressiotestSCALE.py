import subprocess
import numpy
import re 
import csv
list1=[]
list1d=[]
list1m=[]
list2=[]
list2d=[]
list2m=[]
list3=[]
list3d=[]
list3m=[]
list4=[]
list4d=[]
list4m=[]
list5=[]
list5d=[]
list5m=[]
with open('zfp_libpressioSCALE.csv','w',newline='') as csvfile:
    fieldnames=['File','Compressor','0.01% Compression Time','0.01% Decompression Time','0.01% Compression Memory','0.01% Decompression Memory','1% Compression Time','1% Decompression Time','1% Compression Memory','1% Decompression Memory','2% Compression Time','2% Decompression Time','2% Compression Memory','2% Decompression Memory','5% Compression Time', '5% Decompression Time','5% Compression Memory','5% Decompression Memory', '30% Compression Time', '30% Decompression Time','30% Compression Memory','30% Decompression Memory']
    writer=csv.DictWriter(csvfile,fieldnames=fieldnames)
    writer.writeheader()
    i=0
    while i<30:
        process=subprocess.run(["/usr/bin/time","pressio","-i","V-98x1200x1200.dat","-m","time","-M","all","-d","141120000","-t","float","zfp","-o","zfp:accuracy=0.00995761"],capture_output=True,text=True)
        print(process.stdout)
        print(process.stderr)
        m=re.search('time:compress_many <uint32> = (\d+)',process.stderr)
        p=re.search('time:decompress_many <uint32> = (\d+)',process.stderr)
        q=re.search('(\d+)maxresident',process.stderr)
        list1.insert(i,float(m.group(1))*1E-3)
        list1d.insert(i,float(p.group(1))*1E-3)
        list1m.insert(i,q.group(1))
        i+=1
    print("END OF 0.01% --------------------------------------")

    i=0
    while i<30:
        process=subprocess.run(["/usr/bin/time","pressio","-i","V-98x1200x1200.dat","-m","time","-M","all","-d","141120000","-t","float","zfp","-o","zfp:accuracy=0.995761"],capture_output=True,text=True)
        print(process.stdout)
        print(process.stderr)
        m=re.search('time:compress_many <uint32> = (\d+)',process.stderr)
        p=re.search('time:decompress_many <uint32> = (\d+)',process.stderr)
        q=re.search('(\d+)maxresident',process.stderr)
        list2.insert(i,float(m.group(1))*1E-3)
        list2d.insert(i,float(p.group(1))*1E-3)
        list2m.insert(i,q.group(1))
        i+=1
    print("END OF 1% --------------------------------------")
    i=0
    while i<30:
        process=subprocess.run(["/usr/bin/time","pressio","-i","V-98x1200x1200.dat","-m","time","-M","all","-d","141120000","-t","float","zfp","-o","zfp:accuracy=1.991522"],capture_output=True,text=True)
        print(process.stdout)
        print(process.stderr)
        m=re.search('time:compress_many <uint32> = (\d+)',process.stderr)
        p=re.search('time:decompress_many <uint32> = (\d+)',process.stderr)
        q=re.search('(\d+)maxresident',process.stderr)
        list3.insert(i,float(m.group(1))*1E-3)
        list3d.insert(i,float(p.group(1))*1E-3)
        list3m.insert(i,q.group(1))
        i+=1
    print("END OF 2% --------------------------------------")

    i=0 
    while i<30:
        process=subprocess.run(["/usr/bin/time","pressio","-i","V-98x1200x1200.dat","-m","time","-M","all","-d","141120000","-t","float","zfp","-o","zfp:accuracy=4.978805"],capture_output=True,text=True)
        print(process.stdout)
        print(process.stderr)
        m=re.search('time:compress_many <uint32> = (\d+)',process.stderr)
        p=re.search('time:decompress_many <uint32> = (\d+)',process.stderr)
        q=re.search('(\d+)maxresident',process.stderr)
        list4.insert(i,float(m.group(1))*1E-3)
        list4d.insert(i,float(p.group(1))*1E-3)
        list4m.insert(i,q.group(1))
        i+=1
    print("END OF 5% --------------------------------------")
    i=0
    while i<30:
        process=subprocess.run(["/usr/bin/time","pressio","-i","V-98x1200x1200.dat","-m","time","-M","all","-d","141120000","-t","float","zfp","-o","zfp:accuracy=29.87283"],capture_output=True,text=True)
        print(process.stdout)
        print(process.stderr)
        m=re.search('time:compress_many <uint32> = (\d+)',process.stderr)
        p=re.search('time:decompress_many <uint32> = (\d+)',process.stderr)
        q=re.search('(\d+)maxresident',process.stderr)
        list5.insert(i,float(m.group(1))*1E-3)
        list5d.insert(i,float(p.group(1))*1E-3)
        list5m.insert(i,q.group(1))
        i+=1
    print("END OF 30% --------------------------------------")
    x=0
    while x<30:
        writer.writerow({'File':'V-98x1200x1200.dat','Compressor':'Libpresso+ZFP','0.01% Compression Time':list1[x],'0.01% Decompression Time':list1d[x],'0.01% Compression Memory':list1m[x],'0.01% Decompression Memory':'-','1% Compression Time':list2[x],'1% Decompression Time':list2d[x],'1% Compression Memory':list2m[x],'1% Decompression Memory':'-','2% Compression Time':list3[x],'2% Decompression Time':list3d[x],'2% Compression Memory':list3m[x],'2% Decompression Memory':'-','5% Compression Time':list4[x],'5% Decompression Time':list4d[x],'5% Compression Memory':list4m[x],'5% Decompression Memory':'-','30% Compression Time':list5[x],'30% Decompression Time':list5d[x],'30% Compression Memory':list5m[x],'30% Decompression Memory':'-'})
        x+=1
