/*
 * Copyright (C) 1998-2017 ALPS Collaboration. See COPYRIGHT.TXT
 * All rights reserved. Use is subject to license terms. See LICENSE.TXT
 * For use in publications, see ACKNOWLEDGE.TXT
 */
#ifndef ALPS_TRANSFORM_COMMON_HPP
#define ALPS_TRANSFORM_COMMON_HPP

#include <vector>
#include <complex>
#include <stdexcept>

namespace alps { namespace transform {

/**
 * Common traits struct.
 *
 * You should specialize this template for every transformer in such a way:
 *
 *     template <> struct traits<my_transformer>
 *     {
 *         typedef double in_type;                  // input type
 *         typedef double out_type;                 // output type
 *     };
 */
template <typename T>
struct traits;

/**
 * Convenience function that transforms vector
 */
template <typename T>
std::vector<typename traits<T>::out_type> apply(
                    T &tf, const std::vector<typename traits<T>::in_type> &in)
{
    if (tf.in_size() != in.size())
        throw std::runtime_error("size mismatch");

    std::vector<typename traits<T>::out_type> out(tf.out_size(), 0.0);
    tf(&in[0], &out[0]);
    return out;
}

/** Enumeration for different signal periodicities */
enum statistics
{
    bosonic = 0,
    fermionic = 1
};

}}

#endif
