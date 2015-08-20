#ifndef AU_FMT_BGI_CBG_CBG1_DECODER_H
#define AU_FMT_BGI_CBG_CBG1_DECODER_H
#include "io/io.h"
#include "pix/grid.h"

namespace au {
namespace fmt {
namespace bgi {
namespace cbg {

    class Cbg1Decoder final
    {
    public:
        std::unique_ptr<pix::Grid> decode(io::IO &io) const;
    };

} } } }

#endif