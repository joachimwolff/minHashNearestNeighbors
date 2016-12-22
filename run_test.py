#!/usr/bin/env python

from sklearn_freiburg_extension import MinHash
from sklearn_freiburg_extension import MinHashClassifier
from sklearn_freiburg_extension import WtaHash
from sklearn_freiburg_extension import WtaHashClassifier


if __name__ == "__main__":
    n_neighbors_minHash = MinHash(n_neighbors = 4)