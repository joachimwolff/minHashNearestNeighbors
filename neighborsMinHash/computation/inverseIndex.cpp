/**
 Copyright 2015 Joachim Wolff
 Master Thesis
 Tutors: Milad Miladi, Fabrizio Costa
 Winter semester 2015/2016

 Chair of Bioinformatics
 Department of Computer Science
 Faculty of Engineering
 Albert-Ludwig-University Freiburg im Breisgau
**/

#include <algorithm>
#include <iostream>
#include <iterator>
#include <utility>

#ifdef OPENMP
#include <omp.h>
#endif

#include "inverseIndex.h"

class sort_map {
  public:
    size_t key;
    size_t val;
};

bool mapSortDescByValue(const sort_map& a, const sort_map& b) {
        return a.val > b.val;
};

InverseIndex::InverseIndex(size_t pNumberOfHashFunctions, size_t pBlockSize,
                    size_t pNumberOfCores, size_t pChunkSize,
                    size_t pMaxBinSize, size_t pMinimalBlocksInCommon,
                    size_t pExcessFactor, size_t pMaximalNumberOfHashCollisions) {
    
    mNumberOfHashFunctions = pNumberOfHashFunctions;
    mBlockSize = pBlockSize;
    mNumberOfCores = pNumberOfCores;
    mChunkSize = pChunkSize;
    mMaxBinSize = pMaxBinSize;
    // mSizeOfNeighborhood = pSizeOfNeighborhood;
    mMinimalBlocksInCommon = pMinimalBlocksInCommon;
    mExcessFactor = pExcessFactor;
    mMaximalNumberOfHashCollisions = pMaximalNumberOfHashCollisions;
    mInverseIndexUmapVector = new vector__umapVector();
    mSignatureStorage = new umap_uniqueElement();
    size_t inverseIndexSize = ceil(((float) mNumberOfHashFunctions / (float) mBlockSize)+1);
    mInverseIndexUmapVector->resize(inverseIndexSize);
}
 
InverseIndex::~InverseIndex() {

    for (auto it = (*mSignatureStorage).begin(); it != (*mSignatureStorage).end(); ++it) {
        delete it->second->instances;
        delete it->second->signature;
        delete it->second;

    }
    delete mSignatureStorage;
    delete mInverseIndexUmapVector;
}
 // compute the signature for one instance
vsize_t* InverseIndex::computeSignature(const SparseMatrixFloat* pRawData, const size_t pInstance) {

    vsize_t signatureHash;
    signatureHash.reserve(mNumberOfHashFunctions);

    for(size_t j = 0; j < mNumberOfHashFunctions; ++j) {
        size_t minHashValue = MAX_VALUE;
        for (size_t i = 0; i < pRawData->getSizeOfInstance(pInstance); ++i) {
            size_t hashValue = _size_tHashSimple((pRawData->getNextElement(pInstance, i) +1) * (j+1) * A, MAX_VALUE);
            if (hashValue < minHashValue) {
                minHashValue = hashValue;
            }
        }
        signatureHash[j] = minHashValue;
    }
    // reduce number of hash values by a factor of blockSize
    size_t k = 0;
    vsize_t* signature = new vsize_t();
    signature->reserve((mNumberOfHashFunctions / mBlockSize) + 1);
    while (k < (mNumberOfHashFunctions)) {
        // use computed hash value as a seed for the next computation
        size_t signatureBlockValue = signatureHash[k];
        for (size_t j = 0; j < mBlockSize; ++j) {
            signatureBlockValue = _size_tHashSimple((signatureHash[k+j]) * signatureBlockValue * A, MAX_VALUE);
        }
        signature->push_back(signatureBlockValue);
        k += mBlockSize; 
    }
    return signature;
}
umap_uniqueElement* InverseIndex::computeSignatureMap(const SparseMatrixFloat* pRawData) {
    mDoubleElementsQueryCount = 0;
    const size_t sizeOfInstances = pRawData->size();
    umap_uniqueElement* instanceSignature = new umap_uniqueElement();
    (*instanceSignature).reserve(sizeOfInstances);
    if (mChunkSize <= 0) {
        mChunkSize = ceil(pRawData->size() / static_cast<float>(mNumberOfCores));
    }

#ifdef OPENMP
    omp_set_dynamic(0);
#endif


#pragma omp parallel for schedule(static, mChunkSize) num_threads(mNumberOfCores)
    for(size_t index = 0; index < pRawData->size(); ++index) {
        // vsize_t* features = pRawData->getFeatureRow(index);
        // compute unique id
        size_t signatureId = 0;
        for (size_t j = 0; j < pRawData->getSizeOfInstance(index); ++j) {
                signatureId = _size_tHashSimple((pRawData->getNextElement(index, j) +1) * (signatureId+1) * A, MAX_VALUE);
        }
        // signature is in storage && 
        auto signatureIt = (*mSignatureStorage).find(signatureId);
        if (signatureIt != (*mSignatureStorage).end() && (instanceSignature->find(signatureId) != instanceSignature->end())) {
#pragma omp critical
            {
                instanceSignature->operator[](signatureId) = (*mSignatureStorage)[signatureId];
                instanceSignature->operator[](signatureId)->instances->push_back(index);
                mDoubleElementsQueryCount += (*mSignatureStorage)[signatureId]->instances->size();
            }
            continue;
        }

        // for every hash function: compute the hash values of all features and take the minimum of these
        // as the hash value for one hash function --> h_j(x) = argmin (x_i of x) f_j(x_i)
        vsize_t* signature = computeSignature(pRawData, index);
#pragma omp critical
        {
            if (instanceSignature->find(signatureId) == instanceSignature->end()) {
                vsize_t* doubleInstanceVector = new vsize_t(1);
                (*doubleInstanceVector)[0] = index;
                uniqueElement* element = new uniqueElement();;
                element->instances = doubleInstanceVector;
                element->signature = signature;
                instanceSignature->operator[](signatureId) = element;
            } else {
                instanceSignature->operator[](signatureId)->instances->push_back(index);
                mDoubleElementsQueryCount += 1;
            }
        }
    }
    return instanceSignature;
}
void InverseIndex::fit(const SparseMatrixFloat* pRawData) {
    // std::cout << "Fitting started" << std::endl;
    mDoubleElementsStorageCount = 0;
    size_t inverseIndexSize = ceil(((float) mNumberOfHashFunctions / (float) mBlockSize)+1);
    mInverseIndexUmapVector->resize(pRawData->size());
    // std::cout << "155" << std::endl;

    if (mChunkSize <= 0) {
        mChunkSize = ceil(pRawData->size() / static_cast<float>(mNumberOfCores));
    }
#ifdef OPENMP
    omp_set_dynamic(0);
#endif

#pragma omp parallel for schedule(static, mChunkSize) num_threads(mNumberOfCores)

    for (size_t index = 0; index < pRawData->size(); ++index) {
        size_t signatureId = 0;
    // std::cout << "168 " << std::endl;

        for (size_t j = 0; j < pRawData->getSizeOfInstance(index); ++j) {
            signatureId = _size_tHashSimple((pRawData->getNextElement(index, j) +1) * (signatureId+1) * A, MAX_VALUE);
        }
        vsize_t* signature;
        auto itSignatureStorage = mSignatureStorage->find(signatureId);
        if (itSignatureStorage == mSignatureStorage->end()) {
            signature = computeSignature(pRawData, index);
        } else {
            signature = itSignatureStorage->second->signature;
        }
        // insert in inverse index
#pragma omp critical
        {    
    // std::cout << "183" << std::endl;

            if (itSignatureStorage == mSignatureStorage->end()) {
                vsize_t* doubleInstanceVector = new vsize_t(1);
                (*doubleInstanceVector)[0] = index;
                uniqueElement* element = new uniqueElement();
                element->instances = doubleInstanceVector;
                element->signature = signature;
                mSignatureStorage->operator[](signatureId) = element;
            } else {
                 mSignatureStorage->operator[](signatureId)->instances->push_back(index);
                 mDoubleElementsStorageCount += 1;
            }
    // std::cout << "196" << std::endl;

            for (size_t j = 0; j < signature->size(); ++j) {
        // std::cout << "199" << std::endl;

                auto itHashValue_InstanceVector = mInverseIndexUmapVector->operator[](j).find((*signature)[j]);
                // if for hash function h_i() the given hash values is already stored
        // std::cout << "203" << std::endl;

                if (itHashValue_InstanceVector != mInverseIndexUmapVector->operator[](j).end()) {
        // std::cout << "206" << std::endl;

                    // insert the instance id if not too many collisions (maxBinSize)
                    if (itHashValue_InstanceVector->second.size() < mMaxBinSize) {
        // std::cout << "210" << std::endl;

                        // insert only if there wasn't any collisions in the past
                        if (itHashValue_InstanceVector->second.size() > 0) {
        // std::cout << "214" << std::endl;

                            itHashValue_InstanceVector->second.push_back(index);
                        }
                    } else { 
        // std::cout << "219" << std::endl;

                        // too many collisions: delete stored ids. empty vector is interpreted as an error code 
                        // for too many collisions
                        itHashValue_InstanceVector->second.clear();
                    }
                } else {
        // std::cout << "226" << std::endl;

                    // given hash value for the specific hash function was not avaible: insert new hash value
                    vsize_t instanceIdVector;
        // std::cout << "230" << std::endl;

                    instanceIdVector.push_back(index);
        // std::cout << "233" << std::endl;

                    mInverseIndexUmapVector->operator[](j)[(*signature)[j]] = instanceIdVector;
        // std::cout << "236" << std::endl;

                }
            }
        }
    }
    // std::cout << "242" << std::endl;

}

neighborhood* InverseIndex::kneighbors(const umap_uniqueElement* pSignaturesMap, 
                                        const int pNneighborhood, const bool pDoubleElementsStorageCount) {
    size_t doubleElements = 0;
    if (pDoubleElementsStorageCount) {
        doubleElements = mDoubleElementsStorageCount;
    } else {
        doubleElements = mDoubleElementsQueryCount;
    }
#ifdef OPENMP
    omp_set_dynamic(0);
#endif
    vvint* neighbors = new vvint();
    vvfloat* distances = new vvfloat();
    neighbors->resize(pSignaturesMap->size()+doubleElements);
    distances->resize(pSignaturesMap->size()+doubleElements);
    if (mChunkSize <= 0) {
        mChunkSize = ceil(mInverseIndexUmapVector->size() / static_cast<float>(mNumberOfCores));
    }

#pragma omp parallel for schedule(static, mChunkSize) num_threads(mNumberOfCores)
    for (size_t i = 0; i < pSignaturesMap->size(); ++i) {
        umap_uniqueElement::const_iterator instanceId = pSignaturesMap->begin();
        std::advance(instanceId, i); 
        
        std::unordered_map<size_t, size_t> neighborhood;
        const vsize_t* signature = instanceId->second->signature;
        for (size_t j = 0; j < signature->size(); ++j) {
            size_t hashID = (*signature)[j];
            if (hashID != 0 && hashID != MAX_VALUE) {
                size_t collisionSize = 0;
                // std::cout << "276" << std::endl;
                umapVector::const_iterator instances = mInverseIndexUmapVector->at(j).find(hashID);
                // std::cout << "278" << std::endl;

                if (instances != mInverseIndexUmapVector->at(j).end()) {
                    collisionSize = instances->second.size();
                } else { 
                    continue;
                }

                if (collisionSize < mMaxBinSize && collisionSize > 0) {
                    for (size_t k = 0; k < instances->second.size(); ++k) {
                        neighborhood[instances->second.at(k)] += 1;
                    }
                }
            }
        }
        std::vector< sort_map > neighborhoodVectorForSorting;
        
        for (auto it = neighborhood.begin(); it != neighborhood.end(); ++it) {
            sort_map mapForSorting;
            mapForSorting.key = (*it).first;
            mapForSorting.val = (*it).second;
            neighborhoodVectorForSorting.push_back(mapForSorting);
        }

        size_t numberOfElementsToSort = pNneighborhood;
        if (pNneighborhood > neighborhoodVectorForSorting.size()) {
            numberOfElementsToSort = neighborhoodVectorForSorting.size();
        }
        std::partial_sort(neighborhoodVectorForSorting.begin(), neighborhoodVectorForSorting.begin()+numberOfElementsToSort, neighborhoodVectorForSorting.end(), mapSortDescByValue);

        vint neighborhoodVector;
        std::vector<float> distanceVector;
        size_t sizeOfNeighborhoodAdjusted;
        if (pNneighborhood == MAX_VALUE) {
            sizeOfNeighborhoodAdjusted = std::min(static_cast<size_t>(pNneighborhood), neighborhoodVectorForSorting.size());
        } else {
            sizeOfNeighborhoodAdjusted = std::min(static_cast<size_t>(pNneighborhood * mExcessFactor), neighborhoodVectorForSorting.size());
        }

        size_t count = 0;

        for (auto it = neighborhoodVectorForSorting.begin();
                it != neighborhoodVectorForSorting.end(); ++it) {
            neighborhoodVector.push_back((*it).key);
            distanceVector.push_back(1 - ((*it).val / static_cast<float>(mMaximalNumberOfHashCollisions)));
            ++count;
            if (count == sizeOfNeighborhoodAdjusted) {
                break;
            }
        }
#pragma omp critical
        { 
            for (size_t j = 0; j < instanceId->second->instances->size(); ++j) {
                (*neighbors)[instanceId->second->instances->operator[](j)] = neighborhoodVector;
                (*distances)[instanceId->second->instances->operator[](j)] = distanceVector;
            }
        }
    }
    neighborhood* neighborhood_ = new neighborhood();
    neighborhood_->neighbors = neighbors;
    neighborhood_->distances = distances;

    return neighborhood_;
}