#ifndef RUZHOUXIE_INVOKE_H
#define RUZHOUXIE_INVOKE_H

#include "general.h"
#include "get.h"
#include "tree_adaptor.h"
#include "tape.h"
#include "tree_view.h"
#include "relayout.h"

#include "macro_define.h"

namespace ruzhouxie::detail
{
    template<auto Indices, typename V>
    RUZHOUXIE_CONSTEVAL auto indices_adapt_truncate()
    {
        if constexpr(terminal<V>)
        {
            return indices_of_whole_view;
        }
        else
        {
            constexpr size_t i = Indices[0];
            return detail::concat_array(array{ i }, indices_adapt_truncate<detail::array_drop<1uz>(Indices), child_type<V, i>>());
        }
    }

    template<auto Seq, typename V>
    RUZHOUXIE_CONSTEVAL auto sequence_adapt_truncate()
    {
        if constexpr(indicesoid<decltype(Seq)>)
        {
            return indices_adapt_truncate<Seq, V>();
        }
        else return[]<size_t...I>(std::index_sequence<I...>)
        {
            return make_tuple(sequence_adapt_truncate<Seq | child<I>, V>()...);
        }(std::make_index_sequence<child_count<decltype(Seq)>>{});
    }

    template<typename TSeq>
    RUZHOUXIE_CONSTEVAL auto sequence_flatten(const TSeq& seq)
    {
        if constexpr(indicesoid<TSeq>)
        {
            return rzx::make_tuple(seq);
        }
        else return [&]<size_t...I>(std::index_sequence<I...>)
        {
            return detail::concat_to_tuple(sequence_flatten(seq | child<I>)...);
        }(std::make_index_sequence<child_count<TSeq>>{});
    }

    template<typename TSeq, size_t Num = 0uz, size_t I = 0uz>
    RUZHOUXIE_CONSTEVAL auto sequence_flatten_map()
    {
        if constexpr(indicesoid<TSeq>)
        {
            return array{ Num };
        }
        else if constexpr(I >= child_count<TSeq>)
        {
            return tuple{};
        }
        else
        {
            constexpr auto child_map = sequence_flatten_map<child_type<TSeq, I>, Num>();
            constexpr auto rest_map = sequence_flatten_map<TSeq, Num + leaf_count<decltype(child_map)>, I + 1uz>();
            return detail::concat_to_tuple(make_tuple(child_map), rest_map);
        }
    }

    RUZHOUXIE_CONSTEVAL auto sequence_zip(const auto& seq1, const auto& seq2)
    {
        return [&]<size_t...I>(std::index_sequence<I...>)
        {
            return make_tuple(make_tuple(seq1 | child<I>, seq2 | child<I>)...);
        }(std::make_index_sequence<child_count<decltype(seq1)>>{});
    }

    RUZHOUXIE_CONSTEVAL auto sequence_unzip(const auto& seq_zip)
    {
        return [&]<size_t...I>(std::index_sequence<I...>)
        {
            return make_tuple(make_tuple(seq_zip | child<I, 0uz>...), make_tuple(seq_zip | child<I, 1uz>...));
        }(std::make_index_sequence<child_count<decltype(seq_zip)>>{});
    }
    
    template<typename TArr>
    RUZHOUXIE_CONSTEVAL auto index_array_to_sequence(const TArr& arr)
    {
        return [&]<size_t...I>(std::index_sequence<I...>)
        {
            return tuple{ array{ arr[I] }... };
        }(std::make_index_sequence<std::tuple_size_v<TArr>>{});
    }
}

namespace ruzhouxie
{
    template<typename V, typename F>
    struct invoke_view : detail::view_base<V>, wrapper<F>, view_interface<invoke_view<V, F>>
    {
        template<specified<invoke_view> Self>
        RUZHOUXIE_INLINE constexpr auto&& fn_tree(this Self&& self)noexcept
        {
            return rzx::as_base<wrapper<F>>(FWD(self)).value();
        }

        template<size_t I, specified<invoke_view> Self>
        RUZHOUXIE_INLINE friend constexpr decltype(auto) tag_invoke(tag_t<child<I>>, Self&& self)
        {
            if constexpr(I >= child_count<F>)
            {
                return end();
            }      
            else if constexpr(terminal<child_type<F, I>>)
            {
                return (FWD(self).fn_tree() | child<I>)(FWD(self).base() | child<I>);
            }
            else
            {
                return invoke_view<decltype(FWD(self).base() | child<I>), decltype(FWD(self).fn_tree() | child<I>)>
                {
                    FWD(self).base() | child<I>,
                    FWD(self).fn_tree() | child<I>
                };
            }
        }

        template<auto UniqueInSeqZip>
        RUZHOUXIE_INLINE static auto get_result_vec(auto&& base_tape, auto&& fn_tape)
        {
            return [&]<size_t...I>(std::index_sequence<I...>)
            {
                constexpr auto unzip_unique_seq = detail::sequence_unzip(UniqueInSeqZip);
            
                constexpr auto base_tape_unique_seq = unzip_unique_seq.template get<0uz>();
                constexpr auto fn_tape_unique_seq = unzip_unique_seq.template get<1uz>();
                
                auto base_unique_tape = make_tape<base_tape_unique_seq>(FWD(base_tape, data));
                auto fn_unique_tape = make_tape<fn_tape_unique_seq>(FWD(fn_tape, data));
                
                return tuple<decltype(pass<I>(FWD(fn_unique_tape))(pass<I>(FWD(base_unique_tape))))...>
                {
                    pass<I>(FWD(fn_unique_tape))(pass<I>(FWD(base_unique_tape)))...
                };
            }(std::make_index_sequence<child_count<decltype(UniqueInSeqZip)>>{});
        }

        template<auto Seq, typename Self>
        struct data_type
        {
            static constexpr auto adapt_seq = detail::sequence_adapt_truncate<Seq, F>();

            static constexpr auto flat_seq = detail::sequence_flatten(adapt_seq);
            static constexpr auto flat_seq_map = detail::sequence_flatten_map<decltype(adapt_seq)>();
            
            static constexpr auto in_tape_seq = detail::mapped_layout<Seq>(flat_seq_map);

            RUZHOUXIE_INLINE static constexpr auto init_base_tape(auto&& self)
            {
                return FWD(self).base() | get_tape<flat_seq>;
            }

            RUZHOUXIE_INLINE static constexpr auto init_fn_tape(auto&& self)
            {
                return FWD(self).fn_tree() | get_tape<flat_seq>;
            }

            decltype(init_base_tape(std::declval<Self>())) base_tape;
            decltype(init_fn_tape(std::declval<Self>())) fn_tape;

            static constexpr auto base_tape_seq = purified<decltype(base_tape)>::sequence;
            static constexpr auto fn_tape_seq = purified<decltype(fn_tape)>::sequence;
 
            static constexpr auto in_tape_seq_zip = detail::sequence_zip(base_tape_seq, fn_tape_seq);
 
            static constexpr auto unique_in_seq_and_map = detail::get_unique_seq_and_map<in_tape_seq_zip>();
            static constexpr auto unique_in_seq_zip = unique_in_seq_and_map.sequence;
           
            static constexpr auto unique_in_map = detail::index_array_to_sequence(unique_in_seq_and_map.map);
     
            static constexpr auto result_vec_seq = detail::mapped_layout<in_tape_seq>(unique_in_map);

            decltype(get_result_vec<unique_in_seq_zip>(init_base_tape(std::declval<Self>()), init_fn_tape(std::declval<Self>())))
                result_vec;
            
            decltype(get_result_vec<unique_in_seq_zip>(init_base_tape(std::declval<Self>()), init_fn_tape(std::declval<Self>()))
                    | get_tape<result_vec_seq>) result_tape;

            static constexpr auto sequence = purified<decltype(result_tape)>::sequence;

            RUZHOUXIE_INLINE constexpr data_type(Self&& self)
                : base_tape(init_base_tape(FWD(self)))
                , fn_tape(init_fn_tape(FWD(self)))
                , result_vec(get_result_vec<unique_in_seq_zip>(std::move(base_tape), std::move(fn_tape)))
                , result_tape(std::move(result_vec) | get_tape<result_vec_seq>)
            {}

            template<size_t I, specified<data_type> S>
            RUZHOUXIE_INLINE friend constexpr auto tag_invoke(tag_t<child<I>>, S&& self)
            AS_EXPRESSION(getter<purified<decltype(FWD(self, result_tape, data))>>{}.template get<I>(FWD(self, result_tape, data)))
        };

        template<auto Seq, specified<invoke_view> Self> requires(not std::same_as<decltype(Seq), size_t>)
        RUZHOUXIE_INLINE friend constexpr auto tag_invoke(tag_t<get_tape<Seq>>, Self&& self)
        {
            using data_t = data_type<Seq, Self>;
            constexpr auto sequence = data_t::sequence;
            return tape_t<data_t, sequence>{ data_t{ FWD(self) } };

            // constexpr auto adapt_seq = detail::sequence_adapt_truncate<Seq, F>();
            
            // constexpr auto flat_seq = detail::sequence_flatten(adapt_seq);
            // constexpr auto flat_seq_map = detail::sequence_flatten_map<decltype(adapt_seq)>();
            // constexpr auto in_tape_seq = detail::mapped_layout<Seq>(flat_seq_map);

            // auto base_tape = FWD(self).base() | get_tape<flat_seq>;
            // auto fn_tape = FWD(self).fn_tree() | get_tape<flat_seq>;

            // constexpr auto base_tape_seq = base_tape.sequence;
            // constexpr auto fn_tape_seq = fn_tape.sequence;

            // constexpr auto in_tape_seq_zip = detail::sequence_zip(base_tape_seq, fn_tape_seq);

            // constexpr auto unique_in_seq_and_map = detail::get_unique_seq_and_map<in_tape_seq_zip>();
            // constexpr auto unique_in_seq_zip = unique_in_seq_and_map.sequence;
              
            // constexpr auto unique_in_map = detail::index_array_to_sequence(unique_in_seq_and_map.map);
            
            // constexpr auto result_vec_seq = detail::mapped_layout<in_tape_seq>(unique_in_map);

            // auto result_vec = get_result_vec<unique_in_seq_zip>(FWD(base_tape), FWD(fn_tape));

            // return result_vec | get_tape<result_vec_seq>;
        }
    };

    template<typename V, typename F>
    invoke_view(V&&, F&&) -> invoke_view<V, F>;

    constexpr inline auto invoke = [](auto&& view, auto&& fn)
    {
        return invoke_view{ FWD(view), FWD(fn) };
    };
}

#include "macro_undef.h"
#endif