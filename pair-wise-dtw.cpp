#include <iostream>
#include <fstream>
#include <cstring>
#include <cmdparser.h>

#include <fast_dtw.h>
using namespace std;

float calcError(float* s1, float* s2, int N);
distance_fn* initDistanceMeasure(string dist_type, size_t dim, string theta_fn);
// void normalize(float* m, int N, float eta);
// void normalize_in_log(float* m, int N);
void cvtDistanceToSimilarity(float* m, int N);
void print(FILE* fid, float* m, int N);
void loadKaldi(string filename, float* &dataPtr, unsigned int* &offset, int& N, int& dim);

int main (int argc, char* argv[]) {

  CmdParser cmdParser(argc, argv);
  cmdParser
    .add("--ark", "input feature archive")
    .add("-o", "output filename for the acoustic similarity matrix", false);

  cmdParser
    .addGroup("Distance options")
    .add("--type", "choose \"Euclidean (eu)\", \"Diagonal Manalanobis (ma)\", \"Log Inner Product (lip)\"")
    .add("--theta", "specify the file containing the diagnol term of Mahalanobis distance (dim=39)", false)
    .add("--eta", "Specify the coefficient in the smoothing minimum", false, "-2");

  cmdParser
    .addGroup("Example 1: ./pair-wise-dtw --ark=data/example.76.ark --type=eu")
    .addGroup("Example 2: ./pair-wise-dtw --ark=data/example.76.ark --type=ma --theta=<some-trained-theta>");
  
  if(!cmdParser.isOptionLegal())
    cmdParser.showUsageAndExit();

  string archive_fn = cmdParser.find("--ark");
  string output_fn  = cmdParser.find("-o");
  bool isSelfTest   = (cmdParser.find("--self-test") == "true");
  string theta_fn   = cmdParser.find("--theta");
  string dist_type  = cmdParser.find("--type");
  float eta	    = atof(cmdParser.find("--eta").c_str());

  int N, dim; float* data; unsigned int* offset;
  // loadFeatureArchive(archive_fn, data, offset, N, dim); 
  loadKaldi(archive_fn, data, offset, N, dim); 

  distance_fn* dist = initDistanceMeasure(dist_type, dim, theta_fn);

  float* scores = computePairwiseDTW(data, offset, N, dim, *dist, eta);

  cvtDistanceToSimilarity(scores, N);

  FILE* fid = (output_fn.empty()) ? stdout : fopen(output_fn.c_str(), "w");
  print(fid, scores, N);
  if (fid != stdout) 
    fclose(fid);

  delete [] scores;

  return 0;
}

void loadKaldi(string filename, float* &dataPtr, unsigned int* &offsetPtr, int& N, int& dim) {

  FILE* f = fopen(filename.c_str(), "r");
  
  if (f == NULL)
    return;

  size_t line_buffer = 65536;
  char line[line_buffer];

#pragma GCC diagnostic ignored "-Wunused-result"
  fgets(line, line_buffer, f);
  fgets(line, line_buffer, f);

  dim = 0;
  char* tok = strtok(line, " ");
  do { ++dim; } while (strtok(NULL, " ")[0] != '\n');

  fseek(f, 0, SEEK_SET);

  vector<float> data;
  vector<unsigned int> offset;

  while(fgets(line, line_buffer, f)) {
    tok = strtok(line, " ");

    if (string(tok).find("[") != string::npos) {
      offset.push_back(data.size());
      continue;
    }

    data.push_back(atof(tok));
    while((tok = strtok(NULL, " "))) {
      if (tok[0] == ']' || tok[0] == '\n')
	break;
      data.push_back(atof(tok));
    }
  }
  offset.push_back(data.size());

  dataPtr = new float[data.size()];
  memcpy(dataPtr, data.data(), sizeof(float) * data.size());

  offsetPtr = new unsigned int[offset.size()];
  memcpy(offsetPtr, offset.data(), sizeof(unsigned int) * offset.size());

  N = offset.size() - 1;

  fclose(f);
}

distance_fn* initDistanceMeasure(string dist_type, size_t dim, string theta_fn) {
  distance_fn* dist;

  if (dist_type == "ma") {
    dist = new mahalanobis_fn(dim);
    dynamic_cast<mahalanobis_fn*>(dist)->setDiag(theta_fn);
  }
  else if (dist_type == "lip") {
    dist = new log_inner_product_fn(dim);
    dynamic_cast<mahalanobis_fn*>(dist)->setDiag(theta_fn);
  }
  else if (dist_type == "eu")
    dist = new euclidean_fn;
  else {
    fprintf(stderr, "--type unspecified or unknown\n");
    exit(-1);
  }

  return dist;
}

void print(FILE* fid, float* m, int N) {
  for (int i=0; i<N; ++i) {
    for (int j=0; j<N; ++j)
      fprintf(fid, "%.6f ", m[i * N + j]);
    fprintf(fid, "\n");
  }
}

float calcError(float* s1, float* s2, int N) {
  float error = 0;
  for (int i=0; i<N; ++i)
    for (int j=0; j<N; ++j)
      error += pow(s1[i * N + j] - s2[i * N + j], 2.0);

  error /= N*N;
  return error;
}

void normalize(float* m, int N, float eta) {
  const float MIN_EXP = 12;

  float min = m[0];
  float max = m[0];

  for (int i=0; i<N; ++i) {
    for (int j=0; j<N; ++j) {
      if (m[i * N + j] > max) max = m[i * N + j];
      if (m[i * N + j] < min) min = m[i * N + j];
    }
  }

  if (min - max == 0)
    return;

  float normalizer = MIN_EXP / (max - min) / abs(eta);

  for (int i=0; i<N*N; ++i)
    m[i] = (m[i] - min) * normalizer;

  for (int i=0; i<N*N; ++i)
    m[i] = exp(eta * m[i]);
}

void cvtDistanceToSimilarity(float* m, int N) {
  float min = m[0];
  float max = m[0];

  for (int i=0; i<N; ++i) {
    for (int j=0; j<i; ++j) {
      if (m[i * N + j] > max) max = m[i * N + j];
      if (m[i * N + j] < min) min = m[i * N + j];
    }
  }

  printf("max = %.7f, min = %.7f \n", max, min);

  if (min - max == 0)
    return;
  
  for (int i=0; i<N; ++i)
    m[i*N + i] = min;
  
  for (int i=0; i<N*N; ++i)
    m[i] = abs((m[i] - max) / (min - max));
}

void normalize_in_log(float* m, int N) {

  float min = m[0];
  float max = m[0];

  for (int i=0; i<N; ++i) {
    for (int j=0; j<i; ++j) {
      if (m[i * N + j] > max) max = m[i * N + j];
      if (m[i * N + j] < min) min = m[i * N + j];
    }
  }

  printf("max = %.7f, min = %.7f \n", max, min);

  if (min - max == 0)
    return;
  
  for (int i=0; i<N; ++i)
    m[i*N + i] = min;
  
  for (int i=0; i<N*N; ++i)
    m[i] = abs((m[i] - max) / (min - max));
}
