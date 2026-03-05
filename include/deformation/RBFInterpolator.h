#ifndef RBF_INTERPOLATOR_H
#define RBF_INTERPOLATOR_H

#include "core/Types.h"
#include "core/Project.h"
#include <Eigen/Dense>
#include <vector>
#include <functional>

namespace MetaVisage {

// Kernel function type
using KernelFunction = std::function<double(double r)>;

class RBFInterpolator {
public:
    RBFInterpolator();
    ~RBFInterpolator();

    // Set control points (source positions on morph mesh, target positions on target mesh)
    void SetControlPoints(const std::vector<Vector3>& sourcePoints,
                          const std::vector<Vector3>& targetPoints);

    // Set deformation parameters
    void SetKernelType(DeformationAlgorithm kernel);
    void SetStiffness(float stiffness);   // 0.0 = exact interpolation, 1.0 = maximum regularization
    void SetSmoothness(float smoothness); // 0.0 = sharp falloff, 1.0 = broad influence

    // Solve the RBF system (must be called before Evaluate)
    // Returns true if the system was solved successfully
    bool Solve();

    // Evaluate the displacement at a given position
    // Returns the displacement vector to add to the original position
    Vector3 Evaluate(const Vector3& position) const;

    // Check if the system has been solved
    bool IsSolved() const { return solved_; }

    // Get the number of control points
    int GetControlPointCount() const { return static_cast<int>(sourcePoints_.size()); }

private:
    // Kernel functions
    static double TPSKernel(double r);
    static double GaussianKernel(double r, double sigma);
    static double MultiquadricKernel(double r, double c);

    // Get the active kernel function with current parameters
    KernelFunction GetKernelFunction() const;

    // Compute average distance between control points (for kernel width scaling)
    double ComputeAverageDistance() const;

    // Control points
    std::vector<Vector3> sourcePoints_;  // Positions on morph mesh
    std::vector<Vector3> targetPoints_;  // Corresponding positions on target mesh

    // Parameters
    DeformationAlgorithm kernelType_;
    float stiffness_;
    float smoothness_;

    // Solved weights (N weights + 4 polynomial coefficients, for each of x/y/z)
    Eigen::VectorXd weightsX_;
    Eigen::VectorXd weightsY_;
    Eigen::VectorXd weightsZ_;

    bool solved_;
    double kernelWidth_;  // Computed from smoothness and point distribution
};

} // namespace MetaVisage

#endif // RBF_INTERPOLATOR_H
