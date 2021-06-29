#ifndef __MM_DATATYPE__
#define __MM_DATATYPE__

// #include "ap_int.h"

// typedef struct orderBookResponse_t
// {
//     ap_uint<56> timestamp;
//     // ap_uint<8> symbolIndex;
//     ap_uint<4> symbolRow; // here
//     ap_uint<4> symbolCol; // here
//     ap_uint<32> bidCount[NUM_LEVEL];
//     ap_uint<32> bidPrice[NUM_LEVEL];
//     ap_uint<32> bidQuantity[NUM_LEVEL];
//     ap_uint<32> askCount[NUM_LEVEL];
//     ap_uint<32> askPrice[NUM_LEVEL]; // here
//     ap_uint<32> askQuantity[NUM_LEVEL];
// } orderBookResponse_t;

// typedef struct orderEntryOperation_t
// {
//     ap_uint<64> timestamp; // here
//     ap_uint<8> opCode;
//     // ap_uint<8> symbolIndex;
//     ap_uint<4> symbolRow; // here
//     ap_uint<4> symbolCol; // here
//     ap_uint<32> orderId;
//     ap_uint<32> quantity; // here (=1)
//     ap_uint<32> price;
//     ap_uint<8> direction;
// } orderEntryOperation_t;


typedef struct orderBookResponse_t
{
    unsigned int symbolRow;
    unsigned int symbolCol;
    float askPrice;
} orderBookResponse_t;

typedef struct orderEntryOperation_t
{
    unsigned int timestamp;
    unsigned int symbolRow;
    unsigned int symbolCol;
} orderEntryOperation_t;

#endif
