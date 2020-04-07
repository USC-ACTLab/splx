#ifndef SPLX_INTERNAL_TYPES_HPP
#define SPLX_INTERNAL_TYPES_HPP

#include <Eigen/Dense>

namespace splx {

template<typename T>
using Row = Eigen::Matrix<T, 1, Eigen::Dynamic>;

template<typename T>
using Vector = Eigen::Matrix<T, Eigen::Dynamic, 1>;

template<typename T, unsigned int DIM>
using VectorDIM = Eigen::Matrix<T, DIM, 1>;

template<typename T>
using Matrix = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>;

using Index = Eigen::Index;

}

#endif