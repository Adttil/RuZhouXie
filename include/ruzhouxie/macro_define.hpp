#ifndef RUZHOUXIE_MACRO_DEFINE_H
#define RUZHOUXIE_MACRO_DEFINE_H

#ifdef _MSC_VER

#define RUZHOUXIE_MAYBE_EMPTY [[msvc::no_unique_address]]
#define RUZHOUXIE_INLINE [[msvc::forceinline]]
#define RUZHOUXIE_INLINE_LAMBDA
#define RUZHOUXIE_INTRINSIC [[msvc::intrinsic]]
#define RUZHOUXIE_CONSTEVAL constexpr

#else

#ifdef __clang__

#define RUZHOUXIE_MAYBE_EMPTY [[msvc::no_unique_address]] [[no_unique_address]]
#define RUZHOUXIE_INLINE [[clang::always_inline]]
#define RUZHOUXIE_INLINE_LAMBDA [[clang::always_inline]]
#define RUZHOUXIE_INTRINSIC
#define RUZHOUXIE_CONSTEVAL consteval

#else

#define RUZHOUXIE_MAYBE_EMPTY [[no_unique_address]]
#define RUZHOUXIE_INLINE
#define RUZHOUXIE_INLINE_LAMBDA
#define RUZHOUXIE_INTRINSIC
#define RUZHOUXIE_CONSTEVAL consteval

#endif

#endif

#define CONCAT_IMPL(A, B) A##B
#define CONCAT(A, B) CONCAT_IMPL(A, B)

#define GET_N(N, ...) CONCAT(GET_N_, N)(__VA_ARGS__)
#define GET_N_0(_0, ...) _0
#define GET_N_1(_0, _1, ...) _1
#define GET_N_2(_0, _1, _2, ...) _2
#define GET_N_3(_0, _1, _2, _3, ...) _3
#define GET_N_4(_0, _1, _2, _3, _4, ...) _4

#define GET_LENGTH(...) GET_N(4 __VA_OPT__(,) __VA_ARGS__, 4, 3, 2, 1, 0)

#define FWD(...) CONCAT(FWD_, GET_LENGTH(__VA_ARGS__))(__VA_ARGS__)
#define FWD_0()
#define FWD_1(_0) static_cast<decltype(_0)&&>(_0)
#define FWD_2(_0, _1) ::ruzhouxie::fwd<decltype(_0), decltype(_0._1)>(_0._1)
#define FWD_3(_0, _1, _2) ::ruzhouxie::fwd<decltype(_0), decltype(_0._1), decltype(_0._1._2)>(_0._1._2)
#define FWD_4(_0, _1, _2, _3) ::ruzhouxie::fwd<decltype(_0), decltype(_0._1), decltype(_0._1._2), decltype(_0._1._2._3)>(_0._1._2._3)

#define FWDLIKE(...) CONCAT(FWDLIKE_, GET_LENGTH(__VA_ARGS__))(__VA_ARGS__)
#define FWDLIKE_0()
#define FWDLIKE_1(_0) static_cast<decltype(_0)&&>(_0)
#define FWDLIKE_2(_0, _1) ::ruzhouxie::fwd<decltype(_0), decltype(_1)>(_1)
#define FWDLIKE_3(_0, _1, _2) ::ruzhouxie::fwd<decltype(_0), decltype(_1), decltype(_2)>(_2)
#define FWDLIKE_4(_0, _1, _2, _3) ::ruzhouxie::fwd<decltype(_0), decltype(_1), decltype(_2), decltype(_3)>(_3)

#define AS_EXPRESSION(...) noexcept(noexcept(__VA_ARGS__)) -> decltype(auto)\
    requires requires{ __VA_ARGS__; }\
{\
    return __VA_ARGS__;\
}

#endif