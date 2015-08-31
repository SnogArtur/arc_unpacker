#pragma once

#include "fmt/archive.h"

namespace au {
namespace fmt {
namespace whale {

    class DatArchive final : public Archive
    {
    public:
        DatArchive();
        ~DatArchive();
        void set_game_title(const std::string &game_title);
        void add_file_name(const std::string &file_name);
        void add_cli_help(ArgParser &) const override;
        void parse_cli_options(const ArgParser &) override;
    protected:
        bool is_recognized_internal(File &) const override;
        void unpack_internal(File &, FileSaver &) const override;
    private:
        struct Priv;
        std::unique_ptr<Priv> p;
    };

} } }
