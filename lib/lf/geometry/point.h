#ifndef __184598b89ca44fe1a1e7a043bc32da06
#define __184598b89ca44fe1a1e7a043bc32da06

#include <lf/base/base.h>

#include "geometry.h"

namespace lf::geometry {

class Point : public Geometry {


public:

  Point(Eigen::VectorXd coord) : coord_(std::move(coord)) {}

  dim_t DimLocal() const override {return 0;}

  dim_t DimGlobal() const override {return coord_.rows(); }

  base::RefEl RefEl() const override {return base::RefEl::kPoint(); }

  Eigen::MatrixXd Global(const Eigen::MatrixXd& local) const override;

  Eigen::MatrixXd Jacobian(const Eigen::MatrixXd& local) const override;
  Eigen::MatrixXd JacobianInverseGramian(const ::Eigen::MatrixXd& local) const
  override;
  Eigen::VectorXd IntegrationElement(const Eigen::MatrixXd& local) const
  override;
  std::unique_ptr<Geometry> subGeometry(dim_t codim, dim_t i) const override;

private:
  Eigen::VectorXd coord_;
};




}



#endif // __184598b89ca44fe1a1e7a043bc32da06
