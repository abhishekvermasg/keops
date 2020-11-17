#pragma once

#include <assert.h>
#include <iostream>
#include <sstream>
#include <utility>
#include <array>
#include <algorithm>

#include "core/utils/keops_math.h"
#include "core/utils/TypesUtils.h"

#include "core/autodiff/BinaryOp.h"
#include "core/pre_headers.h"

#include "core/formulas/maths/Mult.h"
#include "core/formulas/maths/Exp.h"

namespace keops {

    namespace loop_impl {

        template < typename, size_t... I >
        struct Looper_arr;

        template < size_t... Is >
        struct Looper_arr< std::index_sequence< Is...>> {
            template < typename Func >
            constexpr static DEVICE void f(Func &&func) {
                func(std::index_sequence< Is... >{});
            }
        };

        template < size_t I, size_t... Is, size_t... PIs >
        struct Looper_arr< std::index_sequence< PIs... >, I, Is... > {
            template < size_t... Idx, typename Func >
            constexpr static DEVICE void f_help(std::index_sequence< Idx... >, Func &&func) {
                (void) std::initializer_list< int >{(Looper_arr< std::index_sequence< PIs..., Idx >, Is... >::f(func), 0)...};
            }

            template < typename Func >
            constexpr static DEVICE void f(Func &&func) {
                f_help(std::make_index_sequence< I >{}, func);
            }

        };

        template < typename >
        struct loop_t_arr;

        template < size_t... Is >
        struct loop_t_arr< const std::index_sequence< Is...>> {
            using type = Looper_arr< std::index_sequence<>, Is... >;
        };

    }

    template < typename Is >
    using loop_arr = typename loop_impl::loop_t_arr< Is >::type;

    // Constexpr Array manipulation

    template <auto& Arr, size_t... Is>
    static constexpr auto make_seq_impl(std::index_sequence<Is...>) {
        using T = typename std::decay_t<decltype(Arr)>::value_type;
        return std::integer_sequence<T, Arr[Is]...>{};
    }

    template <auto& Arr>
    static constexpr auto make_seq() {
        return make_seq_impl<Arr>(std::make_index_sequence<Arr.size()>());
    }

    template<std::size_t... I>
    DEVICE static constexpr auto make_array(std::index_sequence<I...>) {
        return std::array<size_t, sizeof...(I)>{ {I...} };
    }

    template<std::size_t N>
    static constexpr size_t prod_array(std::array<size_t, N> arr) {
        size_t res = 1;
        for (size_t i=0; i < N; i++){
            res *= arr[i];
        }
        return res;
    }

    template<std::size_t N>
    static constexpr auto cumprod_array(std::array<size_t, N> arr) {
        std::array<size_t, N> res{};
        res[N-1] = 1;
        if (N > 1) {
            for (int i = N - 2; i > -1; i--)
                res[i] = res[i + 1] * arr[i + 1];
        }
        return res;
    }

    template<std::size_t N>
    HOST_DEVICE static constexpr auto reverse_array(std::array<size_t, N> arr) {
        std::array<size_t, N> res{};
        for (size_t i=0; i < N; i++){
            res[i] = arr[N-i-1];
        }
        return res;
    }

    template<std::size_t N, size_t M>
    static constexpr auto concat_array(std::array<size_t, N> arr0, std::array<size_t, M> arr1) {
        std::array<size_t, N+M> res{};
        //std::copy(arr0.begin(), arr0.end(), res.begin());
        //std::copy(arr1.begin(), arr1.end(), res.begin()+N);
        for (size_t i=0; i<N; i++){
            res[i] = arr0[i];
        }
        for (size_t i=0; i<M; i++){
            res[i+N] = arr1[i];
        }
        return res;
    }

    template<std::size_t N, size_t M>
    HOST_DEVICE static constexpr auto map_array(std::array<size_t, N> ind, std::array<size_t, M> arr1) {
        std::array<size_t, N> res{};
        for (size_t i= 0; i < N; i++){
            res[i] = arr1[ind[i]];
        }
        return res;
    }

    template<std::size_t N>
    HOST_DEVICE static constexpr auto permutate_array(std::array<size_t, N> perm, std::array<size_t, N> arr) {
        std::array<size_t, N> res{};
        for (size_t i= 0; i < N; i++){
            res[perm[i]] = arr[i];
        }
        return res;
    }

    template<std::size_t B, size_t E>
    constexpr auto make_range_array() {
        static_assert(E >= B, "please provide E >= B");
        std::array<size_t, E - B> res{};
        for (size_t i= B; i < E; i++){
            res[i - B] = i;
        }
        return res;
    }

    template< std::size_t N, std::size_t M >
    static constexpr auto diff_array(std::array< size_t, M > ind) {
        auto res = make_range_array<0, N - M >();
        size_t l = 0;
        for (size_t i= 0; i < N; i++){
            bool include = false;
            for (size_t j=0; j < M; j++) {
                if (ind[j] == i)
                    include = true;
            }
            if (!include) {
                res[l] = i;
                l++;
            }
        }
        return res;
    }


    template<std::size_t N>
    HOST_DEVICE constexpr size_t sum_multiplies_array(const std::array<size_t, N> arr0, const std::array<size_t, N>  arr1) {
        size_t res = 0;
        for (size_t i= 0; i < N; i++){
            res += arr0[i] * arr1[i];
        }
        return res;
    }

    template<std::size_t N, size_t M>
    HOST_DEVICE constexpr auto first_array(const std::array<size_t, M> arr) {
        std::array<size_t, N> res;
        for (size_t i= 0; i < N; i++){
            res[i] = arr[i];
        }
        return res;
    }

    namespace cstd {

        template <typename RAIt>
        constexpr RAIt next(RAIt it, typename std::iterator_traits<RAIt>::difference_type n = 1) {
            return it + n;
        }

        template <typename RAIt>
        constexpr auto distance(RAIt first, RAIt last) {
            return last - first;
        }

        template<class ForwardIt1, class ForwardIt2>
        constexpr void iter_swap(ForwardIt1 a, ForwardIt2 b) {
            auto temp = std::move(*a);
            *a = std::move(*b);
            *b = std::move(temp);
        }

        template<class InputIt, class UnaryPredicate>
        constexpr InputIt find_if_not(InputIt first, InputIt last, UnaryPredicate q) {
            for (; first != last; ++first) {
                if (!q(*first)) {
                    return first;
                }
            }
            return last;
        }

        template<class ForwardIt, class UnaryPredicate>
        constexpr ForwardIt partition(ForwardIt first, ForwardIt last, UnaryPredicate p) {
            first = cstd::find_if_not(first, last, p);
            if (first == last) return first;

            for(ForwardIt i = cstd::next(first); i != last; ++i){
                if(p(*i)){
                    cstd::iter_swap(i, first);
                    ++first;
                }
            }
            return first;
        }

    }

    template<class RAIt, class Compare = std::less<>>
    constexpr void quick_sort(RAIt first, RAIt last, Compare cmp = Compare{}) {
        auto const N = cstd::distance(first, last);
        if (N <= 1) return;
        auto const pivot = *cstd::next(first, N / 2);
        auto const middle1 = cstd::partition(first, last, [=](auto const& elem) { return cmp(elem, pivot); });
        auto const middle2 = cstd::partition(middle1, last, [=](auto const& elem){ return !cmp(pivot, elem); });
        quick_sort(first, middle1, cmp); // assert(std::is_sorted(first, middle1, cmp));
        quick_sort(middle2, last, cmp);  // assert(std::is_sorted(middle2, last, cmp));
    }

    template <size_t N>
    constexpr auto sort_indexes_array(const std::array< size_t, N > arr) {

        // initialize original index locations
        auto idx = make_range_array<0, N>();
        quick_sort(idx.begin(), idx.end(),
                   [&arr](size_t i1, size_t i2) {return arr[i1] < arr[i2];});

        return idx;
    }




    // Dummy class that stores the indices computes for tensordot
    struct tensordot_indices_arr {
        size_t out_indices;
        size_t a_indices;
        size_t b_indices;
    };


    template < class DIMFA, class DIMFB, class CONTFA, class CONTFB, class PERMUTE >
    struct tensordot_parameters_arr{

        // Left hand-side
        static constexpr auto contfa_arr = make_array(CONTFA{});
        static constexpr auto dimfa_arr = make_array(DIMFA{});

        static constexpr std::array<size_t, DIMFA::size() - CONTFA::size() > indices_keepdim_a = diff_array< DIMFA::size() >(contfa_arr); // tao::seq::make_index_sequence< tao::seq::impl::sequence_size< DIMFA >::value >, CONTFA >;
        static constexpr std::array<size_t, DIMFA::size() - CONTFA::size() > keepdim_a = map_array(indices_keepdim_a, dimfa_arr);
        static constexpr std::array<size_t, CONTFA::size() > contdim_a = map_array(contfa_arr, dimfa_arr);

#if C_CONTIGUOUS
        static constexpr std::array<size_t, DIMFA::size()> list_stride_dim_a = cumprod_array(dimfa_arr);
#else
        static constexpr std::array<size_t, DIMFA::size()> list_stride_dim_a = cumprod_array(reverse_array(dimfa_arr));
#endif

        // Right hand-side
        static constexpr auto contfb_arr = make_array(CONTFB{});
        static constexpr auto dimfb_arr = make_array(DIMFB{});

        static constexpr std::array<size_t, DIMFB::size() - CONTFB::size() > indices_keepdim_b = diff_array< DIMFB::size() >(contfb_arr); // tao::seq::make_index_sequence< tao::seq::impl::sequence_size< DIMFA >{} >, CONTFA >;
        static constexpr std::array<size_t, DIMFB::size() - CONTFB::size() > keepdim_b = map_array(indices_keepdim_b, dimfb_arr);
        static constexpr std::array<size_t, CONTFB::size() > contdim_b = map_array(contfb_arr, dimfb_arr);

#if C_CONTIGUOUS
        static constexpr std::array<size_t, DIMFB::size()> list_stride_dim_b = cumprod_array(dimfb_arr);
#else
        static constexpr std::array<size_t, DIMFB::size()> list_stride_dim_b = cumprod_array(reverse_array(dimfb_arr));
#endif

        //static_assert(contdim_a == contdim_b, "In TensorDotNoTao: contracting dimensions should be the same"); // TODO

        // Output
        static constexpr size_t keepdim_size = DIMFA::size() - CONTFA::size() + DIMFB::size() - CONTFB::size();
        static constexpr  std::array<size_t, keepdim_size> permute_arr = make_array(PERMUTE{});
        static constexpr std::array<size_t, keepdim_size > keepdim = concat_array(keepdim_a, keepdim_b);
#if C_CONTIGUOUS
        static constexpr std::array<size_t, keepdim_size > list_stride_keepdim = cumprod_array(permutate_array(permute_arr, keepdim);
#else
        static constexpr std::array<size_t, keepdim_size > list_stride_keepdim = cumprod_array( reverse_array(permutate_array(permute_arr, keepdim))) ;
#endif
        static constexpr size_t dimout = prod_array(keepdim);

//    static_assert(permutate_array(permute_arr, permute_arr) == make_range_array<0, keepdim_size >(), "In TensorDotNoTao: PERMUTE should be a permutation index_sequence.");  //TODO

        // Loop: in this code we choose to loop on the keepdims first and then on the contraction dims.
        static constexpr std::array<size_t, keepdim_size + CONTFA::size() > loopdim = concat_array(keepdim, contdim_a);

        constexpr static size_t dimloop = prod_array(loopdim);
        constexpr static size_t number_of_dimloop = DIMFA::size() - CONTFA::size() + DIMFB::size();

        static constexpr std::array<size_t, DIMFA::size() > ala =
                concat_array(make_range_array<0, keepdim_a.size() >(), make_range_array< keepdim.size(), number_of_dimloop>());
        static constexpr std::array<size_t, DIMFA::size() > ali = concat_array(indices_keepdim_a, contfa_arr);
        static constexpr std::array<size_t, DIMFA::size() > list_indices_a_intot = permutate_array(ali, ala);

        static constexpr std::array<size_t, DIMFB::size() > bla =
                concat_array(make_range_array< keepdim_a.size(), keepdim.size()>(), make_range_array< keepdim.size(), number_of_dimloop>());
        static constexpr std::array<size_t, DIMFB::size() > bli = concat_array(indices_keepdim_b, contfb_arr);
        static constexpr std::array<size_t, DIMFB::size() > list_indices_b_intot = permutate_array(bli, bla);

        // used to compute the Gradient
        static constexpr std::array<size_t, DIMFA::size() - CONTFA::size() > list_indices_keepdim_a_inout = make_range_array< 0, keepdim_a.size() >();
        static constexpr std::array<size_t, CONTFA::size() > reordered_contfa = permutate_array(sort_indexes_array(contfb_arr), contfa_arr);
        static constexpr std::array<size_t, DIMFA::size() - CONTFA::size() > reordered_keepdim_a =
                permutate_array(sort_indexes_array(map_array(list_indices_keepdim_a_inout, permute_arr)), indices_keepdim_a );
        static constexpr std::array<size_t, DIMFA::size() > moveaxis_a = concat_array(reordered_keepdim_a, reordered_contfa);

        static constexpr std::array<size_t, DIMFB::size() - CONTFB::size() >  list_indices_keepdim_b_inout = make_range_array< keepdim_a.size(), keepdim.size() >();
        static constexpr std::array<size_t, CONTFB::size() >  reordered_contfb = permutate_array(sort_indexes_array(contfa_arr), contfb_arr);
        static constexpr std::array<size_t, DIMFB::size() - CONTFB::size() >  reordered_keepdim_b =
                permutate_array(sort_indexes_array(map_array(list_indices_keepdim_b_inout, permute_arr)), indices_keepdim_b);
        static constexpr std::array<size_t, DIMFB::size() > moveaxis_b = concat_array(reordered_keepdim_b, reordered_contfb);

        ////////////////////////////////////
        static constexpr std::array< size_t, keepdim_size>  dimfa_grad = permutate_array(permute_arr, keepdim);
        static constexpr auto DIMFA_GRAD_imp = make_seq<dimfa_grad>();
        using DIMFA_GRAD = decltype(DIMFA_GRAD_imp);

        static constexpr std::array< size_t, DIMFB::size() - CONTFB::size() > contfa_grad=map_array(list_indices_keepdim_b_inout, permute_arr);
        static constexpr auto CONTFA_GRAD_imp = make_seq<contfa_grad>();
        using CONTFA_GRAD = decltype(CONTFA_GRAD_imp);

        static constexpr std::array< size_t, DIMFA::size() - CONTFA::size() > contfb_grad=map_array(list_indices_keepdim_a_inout, permute_arr);
        static constexpr auto CONTFB_GRAD_imp = make_seq<contfb_grad>();
        using CONTFB_GRAD = decltype(CONTFB_GRAD_imp);

        static constexpr auto loopdim_imp = make_seq<loopdim>();
        using loopdim_t = decltype(loopdim_imp);

        static constexpr auto indices_keepdim_b_impl = make_seq<indices_keepdim_b>();
        using indices_keepdim_b_t = decltype(indices_keepdim_b_impl);

        static constexpr auto indices_keepdim_a_impl = make_seq<indices_keepdim_a>();
        using indices_keepdim_a_t = decltype(indices_keepdim_a_impl);

        static constexpr auto moveaxis_a_impl = make_seq<moveaxis_a>();
        using moveaxis_a_t = decltype(moveaxis_a_impl);

        static constexpr auto moveaxis_b_impl = make_seq<moveaxis_b>();
        using moveaxis_b_t = decltype(moveaxis_b_impl);

        /////////////////////////////////////
        static constexpr size_t DIMFA_SIZE =  DIMFA::size();
        static constexpr size_t DIMFB_SIZE =  DIMFB::size();

        template < class IND >
        DEVICE constexpr static tensordot_indices_arr compute_tensordot_indices_arr(IND) {
            auto ind_arr = make_array(IND{});

            // a_indices
            std::array<size_t, DIMFA_SIZE > list_indices_a = map_array(list_indices_a_intot, ind_arr);
#if C_CONTIGUOUS
            size_t a_indices = sum_multiplies_array(list_stride_dim_a, list_indices_a);
#else
            size_t a_indices = sum_multiplies_array(list_stride_dim_a, reverse_array(list_indices_a));
#endif

            // b_indices
            std::array<size_t, DIMFB_SIZE >  list_indices_b  = map_array(list_indices_b_intot, ind_arr);
#if C_CONTIGUOUS
            size_t b_indices = sum_multiplies_array(list_stride_dim_b, list_indices_b);
#else
            size_t b_indices = sum_multiplies_array(list_stride_dim_b, reverse_array(list_indices_b));
#endif

            // out_indices
            std::array<size_t, keepdim_size >  list_indices_keepdim = permutate_array(permute_arr, first_array< keepdim_size >(ind_arr ));
#if C_CONTIGUOUS
            size_t out_indices = sum_multiplies_array(list_stride_keepdim, list_indices_keepdim);
#else
            size_t out_indices = sum_multiplies_array(list_stride_keepdim, reverse_array(list_indices_keepdim));
#endif

            return tensordot_indices_arr{out_indices, a_indices, b_indices};
        }

        template < typename Func >
        struct compute_tensordot_indices_t_arr {
            template < size_t... Is >
            DEVICE void operator()(std::index_sequence< Is... > x) {
                _f(compute_tensordot_indices_arr(x));
            }

            Func &_f;
            DEVICE compute_tensordot_indices_t_arr(Func &&f) : _f(f) {}
        };

        template < typename Func >
        static DEVICE auto compute_tensordot_indices_apply_arr(Func &&f) {
            return compute_tensordot_indices_t_arr< Func >(std::forward< Func >(f));
        }

    };

/////////////////////////////////////////////////////////////////////////
////              Tensor dot product      A : b                      ////
/////////////////////////////////////////////////////////////////////////


    template < class A, class B, class DIMFA, class DIMFB, class CONTFA, class CONTFB, class PERMUTEDIMOUT=std::make_index_sequence<
            DIMFA::size() + DIMFB::size() - 2 * CONTFA::size()>>
    struct TensorDotNoTao : BinaryOp< TensorDotNoTao, A, B, DIMFA, DIMFB, CONTFA, CONTFB, PERMUTEDIMOUT > {
        // A is vector of size p ** n, interpreted as matrix (column major), B is vector of size p ** m, interpreted as column vector
        // n=3 and m=2 are assume to be known
        // output is vector of size n

        static_assert(DIMFA::size() > 0, "Please provide a non empty DIMA");
        static_assert(DIMFB::size() > 0, "Please provide a non empty DIMB");
        //static_assert(tao::seq::prod< DIMFA >::value == A::DIM, "DIMA is not consistant with dimension of A");
        //static_assert(tao::seq::prod< DIMFB >::value == B::DIM, "DIMB is not consistant with dimension of B");

        //static_assert(::std::is_same< tao::seq::permutate_t< PERMUTEDIMOUT, PERMUTEDIMOUT >, tao::seq::make_index_range< 0, keepdim_t::size()>>::value,
        //                "In TensorDotNoTao: PERMUTE should be a permutation index_sequence.");

        using parameters = tensordot_parameters_arr< DIMFA, DIMFB, CONTFA, CONTFB, PERMUTEDIMOUT >;

        static const int DIM = parameters::dimout;

        static void PrintIdString(std::stringstream &str) { str << ":"; }

        template < typename TYPE >
        static DEVICE INLINE void Operation(TYPE *out, TYPE *inA, TYPE *inB) {
#pragma unroll
            for (int i = 0; i < DIM; i++)
                out[i] = cast_to<TYPE>(0.0f);
            loop_arr< typename parameters::loopdim_t >::f(parameters::compute_tensordot_indices_apply_arr([&out, &inA, &inB](
                    tensordot_indices_arr td) {
                out[td.out_indices] = keops_fma(inA[td.a_indices], inB[td.b_indices], out[td.out_indices]);
            }));
        }

        template < class V, class GRADIN >
        using DiffTA = typename A::template DiffT< V, GRADIN >;

        template < class V, class GRADIN >
        using DiffTB = typename B::template DiffT< V, GRADIN >;

  template < class V, class GRADIN >
  using DiffT = Add<
      DiffTA< V, TensorDotNoTao< GRADIN, B,
                            typename parameters::DIMFA_GRAD,                // @fradav : c'est ca qui merdoit.. Ca marche si on remplace par std::integer_sequence<int,3>
                            DIMFB,                                          // 4 2
                            typename parameters::CONTFA_GRAD,               // .
                            typename parameters::indices_keepdim_b_t,       // .
                            typename parameters::moveaxis_a_t>>,              // 1, 2 0
      DiffTB< V, TensorDotNoTao< GRADIN, A,
                            typename parameters::DIMFA_GRAD,                // @fradav : c'est ca qui merdoit.. Ca marche si on remplace par std::integer_sequence<int,3>
                            DIMFA,                                          // 2, 3, 4
                            typename parameters::CONTFB_GRAD,               //0
                            typename parameters::indices_keepdim_a_t,       //1
                            typename parameters::moveaxis_b_t>>               // . , 0 1
  >;

    };

#define TensorDotNoTao(f, g, ...) KeopsNS<TensorDotNoTao<decltype(InvKeopsNS(f)),decltype(InvKeopsNS(g)), __VA_ARGS__>>()

}
