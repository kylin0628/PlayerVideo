//
// Created by 王奇 on 2020/10/24.
//
#include "Test.h"

extern "C" {
#include "include/libavformat/avformat.h"
}

const char *Test::getIntString() {
    sloge("%s", av_version_info());
    return "av_version_info()";
}

Test::Test() {

}

Test::Test(Test *pTest) {

}

int Test::getInt() {
    return 100;
}
