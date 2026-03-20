#include "deformation/RBFInterpolator.h"
#include <QDebug>
#include <cmath>
#include <algorithm>
#include <numeric>

namespace MetaVisage {

RBFInterpolator::RBFInterpolator()
    : kernelType_(DeformationAlgorithm::RBF_TPS),
      stiffness_(0.5f),
      smoothness_(0.5f),
      solved_(false),
      kernelWidth_(1.0) {
}

RBFInterpolator::~RBFInterpolator() {
}

void RBFInterpolator::SetControlPoints(const std::vector<Vector3>& sourcePoints,
                                        const std::vector<Vector3>& targetPoints) {
    sourcePoints_ = sourcePoints;
    targetPoints_ = targetPoints;
    solved_ = false;
}

void RBFInterpolator::SetKernelType(DeformationAlgorithm kernel) {
    kernelType_ = kernel;
    solved_ = false;
}

void RBFInterpolator::SetStiffness(float stiffness) {
    stiffness_ = std::clamp(stiffness, 0.0f, 1.0f);
    solved_ = false;
}

void RBFInterpolator::SetSmoothness(float smoothness) {
    smoothness_ = std::clamp(smoothness, 0.0f, 1.0f);
    solved_ = false;
}

double RBFInterpolator::TPSKernel(double r) {
    // Thin-Plate Spline kernel for 3D: phi(r) = r
    // This gives C1-smooth interpolation in 3D
    return r;
}

double RBFInterpolator::GaussianKernel(double r, double sigma) {
    // Gaussian kernel: phi(r) = exp(-(r/sigma)^2)
    if (sigma <= 0.0) return 0.0;
    double ratio = r / sigma;
    return std::exp(-(ratio * ratio));
}

double RBFInterpolator::MultiquadricKernel(double r, double c) {
    // Multiquadric kernel: phi(r) = sqrt(r^2 + c^2)
    return std::sqrt(r * r + c * c);
}

KernelFunction RBFInterpolator::GetKernelFunction() const {
    switch (kernelType_) {
        case DeformationAlgorithm::RBF_TPS:
            return [](double r) -> double {
                return TPSKernel(r);
            };

        case DeformationAlgorithm::RBF_GAUSSIAN: {
            double sigma = kernelWidth_;
            return [sigma](double r) -> double {
                return GaussianKernel(r, sigma);
            };
        }

        case DeformationAlgorithm::RBF_MULTIQUADRIC: {
            double c = kernelWidth_;
            return [c](double r) -> double {
                return MultiquadricKernel(r, c);
            };
        }

        default:
            // Default to TPS
            return [](double r) -> double {
                return TPSKernel(r);
            };
    }
}

double RBFInterpolator::ComputeAverageDistance() const {
    if (sourcePoints_.size() < 2) return 1.0;

    double totalDist = 0.0;
    int count = 0;

    for (size_t i = 0; i < sourcePoints_.size(); ++i) {
        for (size_t j = i + 1; j < sourcePoints_.size(); ++j) {
            Vector3 diff = sourcePoints_[i] - sourcePoints_[j];
            totalDist += static_cast<double>(diff.Length());
            count++;
        }
    }

    return (count > 0) ? (totalDist / count) : 1.0;
}

bool RBFInterpolator::Solve() {
    solved_ = false;

    const int N = static_cast<int>(sourcePoints_.size());

    if (N == 0) {
        qWarning() << "RBFInterpolator: No control points set";
        return false;
    }

    if (sourcePoints_.size() != targetPoints_.size()) {
        qWarning() << "RBFInterpolator: Source and target point counts don't match";
        return false;
    }

    // Compute kernel width from smoothness and point distribution
    double avgDist = ComputeAverageDistance();
    // Smoothness maps to kernel width:
    // smoothness=0.0 -> narrow influence (0.1 * avgDist)
    // smoothness=1.0 -> broad influence (3.0 * avgDist)
    kernelWidth_ = avgDist * (0.1 + 2.9 * static_cast<double>(smoothness_));

    auto kernel = GetKernelFunction();

    // Build the augmented system:
    // [Phi + lambda*I   P] [w]   [d]
    // [P^T              0] [a] = [0]
    //
    // Phi is NxN kernel matrix
    // P is Nx4 polynomial matrix [1, x, y, z]
    // lambda is regularization from stiffness

    const int systemSize = N + 4;

    Eigen::MatrixXd A = Eigen::MatrixXd::Zero(systemSize, systemSize);
    Eigen::VectorXd bx = Eigen::VectorXd::Zero(systemSize);
    Eigen::VectorXd by = Eigen::VectorXd::Zero(systemSize);
    Eigen::VectorXd bz = Eigen::VectorXd::Zero(systemSize);

    // Fill Phi matrix (upper-left NxN block)
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (i == j) {
                A(i, j) = 0.0; // phi(0) should be 0 for TPS
            } else {
                Vector3 diff = sourcePoints_[i] - sourcePoints_[j];
                double r = static_cast<double>(diff.Length());
                A(i, j) = kernel(r);
            }
        }
    }

    // Add regularization (stiffness)
    // stiffness=0.0 -> no regularization (exact interpolation)
    // stiffness=1.0 -> heavy regularization
    if (stiffness_ > 0.0f) {
        // Scale regularization relative to the kernel matrix magnitude
        double maxPhi = 0.0;
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                maxPhi = std::max(maxPhi, std::abs(A(i, j)));
            }
        }
        // Regularization lambda: exponential scaling for more intuitive control
        double lambda = static_cast<double>(stiffness_) * static_cast<double>(stiffness_) * maxPhi * 0.1;
        for (int i = 0; i < N; ++i) {
            A(i, i) += lambda;
        }
    }

    // Fill polynomial matrix P (upper-right Nx4 block and lower-left 4xN block)
    for (int i = 0; i < N; ++i) {
        double px = static_cast<double>(sourcePoints_[i].x);
        double py = static_cast<double>(sourcePoints_[i].y);
        double pz = static_cast<double>(sourcePoints_[i].z);

        // Upper-right block
        A(i, N + 0) = 1.0;
        A(i, N + 1) = px;
        A(i, N + 2) = py;
        A(i, N + 3) = pz;

        // Lower-left block (transpose)
        A(N + 0, i) = 1.0;
        A(N + 1, i) = px;
        A(N + 2, i) = py;
        A(N + 3, i) = pz;
    }

    // Fill right-hand side with displacements (target - source)
    for (int i = 0; i < N; ++i) {
        bx(i) = static_cast<double>(targetPoints_[i].x - sourcePoints_[i].x);
        by(i) = static_cast<double>(targetPoints_[i].y - sourcePoints_[i].y);
        bz(i) = static_cast<double>(targetPoints_[i].z - sourcePoints_[i].z);
    }

    // Solve the linear system using QR decomposition (robust for potentially ill-conditioned systems)
    Eigen::ColPivHouseholderQR<Eigen::MatrixXd> solver(A);

    if (!solver.isInvertible() && stiffness_ < 0.01f) {
        // If system is singular without regularization, add minimal regularization
        qDebug() << "RBFInterpolator: System near-singular, adding minimal regularization";
        for (int i = 0; i < N; ++i) {
            A(i, i) += 1e-6;
        }
        solver.compute(A);

        if (!solver.isInvertible()) {
            qWarning() << "RBFInterpolator: System still singular after regularization";
            return false;
        }
    }

    weightsX_ = solver.solve(bx);
    weightsY_ = solver.solve(by);
    weightsZ_ = solver.solve(bz);

    // Verify solution quality
    double residualX = (A * weightsX_ - bx).norm();
    double residualY = (A * weightsY_ - by).norm();
    double residualZ = (A * weightsZ_ - bz).norm();
    double totalResidual = residualX + residualY + residualZ;

    qDebug() << "RBFInterpolator: System solved with residual:" << totalResidual;
    qDebug() << "  Control points:" << N;
    qDebug() << "  Kernel width:" << kernelWidth_;
    qDebug() << "  Stiffness:" << stiffness_;
    qDebug() << "  Smoothness:" << smoothness_;

    if (std::isnan(totalResidual) || std::isinf(totalResidual)) {
        qWarning() << "RBFInterpolator: Solution contains NaN or Inf values";
        return false;
    }

    solved_ = true;
    return true;
}

Vector3 RBFInterpolator::Evaluate(const Vector3& position) const {
    if (!solved_) {
        return Vector3(0.0f, 0.0f, 0.0f);
    }

    const int N = static_cast<int>(sourcePoints_.size());
    auto kernel = GetKernelFunction();

    double dx = 0.0, dy = 0.0, dz = 0.0;

    // Sum RBF contributions
    for (int i = 0; i < N; ++i) {
        Vector3 diff = position - sourcePoints_[i];
        double r = static_cast<double>(diff.Length());
        double phi = kernel(r);

        dx += weightsX_(i) * phi;
        dy += weightsY_(i) * phi;
        dz += weightsZ_(i) * phi;
    }

    // Add polynomial (affine) term
    double px = static_cast<double>(position.x);
    double py = static_cast<double>(position.y);
    double pz = static_cast<double>(position.z);

    dx += weightsX_(N) + weightsX_(N + 1) * px + weightsX_(N + 2) * py + weightsX_(N + 3) * pz;
    dy += weightsY_(N) + weightsY_(N + 1) * px + weightsY_(N + 2) * py + weightsY_(N + 3) * pz;
    dz += weightsZ_(N) + weightsZ_(N + 1) * px + weightsZ_(N + 2) * py + weightsZ_(N + 3) * pz;

    return Vector3(static_cast<float>(dx), static_cast<float>(dy), static_cast<float>(dz));
}

} // namespace MetaVisage
