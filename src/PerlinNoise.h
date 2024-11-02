// PerlinNoise.h
#ifndef PERLINNOISE_H
#define PERLINNOISE_H

#include <cmath>
#include <vector>
#include <numeric>
#include <algorithm>
#include <random>

// Simple Perlin Noise implementation
class PerlinNoise {
public:
    PerlinNoise(unsigned int seed = 237);
    double noise(double x, double y, double z) const;

private:
    std::vector<int> p;
    double fade(double t) const;
    double lerp(double t, double a, double b) const;
    double grad(int hash, double x, double y, double z) const;
};

#endif // PERLINNOISE_H