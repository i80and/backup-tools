#include "util.hpp"
#include "paddedbuffer.hpp"

int main(int argc, char** argv) {
    PaddedBuffer<1, 5> pb(1);

    verify(pb.padding() == 1);
    verify(pb.rawSize() == 1);
    verify(pb.size() == 0);

    pb.setSize(6);
    verify(pb.size() == 6);
    verify(pb.rawSize() == 7);

    pb.data()[0] = 'f';
    for(size_t i = 0; i < pb.padding(); i += 1) {
        verify(pb.rawData()[i] == 0);
    }

    verify(!pb.canSetSize(7));

    return 0;
}
