// Appendix B: Distribution functions.
//
// The standard normal density, its cumulative distribution and the inverse
// cumulative distribution. The inverse is the Beasley-Springer-Moro
// rational/Chebyshev approximation the book uses to turn uniforms into
// Gaussians (|error| < 3e-9 in x over [1e-10, 1 - 1e-10]), plus a
// variant polished by one Halley step against the exact cdf, which is
// accurate to a few ulps and costs one erfc and one exp more.

#ifndef CPP4Q_JOSHI_APPENDIX_B_DISTRIBUTIONS_NORMALS_H_
#define CPP4Q_JOSHI_APPENDIX_B_DISTRIBUTIONS_NORMALS_H_

namespace cpp4q::joshi {

// phi(x) = exp(-x^2 / 2) / sqrt(2 pi).
double NormalDensity(double x);

// N(x), computed as erfc(-x / sqrt 2) / 2 so the lower tail keeps full
// relative precision.
double CumulativeNormal(double x);

// N^{-1}(u) for u in (0, 1) by Beasley-Springer-Moro. Returns -inf / +inf
// at u = 0 / 1 and NaN outside [0, 1].
double InverseCumulativeNormal(double u);

// InverseCumulativeNormal followed by one Halley step on N(x) - u.
double InverseCumulativeNormalRefined(double u);

}  // namespace cpp4q::joshi

#endif  // CPP4Q_JOSHI_APPENDIX_B_DISTRIBUTIONS_NORMALS_H_
