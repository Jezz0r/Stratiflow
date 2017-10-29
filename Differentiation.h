#pragma once

#include <Eigen/Core>

#include "Field.h"

MatrixXd ChebSecondDerivativeMatrix(BoundaryCondition bc, double L, int N);
MatrixXd ChebDerivativeMatrix(BoundaryCondition originalBC, double L, int N);

DiagonalMatrix<double, -1> FourierSecondDerivativeMatrix(double L, int N);
DiagonalMatrix<complex, -1> FourierDerivativeMatrix(double L, int N);