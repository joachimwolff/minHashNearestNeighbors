#! /usr/bin/python
# Copyright 2015 Joachim Wolff
# Master Project
# Tutors: Milad Miladi, Fabrizio Costa
# Summer semester 2015
#
# Chair of Bioinformatics
# Department of Computer Science
# Faculty of Engineering
# Albert-Ludwig-University Freiburg im Breisgau
#
__author__ = 'wolffjoachim'

#
# import sys, os

# os.system("(cd lib/src_c && python setup.py install --user) ")
# os.system("cd ../..")
# # --build-lib ../computation/bin
# lib_path = os.path.abspath('./lib/')
# sys.path.append(lib_path)

import numpy as np
from scipy.sparse import dok_matrix
from scipy.sparse import rand
from scipy.sparse import vstack

from sklearn.neighbors import KNeighborsClassifier
from sklearn.neighbors import NearestNeighbors
import random
import time
import cPickle as pickle
# from neighborsMinHash import MinHashKNeighborsClassifier
from neighborsMinHash import MinHash
# from neighborsMinHash.computation import InverseIndex
import inspect
from scipy.sparse import csr_matrix

from eden.converter.graph.gspan import gspan_to_eden
from eden.graph import Vectorizer

from sklearn.metrics import accuracy_score
from neighborsMinHash.util import accuracy

def test(data):


    graphs = gspan_to_eden( 'http://www.bioinf.uni-freiburg.de/~costa/bursi.gspan' )
    vectorizer = Vectorizer( r=2,d=5 )
    datasetBursi = vectorizer.transform( graphs )

    # if not os.path.exists("inverse_index.approx"):
    print "Build inverse index for approximate..."
    time_build_approx_start = time.time()
    minHash = MinHash(number_of_hash_functions=400, similarity=True, bloomierFilter=True)
    # minHash.fit(data[0])
    minHash.fit(datasetBursi)

    # time_build_approx_end = time.time()
    # print "Build inverse index for approximate... Done!"
    # print "Time: ", time_build_approx_end - time_build_approx_start


    # centroids_list = []
    # for i in range(50):
    #     centroids_list.append(data[0].getrow(i))
    # centroids_neighborhood = vstack(centroids_list)
    # print "starting computation..."



    time_comp_approx = time.time()
    approx1 = minHash.kneighbors(fast=True,return_distance=False)
    print "Approx solution, distance=False: ", approx1
    # print "Approx solution, distance=True: ", minHash_approximate.kneighbors(centroids_neighborhood,return_distance=True)

    time_comp_approx_end = time.time()
    print "Time: ", time_comp_approx_end  - time_comp_approx
    print "\n\n"

    time_comp_approx = time.time()
    exact1 = minHash.kneighbors(fast=False,return_distance=False)
    print "Exact solution, distance=False: ", exact1
    # print "Approx solution, distance=True: ", minHash_approximate.kneighbors(centroids_neighborhood,return_distance=True)

    time_comp_approx_end = time.time()
    print "Time: ", time_comp_approx_end  - time_comp_approx
    print "\n\n"

    minHash2 = MinHash(number_of_hash_functions=400)
    
    time_comp_exact = time.time()
    # print "Exact solution, distance=True: ", minHash_exact.kneighbors( centroids_neighborhood,return_distance=True)
    approx_fit = minHash2.fit_kneighbors(X=datasetBursi, fast=True, return_distance=False)
    print "Approx solution, distance=False: ", approx_fit

    time_comp_exact_end = time.time()
    print "Time: ", time_comp_exact_end  - time_comp_exact
    print "\n\n"
    minHash3 = MinHash(number_of_hash_functions=400)

    time_comp_exact = time.time()
    exact_fit = minHash3.fit_kneighbors(X=datasetBursi, fast=False, return_distance=False)
    # print "Exact solution, distance=True: ", minHash_exact.kneighbors( centroids_neighborhood,return_distance=True)
    print "Exact solution, distance=False: ", exact_fit

    time_comp_exact_end = time.time()
    print "Time: ", time_comp_exact_end  - time_comp_exact
    print "\n\n"


    # dataY = self.create_dataset(seed+1, 1,  1, data_instances)
    time_exact_fit = time.time()
    # nearest_Neighbors = KNeighborsClassifier()
    nearest_Neighbors = NearestNeighbors()
    nearest_Neighbors.fit(datasetBursi)
    time_exact_fit_end = time.time()
    print "Time to build index exact: ", time_exact_fit_end - time_exact_fit
    #print data.getrow(1)
    time_comp = time.time()
    sklearn_ = nearest_Neighbors.kneighbors(return_distance=False)
    print "Exact solution with nearestNeighborsClassifier, distance=False: \n\t", sklearn_
    # print "Exact solution with nearestNeighborsClassifier, distance=True: \n\t" ,nearest_Neighbors.kneighbors(return_distance=True)
    time_comp_end = time.time()
    print "Time: ", time_comp_end - time_comp

    
    accuracy_score_ = 0.0
    for x, y in zip(approx1, sklearn_):
        accuracy_score_ += accuracy_score(x, y)
    print "Accuracy_approx: ", accuracy_score_ / float(len(approx1))

    accuracy_score_ = 0.0
    for x, y in zip(exact1, sklearn_):
        accuracy_score_ += accuracy_score(x, y)
    print "Accuracy_exact: ", accuracy_score_ / float(len(exact1))

    accuracy_score_ = 0.0
    for x, y in zip(approx_fit, sklearn_):
        accuracy_score_ += accuracy_score(x, y)
    print "Accuracy_approx: ", accuracy_score_ / float(len(approx_fit))

    accuracy_score_ = 0.0
    for x, y in zip(exact_fit, sklearn_):
        accuracy_score_ += accuracy_score(x, y)
    print "Accuracy_exact: ", accuracy_score_ / float(len(exact_fit))

    exact_a, approx_a, exacr_approx = accuracy(exact1, approx1, sklearn_)
    print "Accuracy neighborhood: "
    print "Accuracy_approx: ", approx_a
    print "Accuracy_exact: ", exact_a
def create_dataset(seed=None,
                   number_of_centroids=None,
                   number_of_instances=None,
                   number_of_features=None,
                   size_of_dataset=None,
                   density=None,
                   fraction_of_density=None
                   ):
    dataset_neighborhood_list = []
    number_of_swapping_elements = int(number_of_features * density * fraction_of_density)
    y = []
    random_local = random.randint
    number_of_features_density = int(number_of_features*density)-1
    for k in xrange(number_of_centroids):
        dataset_neighbor = rand(1, number_of_features, density=density, format='lil', random_state=seed*k)
        nonzero_elements =  dataset_neighbor.nonzero()[1]
        for i in xrange(size_of_dataset):
            neighbor = dataset_neighbor.copy()
            # random.seed(seed*k)
            for j in xrange(number_of_swapping_elements):
                index = random_local(0, number_of_features_density)
                index_swap = random_local(0, number_of_features-1)
                neighbor[0, nonzero_elements[index]] = 0
                neighbor[0, index_swap] = 1
            dataset_neighborhood_list.append(neighbor)
            y.append(k)

    dataset_neighborhood = vstack(dataset_neighborhood_list)
    size_of_noise = number_of_instances-(number_of_centroids*size_of_dataset)
    print "size_of_noise: ", size_of_noise

    if size_of_noise > 0:
            dataset_noise = rand(size_of_noise, number_of_features, format='lil', density=density, random_state=seed*seed)
            dataset = vstack([dataset_neighborhood, dataset_noise])
    else:
        dataset = vstack([dataset_neighborhood])
    random_value_generator = random.randint

    # add classes for noisy data
    for i in range(0, size_of_noise):
        y.append(random_value_generator(0, number_of_centroids))

    return dataset, y

def test_kneighbor_graph(data):
    # if not os.path.exists("inverse_index.approx"):
    #     print "Build inverse index for approximate..."
    #     time_build_approx_start = time.time()
    #     minHash_approximate = MinHashKNeighborsClassifier(algorithm='approximate')
    #     minHash_approximate.fit(data[0], data[1])
    #     time_build_approx_end = time.time()
    #     print "Build inverse index for approximate... Done!"
    #     print "Time: ", time_build_approx_end - time_build_approx_start
    #     pickle.dump( minHash_approximate, open( "inverse_index.approx", "wb" ) ) #write
    # else:
    #     print "loading data..."
    #     minHash_approximate = pickle.load( open( "inverse_index.approx", "rb" ) ) #load
    #     print "loading data... Done!"
    # if not os.path.exists("inverse_index.exact"):
    #     print "Build inverse index for exact solution..."
    #     time_build_exact_start = time.time()
    #     minHash_exact = MinHashKNeighborsClassifier(algorithm='exact')
    #     minHash_exact.fit(data[0], data[1])
    #     time_build_exact_end= time.time()
    #     print "Build inverse index for exact... Done!"
    #     print "Time: ", time_build_exact_end - time_build_exact_start
    #     pickle.dump( minHash_exact, open( "inverse_index.exact", "wb" ) ) #write
    # else:
    #     print "loading data..."
    #     minHash_exact = pickle.load( open( "inverse_index.exact", "rb" ) ) #load
    #     print "loading data... Done!\n\n"
    graphs = gspan_to_eden( 'http://www.bioinf.uni-freiburg.de/~costa/bursi.gspan' )
    vectorizer = Vectorizer( r=2,d=5 )
    datasetBursi = vectorizer.transform( graphs )


    minHash = MinHash(number_of_hash_functions=400)
    # minHash.fit(data[0])
    minHash.fit(datasetBursi)

    print "exact:connectivity: ", minHash.kneighbors_graph(mode='connectivity', fast=False)
    print "exact:distance: ", minHash.kneighbors_graph(mode='distance', fast = False)

    print "approx:connectivity: ",minHash.kneighbors_graph(mode='connectivity', fast=True)
    print "approx:distance: ", minHash.kneighbors_graph(mode='distance', fast=True)
    nearest_Neighbors = KNeighborsClassifier()
    nearest_Neighbors.fit(datasetBursi)
    print "exact: ", nearest_Neighbors.kneighbors_graph(mode='connectivity')

def test_predict(data):
    
    print "Len of X: ", data[0].shape[0]
    print "Len of y: ", len(data[1])
    time_build_approx_start = time.time()
    minHash_approximate = MinHashKNeighborsClassifier(fast=True)
    minHash_approximate.fit(data[0], data[1])
    time_build_approx_end = time.time()
    print "Build inverse index for approximate... Done!"
    print "Time: ", time_build_approx_end - time_build_approx_start
   
    
    print "Build inverse index for exact solution..."
    time_build_exact_start = time.time()
    minHash_exact = MinHashKNeighborsClassifier(fast=False)
    minHash_exact.fit(data[0], data[1])
    time_build_exact_end= time.time()
    print "Build inverse index for exact... Done!"
    print "Time: ", time_build_exact_end - time_build_exact_start
       
    nearest_Neighbors = KNeighborsClassifier()
    nearest_Neighbors.fit(data[0], data[1])
    precision_exact = 0
    precision_approx = 0
    precision_lib = 0
    value_exact = []
    value_approx = []
    value_lib = []
    # print "X=None: c", minHash_approximate.predict(X=None)
    print "number of datasets: ", data[0].shape[0]
    # for i in range(data[0].shape[0]):
    # print minHash_exact.predict(data[0])

        # if dataset[1][i] == value_exact[i]:
        #     precision_exact += 1

    print minHash_approximate.predict_proba(data[0])
    print minHash_approximate.predict(data[0])

        # if dataset[1][i] == value_approx[i]:
        #     precision_approx += 1
    print nearest_Neighbors.predict(data[0])

    print nearest_Neighbors.predict_proba(data[0])
        # if dataset[1][i] == value_lib[i]:
        #     precision_lib += 1

        # print "class if element: ", dataset[1][i]
        # print "exact predict: ", minHash_exact.predict(data[0].getrow(i))
        # print "classifier predict: ", nearest_Neighbors.predict(data[0].getrow(i))
    # print "prediction_exact: ", precision_exact
    # print "prediction_approx: ", precision_approx
    # print "prediction_lib: ", precision_lib

    # print value_lib
    print "value_exact: ", value_exact
    print "value_approx: ", value_approx
    print "value_lib: ", value_lib
    # print "real values: ", data[1]
def test_radius_neighbors(data):
    if not os.path.exists("inverse_index.approx"):
        print "Build inverse index for approximate..."
        time_build_approx_start = time.time()
        minHash_approximate = MinHashNearestNeighbors(algorithm='approximate')
        minHash_approximate.fit(data[0], data[1])
        time_build_approx_end = time.time()
        print "Build inverse index for approximate... Done!"
        print "Time: ", time_build_approx_end - time_build_approx_start
        pickle.dump( minHash_approximate, open( "inverse_index.approx", "wb" ) ) #write
    else:
        print "loading data..."
        minHash_approximate = pickle.load( open( "inverse_index.approx", "rb" ) ) #load
        print "loading data... Done!"
    if not os.path.exists("inverse_index.exact"):
        print "Build inverse index for exact solution..."
        time_build_exact_start = time.time()
        minHash_exact = MinHashNearestNeighbors(algorithm='exact')
        minHash_exact.fit(data[0], data[1])
        time_build_exact_end= time.time()
        print "Build inverse index for exact... Done!"
        print "Time: ", time_build_exact_end - time_build_exact_start
        pickle.dump( minHash_exact, open( "inverse_index.exact", "wb" ) ) #write
    else:
        print "loading data..."
        minHash_exact = pickle.load( open( "inverse_index.exact", "rb" ) ) #load
        print "loading data... Done!\n\n"

    # print minHash_approximate.nearestNeighbors.inverseIndex.inverse_index
    # print minHash_approximate.inverseIndex.signature_storage

    # dataset_to_predict = dok_matrix((2, 1000), dtype=np.int)

    # for i in range(1000):
    #     if i % 20 == 0:
            # dataset_to_predict[0, i] = 1
        # if i % 30 == 0:
            # dataset_to_predict[1, i] = 1
    print "starting computation..."
    time_comp_exact = time.time()
    # print "Exact solution, distance=True: ", minHash_exact.radius_neighbors_graph(radius = 10, mode='distance')
    print "Exact solution, distance=False: ", minHash_exact.radius_neighbors_graph(radius = 10,  mode='connectivity')
    # print "approx solution, distance=True: ", minHash_approximate.radius_neighbors_graph(radius = 10, mode='distance')
    print "approx solution, distance=False: ", minHash_approximate.radius_neighbors_graph(radius = 10, mode='connectivity')
    nearest_Neighbors = NearestNeighbors()
    nearest_Neighbors.fit(data[0])
    print "NearestNeighbirs: ", nearest_Neighbors.radius_neighbors_graph(radius = 10, mode='connectivity')

    print "NearestNeighbirs sitance: ", nearest_Neighbors.radius_neighbors_graph(radius = 7, mode='distance')


if __name__ == "__main__":
# seed, number_of_centroids, number_of_instances, number_of_features, density, fraction_of_density
#     seed = 6
#     centroids = 1
#     size_of_datasets = 2
#     number_of_instances = 10
#     number_of_features = 10
#     density = 0.065
#     fraction_of_density = 0.2
#     n_neighbors=1
#     radius=1.0
    seed = 6
    centroids = 7
    size_of_datasets = 7
    number_of_instances = 100
    number_of_features = 10000
    density = 0.01
    fraction_of_density = 0.2
    n_neighbors=5
    radius=1.0
    # number_of_hash_functions = 400
    # number_of_iterations = 20
    # data_instances = 100
    # data_features = 1000
    # number_of_nearest_neighbors = 5
    # eccess_neighbor_size_factor = 10
    # max_bin_size = 70
    # iterations_data = 5

    # if not ((os.path.exists("inverse_index.approx") and os.path.exists("inverse_index.exact") )
    #         or os.path.exists("dataset")):
    time_create_dataset_start = time.time()
    print "Create dataset..."
# seed=None,
#                    number_of_centroids=None,
#                    number_of_instances=None,
#                    number_of_features=None,
#                    size_of_dataset=None,
#                    density=None,
#                    fraction_of_density=None
#
    # dataset = create_dataset(seed, iterations_data, data_instances, data_features)
    dataset = create_dataset(seed=seed, number_of_centroids=centroids,
                             number_of_instances=number_of_instances,
                             number_of_features=number_of_features,
                             density = density,
                             fraction_of_density=fraction_of_density, size_of_dataset = size_of_datasets)
    time_create_dataset_end = time.time()
    print "Create dataset... Done!"
    print "Time: ", time_create_dataset_end - time_create_dataset_start
    print "\n"
    #     pickle.dump( dataset, open( "dataset", "wb" ) ) #write
    # else:
    #     dataset = pickle.load( open( "dataset", "rb" ) ) #load
    # print "row: ", dataset[0].getrow(0).keys()[0]
    # print "nnz: ", dataset[0].getrow(0).getnnz()
    # test_kneighbor_graph(dataset)
    # inverse_index_obj = InverseIndex()
    # print inverse_index_obj.signature(dataset[0].getrow(0))
    test(dataset)
    # test_predict(dataset)
    #test_radius_neighbors(dataset)
