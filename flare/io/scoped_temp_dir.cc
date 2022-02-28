// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flare/io/scoped_temp_dir.h"
#include "flare/io/temp_file.h"
#include "flare/log/logging.h"

#define BASE_FILES_TEMP_DIR_PATTERN "temp_dir_XXXXXX"

namespace flare::io {

    static bool create_temporary_dir_in_dir_impl(const flare::filesystem::path &base_dir,
                                                 const std::string &name_tmpl,
                                                 flare::filesystem::path *new_dir);

    bool create_temporary_dir_in_dir_impl(const flare::filesystem::path &base_dir,
                                          const std::string &name_tmpl,
                                          flare::filesystem::path *new_dir) {
        DCHECK(name_tmpl.find("XXXXXX") != std::string::npos)
                            << "Directory name template must contain \"XXXXXX\".";

        flare::filesystem::path sub_dir = base_dir;
        sub_dir /= name_tmpl;
        std::string sub_dir_string = sub_dir.generic_string();

        // this should be OK since mkdtemp just replaces characters in place
        char *buffer = const_cast<char *>(sub_dir_string.c_str());
        char *dtemp = mkdtemp(buffer);
        if (!dtemp) {
            DPLOG(ERROR) << "mkdtemp";
            return false;
        }
        *new_dir = flare::filesystem::path(dtemp);
        return true;
    }

    bool create_new_temp_directory(const flare::filesystem::path &prefix, flare::filesystem::path *newPath) {
        flare::filesystem::path tmp_dir;
        if (prefix.empty()) {
            return false;
        }
        if (prefix.is_absolute()) {
            tmp_dir = prefix;
        } else {
            std::error_code ec;
            tmp_dir = flare::filesystem::temp_directory_path(ec);
            if (ec) {
                return false;
            }
            tmp_dir /= prefix;
        }
        return create_temporary_dir_in_dir_impl(tmp_dir, BASE_FILES_TEMP_DIR_PATTERN, newPath);
    }

    bool create_temporary_dir_in_dir(const flare::filesystem::path &base, const std::string &prefix,
                                     flare::filesystem::path *newPath) {
        std::string mkdtemp_template = prefix + "XXXXXX";
        return create_temporary_dir_in_dir_impl(base, mkdtemp_template, newPath);
    }

    scoped_temp_dir::scoped_temp_dir() {
    }

    scoped_temp_dir::~scoped_temp_dir() {
        if (!_path.empty() && !remove())
            DLOG(WARNING) << "Could not delete temp dir in dtor.";
    }

    bool scoped_temp_dir::create_unique_temp_dir() {
        if (!_path.empty())
            return false;
        // This "scoped_dir" prefix is only used on Windows and serves as a template
        // for the unique name.
        static flare::filesystem::path kScopedDir("scoped_dir");
        if (!create_new_temp_directory(kScopedDir, &_path))
            return false;

        return true;
    }

    bool scoped_temp_dir::create_unique_temp_dir_under_path(const flare::filesystem::path &base_path) {
        if (!_path.empty())
            return false;

        // If |base_path| does not exist, create it.
        std::error_code ec;
        if (!flare::filesystem::create_directories(base_path, ec))
            return false;

        // Create a new, uniquely named directory under |base_path|.
        if (!create_temporary_dir_in_dir(base_path, "scoped_dir_",
                                         &_path))
            return false;

        return true;
    }

    bool scoped_temp_dir::set(const flare::filesystem::path &path) {
        if (!_path.empty())
            return false;

        std::error_code ec;
        if (!flare::filesystem::exists(path, ec) && !flare::filesystem::create_directories(path, ec))
            return false;

        _path = path;
        return true;
    }

    bool scoped_temp_dir::remove() {
        if (_path.empty())
            return false;
        std::error_code ec;
        bool ret = flare::filesystem::remove_all(_path, ec);
        if (ret) {
            // We only clear the path if deleted the directory.
            _path.clear();
        }

        return ret;
    }

    flare::filesystem::path scoped_temp_dir::take() {
        flare::filesystem::path ret = _path;
        _path = flare::filesystem::path();
        return ret;
    }

    bool scoped_temp_dir::is_valid() const {
        std::error_code ec;
        return !_path.empty() && flare::filesystem::exists(_path, ec) && flare::filesystem::is_directory(_path, ec);
    }

}  // namespace flare::io
