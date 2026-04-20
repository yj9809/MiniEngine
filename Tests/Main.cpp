#include <iostream>
#include <gtest/gtest.h>

// 특정 테스트만 실행하려면 필터를 변경한다.
// 전체 실행: "--gtest_filter=*"
// 예)  "--gtest_filter=TimeTest.*"   "--gtest_filter=Matrix4Test.*"
#define TEST_FILTER "--gtest_filter=TimeTest.*"

int main(int argc, char** argv)
{
    const char* args[] = { argv[0], TEST_FILTER };
    int newArgc = 2;
    ::testing::InitGoogleTest(&newArgc, const_cast<char**>(args));
    return RUN_ALL_TESTS();
}
