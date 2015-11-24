#include "inverseIndexStorage.h"
#include "bloomierFilter/bloomierFilter.h"

#ifndef INVERSE_INDEX_STORAGE_BLOOMIER_FILTER_H
#define INVERSE_INDEX_STORAGE_BLOOMIER_FILTER_H
class InverseIndexStorageBloomierFilter : public InverseIndexStorage {
  private:
	std::vector<BloomierFilter* >* mInverseIndex;
	vvsize_t* mKeys;
	vvsize_t* mValues;
	size_t mMaxBinSize;
	size_t mM;
	size_t mK;
	size_t mQ;
  public:
    InverseIndexStorageBloomierFilter(size_t pSizeOfInverseIndex, size_t pMaxBinSize);
	~InverseIndexStorageBloomierFilter();
  	size_t size();
	vsize_t* getElement(size_t pVectorId, size_t pHashValue);
	void insert(size_t pVectorId, size_t pHashValue, size_t pInstance);
	// void create();
};
#endif // INVERSE_INDEX_STORAGE_BLOOMIER_FILTER_H