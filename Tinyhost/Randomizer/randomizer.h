// randomizer.h
// (C) 2013-2018 Nicholas G Davies

#ifndef RANDOMIZER_H
#define RANDOMIZER_H

#include <random>
#include <ext/random>
#include <vector>
#include <limits>

// struct _msws    // Middle square Weyl sequence RNG
// {
// public:
//     typedef uint32_t result_type;
//     static result_type min() { return std::numeric_limits<result_type>::min(); }
//     static result_type max() { return std::numeric_limits<result_type>::max(); }

//     _msws() : x(0), w(0), s(0xb5ad4eceda1ce2a9) { }
//     result_type operator()() { x *= x; x += (w += s); return x = (x >> 32) | (x << 32); }

// private:
//     uint64_t x, w, s;
// };

class Randomizer
{
public:
    Randomizer();

    void Reset();

    double Uniform(double min = 0.0, double max = 1.0);
    double Normal(double mean = 0.0, double sd = 1.0);
    double Normal(double mean, double sd, double clamp);
    double Cauchy(double x0 = 0.0, double gamma = 1.0);
    double LogNormal(double mean = 0.0, double sd = 1.0);
    double Exponential(double rate = 1.0);
    double Gamma(double alpha, double beta);
    double Beta(double alpha, double beta);
    unsigned int Discrete(unsigned int size);
    int Discrete(int min, int max);
    int Discrete(std::vector<unsigned int>& cumulative_weights);
    int Discrete(std::vector<double>& cumulative_weights);
    int FlipCoin();
    void Pick(int min, int max, int n, std::vector<int>& picks);
    bool Bernoulli(double p);
    int Binomial(int n, double p);
    int Poisson(double mean);
    int Geometric(double p);
    int NonzeroPoisson(double mean);
    int FoundressDual(double a, int n_max);
    // Approximate. Accurate within ~0.025% for a = 0, a = 1, and 0.01 < a < 0.99.
    int FoundressPoisson(double a, int n_max);
    int Round(double x);
    void SetEventRate(unsigned int handle, double p);
    bool Event(unsigned int handle);
    template <typename RandomAccessIterator>
    void Shuffle(RandomAccessIterator first, RandomAccessIterator last, int n = -1);

    void DiehardOutput(const char* filename);

private:
    double LambertW0(const double x);
    static double FoundressPoisson_LogLambda[256];

    typedef __gnu_cxx::sfmt19937 engine_type;/////
    /////typedef _msws engine_type;
    std::seed_seq seed;
    engine_type engine;

    engine_type::result_type fast_bits;
    int fast_shift;

    std::vector<std::geometric_distribution<unsigned int>> event_distributions;
    std::vector<unsigned int> steps_to_next_event;
};

template <typename RandomAccessIterator>
void Randomizer::Shuffle(RandomAccessIterator first, RandomAccessIterator last, int n)
{
    if (n < 0)
        std::shuffle(first, last, engine);

    else for (size_t i = 0, size = last - first; i < (size_t)n; ++i)
        std::swap(*(first + i), *(first + i + Discrete(size - i)));
}

#endif
