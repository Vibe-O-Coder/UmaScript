#include <chrono>
#include <cstdint>
#include <iostream>
std::int64_t compute(std::int64_t count,std::int64_t seed) {
    std::int64_t checksum=0;
    for(std::int64_t i=0;i<count;++i){seed=(seed*48271)%2147483647;checksum^=seed;}
    return checksum;
}
int main(){std::int64_t count,seed;std::cin>>count>>seed;auto start=std::chrono::steady_clock::now();auto checksum=compute(count,seed);auto elapsed=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-start).count();std::cout<<checksum<<' '<<elapsed<<'\n';}
