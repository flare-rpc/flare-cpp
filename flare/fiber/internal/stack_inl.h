

#ifndef FLARE_FIBER_INTERNAL_STACK_INL_H_
#define FLARE_FIBER_INTERNAL_STACK_INL_H_

DECLARE_int32(guard_page_size);
DECLARE_int32(tc_stack_small);
DECLARE_int32(tc_stack_normal);

namespace flare::fiber_internal {

    struct MainStackClass {
    };

    struct SmallStackClass {
        static int *stack_size_flag;
        // Older gcc does not allow static const enum, use int instead.
        static const int stacktype = (int) STACK_TYPE_SMALL;
    };

    struct NormalStackClass {
        static int *stack_size_flag;
        static const int stacktype = (int) STACK_TYPE_NORMAL;
    };

    struct LargeStackClass {
        static int *stack_size_flag;
        static const int stacktype = (int) STACK_TYPE_LARGE;
    };

    template<typename StackClass>
    struct StackFactory {
        struct Wrapper : public fiber_contextual_stack {
            explicit Wrapper(void (*entry)(intptr_t)) {
                if (allocate_stack_storage(&storage, *StackClass::stack_size_flag,
                                           FLAGS_guard_page_size) != 0) {
                    storage.zeroize();
                    context = NULL;
                    return;
                }
                context = flare_fiber_make_context(storage.bottom, storage.stacksize, entry);
                stacktype = (fiber_stack_type) StackClass::stacktype;
            }

            ~Wrapper() {
                if (context) {
                    context = NULL;
                    deallocate_stack_storage(&storage);
                    storage.zeroize();
                }
            }
        };

        static fiber_contextual_stack *get_stack(void (*entry)(intptr_t)) {
            return flare::get_object<Wrapper>(entry);
        }

        static void return_stack(fiber_contextual_stack *sc) {
            flare::return_object(static_cast<Wrapper *>(sc));
        }
    };

    template<>
    struct StackFactory<MainStackClass> {
        static fiber_contextual_stack *get_stack(void (*)(intptr_t)) {
            fiber_contextual_stack *s = new(std::nothrow) fiber_contextual_stack;
            if (NULL == s) {
                return NULL;
            }
            s->context = NULL;
            s->stacktype = STACK_TYPE_MAIN;
            s->storage.zeroize();
            return s;
        }

        static void return_stack(fiber_contextual_stack *s) {
            delete s;
        }
    };

    inline fiber_contextual_stack *get_stack(fiber_stack_type type, void (*entry)(intptr_t)) {
        switch (type) {
            case STACK_TYPE_PTHREAD:
                return NULL;
            case STACK_TYPE_SMALL:
                return StackFactory<SmallStackClass>::get_stack(entry);
            case STACK_TYPE_NORMAL:
                return StackFactory<NormalStackClass>::get_stack(entry);
            case STACK_TYPE_LARGE:
                return StackFactory<LargeStackClass>::get_stack(entry);
            case STACK_TYPE_MAIN:
                return StackFactory<MainStackClass>::get_stack(entry);
        }
        return NULL;
    }

    inline void return_stack(fiber_contextual_stack *s) {
        if (NULL == s) {
            return;
        }
        switch (s->stacktype) {
            case STACK_TYPE_PTHREAD:
                assert(false);
                return;
            case STACK_TYPE_SMALL:
                return StackFactory<SmallStackClass>::return_stack(s);
            case STACK_TYPE_NORMAL:
                return StackFactory<NormalStackClass>::return_stack(s);
            case STACK_TYPE_LARGE:
                return StackFactory<LargeStackClass>::return_stack(s);
            case STACK_TYPE_MAIN:
                return StackFactory<MainStackClass>::return_stack(s);
        }
    }

    inline void jump_stack(fiber_contextual_stack *from, fiber_contextual_stack *to) {
        flare_fiber_jump_context(&from->context, to->context, 0/*not skip remained*/);
    }

}  // namespace flare::fiber_internal

namespace flare {

    template<>
    struct ObjectPoolBlockMaxItem<
            flare::fiber_internal::StackFactory<flare::fiber_internal::LargeStackClass>::Wrapper> {
        static const size_t value = 64;
    };
    template<>
    struct ObjectPoolBlockMaxItem<
            flare::fiber_internal::StackFactory<flare::fiber_internal::NormalStackClass>::Wrapper> {
        static const size_t value = 64;
    };

    template<>
    struct ObjectPoolBlockMaxItem<
            flare::fiber_internal::StackFactory<flare::fiber_internal::SmallStackClass>::Wrapper> {
        static const size_t value = 64;
    };

    template<>
    struct ObjectPoolFreeChunkMaxItem<
            flare::fiber_internal::StackFactory<flare::fiber_internal::SmallStackClass>::Wrapper> {
        inline static size_t value() {
            return (FLAGS_tc_stack_small <= 0 ? 0 : FLAGS_tc_stack_small);
        }
    };

    template<>
    struct ObjectPoolFreeChunkMaxItem<
            flare::fiber_internal::StackFactory<flare::fiber_internal::NormalStackClass>::Wrapper> {
        inline static size_t value() {
            return (FLAGS_tc_stack_normal <= 0 ? 0 : FLAGS_tc_stack_normal);
        }
    };

    template<>
    struct ObjectPoolFreeChunkMaxItem<
            flare::fiber_internal::StackFactory<flare::fiber_internal::LargeStackClass>::Wrapper> {
        inline static size_t value() { return 1UL; }
    };

    template<>
    struct ObjectPoolValidator<
            flare::fiber_internal::StackFactory<flare::fiber_internal::LargeStackClass>::Wrapper> {
        inline static bool validate(
                const flare::fiber_internal::StackFactory<flare::fiber_internal::LargeStackClass>::Wrapper *w) {
            return w->context != NULL;
        }
    };

    template<>
    struct ObjectPoolValidator<
            flare::fiber_internal::StackFactory<flare::fiber_internal::NormalStackClass>::Wrapper> {
        inline static bool validate(
                const flare::fiber_internal::StackFactory<flare::fiber_internal::NormalStackClass>::Wrapper *w) {
            return w->context != NULL;
        }
    };

    template<>
    struct ObjectPoolValidator<
            flare::fiber_internal::StackFactory<flare::fiber_internal::SmallStackClass>::Wrapper> {
        inline static bool validate(
                const flare::fiber_internal::StackFactory<flare::fiber_internal::SmallStackClass>::Wrapper *w) {
            return w->context != NULL;
        }
    };

}  // namespace flare

#endif  // FLARE_FIBER_INTERNAL_STACK_INL_H_
