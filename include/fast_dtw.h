#ifndef __FAST_DTW_H_
#define __FAST_DTW_H_

#include <string>
#include <cmath>
#include <vector>
#include <cassert>
#include <fstream>
using namespace std;

class distance_fn {
public:
  virtual float operator() (const float* x, const float* y, size_t dim) = 0;
};

class euclidean_fn : public distance_fn {
  public:
    virtual float operator() (const float* x, const float* y, size_t dim) {
      float d = 0;
      for (size_t i=0; i<dim; ++i)
	d += pow(x[i] - y[i], 2.0);
      return sqrt(d);
    }
};

class mahalanobis_fn : public distance_fn {
public:

  mahalanobis_fn(size_t dim): _diag(NULL), _dim(dim) {
    _diag = new float[_dim];
    for (size_t i=0; i<_dim; ++i)
      _diag[i] = 1;
  }

  virtual float operator() (const float* x, const float* y, size_t dim) {
    float d = 0;
    for (size_t i=0; i<dim; ++i)
      d += pow(x[i] - y[i], 2.0) * _diag[i];

    return sqrt(d); 
  }

  virtual void setDiag(string filename) {
    if (filename.empty())
      return;

    ifstream f(filename.c_str());
    vector<float> diag;
    float c;
    while (f >> c)
      diag.push_back(c);
    f.close();

    this->setDiag(diag);
  }

  virtual void setDiag(const vector<float>& diag) {
    assert(_dim == diag.size());
    if (_diag != NULL)
      delete [] _diag;

    _diag = new float[_dim];
    for (size_t i=0; i<_dim; ++i)
      _diag[i] = diag[i];
  }

  float* _diag;
  size_t _dim;

private:
  mahalanobis_fn();
};

class log_inner_product_fn : public mahalanobis_fn {
public:

  log_inner_product_fn(size_t dim): mahalanobis_fn(dim) {}

  virtual float operator() (const float* x, const float* y, size_t dim) {
    float d = 0;
    for (size_t i=0; i<dim; ++i)
      d += x[i] * y[i] * _diag[i];
    return -log(d);
  }
};

// =======================================
// ===== Dynamic Time Warping in CPU =====
// =======================================
inline float addlog(float x, float y);
inline float smin(float x, float y, float z, float eta);
size_t findMaxLength(const unsigned int* offset, int N, int dim);

float fast_dtw(
    float* pdist,
    size_t rows, size_t cols, size_t dim,
    float eta, 
    float* alpha = NULL,
    float* beta = NULL);

float* computePairwiseDTW(const float* data, const unsigned int* offset, int N, int dim, distance_fn& fn, float eta);

void pair_distance(const float* f1, const float* f2, size_t rows, size_t cols, size_t dim, float eta, float* pdist, distance_fn& fn);

void free2D(float** p, size_t m);
float** malloc2D(size_t m, size_t n);

#endif // __FAST_DTW_H_
