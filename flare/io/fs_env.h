//
// Created by liyinbin on 2022/2/6.
//

#ifndef FLARE_IO_FS_ENV_H_
#define FLARE_IO_FS_ENV_H_

#include <string>
#include <memory>
#include <type_traits>
#include "flare/base/profile.h"
#include "flare/base/status.h"
#include "flare/io/cord_buf.h"

namespace flare {

    using flare_status = flare::base::flare_status;

    class open_close {
    public:
        virtual ~open_close() = default;

        virtual flare_status open() = 0;

        virtual void close() = 0;
    };

    namespace detail {
        template<typename T>
        struct auto_close_object {
            static_assert(std::is_base_of<flare::open_close, T>::value, "must inherit from close_able");

            void operator()(T *obj) {
                if (obj) {
                    obj->close();
                }
            }
        };
    }
    template<typename T>
    class auto_close_ptr : public std::unique_ptr<T, detail::auto_close_object<T>> {
    public:
        auto_close_ptr() = default;

        explicit auto_close_ptr(T *p) : std::unique_ptr<T, detail::auto_close_object<T>>(p) {}
    };

    class random_access_file : public open_close {
    public:
        random_access_file() = default;

        ~random_access_file() override = default;

        virtual flare_status read(uint64_t offset, size_t n, std::string *content) = 0;

        virtual flare_status read(uint64_t offset, size_t n, flare::cord_buf *buf) = 0;

    private:
        FLARE_DISALLOW_COPY_AND_ASSIGN(random_access_file);
    };

    class sequential_access_file : public open_close {
    public:
        sequential_access_file() = default;

        ~sequential_access_file() override = default;

        virtual flare_status read(size_t n, std::string *content) = 0;

        virtual flare_status read(size_t n, flare::cord_buf *buf) = 0;

        virtual flare_status skip(size_t n) = 0;

    private:
        FLARE_DISALLOW_COPY_AND_ASSIGN(sequential_access_file);
    };

    class writeable_file : public open_close {
    public:
        writeable_file() = default;

        ~writeable_file() override = default;

        virtual flare_status pos_write(uint64_t offset, flare::cord_buf *content) = 0;

        virtual flare_status pos_write(uint64_t offset, const std::string_view &content) = 0;

        virtual flare_status flush() = 0;

        virtual flare_status sync() = 0;

    private:
        FLARE_DISALLOW_COPY_AND_ASSIGN(writeable_file);
    };

    class append_file : public open_close {
    public:
        append_file() = default;

        ~append_file() override = default;

        virtual flare_status append(flare::cord_buf *content) = 0;

        virtual flare_status append(const std::string_view &content) = 0;

        virtual flare_status flush() = 0;

        virtual flare_status sync() = 0;

    private:
        FLARE_DISALLOW_COPY_AND_ASSIGN(append_file);
    };

    class FLARE_EXPORT fs_env {
    public:
        fs_env() = default;

        virtual ~fs_env() = default;

        static fs_env *default_disk_env();

        virtual flare_status
        get_random_access_file(const std::string &path, auto_close_ptr<random_access_file> &file) = 0;

        virtual flare_status
        get_sequential_access_file(const std::string &path, auto_close_ptr<sequential_access_file> &file) = 0;

        virtual flare_status
        get_writeable_file(const std::string &path, auto_close_ptr<writeable_file> &file) = 0;

        virtual flare_status
        get_append_file(const std::string &path, auto_close_ptr<append_file> &file) = 0;

    private:
        FLARE_DISALLOW_COPY_AND_ASSIGN(fs_env);
    };


}  // namespace flare
#endif // FLARE_IO_FS_ENV_H_

