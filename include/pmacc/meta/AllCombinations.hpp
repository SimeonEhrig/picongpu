/* Copyright 2014-2021 Rene Widera, Benjamin Worpitz
 *
 * This file is part of PMacc.
 *
 * PMacc is free software: you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PMacc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License and the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with PMacc.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "pmacc/math/Vector.hpp"
#include "pmacc/meta/conversion/MakeSeq.hpp"
#include "pmacc/meta/conversion/MakeSeqFromNestedSeq.hpp"
#include "pmacc/meta/conversion/SeqToMap.hpp"
#include "pmacc/meta/conversion/TypeToAliasPair.hpp"
#include "pmacc/meta/conversion/TypeToPair.hpp"
#include "pmacc/types.hpp"

#include <boost/mpl/at.hpp>
#include <boost/mpl/copy.hpp>
#include <boost/mpl/empty.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/pop_back.hpp>

namespace pmacc
{
    namespace bmpl = boost::mpl;

    namespace detail
    {
        /** Create tuples out of the elements of N sequences
         *
         * Combines all elements of N given sequences in T_MplSeq into N-tuples.
         * If the number of elements in each sequence is S0, S1, ... S(N-1)
         * than the resulting sequence will contain S0 * S1 * ... S(N-1) tuples.
         *
         * @tparam T_MplSeq sequence of input sequences
         * @tparam T_TmpResult temporary result
         * @tparam T_isEmpty true if T_MplSeq is empty else false
         */
        template<
            typename T_MplSeq,
            typename T_TmpResult = bmpl::vector0<>,
            bool T_isEmpty = bmpl::empty<T_MplSeq>::value>
        struct AllCombinations;

        /** implementation for inner recursive creation
         */
        template<typename T_MplSeq, typename T_TmpResult>
        struct AllCombinations<T_MplSeq, T_TmpResult, false>
        {
            using MplSeq = T_MplSeq;
            using TmpResult = T_TmpResult;

            static constexpr uint32_t rangeVectorSize = bmpl::size<MplSeq>::value;
            using LastElement = typename bmpl::at<MplSeq, bmpl::integral_c<uint32_t, rangeVectorSize - 1>>::type;
            using IsLastElementEmpty = bmpl::empty<LastElement>;
            using LastElementAsSequence = typename MakeSeq<LastElement>::type;
            using ShrinkedRangeVector = typename bmpl::pop_back<MplSeq>::type;

            /* copy last given sequence to a mpl::vector to be sure that we can later on
             * call mpl::transform even if the input sequence is mpl::range_c
             */
            using TmpVector = typename bmpl::copy<LastElementAsSequence, bmpl::back_inserter<bmpl::vector0<>>>::type;

            /** Assign to each element in a sequence of CT::Vector(s) a type at a given
             *  component position
             *
             * @tparam T_ComponentPos position of the component to be changed (type must be
             * bmpl::integral_c<uint32_t,X>)
             * @tparam T_Element value (type) which should replace the component at position T_Component
             *                   in the CT::Vector elements
             */
            template<typename T_ComponentPos, typename T_Element>
            struct AssignToAnyElementInVector
            {
                using InVector = TmpResult;
                using Element = T_Element;

                using type = typename bmpl::
                    transform<InVector, pmacc::math::CT::Assign<bmpl::_1, T_ComponentPos, Element>>::type;
            };

            using NestedSeq = typename bmpl::transform<
                TmpVector,
                AssignToAnyElementInVector<bmpl::integral_c<uint32_t, rangeVectorSize - 1>, bmpl::_1>>::type;

            using OneSeq = typename MakeSeqFromNestedSeq<NestedSeq>::type;

            using ResultIfNotEmpty = typename detail::AllCombinations<ShrinkedRangeVector, OneSeq>::type;
            using type = typename bmpl::if_<IsLastElementEmpty, bmpl::vector0<>, ResultIfNotEmpty>::type;
        };

        /** recursive end implementation
         */
        template<typename T_MplSeq, typename T_TmpResult>
        struct AllCombinations<T_MplSeq, T_TmpResult, true>
        {
            using type = T_TmpResult;
        };

    } // namespace detail


    /** Create tuples out of the elements of N sequences
     *
     * Combines all elements of N given sequences in T_MplSeq into N-tuples.
     * If the number of elements in each sequence is S0, S1, ... S(N-1)
     * than the resulting sequence will contain S0 * S1 * ... S(N-1) tuples.
     *
     * example:
     *
     * sequence  == [ ]
     * tuple     == ( )
     *
     * T_MplSeq = [[1,2],[1],[4,3]]
     * combined to
     * AllCombinations<T_MplSeq>::type = [(1,1,4),(1,1,3),(2,1,4),(2,1,3)]
     *
     * @tparam T_MplSeq N-dimensional sequence with input values
     *                  or single type (e.g. `bmpl::integral_c<uint32_t,5>`)
     *                  (if `T_MplSeq` is only one type it will be transformed to a sequence)
     * @typedef AllCombinations<T_MplSeq>::type
     *          MplSequence of N-tuples
     */
    template<typename T_MplSeq>
    struct AllCombinations
    {
        /* if T_MplSeq is no sequence it is a single type, we put this type in
         * a sequence because all next algorithms can only work with sequences */
        using MplSeq = typename MakeSeq<T_MplSeq>::type;

        static constexpr uint32_t rangeVectorSize = bmpl::size<MplSeq>::value;
        using LastElement = typename bmpl::at<MplSeq, bmpl::integral_c<uint32_t, rangeVectorSize - 1>>::type;
        using IsLastElementEmpty = bmpl::empty<LastElement>;
        using LastElementAsSequence = typename MakeSeq<LastElement>::type;

        using ShrinkedRangeVector = typename bmpl::pop_back<MplSeq>::type;
        /* copy last given sequence to a mpl::vector to be sure that we can later on
         * call mpl::transform even if the input sequence is mpl::range_c
         */
        using TmpVector = typename bmpl::copy<LastElementAsSequence, bmpl::back_inserter<bmpl::vector0<>>>::type;


        /* transform all elements in the vector to math::CT::vector<> */
        using EmptyVector = math::CT::Vector<>;
        using FirstList = typename bmpl::transform<
            TmpVector,
            pmacc::math::CT::Assign<EmptyVector, bmpl::integral_c<uint32_t, rangeVectorSize - 1>, bmpl::_1>>::type;

        /* result type: MplSequence of N-tuples */
        using ResultIfNotEmpty = typename detail::AllCombinations<ShrinkedRangeVector, FirstList>::type;
        using type = typename bmpl::if_<IsLastElementEmpty, bmpl::vector0<>, ResultIfNotEmpty>::type;
    };


} // namespace pmacc
