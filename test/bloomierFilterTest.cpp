#include <gtest/gtest.h>
#include "../neighborsMinHash/computation/bloomierFilter/bloomierFilter.h"
#include "../neighborsMinHash/computation/bloomierFilter/encoder.h"

TEST(BloomierFilterTest, setAndGetTest) {
    BloomierFilter* bloomierFilter = new BloomierFilter(10, 3, 8);
    bloomierFilter->set(1, 10);
    bloomierFilter->set(2, 20);
    bloomierFilter->set(3, 30);
    bloomierFilter->set(3, 40);
    EXPECT_EQ(bloomierFilter->get(1)->size(), 1);
    EXPECT_EQ(bloomierFilter->get(2)->size(), 1);
    EXPECT_EQ(bloomierFilter->get(3)->size(), 2);
    EXPECT_EQ(bloomierFilter->get(1)->at(0), 10);
    EXPECT_EQ(bloomierFilter->get(2)->at(0), 20);
    EXPECT_EQ(bloomierFilter->get(3)->at(0), 30);
    EXPECT_EQ(bloomierFilter->get(3)->at(1), 40);
    delete bloomierFilter;
}
TEST(EncoderTest, encodeTest) {
    Encoder* encodeObj = new Encoder(4);
    vsize_t* maskingValues = encodeObj->getMaskingValues();
    EXPECT_EQ(maskingValues->size(), 4);
    EXPECT_EQ(maskingValues->at(0), 255);
    EXPECT_EQ(maskingValues->at(1), 65280);
    EXPECT_EQ(maskingValues->at(2), 16711680);
    EXPECT_EQ(maskingValues->at(3), 4278190080);
    
    delete encodeObj;
    encodeObj = new Encoder(2);
    
    EXPECT_EQ(encodeObj->encode(5)->size(), 2);
    EXPECT_EQ(encodeObj->encode(5)->at(0), 5);
    EXPECT_EQ(encodeObj->encode(5)->at(1), 0);

    delete encodeObj;
    encodeObj = new Encoder(3);
    
    EXPECT_EQ(encodeObj->encode(1024)->size(), 3);
    EXPECT_EQ(encodeObj->encode(1024)->at(0), 0);
    EXPECT_EQ(encodeObj->encode(1024)->at(1), 4);
    EXPECT_EQ(encodeObj->encode(1024)->at(2), 0);
    char value = 255;
    EXPECT_EQ(encodeObj->encode(255)->size(), 3);
    EXPECT_EQ(encodeObj->encode(255)->at(0), value);
    EXPECT_EQ(encodeObj->encode(255)->at(1), 0);
    EXPECT_EQ(encodeObj->encode(255)->at(2), 0);

    EXPECT_EQ(encodeObj->encode(257)->size(), 3);    
    EXPECT_EQ(encodeObj->encode(257)->at(0), 1);
    EXPECT_EQ(encodeObj->encode(257)->at(1), 1);
    EXPECT_EQ(encodeObj->encode(257)->at(2), 0);
    
    delete encodeObj;
}

TEST(EncoderTest, decodeTest) {
    Encoder* encodeObj = new Encoder(3);
    bitVector* encodedValue = encodeObj->encode(1024);
    EXPECT_EQ(encodedValue->size(), 3);
    EXPECT_EQ(encodedValue->at(0), 0);
    EXPECT_EQ(encodedValue->at(1), 4);
    EXPECT_EQ(encodedValue->at(2), 0);
    
    EXPECT_EQ(encodeObj->decode(encodedValue), 1024);
    
    delete encodeObj;
}

TEST(BloomierHashTest, getMaskTest) {
    BloomierHash* bloomierHash = new BloomierHash(19, 3, 2);
    bitVector* mask = bloomierHash->getMask(1);
    EXPECT_EQ(mask->at(0), 103);
    EXPECT_EQ(mask->at(1), 69);
    delete bloomierHash;
}

TEST(BloomierHashTest, getKNeighborsTest) {
    BloomierHash* bloomierHash = new BloomierHash(19, 3, 2);
    vsize_t* neighbors = bloomierHash->getKNeighbors(1, 3, 255);

    EXPECT_EQ(neighbors->size(), 3);
    EXPECT_EQ(neighbors->at(0), 220);
    EXPECT_EQ(neighbors->at(1), 234);
    EXPECT_EQ(neighbors->at(2), 214);
    delete bloomierHash;
}

TEST(OrderAndMatchFinderTest, findTest) {
    BloomierHash* bloomierHash = new BloomierHash(19, 3, 2);
    OrderAndMatchFinder* orderAndMatchFinder = new OrderAndMatchFinder(19, 3, 8, bloomierHash);
    vsize_t* pSubset = new vsize_t();
    pSubset->push_back(3);
    pSubset->push_back(4);
    pSubset->push_back(1);
    pSubset->push_back(8);
    orderAndMatchFinder->find(pSubset);
    vsize_t* piVector = orderAndMatchFinder->getPiVector();
    vsize_t* tauVector = orderAndMatchFinder->getTauVector();
    for (size_t i = 0; i < piVector->size(); ++i) {
        std::cout << "piVector: " << piVector->at(i) << std::endl;
    }
    for (size_t i = 0; i < tauVector->size(); ++i) {
        std::cout << "tauVector: " << tauVector->at(i) << std::endl;
    }
}

TEST(OrderAndMatchFinderTest, tweakTest) {
    BloomierHash* bloomierHash = new BloomierHash(10, 3, 2);
    OrderAndMatchFinder* orderAndMatchFinder = new OrderAndMatchFinder(19, 3, 8, bloomierHash);
    
}

TEST(OrderAndMatchFinderTest, computeNonSingeltonsTest) {
    BloomierHash* bloomierHash = new BloomierHash(19, 3, 2);
    OrderAndMatchFinder* orderAndMatchFinder = new OrderAndMatchFinder(19, 3, 8, bloomierHash);
}
int main (int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return  RUN_ALL_TESTS();
}