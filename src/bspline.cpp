#include "bspline.h"
#include <cassert>
#include <vector>
#include <iostream>
#include <eigen3/Eigen/Dense>
#include <cmath>
#include <limits>
#include <numeric>
#include <algorithm>

using std::cout;
using std::endl;
using std::cin;

splx::BSpline::BSpline(unsigned int deg, unsigned int dim, double A, double B)
                      : m_degree(deg), m_dimension(dim), m_a(A), m_b(B) {
  assert(m_a <= m_b);
}

splx::BSpline::BSpline(unsigned int deg, unsigned int dim, double A, double B,
                 const std::vector<Vec>& cpts)
                : BSpline(deg, dim, A, B) {
  for(size_t i = 0; i < cpts.size(); i++) {
    assert(cpts[i].rows() == m_dimension);
  }
  m_controlPoints = cpts;
  generateClampedUniformKnotVector();
}

void splx::BSpline::generateClampedUniformKnotVector() {
  assert(m_controlPoints.size() >= m_degree + 1);
  m_knotVector.clear();
  m_knotVector.insert(m_knotVector.begin(), m_degree+1, m_a);
  unsigned int insert_count = m_controlPoints.size() - m_degree - 1;
  double step = (m_b - m_a)/(insert_count+1);
  for(int i = 0; i < insert_count; i++)
    m_knotVector.push_back(m_a + (i+1)*step);
  m_knotVector.insert(m_knotVector.end(), m_degree+1, m_b);
}

void splx::BSpline::generateNonclampedUniformKnotVector() {
  m_knotVector.clear();
  unsigned int insert_count = m_controlPoints.size() + m_degree + 1;
  double step = (m_b - m_a) / (insert_count - 1);
  for(unsigned int i = 0; i < insert_count - 1; i++) {
    m_knotVector.push_back(m_a + i * step);
  }
  m_knotVector.push_back(m_b); // seperated for numerical reasons
}

void splx::BSpline::generateClampedNonuniformKnotVector(const std::vector<double>& w) {
  assert(m_controlPoints.size() >= m_degree + 1);
  m_knotVector.clear();
  m_knotVector.insert(m_knotVector.begin(), m_degree + 1, m_a);
  unsigned int insert_count = m_controlPoints.size() - m_degree;
  unsigned int insert_per_step = insert_count / w.size();
  double totalw = std::accumulate(w.begin(), w.end(), 0.0);
  double last_end = m_a;
  for(size_t i = 0; i < w.size() - 1; i++) {
    double ratio = w[i] / totalw;
    double cur_end = last_end + (m_b - m_a) * ratio;
    double step = (cur_end - last_end) / insert_per_step;
    for(unsigned int j = 0; j < insert_per_step; j++) {
      m_knotVector.push_back(last_end + step * (j+1));
    }
    last_end = cur_end;
  }

  /* last element is handled seperately to deal with under/overflows */
  insert_per_step = insert_count - insert_per_step * (w.size() - 1);
  double step = (m_b - last_end) / insert_per_step;
  for(unsigned int j = 0; j < insert_per_step; j++) {
    m_knotVector.push_back(last_end + step * (j+1));
  }

  m_knotVector.insert(m_knotVector.end(), m_degree, m_b);

}


void splx::BSpline::generateNonclampedNonuniformKnotVector(const std::vector<double>& w) {
  m_knotVector.clear();
  unsigned int insert_count = m_controlPoints.size() + m_degree + 1;
  double span = m_b - m_a;
  double step = span / (insert_count - 1);
  m_knotVector.push_back(m_a); // seperated for numerical issues
  for(unsigned int i = 1; i < insert_count - 1; i++) {
    m_knotVector.push_back(m_a + step * i);
  }
  m_knotVector.push_back(m_b); // seperated for numerical issues
  std::cout << "kv size: " << m_knotVector.size() << endl;
  int a; std::cin >> a;
  std::cout << "cp size: " << m_controlPoints.size() << endl;
  std::cin >> a;
}

void splx::BSpline::printKnotVector() const {
  for(size_t i = 0; i < m_knotVector.size()-1; i++) {
    std::cout << m_knotVector[i] << " ";
  }
  std::cout << m_knotVector[m_knotVector.size() - 1] << std::endl;
}

void splx::BSpline::printControlPoints() const {
  for(size_t i = 0; i < m_controlPoints.size()-1; i++) {
    std::cout << m_controlPoints[i] << " ";
  }
  std::cout << m_controlPoints[m_controlPoints.size() - 1] << std::endl;
}

void splx::BSpline::printKnotVectorNumbered() const {
  for(size_t i = 0; i < m_knotVector.size(); i++) {
    std::cout << i << " " << m_knotVector[i] << std::endl;
  }
}

unsigned int splx::BSpline::findSpan(double u) const {
  assert(u >= m_a && u <= m_b);

  if(u == m_b) {
    unsigned int idx = m_knotVector.size() - 1;
    while(m_knotVector[idx] == u) {
      idx--;
    }
    return idx;
  }
  auto it = std::upper_bound(m_knotVector.begin(), m_knotVector.end(), u);
  return it - m_knotVector.begin() - 1;
}

std::vector<double> splx::BSpline::evalBasisFuncs(double u, unsigned int deg, unsigned int k, unsigned int from, unsigned int to) const {
  //cout << "evaluating for u = " << u << endl;
  if(from > to) {
    return std::vector<double>();
  }

  if(k > deg) {
    return std::vector<double>(to - from + 1, 0.0);
  }

  std::vector<std::vector<double> > N(2);
  N[0].resize(to + deg - from + 1, 0.0);
  N[1].resize(to + deg - from + 1, 0.0);

  unsigned int end_span = findSpan(m_b);

  for(unsigned int j = from; j <= to + deg; j++) {
    N[0][j - from] = (u >= m_knotVector[j] && u < m_knotVector[j+1] ? 1.0 : 0.0);
    if(u == m_b && j == end_span) {
      N[0][j - from] = 1.0;
    }
    //cout << N[0][j - from] << endl;
  }
  //int a; cin >> a;

  for(unsigned int p = 1; p <= deg - k; p++) {
    int i = p & 0x1;
    int pi = (p - 1) & 0x1;
    for(unsigned int j = from; j <= to + deg - p; j++) {
      N[i][j-from] =
        (N[pi][j-from] == 0.0 ? 0.0 : N[pi][j-from] * (u - m_knotVector[j]) / (m_knotVector[j+p] - m_knotVector[j]))
      + (N[pi][j-from+1] == 0.0 ? 0.0 : N[pi][j-from+1] * (m_knotVector[j+p+1] - u) / (m_knotVector[j+p+1] - m_knotVector[j+1]));
      //cout << N[i][j-from] << endl;
    }
    //cin >> a;
  }

  for(unsigned int  p = deg - k + 1; p <= deg; p++) {
    int i = p & 0x1;
    int pi = (p - 1) & 0x1;
    for(unsigned int j = from; j <= to  + deg - p; j++) {
      N[i][j-from] = p * ((N[pi][j-from] == 0.0 ? 0.0 : N[pi][j-from] / (m_knotVector[j+p] - m_knotVector[j]))
                   - (N[pi][j-from+1] == 0.0 ? 0.0 : N[pi][j-from+1] / (m_knotVector[j+p+1] - m_knotVector[j+1])));
    }
  }

  //N[deg & 0x1].resize(to - from + 1);
  return N[deg & 0x1];
}


splx::Vec splx::BSpline::eval(double u, unsigned int k) const {
  assert(u >= m_a && u <= m_b);
  Vec result(m_dimension);
  for(unsigned int i = 0; i < m_dimension; i++) {
    result(i) = 0.0;
  }

  unsigned int je = findSpan(u);
  unsigned int js = je < m_degree ? 0 : je - m_degree;

  std::vector<double> N = evalBasisFuncs(u, m_degree, k, js, je);

  for(unsigned int j = js; j <= je; j++) {
    result += m_controlPoints[j] * N[j - js];
  }

  return result;
}

splx::QPMatrices splx::BSpline::getQPMatrices() const {
  QPMatrices QP;
  unsigned int S = m_controlPoints.size() * m_dimension;
  QP.H.resize(S, S);
  QP.g.resize(S);
  for(unsigned int i = 0; i < S; i++) {
    QP.g(i) = 0.0;
    for(unsigned int j = 0; j < S; j++)
      QP.H(i, j) = 0.0;
  }
  QP.A.resize(0, S);
  QP.lbA.resize(0);
  QP.ubA.resize(0);

  QP.lbX.resize(S);
  QP.ubX.resize(S);
  for(unsigned int i = 0; i < S; i++) {
    QP.lbX(i) = std::numeric_limits<double>::lowest();
    QP.ubX(i) = std::numeric_limits<double>::max();
  }

  QP.x.resize(S);
  for(unsigned int i = 0; i < m_controlPoints.size(); i++) {
    for(unsigned int d = 0; d < m_dimension; d++) {
      QP.x(d*m_controlPoints.size() + i) = m_controlPoints[i](d);
    }
  }
  return QP;
}



void splx::BSpline::extendQPIntegratedSquaredDerivative(QPMatrices& QP, unsigned int k, double lambda) const {
  if(k > m_degree)
    return;

  Matrix D(m_degree+1, m_degree+1); // get k^th derivative coefficients from a0 + a1u + ... a(m_degree)u^m_degree
  for(unsigned int m = 0; m < m_degree+1; m++) {
    for(unsigned int n = 0; n < m_degree+1; n++) {
      D(m, n) = 0.0;
    }
    if(m <= m_degree-k) {
      D(m, m+k) = perm(m+k, k);
    }
  }


  for(unsigned int j = 0; j < m_knotVector.size() - 1; j++) {
    // integrate from m_knotVector[j] to m_knotVector[j+1]
    if(m_knotVector[j] == m_knotVector[j+1])
      continue;
    unsigned int js = j < m_degree ? 0U : j - m_degree;
    Matrix M = getBasisCoefficientMatrix(js, j, m_degree, j).transpose();
    Matrix Mext(m_degree+1, m_controlPoints.size());
    for(unsigned int m = 0; m <= m_degree; m++) {
      for(unsigned int n = 0; n < m_controlPoints.size(); n++) {
        if(n >= js && n <= j) {
          Mext(m, n) = M(m, n-js);
        } else {
          Mext(m, n) = 0.0;
        }
      }
    }
    Matrix SQI(m_degree+1, m_degree+1); // get the integral of the square of the polynomial.
    for(unsigned m = 0; m <= m_degree; m++) {
      for(unsigned int n = 0; n <= m_degree; n++) {
        SQI(m, n) = 2.0 * (std::pow(m_knotVector[j+1], m+n+1) - std::pow(m_knotVector[j], m+n+1)) / (m+n+1);
      }
    }

    Matrix Hext = lambda * Mext.transpose() * D.transpose() * SQI * D * Mext;
    for(unsigned int d = 0; d < m_dimension; d++) {
      QP.H.block(d*m_controlPoints.size(), d*m_controlPoints.size(), m_controlPoints.size(), m_controlPoints.size()) += Hext;
    }
  }
}


void splx::BSpline::extendQPPositionAt(QPMatrices& QP, double u, const splx::Vec& pos, double theta) const {
  assert(u >= m_a && u <= m_b);
  assert(pos.rows() == m_dimension);
  unsigned int je = findSpan(u);
  unsigned int js = je < m_degree ? 0 : je - m_degree;

  std::vector<double> basis = evalBasisFuncs(u, m_degree, 0, js, je);
  Vec Mext(m_controlPoints.size());
  for(unsigned int i = 0; i<m_controlPoints.size(); i++) {
    if(i >= js && i<=je) {
      Mext(i) = basis[i-js];
    } else {
      Mext(i) = 0.0;
    }
  }

  Matrix Hext = 2 * theta * Mext * Mext.transpose();

  for(unsigned int d = 0; d < m_dimension; d++) {
    Vec Gext = -2 * theta * pos[d] * Mext;
    QP.g.block(d*m_controlPoints.size(), 0, m_controlPoints.size(), 1) += Gext;
    QP.H.block(d*m_controlPoints.size(), d*m_controlPoints.size(), m_controlPoints.size(), m_controlPoints.size()) += Hext;
  }
}


splx::Matrix splx::BSpline::getBasisCoefficientMatrix(unsigned int from, unsigned int to, unsigned int p, unsigned int i) const {
  Matrix result(to-from+1, p+1);
  for(unsigned int ix = 0; ix < to-from+1; ix++) {
    for(unsigned int iy = 0; iy < p+1; iy++) {
      result(ix, iy) = 0.0;
    }
  }

  if(p == 0) {
    for(unsigned int j = from; j <= to; j++) {
      result(j-from, 0) = (j == i ? 1.0 : 0.0);
    }
  } else {
    Matrix prevpower = getBasisCoefficientMatrix(from ,to+1, p-1, i);

    for(unsigned int j = from; j<=to; j++) {
      result(j-from, 0) += (m_knotVector[j+p] == m_knotVector[j] ? 0.0 :
          (-m_knotVector[j] * prevpower(j-from, 0) / (m_knotVector[j+p] - m_knotVector[j])));
      result(j-from, 0) += (m_knotVector[j+p+1] == m_knotVector[j+1] ? 0.0 :
          (m_knotVector[j+p+1] * prevpower(j-from+1, 0) / (m_knotVector[j+p+1] - m_knotVector[j+1])));

      result(j-from, p) += (m_knotVector[j+p] == m_knotVector[j] ? 0.0 :
          (prevpower(j-from, p-1) / (m_knotVector[j+p] - m_knotVector[j])));
      result(j-from, p) += (m_knotVector[j+p+1] == m_knotVector[j+1] ? 0.0 :
          (-prevpower(j-from+1, p-1) / (m_knotVector[j+p+1] - m_knotVector[j+1])));
      for(unsigned int k = 1; k < p; k++) {
        result(j-from, k) += (m_knotVector[j+p] == m_knotVector[j] ? 0.0:
                        (prevpower(j-from, k-1) / (m_knotVector[j+p] - m_knotVector[j])));
        result(j-from, k) += (m_knotVector[j+p] == m_knotVector[j] ? 0.0:
                        (-m_knotVector[j] * prevpower(j-from, k) / (m_knotVector[j+p] - m_knotVector[j])));
        result(j-from, k) += (m_knotVector[j+p+1] == m_knotVector[j+1] ? 0.0:
                        (-prevpower(j+1-from, k-1) / (m_knotVector[j+p+1] - m_knotVector[j+1])));
        result(j-from, k) += (m_knotVector[j+p+1] == m_knotVector[j+1] ? 0.0:
                        (m_knotVector[j+p+1] * prevpower(j+1-from, k) / (m_knotVector[j+p+1] - m_knotVector[j+1])));
      }
    }
  }

  return result;
}

splx::Vec splx::BSpline::eval_dbg(double u) const {
  unsigned int je = findSpan(u);
  unsigned int js = je < m_degree ? 0 : je - m_degree;
  Matrix mtr = getBasisCoefficientMatrix(js, je, m_degree, je);

  Vec uvec(m_degree+1);
  double uy = 1;
  for(unsigned int i=0; i<m_degree+1; i++) {
    uvec(i) = uy;
    uy *= u;
  }

  Vec basis = mtr * uvec;

  Vec res(m_dimension);
  for(unsigned int i=0;i<m_dimension;i++)
    res(i) = 0.0;

  for(unsigned int i = 0; i<m_degree+1; i++) {
    res += basis(i) * m_controlPoints[i+js];
  }
  return res;
}


void splx::BSpline::extendQPBeginningConstraint(QPMatrices& QP, unsigned int k, const Vec& target) const {
  assert(k <= m_degree);
  assert(target.rows() == m_dimension);
  unsigned int ridx = QP.A.rows();
  QP.A.conservativeResize(QP.A.rows() + m_dimension, QP.A.cols());
  QP.lbA.conservativeResize(QP.lbA.rows() + m_dimension);
  QP.ubA.conservativeResize(QP.ubA.rows() + m_dimension);

  unsigned int je = findSpan(m_a);
  unsigned int js = je < m_degree ? 0 : je - m_degree;

  std::vector<double> basis = evalBasisFuncs(m_a, m_degree, k, js, je);
/*
  for(int i = 0; i < basis.size(); i++) {
    cout << basis[i] << " " << endl;
  }
  int a; cin >> a;
*/
  unsigned int S = m_controlPoints.size() * m_dimension;


  for(unsigned int d = 0; d < m_dimension; d++) {
    for(unsigned int m = 0; m < S; m++) {
      if(m >= d*m_controlPoints.size()+js && m <= d*m_controlPoints.size()+je) {
        QP.A(ridx+d, m) = basis[m - d*m_controlPoints.size() - js];
      } else {
        QP.A(ridx+d, m) = 0.0;
      }
    }
    QP.lbA(ridx+d) = target(d);
    QP.ubA(ridx+d) = target(d);
  }
}

void splx::BSpline::extendQPHyperplaneConstraint(QPMatrices& QP, unsigned int from, unsigned int to, const Hyperplane& hp) const {
  assert(to < m_controlPoints.size());

  unsigned int ridx = QP.A.rows();
  QP.A.conservativeResize(QP.A.rows() + to-from+1, QP.A.cols());
  QP.lbA.conservativeResize(QP.lbA.rows() + to-from+1);
  QP.ubA.conservativeResize(QP.ubA.rows() + to-from+1);


  unsigned int S = m_dimension * m_controlPoints.size();

  for(unsigned int i = from; i<=to; i++) {
    for(unsigned int s = 0; s < S; s++) {
      QP.A(ridx + i - from, s) = 0.0;
    }
    for(unsigned int d = 0; d < m_dimension; d++) {
      QP.A(ridx + i - from, d*m_controlPoints.size() + i) = hp.normal()(d);
    }
    QP.lbA(ridx + i - from) = std::numeric_limits<double>::lowest();
    QP.ubA(ridx + i - from) = hp.offset();
  }
}

void splx::BSpline::extendQPHyperplaneConstraint(QPMatrices& QP, double from, double to, const Hyperplane& hp) const {
  assert(hp.normal().rows() == m_dimension);
  assert(from >= m_a && from <= m_b);
  assert(to >= m_a && to <= m_b);

  auto aff = affectingPoints(from, to);

  extendQPHyperplaneConstraint(QP, aff.first, aff.second, hp);
}

std::pair<unsigned int, unsigned int> splx::BSpline::affectingPoints(double from, double to) const {
  assert(to >= from && to <= m_b && from >= m_a);
  unsigned int js = findSpan(from);
  unsigned int je = findSpan(to);

  js = js < m_degree ? 0 : js - m_degree;

  return std::make_pair(js, je);
}

const splx::Vec& splx::BSpline::getCP(unsigned int k) const {
  return m_controlPoints[k];
}


std::pair<unsigned int, unsigned int> splx::BSpline::interpolateEndAtTo(const Vec& from, const Vec& to, unsigned int n) {
  assert(n >= 2);
  std::pair<unsigned int, unsigned int> result;
  result.first = m_controlPoints.size();
  Vec step = (to - from) / (n-1);
  for(unsigned int i = 0; i < n; i++) {
    m_controlPoints.push_back(from + step * i);
  }

  for(unsigned int i = 0; i < m_degree; i++) {
    m_controlPoints.push_back(to);
  }
  result.second = m_controlPoints.size() - 1;
  return result;
}

void splx::BSpline::loadControlPoints(const QPMatrices& QP) {
  for(unsigned int i = 0; i < m_controlPoints.size(); i++) {
    for(unsigned int d = 0; d < m_dimension; d++) {
      m_controlPoints[i](d) = QP.x(d * m_controlPoints.size() + i);
    }
  }
}

void splx::BSpline::clearControlPoints() {
  m_controlPoints.clear();
}

void splx::BSpline::extendQPDecisionConstraint(QPMatrices& QP, double lb, double ub) const {
  for(unsigned int i=0; i < QP.lbX.rows(); i++) {
    QP.lbX(i) = lb;
    QP.ubX(i) = ub;
  }
}

void splx::BSpline::extendQPHyperplanePenalty(QPMatrices& QP, unsigned int from, unsigned int to, const Hyperplane& hp, double alpha) const {
  assert(hp.normal().rows() == m_dimension);
  assert(to >= from);
  assert(to < m_controlPoints.size());
  const Vec& normal = hp.normal();
  const double dist = hp.offset();
  for(unsigned int i = from; i <= to; i++) {
    for(unsigned int d = 0; d < m_dimension; d++) {
      QP.g(d * m_controlPoints.size() + i) += alpha * normal(d);
    }
  }
}
