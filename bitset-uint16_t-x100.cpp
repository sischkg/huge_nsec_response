#include <vector>
#include <bitset>
#include <iostream>
#include <unistd.h>

// $ g++ -std=c++11 bitset-uint16_t-x100.cpp
// $ ps -C a.out -o rss,cmd
//   RSS CMD
//   311268 ./a.out

typedef std::bitset<65536> BitMaps;

int main()
{
        std::vector<BitMaps> bitmaps;
        for ( unsigned int j = 0 ; j < 100 ; j++ ) {
                BitMaps bitmap;
                for ( uint16_t i = 0 ; i < 0xffff ; i++ ) {
                        bitmap.set( i );
                }
                bitmaps.push_back( bitmap );
        }

        sleep( 60 );
        return 0;
}
