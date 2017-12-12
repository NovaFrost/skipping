#!/usr/bin/env python
# -*- coding: utf-8 -*-
from essentia.standard import *
import numpy as np
import sys, os
from scipy.cluster.vq import kmeans2
from sklearn.cluster import KMeans
import argparse

parser = argparse.ArgumentParser(description='Vector Quantization')
parser.add_argument('output', help='output directory')
parser.add_argument('list', help='list of dataset')
parser.add_argument('source', help='source directory')
parser.add_argument('M', help='embedding dimension', type=int)
parser.add_argument('K', help='extent of codebook', type=int)
parser.add_argument('start', help='where to start (begin from 1)', type=int)
parser.add_argument('end', help='where to end', type=int)

args = parser.parse_args()
out_dir = args.output
list_name = args.list
dir_name = args.source
M = args.M
K = args.K
start = args.start
end = args.end

with open(list_name) as list_file:
    file_list = [line.rstrip() for line in list_file]

os.mkdir(out_dir)
tmp_out_dir = out_dir
out_dir += 'merge_data_test_%d_%d/' % (M, K)
if not os.path.exists(out_dir):
    os.mkdir(out_dir)
data = np.ndarray(shape=(0, 12*M), dtype='float32')
data = []
song_data = []

for i, hpcp_file in enumerate(file_list):
    pool = YamlInput(filename=dir_name + hpcp_file)()
    hpcp = pool['down_sample_hpcp']
    song_data.append((hpcp_file, len(hpcp)))
    if start == 0 or (i >= start - 1 and i < end):
        for j in range(M-1, len(hpcp)):
            sample = hpcp[j-(M-1):j+1].flatten() * 1000
            data.append(sample)
data = np.array(data, dtype='float32')
centroid, labels = kmeans2(data, K, iter=20, thresh=1e-05, minit='points', missing='warn')

np.savetxt(tmp_out_dir + 'center', centroid, '%.18f')
sample_id = 0
for i, (song_name, song_len) in enumerate(song_data):
    pool = YamlInput(filename=dir_name + song_name)()  
    if start ==0 or (i >= start - 1 and i < end):
        sym_seq = []
        for j in range(M-1, song_len):
            sym_seq.append(labels[sample_id])
            sample_id += 1
        sym_seq = np.array(sym_seq, dtype='int')
        pool.set('0', sym_seq)
    YamlOutput(filename=out_dir+song_name)(pool)
