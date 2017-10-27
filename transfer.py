import essentia
from essentia.standard import *
import scipy.io as sio
import matplotlib.pyplot as plt
import numpy as np
import os

def output(data, path):
    # data: list
    # row: (version_id, down_sample_hpcp, glo_hpcp)
    if not os.path.exists(path):
        os.mkdir(path) 
    for i, (version_id, down_sample_hpcp, glo_hpcp) in enumerate(data):
        pool = essentia.Pool()
        for hpcp in down_sample_hpcp:
            pool.add('down_sample_hpcp', hpcp)
        pool.add('version_id', version_id)
        pool.set('glo_hpcp', glo_hpcp)
        YamlOutput(filename=path + str(i+1) + '.hpcp')(pool)

def get_you_seq():
    data = sio.loadmat('extra/dataYoutubeCovers.mat')
    testData = data['testData']
    trainData = data['trainData']
    test = []
    train = []
    for row in testData[0]:
        track = np.array(row[0].T, dtype='float32')
        glo_track = np.sum(track, axis=0)
        glo_track /= np.max(glo_track) 
        test.append([int(row[2]), track, glo_track])
    for row in trainData[0]:
        track = np.array(row[0].T, dtype='float32')
        glo_track = np.sum(track, axis=0)
        glo_track /= np.max(glo_track) 
        train.append([int(row[2]), track, glo_track])
    return test, train

def get_you_id():
    data = sio.loadmat('extra/labels.mat')
    testData = data['teIdx']
    trainData = data['trIdx']
    test = []
    train = []
    for row in testData:
        test.append(row[0])
    for row in trainData:
        train.append(row[0])
    return test, train

test, train = get_you_seq()
testid, trainid = get_you_id()
i = 0
j = 0
result = []
while i < len(testid) and j < len(trainid):
    if testid[i] < trainid[j]:
        result.append(test[i])
        i += 1
    else:
        result.append(train[j])
        j += 1

while i < len(testid):
    result.append(test[i])
    i += 1
while j < len(trainid):
    result.append(train[j])
    j += 1
with open('you_list', 'w') as fp:
     for id in trainid:
         fp.write(str(id) + '.hpcp\n')
     for id in testid:
         fp.write(str(id) + '.hpcp\n')
output(result, 'you_hpcp/')
