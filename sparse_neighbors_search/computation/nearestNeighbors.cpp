/**
 Copyright 2016, 2017, 2018, 2019, 2020 Joachim Wolff
 PhD Thesis

 Copyright 2015, 2016 Joachim Wolff
 Master Thesis
 Tutor: Fabrizio Costa
 Winter semester 2015/2016

 Chair of Bioinformatics
 Department of Computer Science
 Faculty of Engineering
 Albert-Ludwigs-University Freiburg im Breisgau
**/

#include <iostream>
#include <set>
#include "nearestNeighbors.h"


#include "sparseMatrix.h"

#ifdef OPENMP
#include <omp.h>
#endif

NearestNeighbors::NearestNeighbors(){};
NearestNeighbors::NearestNeighbors(size_t pNumberOfHashFunctions, size_t pShingleSize,
                    size_t pNumberOfCores, size_t pChunkSize,
                    size_t pMaxBinSize,
                    size_t pSizeOfNeighborhood, size_t pMinimalBlocksInCommon,
                    size_t pExcessFactor, size_t pMaximalNumberOfHashCollisions,
                    int pFast, int pSimilarity, int pPruneInverseIndex, float pPruneInverseIndexAfterInstance, 
                    int pRemoveHashFunctionWithLessEntriesAs, size_t pHashAlgorithm,
                    size_t pBlockSize, size_t pShingle, size_t pRemoveValueWithLeastSigificantBit,
                    float pCpuGpuLoadBalancing, size_t pGpuHash, size_t pRangeK_Wta) {

        mInverseIndex = new InverseIndex(pNumberOfHashFunctions, pShingleSize,
                                    pNumberOfCores, pChunkSize,
                                    pMaxBinSize, pMinimalBlocksInCommon, 
                                    pExcessFactor, pMaximalNumberOfHashCollisions,
                                    pPruneInverseIndex, pPruneInverseIndexAfterInstance, 
                                    pRemoveHashFunctionWithLessEntriesAs, pHashAlgorithm, pBlockSize, pShingle,
                                    pRemoveValueWithLeastSigificantBit, 
                                    pCpuGpuLoadBalancing, pGpuHash, pRangeK_Wta);

        mNneighbors = pSizeOfNeighborhood;
        mFast = pFast;
        mNumberOfCores = pNumberOfCores;
        mChunkSize = pChunkSize;
        mSimilarity = pSimilarity;
        mExcessFactor = pExcessFactor;
        mCpuGpuLoadBalancing = pCpuGpuLoadBalancing;
        mGpuHash = pGpuHash;
        mHash = new Hash();
        #ifdef CUDA
        mNearestNeighborsCuda = new NearestNeighborsCuda();
        #endif
}

NearestNeighbors::~NearestNeighbors() {
    delete mInverseIndex;
    
    delete mOriginalData;

    delete mHash;
    #ifdef CUDA
        delete mNearestNeighborsCuda;
    #endif
    
}

void NearestNeighbors::fit(SparseMatrixFloat* pRawData) {
    mInverseIndex->fit(pRawData);
    return;
}

void NearestNeighbors::partialFit(SparseMatrixFloat* pRawData, size_t pStartIndex) {
    mInverseIndex->fit(pRawData, pStartIndex);
    return;
}

neighborhood* NearestNeighbors::kneighbors(SparseMatrixFloat* pRawData,
                                                size_t pNneighbors, int pFast, int pSimilarity, float pRadius, bool pAbsoluteNumbers) {

    size_t limit_neighbors_of_neighbors = 100;
    if (pNneighbors+mExcessFactor < limit_neighbors_of_neighbors) {
        limit_neighbors_of_neighbors = pNneighbors+mExcessFactor;
    }
    if (pFast == -1) {
        pFast = mFast;
    } 
    if (pNneighbors == 0) {
        pNneighbors = mNneighbors;
    }
    
    if (pSimilarity == -1) {
        pSimilarity = mSimilarity;
    }
    bool doubleElementsStorageCount = false;
    neighborhood* neighborhood_;
    umap_uniqueElement* x_inverseIndex;
    if (pRawData == NULL) {
        
        // no query data given, use stored signatures
        x_inverseIndex = mInverseIndex->getSignatureStorage();
        neighborhood_  = mInverseIndex->kneighbors(x_inverseIndex, 
                                                    pNneighbors, true, true, pRadius, pAbsoluteNumbers);
        doubleElementsStorageCount = true;
    } else {
        pRawData->precomputeDotProduct();
        x_inverseIndex = (mInverseIndex->computeSignatureMap(pRawData));
        neighborhood_ = mInverseIndex->kneighbors(x_inverseIndex, pNneighbors, 
                                                doubleElementsStorageCount, true, pRadius, pAbsoluteNumbers);
       for (auto it = x_inverseIndex->begin(); it != x_inverseIndex->end(); ++it) {
            delete (*it).second.instances;
            delete (*it).second.signature;
       }
       delete x_inverseIndex;                                          
    }
    
    if (pFast) {    

        return neighborhood_;
    }
    mOriginalData->precomputeDotProduct();
    if (mChunkSize <= 0) {
            mChunkSize = ceil(neighborhood_->neighbors->size() / static_cast<float>(mNumberOfCores));
        }
    #ifdef OPENMP
        omp_set_dynamic(0);
    #endif
    vvsize_t neighborsListFirstRound(neighborhood_->neighbors->size(), vsize_t(0));
   
    neighborhood* neighborhoodCandidates = new neighborhood();
    neighborhoodCandidates->neighbors = new vvsize_t(mOriginalData->size());
    // compute the exact neighbors based on the candidates given by the inverse index
    // store the neighborhood in neighborhoodCandidates to reuse neighbor if needed
    // and store neighbors list per requested instance in neighborsListFirstRound
    //
    // neighborsListFirstRound is needed to get replace the candidates in neighborhood_
    // if it would not be used, it would be iterated over neighborhood_ and manipulated
    // at the same time
    #ifdef CUDA
    if (mCpuGpuLoadBalancing == 0){
    #endif
        #ifdef OPENMP
        #pragma omp parallel for schedule(static, mChunkSize) num_threads(mNumberOfCores)
        #endif
        for (size_t i = 0; i < neighborhood_->neighbors->size(); ++i) {
            
            if (neighborhood_->neighbors->operator[](i).size() > 0) {
                
                std::vector<sortMapFloat> exactNeighbors;
                if (pSimilarity) {
                    exactNeighbors = 
                        mOriginalData->cosineSimilarity(neighborhood_->neighbors->operator[](i), limit_neighbors_of_neighbors, i, pRawData);
                } else {
                    exactNeighbors = 
                        mOriginalData->euclidianDistance(neighborhood_->neighbors->operator[](i), limit_neighbors_of_neighbors, i, pRawData);
                } 
                std::vector<size_t> neighborsVector;
                if (pRadius == -1.0) {
                    size_t vectorSize = std::min(exactNeighbors.size(),limit_neighbors_of_neighbors);
                    neighborsVector.resize(vectorSize);
                    for (size_t j = 0; j < vectorSize; ++j) {
                        neighborsVector[j] = exactNeighbors[j].key;
                        neighborsListFirstRound[i].push_back(exactNeighbors[j].key);
                    } 
                } else {
                    
                    for (size_t j = 0; j < exactNeighbors.size(); ++j) {
                        if (exactNeighbors[j].val <= pRadius) {
                            neighborsVector.push_back(exactNeighbors[j].key);
                            neighborsListFirstRound[i].push_back(exactNeighbors[j].key);
                        } else {
                            break;
                        }
                    } 
                }
                #ifdef OPENMP
                #pragma omp critical
                #endif
                {
                    if (neighborsVector.size() > 1) {
                        neighborhoodCandidates->neighbors->operator[](exactNeighbors[0].key) = neighborsVector;
                    }
                }
            }
        }
        
    #ifdef CUDA
    } else {
       
        neighborhood* neighbors_ = mNearestNeighborsCuda->computeNearestNeighbors(neighborhood_, pSimilarity, pRawData, mOriginalData, limit_neighbors_of_neighbors);
        
        #pragma omp parallel for schedule(static, mChunkSize) num_threads(mNumberOfCores)
        for (size_t i = 0; i < neighbors_->neighbors->size(); ++i) {
            size_t vectorSize = neighbors_->neighbors->operator[](i).size();
            if (pRadius == -1.0) {
                std::vector<size_t> neighborsVector(vectorSize);
                for (size_t j = 0; j < vectorSize; ++j) {
                    neighborsVector[j] = neighbors_->neighbors->operator[](i)[j];
                    neighborsListFirstRound[i].push_back(neighbors_->neighbors->operator[](i)[j]);
                } 
                if (neighbors_->neighbors->operator[](i).size() > 1) {
                    neighborhoodCandidates->neighbors->operator[](i) = neighborsVector;
                }
            } else {
                std::vector<size_t> neighborsVector;
                for (size_t j = 0; j < vectorSize; ++j) {
                    if (neighbors_->distances->operator[](i)[j] <= pRadius) {
                        neighborsVector.push_back(neighbors_->neighbors->operator[](i)[j]);
                        neighborsListFirstRound[i].push_back(neighbors_->neighbors->operator[](i)[j]);
                    }
                } 
                if (neighborsVector.size() > 1) {
                    neighborhoodCandidates->neighbors->operator[](i) = neighborsVector;
                }
            }
        }
        #pragma omp barrier
        delete neighbors_->neighbors;
        delete neighbors_->distances;
        delete neighbors_;
    }
    
    #endif
    x_inverseIndex = mInverseIndex->getSignatureStorage();
    
    #ifdef OPENMP
    #pragma omp parallel for schedule(static, mChunkSize) num_threads(mNumberOfCores)
    #endif   
    // for all requested instances get the neighbors+mExcessFactor of the neighbors
    for (size_t i = 0; i < neighborsListFirstRound.size(); ++i) {
        size_t sizeOfExtended = neighborsListFirstRound[i].size();
        vsize_t dublicateElements((mOriginalData->size()/sizeof(size_t))+1,0);
        size_t bucketIndex;
        size_t element;
        size_t valueSeen;
        size_t instance;
        size_t queryInstance;
        if (neighborhood_->neighbors->operator[](i).size() > 0) {
            queryInstance = neighborhood_->neighbors->operator[](i)[0];
        }
        neighborhood_->neighbors->operator[](i).clear();
        neighborhood_->neighbors->operator[](i).push_back(queryInstance); 
        bucketIndex = queryInstance / sizeof(size_t);
        element = 1 << queryInstance % sizeof(size_t); 
        dublicateElements[bucketIndex] = dublicateElements[bucketIndex] | element;
        
        for (size_t j = 0; j < pNneighbors && j < sizeOfExtended; ++j) { 
            instance = neighborsListFirstRound[i][j];
            // neighborhood for instance was already computed?

           
            if (neighborhoodCandidates->neighbors->operator[](instance).size() == 0) {
                // limit neighbors of neighbors to the first 100
                
                umap_uniqueElement* instance_signature = new umap_uniqueElement();
                size_t signatureId = 0;
                for (size_t k = 0; k < mOriginalData->getSizeOfInstance(instance); ++k) {
                        signatureId = mHash->hash((mOriginalData->getNextElement(instance, k) +1), (signatureId+1), MAX_VALUE);
                }
                (*instance_signature)[signatureId] = (*x_inverseIndex)[signatureId];
                 neighborhood* neighborhood_instance = mInverseIndex->kneighbors(instance_signature, 
                                                                pNneighbors, false, false, pAbsoluteNumbers);
                if (neighborhood_instance->neighbors->operator[](0).size() != 0) { 
                    std::vector<sortMapFloat> exactNeighbors;
                    if (pSimilarity) {
                        exactNeighbors = 
                            mOriginalData->cosineSimilarity(neighborhood_instance->neighbors->operator[](0), limit_neighbors_of_neighbors, instance, pRawData);
                    } else {
                        exactNeighbors = 
                            mOriginalData->euclidianDistance(neighborhood_instance->neighbors->operator[](0), limit_neighbors_of_neighbors, instance, pRawData);
                    }
                    size_t vectorSize = std::min(exactNeighbors.size(), limit_neighbors_of_neighbors);
                    std::vector<size_t> neighborsVector(vectorSize);
                    for (size_t j = 0; j < vectorSize; ++j) {
                        neighborsVector[j] = exactNeighbors[j].key;
                    }
                    #ifdef OPENMP
                    #pragma omp critical
                    #endif
                    {
                        if (exactNeighbors.size() > 1) {
                            neighborhoodCandidates->neighbors->operator[](instance) = neighborsVector;
                        }
                    } 
                    
                }
                delete instance_signature;
                delete neighborhood_instance->neighbors;
                delete neighborhood_instance->distances;
                delete neighborhood_instance;
            }
            // add the neighbors + mExcessFactor to the candidate list 
            for (size_t k = 0; k < neighborhoodCandidates->neighbors->operator[](instance).size() && k < limit_neighbors_of_neighbors; ++k) {
                bucketIndex = neighborhoodCandidates->neighbors->operator[](instance)[k] / sizeof(size_t);
                element = 1 << neighborhoodCandidates->neighbors->operator[](instance)[k] % sizeof(size_t);

                #ifdef OPENMP
                #pragma omp critical
                #endif
                {
                    valueSeen = dublicateElements[bucketIndex] & element;
                    // if candidate was already inserted, do not inserte it a second time
                    if (valueSeen != element) {
                        neighborhood_->neighbors->operator[](i).push_back(neighborhoodCandidates->neighbors->operator[](instance)[k]);
                        dublicateElements[bucketIndex] = dublicateElements[bucketIndex] | element;
                    }
                }
            }
        }
    }

    delete neighborhoodCandidates->neighbors;
    delete neighborhoodCandidates->distances;
    delete neighborhoodCandidates;
    
    // compute the exact neighbors based on the candidate selection before.
    #ifdef CUDA
    if (mCpuGpuLoadBalancing == 0){
    #endif
    
    neighborhood* neighborhoodExact = new neighborhood();
    neighborhoodExact->neighbors = new vvsize_t(neighborhood_->neighbors->size());
    neighborhoodExact->distances = new vvfloat(neighborhood_->neighbors->size());
    #ifdef OPENMP
    #pragma omp parallel for schedule(static, mChunkSize) num_threads(mNumberOfCores)
    #endif   
        for (size_t i = 0; i < neighborhood_->neighbors->size(); ++i) {
        if (neighborhood_->neighbors->operator[](i).size() != 1) {
                    std::vector<sortMapFloat> exactNeighbors;
                    if (0 < neighborhood_->neighbors->operator[](i).size()) {
                        if (pSimilarity) {
                            exactNeighbors = 
                                mOriginalData->cosineSimilarity(neighborhood_->neighbors->operator[](i), pNneighbors, i, pRawData);
                        } else {
                            exactNeighbors = 
                                mOriginalData->euclidianDistance(neighborhood_->neighbors->operator[](i), pNneighbors, i, pRawData);
                        }
                    }
                size_t vectorSize = exactNeighbors.size();
                
                std::vector<size_t> neighborsVector(vectorSize);
                std::vector<float> distancesVector(vectorSize);
                if (vectorSize == 0) {
                    neighborsVector.push_back(i);
                    distancesVector.push_back(0.0);
                }
                for (size_t j = 0; j < vectorSize; ++j) {
                        neighborsVector[j] = exactNeighbors[j].key;
                        if (pSimilarity) {
                            distancesVector[j] = exactNeighbors[j].val;
                        } else {
                            distancesVector[j] = sqrt(exactNeighbors[j].val);
                        }
                }
    #ifdef OPENMP
    #pragma omp critical
    #endif
                {
                    neighborhoodExact->neighbors->operator[](i) = neighborsVector;
                    neighborhoodExact->distances->operator[](i) = distancesVector;
                }
            } else {
    #ifdef OPENMP
    #pragma omp critical
    #endif
                {
                    std::vector<size_t> neighborsVector(1, i);
                    std::vector<float> distancesVector(1, 0);
                    neighborhoodExact->neighbors->operator[](i) = neighborsVector;
                    neighborhoodExact->distances->operator[](i) = distancesVector;
                }
            }
        }
        delete neighborhood_->neighbors;
        delete neighborhood_->distances;
        delete neighborhood_;
        return neighborhoodExact;
    #ifdef CUDA
    } else {
        
        neighborhood* neighbors_part2 = mNearestNeighborsCuda->computeNearestNeighbors(neighborhood_, pSimilarity, pRawData, mOriginalData, limit_neighbors_of_neighbors);
        delete neighborhood_->neighbors;
        delete neighborhood_->distances;
        delete neighborhood_;
        return neighbors_part2;
    } 
    #endif
    
}

distributionInverseIndex* NearestNeighbors::getDistributionOfInverseIndex() {
    return mInverseIndex->getDistribution();
}
