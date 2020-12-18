//
// Created by 王奇 on 2020/10/24.
//
#ifndef PLAYERVIDEO_TEST_H
#define PLAYERVIDEO_TEST_H

#include "logutil.h"

class Test {
public:
    Test();

    Test(Test *pTest);

    const char *getIntString();
    int getInt();
};


#endif //PLAYERVIDEO_TEST_H
