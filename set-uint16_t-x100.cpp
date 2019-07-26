#include <vector>
#include <set>
#include <iostream>
#include <unistd.h>

// $ g++ -std=c++11 set-uint16_t-x100.cpp
// $ ps -C a.out -o rss,cmd
//   RSS CMD
//   311268 ./a.out

int main()
{
        typedef std::_Rb_tree_node<uint16_t> node_type;
        std::cout << "size of node: " << sizeof(node_type) << std::endl;

        std::vector<std::set<uint16_t>> bitmaps;
        for ( unsigned int j = 0 ; j < 100 ; j++ ) {
                std::set<uint16_t> bitmap;
                for ( uint16_t i = 0 ; i < 0xffff ; i++ ) {
                        bitmap.insert( i );
                }
                bitmaps.push_back( bitmap );
        }

        sleep( 60 );
        return 0;
}
