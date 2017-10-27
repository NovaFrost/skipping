# CoverSong
## Preparation
### Requires  
<pre>essentia (http://essentia.upf.edu/documentation/)</pre>

### Compile  
<pre>make</pre>

### Download data
<pre>
1. mkdir extra && cd extra
2. download files in the current directory  
https://www.dropbox.com/s/21zvg023pcybywt/labels.mat?dl=0
https://sites.google.com/site/ismir2015shapelets/dataYoutubeCovers.mat?attredirects=0&d=1
</pre>
### Transform .mat to .hpcp (the program use YAML format to save files)  
<pre>python transform.py</pre>

### Cluster  
<pre>
chmod u+x cluster.sh  
./cluster.sh
</pre>

## Run  
The code is run on multiple threads. Defaultly, I use 32 threads to accelerate the calculation. My experimental enviroment is a Dell PowerEdge-R730 server with two Intel(R) Xeon(R) CPU E5-2640 v3 and 128GB RAM. Under this circumstance, processing a query takes 3.40s per thread for Youtube dataset. 
To run the program, one could use fewer threads to compute (see Arguments of model for more details). Besides, 200MB of RAM is enough to run it.
<pre>
chmod u+x model.sh  
./model.sh
</pre>

## Arguments of model
<pre>
usage: model out_dir_name dir_name list_name ref_cnt process_cnt skipping transposition cen_name BIT M K  
out_dir_name    directory for storing results  
dir_name        directory that contains .hpcp files  
list_name       song list  
ref_cnt         number of reference songs  
process_cnt     number of threads  
skipping        skipping distance  
transposition   transposition of orignal tracks  
cen_name        codebook  
BIT             shift width  
M               embedding dimension  
K               extent of codebook  
</pre>

## Argmuents of cluser.py
<pre>
usage: cluster.py [-h] output list source M K start end  

positional arguments:  

output        output directory
list          list of dataset  
source        source directory  
M             embedding dimension  
K             extent of codebook  
start         where to start (begin from 1)  
end           where to end  

optional arguments:  
-h, --help  show this help message and exit  
</pre>
