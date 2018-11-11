#ifndef DEPS_ENTRY_H
#define DEPS_ENTRY_H

#include <iostream>
#include <array>
#include <vector>
#include "pal.h"
#include "version.h"

namespace coreload {
    struct deps_asset_t
    {
        deps_asset_t() : deps_asset_t(_X(""), _X(""), version_t(), version_t()) { }

        deps_asset_t(const pal::string_t& name, const pal::string_t& relative_path_, const version_t& assembly_version, const version_t& file_version)
            : name(name)
            , relative_path_(get_replaced_char(relative_path_, _X('\\'), _X('/'))) // Deps file does not follow spec. It uses '\\', should use '/'
            , assembly_version_(assembly_version)
            , file_version_(file_version) { }

        pal::string_t name;
        pal::string_t relative_path_;
        version_t assembly_version_;
        version_t file_version_;
    };

    class DepsEntry {
    public:
        enum AssetTypes {
            RUNTIME = 0,
            RESOURCES,
            NATIVE,
            COUNT
        };

        static const std::array<const pal::char_t*, DepsEntry::AssetTypes::COUNT> s_known_asset_types_;

        pal::string_t deps_file_;
        pal::string_t library_type_;
        pal::string_t library_name_;
        pal::string_t library_version_;
        pal::string_t library_hash_;
        pal::string_t library_path_;
        pal::string_t library_hash_path_;
        pal::string_t runtime_store_manifest_list_;
        AssetTypes asset_type_;
        deps_asset_t asset_;
        bool is_serviceable_;
        bool is_rid_specific_;

        // Given a "base" dir, yield the filepath within this directory or relative to this directory based on "look_in_base"
        bool to_path(const pal::string_t& base, bool look_in_base, pal::string_t* str) const;

        // Given a "base" dir, yield the file path within this directory.
        bool to_dir_path(const pal::string_t& base, pal::string_t* str) const;

        // Given a "base" dir, yield the relative path in the package layout.
        bool to_rel_path(const pal::string_t& base, pal::string_t* str) const;

        // Given a "base" dir, yield the relative path with package name, version in the package layout.
        bool to_full_path(const pal::string_t& root, pal::string_t* str) const;
    };
}

#endif // DEPS_ENTRY_H