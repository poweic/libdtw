#include <fast_dtw.h>
#define __pow__(x) ((x)*(x))

// =======================================
// ===== Dynamic Time Warping in CPU =====
// =======================================

void pair_distance(const float* f1, const float* f2, size_t rows, size_t cols, size_t dim, float eta, float* pdist, distance_fn& d) {
  for (size_t x=0; x<rows; ++x)
    for (size_t y=0; y<cols; ++y)
      pdist[x * cols + y] = d(f1 + x * dim, f2 + y * dim, dim);
}

float* computePairwiseDTW(const float* data, const unsigned int* offset, int N, int dim, distance_fn& fn, float eta) {

  size_t MAX_LENGTH = 0;
  for (int i=0; i<N; ++i) {
    unsigned int length = (offset[i+1] - offset[i]) / dim;
    if ( length > MAX_LENGTH)
	MAX_LENGTH = length;
  }

  float* alpha = new float[MAX_LENGTH * MAX_LENGTH];
  float* pdist = new float[MAX_LENGTH * MAX_LENGTH];

  float* scores = new float[N * N];

  for (int i=0; i<N; ++i) {

    scores[i * N + i] = 0;
    for (int j=0; j<i; ++j) {
      size_t length1 = (offset[i + 1] - offset[i]) / dim;
      size_t length2 = (offset[j + 1] - offset[j]) / dim;

      const float *f1 = data + offset[i];
      const float *f2 = data + offset[j];

      pair_distance(f1, f2, length1, length2, dim, eta, pdist, fn);
      float s = fast_dtw(pdist, length1, length2, dim, eta, alpha);
      scores[i * N + j] = scores[j * N + i] = s;
    }
  }

  delete [] alpha;
  delete [] pdist;

  return scores;
}

inline float addlog(float x, float y) {
  const float MAX_DIFF = -708;

  if (x < y)
    std::swap(x, y);

  float diff = y - x;
  if ( diff < MAX_DIFF )
    return x;

  return x + log(1.0 + exp(diff));
}

inline float smin(float x, float y, float z, float eta) {
  return addlog(addlog(eta * x, eta * y), eta * z) / eta;
}

size_t findMaxLength(const unsigned int* offset, int N, int dim) {
  size_t MAX_LENGTH = 0;
  for (int i=0; i<N; ++i) {
    unsigned int length = (offset[i+1] - offset[i]) / dim;
    if ( length > MAX_LENGTH)
	MAX_LENGTH = length;
  }
  return MAX_LENGTH;
}

float** malloc2D(size_t m, size_t n) {
  float** p = new float*[m];
  for (size_t i=0; i<m; ++i)
    p[i] = new float[n];
  return p;
}

void free2D(float** p, size_t m) {
  assert(p != NULL);

  for (size_t i=0; i<m; ++i)
    delete p[i];
  delete [] p;
}

float fast_dtw(float* pdist, size_t rows, size_t cols, size_t dim, float eta, float* alpha, float* beta) {
  
  float distance = 0;

  bool isAlphaNull = (alpha == NULL);

  if (isAlphaNull)
    alpha = new float[rows * cols];

  // Calculate Alpha
  // x == y == 0 
  alpha[0] = pdist[0];

  // y == 0
  for (size_t x = 1; x < rows; ++x)
    alpha[x * cols] = alpha[(x-1) * cols] + pdist[x * cols];

  // x == 0
  for (size_t y = 1; y < cols; ++y)
    alpha[y] = alpha[y-1] + pdist[y];

  // interior points
  for (size_t x = 1; x < rows; ++x) {
    for (size_t y = 1; y < cols; ++y) {
      alpha[x * cols + y] = (float) smin(alpha[(x-1) * cols + y], alpha[x * cols + y-1], alpha[(x-1) * cols + y-1], eta) + pdist[x * cols + y];
    }
  }

  distance = alpha[rows * cols - 1];

  // Calculate Beta in Forward-Backward (if neccessary)
  if (beta != NULL) {
    beta[rows * cols - 1] = 0;
    size_t x, y;
    y = cols - 1;
    for (x = rows - 2; x >= 0; --x)
      beta[x * cols + y] = beta[(x+1) * cols + y] + pdist[(x+1) * cols + y];

    x = rows - 1;
    for (y = cols - 2; y >= 0; --y)
      beta[x * cols + y] = beta[x * cols + (y+1)] + pdist[x * cols + (y+1)];

    for (x = rows - 2; x >= 0; --x) {
      for (y = cols - 2; y >= 0; --y) {
	size_t p1 =  x    * cols + y + 1,
	       p2 = (x+1) * cols + y    ,
	       p3 = (x+1) * cols + y + 1;

	float s1 = beta[p1] + pdist[p1],
	      s2 = beta[p2] + pdist[p2],
	      s3 = beta[p3] + pdist[p3];

	beta[x * cols + y] = smin(s1, s2, s3, eta);
      }
    }
  }

  if (isAlphaNull) delete [] alpha;

  return distance;
}
