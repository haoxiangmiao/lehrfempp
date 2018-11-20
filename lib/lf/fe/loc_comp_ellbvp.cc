/* **************************************************************************
 * LehrFEM++ - A simple C++ finite element libray for teaching
 * Developed from 2018 at the Seminar of Applied Mathematics of ETH Zurich,
 * lead developers Dr. R. Casagrande and Prof. R. Hiptmair
 ***************************************************************************/

/**
 * @file
 * @brief Implementation of local computations for Lagrange FE for
 * 2nd-order linear elliptic boundary value problems.
 * @author Ralf Hiptmair
 * @date October 2018
 * @copyright MIT License
 */

#include "loc_comp_ellbvp.h"

namespace lf::fe {
// Implementation of LocCompLagrFEPreprocessor

CONTROLDECLARECOMMENT(LocCompLagrFEPreprocessor, ctrl_,
                      "LocCompLagrFEPreprocessor_ctrl",
                      "Output control for LocCompLagrFEPreprocessor");

LocCompLagrFEPreprocessor::LocCompLagrFEPreprocessor(
    const ScalarReferenceFiniteElement<double> &fe_tria,
    const ScalarReferenceFiniteElement<double> &fe_quad,
    lf::quad::quadOrder_t loc_quad_order)
    : fe_tria_(fe_tria), fe_quad_(fe_quad) {
  LF_ASSERT_MSG((fe_tria_.Dimension() == 2) && (fe_quad_.Dimension() == 2),
                "Implemented only in 2D!");
  // Compatibility check for numbers of local shape functions
  LF_ASSERT_MSG((fe_tria_.RefEl() == lf::base::RefEl::kTria()) ||
                    (fe_quad_.RefEl() == lf::base::RefEl::kQuad()),
                "Unexpected type of reference cell");
  LF_ASSERT_MSG((fe_tria_.NumRefShapeFunctions(2, 0) == 1) &&
                    (fe_quad_.NumRefShapeFunctions(2, 0) == 1),
                "Exactly one shape function must be assigned to each vertex");
  LF_ASSERT_MSG((fe_tria_.NumRefShapeFunctions(1, 0) ==
                 fe_quad_.NumRefShapeFunctions(1, 0)),
                "#RSF mismatch on edges "
                    << fe_tria_.NumRefShapeFunctions(1, 0) << " <-> "
                    << fe_quad_.NumRefShapeFunctions(1, 0));

  // Maximal degree of both finite elements
  unsigned int poly_order = std::max(fe_tria_.order(), fe_quad_.order());

  // Obtain quadrature rules for both triangles and quadrilaterals
  // If no quadrature order has been preselected,
  // we choose the order twice the polynomial degree of the finite element
  // space. This is slightly more than required for an admissible variational
  // crime.
  lf::quad::quadOrder_t quad_order = loc_quad_order;
  if (quad_order == 0) {
    quad_order = 2 * poly_order;
  }

  {
    // Preprocessing for triangles
    Nrsf_tria_ = fe_tria_.NumRefShapeFunctions();

    // Fetch suitable predefined quadrature rule
    qr_tria_ = lf::quad::make_QuadRule(lf::base::RefEl::kTria(), quad_order);
    Nqp_tria_ = qr_tria_.NumPoints();
    SWITCHEDSTATEMENT(ctrl_, kout_qr,
                      std::cout << "LagrEM(Tria): " << qr_tria_ << std::endl);

    // Obtain value of reference shape functions in all quadrature points
    const std::vector<Eigen::Matrix<double, 1, Eigen::Dynamic>> rsf_val{
        fe_tria_.EvalReferenceShapeFunctions(qr_tria_.Points())};
    LF_ASSERT_MSG(Nrsf_tria_ == rsf_val.size(),
                  "Mismatch in length of value vector " << Nrsf_tria_ << " <-> "
                                                        << rsf_val.size());
    // Store the values for reuse for all cells
    rsf_quadpoints_tria_.resize(Nrsf_tria_, Nqp_tria_);

    for (int i = 0; i < Nrsf_tria_; ++i) {
      LF_ASSERT_MSG(
          (rsf_val[i].cols() == Nqp_tria_),
          "Length mismatch " << rsf_val[i].cols() << " <-> " << Nqp_tria_);
      for (int j = 0; j < Nqp_tria_; ++j) {
        rsf_quadpoints_tria_(i, j) = rsf_val[i][j];
      }
    }
    SWITCHEDSTATEMENT(ctrl_, kout_rsfvals,
                      std::cout << "LagrEM(Tria): values of RSFs\n"
                                << rsf_quadpoints_tria_ << std::endl);

    // Store the gradients for the reference shape functions
    std::vector<Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>> rsf_grad{
        fe_tria_.GradientsReferenceShapeFunctions(qr_tria_.Points())};
    LF_ASSERT_MSG(Nrsf_tria_ == rsf_grad.size(),
                  "Mismatch in length of gradient vector"
                      << Nrsf_tria_ << " <-> " << rsf_grad.size());

    // Store the gradients internally in an array of stacked vectors
    grad_quadpoint_tria_.resize(
        Nqp_tria_, Eigen::Matrix<double, 2, Eigen::Dynamic>(2, Nrsf_tria_));
    for (int i = 0; i < Nqp_tria_; ++i) {
      for (int j = 0; j < Nrsf_tria_; ++j) {
        grad_quadpoint_tria_[i].col(j) = rsf_grad[j].col(i);
      }
    }
    SWITCHEDSTATEMENT(ctrl_, kout_gradvals,
                      std::cout << "LagrEM(Tria): gradients:" << std::endl;
                      for (int i = 0; i < Nqp_tria_; ++i) {
                        std::cout << "QP " << i << " = \n"
                                  << grad_quadpoint_tria_[i] << std::endl;
                      });
  }

  {
    // Preprocessing for quadrilaterals
    Nrsf_quad_ = fe_quad_.NumRefShapeFunctions();

    // Fetch suitable predefined quadrature rule
    qr_quad_ = lf::quad::make_QuadRule(lf::base::RefEl::kQuad(), quad_order);
    Nqp_quad_ = qr_quad_.NumPoints();
    SWITCHEDSTATEMENT(ctrl_, kout_qr,
                      std::cout << "LagrEM(Quad): " << qr_quad_ << std::endl);

    // Obtain value of reference shape functions in all quadrature points
    const std::vector<Eigen::Matrix<double, 1, Eigen::Dynamic>> rsf_val{
        fe_quad_.EvalReferenceShapeFunctions(qr_quad_.Points())};
    LF_ASSERT_MSG(Nrsf_quad_ == rsf_val.size(),
                  "Mismatch in length of value vector " << Nrsf_quad_ << " <-> "
                                                        << rsf_val.size());
    // Store the values for reuse for all cells
    rsf_quadpoints_quad_.resize(Nrsf_quad_, Nqp_quad_);

    for (int i = 0; i < Nrsf_quad_; ++i) {
      LF_ASSERT_MSG(
          (rsf_val[i].cols() == Nqp_quad_),
          "Length mismatch " << rsf_val[i].cols() << " <-> " << Nqp_quad_);
      for (int j = 0; j < Nqp_quad_; ++j) {
        rsf_quadpoints_quad_(i, j) = rsf_val[i][j];
      }
    }
    SWITCHEDSTATEMENT(ctrl_, kout_rsfvals,
                      std::cout << "LagrEM(Quad): values of RSFs\n"
                                << rsf_quadpoints_quad_ << std::endl);

    // Store the gradients for the reference shape functions
    std::vector<Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>> rsf_grad{
        fe_quad_.GradientsReferenceShapeFunctions(qr_quad_.Points())};
    LF_ASSERT_MSG(Nrsf_quad_ == rsf_grad.size(),
                  "Mismatch in length of gradient vector"
                      << Nrsf_quad_ << " <-> " << rsf_grad.size());

    // Store the gradients internally in an array of stacked vectors
    grad_quadpoint_quad_.resize(
        Nqp_quad_, Eigen::Matrix<double, 2, Eigen::Dynamic>(2, Nrsf_quad_));
    for (int i = 0; i < Nqp_quad_; ++i) {
      for (int j = 0; j < Nrsf_quad_; ++j) {
        grad_quadpoint_quad_[i].col(j) = rsf_grad[j].col(i);
      }
    }
    SWITCHEDSTATEMENT(ctrl_, kout_gradvals,
                      std::cout << "LagrEM(Quad): gradients:" << std::endl;
                      for (int i = 0; i < Nqp_quad_; ++i) {
                        std::cout << "QP " << i << " = \n"
                                  << grad_quadpoint_quad_[i] << std::endl;
                      });
  }
}

}  // end namespace lf::fe