/* =========================================================================
   Copyright (c) 2010-2016, Institute for Microelectronics,
                            Institute for Analysis and Scientific Computing,
                            TU Wien.
   Portions of this software are copyright by UChicago Argonne, LLC.

                            -----------------
                  ViennaCL - The Vienna Computing Library
                            -----------------

   Project Head:    Karl Rupp                   rupp@iue.tuwien.ac.at

   (A list of authors and contributors can be found in the PDF manual)

   License:         MIT (X11), see file LICENSE in the base directory
============================================================================= */

#include <utility>
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <ctime>

#include "viennacl/scalar.hpp"
#include "viennacl/matrix.hpp"
#include "viennacl/linalg/prod.hpp"
#include "viennacl/linalg/norm_1.hpp"
#include "viennacl/linalg/norm_inf.hpp"
#include "viennacl/linalg/norm_frobenius.hpp"
#include "viennacl/matrix_proxy.hpp"
#include "viennacl/vector_proxy.hpp"
#include "viennacl/linalg/norm_1.hpp"
#include "viennacl/linalg/norm_2.hpp"
#include "viennacl/linalg/norm_inf.hpp"
#include "viennacl/linalg/norm_frobenius.hpp"

template<typename NumericT, typename VCLMatrixType>
bool check_for_equality(std::vector<std::vector<NumericT> > const & std_A, VCLMatrixType const & vcl_A, double epsilon)
{
  std::vector<std::vector<NumericT> > vcl_A_cpu(vcl_A.size1(), std::vector<NumericT>(vcl_A.size2()));
  viennacl::backend::finish();  //workaround for a bug in APP SDK 2.7 on Trinity APUs (with Catalyst 12.8)
  viennacl::copy(vcl_A, vcl_A_cpu);

  for (std::size_t i=0; i<std_A.size(); ++i)
  {
    for (std::size_t j=0; j<std_A[i].size(); ++j)
    {
      if (std::fabs(std_A[i][j] - vcl_A_cpu[i][j]) > 0)
      {
        if ( (std::abs(std_A[i][j] - vcl_A_cpu[i][j]) / std::max(std::fabs(std_A[i][j]), std::fabs(vcl_A_cpu[i][j])) > epsilon) || std::fabs(vcl_A_cpu[i][j] - vcl_A_cpu[i][j]) > 0 )
        {
          std::cout << "Error at index (" << i << ", " << j << "): " << std_A[i][j] << " vs " << vcl_A_cpu[i][j] << std::endl;
          std::cout << std::endl << "TEST failed!" << std::endl;
          return false;
        }
      }
    }
  }

  std::cout << "PASSED!" << std::endl;
  return true;
}




template<typename STLMatrixType,
          typename ViennaCLMatrixType1, typename ViennaCLMatrixType2, typename ViennaCLMatrixType3>
int run_test(double epsilon,
             STLMatrixType & std_A, STLMatrixType & std_B, STLMatrixType & std_C,
             ViennaCLMatrixType1 & vcl_A, ViennaCLMatrixType2 & vcl_B, ViennaCLMatrixType3 vcl_C)
{

  typedef typename viennacl::result_of::cpu_value_type<typename ViennaCLMatrixType1::value_type>::type  cpu_value_type;

  cpu_value_type alpha = cpu_value_type(3.1415);
  viennacl::scalar<cpu_value_type>   gpu_alpha = alpha;

  cpu_value_type beta = cpu_value_type(2.7182);
  viennacl::scalar<cpu_value_type>   gpu_beta = beta;


  //
  // Initializer:
  //
  std::cout << "Checking for zero_matrix initializer..." << std::endl;
  std_A = std::vector<std::vector<cpu_value_type> >(std_A.size(), std::vector<cpu_value_type>(std_A[0].size()));
  vcl_A = viennacl::zero_matrix<cpu_value_type>(vcl_A.size1(), vcl_A.size2());
  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  std::cout << "Checking for scalar_matrix initializer..." << std::endl;
  std_A = std::vector<std::vector<cpu_value_type> >(std_A.size(), std::vector<cpu_value_type>(std_A[0].size(), alpha));
  vcl_A = viennacl::scalar_matrix<cpu_value_type>(vcl_A.size1(), vcl_A.size2(), alpha);
  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  std_A = std::vector<std::vector<cpu_value_type> >(std_A.size(), std::vector<cpu_value_type>(std_A[0].size(), gpu_beta));
  vcl_A = viennacl::scalar_matrix<cpu_value_type>(  vcl_A.size1(),   vcl_A.size2(), gpu_beta);
  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  /*std::cout << "Checking for identity initializer..." << std::endl;
  std_A = ...;
  vcl_A = viennacl::identity_matrix<cpu_value_type>(vcl_A.size1());
  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;*/


  std::cout << std::endl;
  //std::cout << "//" << std::endl;
  //std::cout << "////////// Test: Assignments //////////" << std::endl;
  //std::cout << "//" << std::endl;

  if (!check_for_equality(std_B, vcl_B, epsilon))
    return EXIT_FAILURE;

  std::cout << "Testing matrix assignment... ";
  //std::cout << std_B(0,0) << " vs. " << vcl_B(0,0) << std::endl;
  std_A = std_B;
  vcl_A = vcl_B;
  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  if (std_A.size() == std_A[0].size())
  {
    std::cout << "Testing matrix assignment (transposed)... ";
    for (std::size_t i=0; i<std_A.size(); ++i)
      for (std::size_t j=0; j<std_A[i].size(); ++j)
        std_A[i][j] = std_B[j][i];
    vcl_A   = viennacl::trans(vcl_B);

    if (!check_for_equality(std_A, vcl_A, epsilon))
      return EXIT_FAILURE;
  }



  //std::cout << std::endl;
  //std::cout << "//" << std::endl;
  //std::cout << "////////// Test 1: Copy to GPU //////////" << std::endl;
  //std::cout << "//" << std::endl;

  std_A = std_B;
  viennacl::copy(std_B, vcl_A);
  std::cout << "Testing upper left copy to GPU... ";
  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;


  std_C = std_B;
  viennacl::copy(std_B, vcl_C);
  std::cout << "Testing lower right copy to GPU... ";
  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;


  //std::cout << std::endl;
  //std::cout << "//" << std::endl;
  //std::cout << "////////// Test 2: Copy from GPU //////////" << std::endl;
  //std::cout << "//" << std::endl;

  std::cout << "Testing upper left copy to A... ";
  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  std::cout << "Testing lower right copy to C... ";
  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;



  //std::cout << "//" << std::endl;
  //std::cout << "////////// Test 3: Addition //////////" << std::endl;
  //std::cout << "//" << std::endl;
  viennacl::copy(std_C, vcl_C);

  std::cout << "Inplace add: ";
  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] += std_C[i][j];
  vcl_C   +=   vcl_C;

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  if (std_C.size() == std_C[0].size())
  {
    std::cout << "Inplace add (transposed): ";
    STLMatrixType std_C2(std_C);
    for (std::size_t i=0; i<std_C.size(); ++i)
      for (std::size_t j=0; j<std_C[i].size(); ++j)
        std_C[i][j] += std_C2[j][i];
    vcl_C   += viennacl::trans(vcl_C);

    if (!check_for_equality(std_C, vcl_C, epsilon))
      return EXIT_FAILURE;

    std::cout << "Inplace add (transposed, expression): ";
    std_C2 = std_C;
    for (std::size_t i=0; i<std_C.size(); ++i)
      for (std::size_t j=0; j<std_C[i].size(); ++j)
        std_C[i][j] += std_C2[j][i] + std_C2[j][i];
    vcl_C   += viennacl::trans(vcl_C + vcl_C);

    if (!check_for_equality(std_C, vcl_C, epsilon))
      return EXIT_FAILURE;
  }

  std::cout << "Scaled inplace add: ";
  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] += beta * std_A[i][j];
  vcl_C   += gpu_beta * vcl_A;

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  std::cout << "Add: ";
  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] = std_A[i][j] + std_B[i][j];
  vcl_C   =   vcl_A +   vcl_B;

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  std::cout << "Add with flipsign: ";
  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] = -std_A[i][j] + std_B[i][j];
  vcl_C   = -   vcl_A +   vcl_B;

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;


  std::cout << "Scaled add (left): ";
  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] = cpu_value_type(long(alpha)) * std_A[i][j] + std_B[i][j];
  vcl_C   = long(alpha) *   vcl_A +   vcl_B;

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] = cpu_value_type(float(alpha)) * std_A[i][j] + std_B[i][j];
  vcl_C   = float(alpha) *   vcl_A +   vcl_B;

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] = cpu_value_type(double(alpha)) * std_A[i][j] + std_B[i][j];
  vcl_C   = double(alpha) *   vcl_A +   vcl_B;

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  std::cout << "Scaled add (left): ";
  vcl_C = gpu_alpha * vcl_A + vcl_B;
  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;


  std::cout << "Scaled add (right): ";
  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] = std_A[i][j] + std_B[i][j] * cpu_value_type(long(beta));
  vcl_C   =   vcl_A + vcl_B   * long(beta);

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] = std_A[i][j] + std_B[i][j] * cpu_value_type(float(beta));
  vcl_C   =   vcl_A + vcl_B   * float(beta);

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] = std_A[i][j] + std_B[i][j] * cpu_value_type(double(beta));
  vcl_C   =   vcl_A + vcl_B   * double(beta);

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  std::cout << "Scaled add (right): ";
  vcl_C = vcl_A + gpu_beta * vcl_B;
  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  std::cout << "Scaled add (right, with division): ";
  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] = std_A[i][j] + std_B[i][j] / cpu_value_type(long(beta));
  vcl_C   =   vcl_A + vcl_B   / long(beta);

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] = std_A[i][j] + std_B[i][j] / cpu_value_type(float(beta));
  vcl_C   =   vcl_A + vcl_B   / float(beta);

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] = std_A[i][j] + std_B[i][j] / cpu_value_type(double(beta));
  vcl_C   =   vcl_A + vcl_B   / double(beta);

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;


  std::cout << "Scaled add (both): ";
  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] = alpha * std_A[i][j] + beta * std_B[i][j];
  vcl_C   = alpha *   vcl_A + beta *   vcl_B;

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  std::cout << "Scaled add (both): ";
  vcl_C = gpu_alpha * vcl_A + gpu_beta * vcl_B;
  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  //std::cout << "//" << std::endl;
  //std::cout << "////////// Test 4: Subtraction //////////" << std::endl;
  //std::cout << "//" << std::endl;
  viennacl::copy(std_C, vcl_C);

  std::cout << "Inplace sub: ";
  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] -= std_B[i][j];
  vcl_C -= vcl_B;

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  if (std_C.size() == std_C[0].size())
  {
    std::cout << "Inplace sub (transposed): ";
    STLMatrixType std_C2(std_C);
    for (std::size_t i=0; i<std_C.size(); ++i)
      for (std::size_t j=0; j<std_C[i].size(); ++j)
        std_C[i][j] -= cpu_value_type(2) * std_C2[j][i];
    vcl_C   -= cpu_value_type(2) * viennacl::trans(vcl_C);

    if (!check_for_equality(std_C, vcl_C, epsilon))
      return EXIT_FAILURE;

    std::cout << "Inplace sub (transposed, expression): ";
    std_C2 = std_C;
    for (std::size_t i=0; i<std_C.size(); ++i)
      for (std::size_t j=0; j<std_C[i].size(); ++j)
        std_C[i][j] -= std_C2[j][i] + std_C2[j][i];
    vcl_C   -= viennacl::trans(vcl_C + vcl_C);

    if (!check_for_equality(std_C, vcl_C, epsilon))
      return EXIT_FAILURE;
  }

  std::cout << "Scaled Inplace sub: ";
  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] -= alpha * std_B[i][j];
  vcl_C -= alpha * vcl_B;

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;




  std::cout << "Sub: ";
  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] = std_A[i][j] - std_B[i][j];
  vcl_C = vcl_A - vcl_B;

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  std::cout << "Scaled sub (left): ";
  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_B[i][j] = alpha * std_A[i][j] - std_C[i][j];
  vcl_B   = alpha *   vcl_A - vcl_C;

  if (!check_for_equality(std_B, vcl_B, epsilon))
    return EXIT_FAILURE;

  std::cout << "Scaled sub (left): ";
  vcl_B = gpu_alpha * vcl_A - vcl_C;
  if (!check_for_equality(std_B, vcl_B, epsilon))
    return EXIT_FAILURE;


  std::cout << "Scaled sub (right): ";
  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_B[i][j] = std_A[i][j] - beta * std_C[i][j];
  vcl_B   =   vcl_A - vcl_C * beta;

  if (!check_for_equality(std_B, vcl_B, epsilon))
    return EXIT_FAILURE;

  std::cout << "Scaled sub (right): ";
  vcl_B = vcl_A - vcl_C * gpu_beta;
  if (!check_for_equality(std_B, vcl_B, epsilon))
    return EXIT_FAILURE;


  std::cout << "Scaled sub (both): ";
  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_B[i][j] = alpha * std_A[i][j] - beta * std_C[i][j];
  vcl_B = alpha * vcl_A - vcl_C * beta;

  if (!check_for_equality(std_B, vcl_B, epsilon))
    return EXIT_FAILURE;

  std::cout << "Scaled sub (both): ";
  vcl_B = gpu_alpha * vcl_A - vcl_C * gpu_beta;
  if (!check_for_equality(std_B, vcl_B, epsilon))
    return EXIT_FAILURE;


  std::cout << "Unary operator-: ";
  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] = - std_A[i][j];
  vcl_C = -vcl_A;

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;



  //std::cout << "//" << std::endl;
  //std::cout << "////////// Test 5: Scaling //////////" << std::endl;
  //std::cout << "//" << std::endl;
  viennacl::copy(std_A, vcl_A);

  std::cout << "Multiplication with CPU scalar: ";
  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] *= cpu_value_type(long(alpha));
  vcl_A   *= long(alpha);

  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] *= cpu_value_type(float(alpha));
  vcl_A   *= float(alpha);

  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] *= cpu_value_type(double(alpha));
  vcl_A   *= double(alpha);

  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  std::cout << "Multiplication with GPU scalar: ";
  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] *= beta;
  vcl_A *= gpu_beta;

  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;


  std::cout << "Division with CPU scalar: ";
  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] /= cpu_value_type(long(alpha));
  vcl_A   /= long(alpha);

  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] /= cpu_value_type(float(alpha));
  vcl_A   /= float(alpha);

  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] /= cpu_value_type(double(alpha));
  vcl_A   /= double(alpha);

  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  std::cout << "Division with GPU scalar: ";
  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] /= beta;
  vcl_A /= gpu_beta;

  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;



  std::cout << "Testing elementwise multiplication..." << std::endl;
  std_B = std::vector<std::vector<cpu_value_type> >(std_B.size(), std::vector<cpu_value_type>(std_B[0].size(), cpu_value_type(1.4142)));
  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] = cpu_value_type(3.1415) * std_B[i][j];
  viennacl::copy(std_A, vcl_A);
  viennacl::copy(std_B, vcl_B);
  viennacl::copy(std_B, vcl_B);
  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] = std_A[i][j] * std_B[i][j];
  vcl_A = viennacl::linalg::element_prod(vcl_A, vcl_B);

  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] += std_A[i][j] * std_B[i][j];
  vcl_A += viennacl::linalg::element_prod(vcl_A, vcl_B);

  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] -= std_A[i][j] * std_B[i][j];
  vcl_A -= viennacl::linalg::element_prod(vcl_A, vcl_B);

  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  ///////
  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] = (std_A[i][j] + std_B[i][j]) * std_B[i][j];
  vcl_A = viennacl::linalg::element_prod(vcl_A + vcl_B, vcl_B);

  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] += (std_A[i][j] + std_B[i][j]) * std_B[i][j];
  vcl_A += viennacl::linalg::element_prod(vcl_A + vcl_B, vcl_B);

  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] -= (std_A[i][j] + std_B[i][j]) * std_B[i][j];
  vcl_A -= viennacl::linalg::element_prod(vcl_A + vcl_B, vcl_B);

  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  ///////
  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] = std_A[i][j] * (std_B[i][j] + std_A[i][j]);
  vcl_A = viennacl::linalg::element_prod(vcl_A, vcl_B + vcl_A);

  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] += std_A[i][j] * (std_B[i][j] + std_A[i][j]);
  vcl_A += viennacl::linalg::element_prod(vcl_A, vcl_B + vcl_A);

  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] -= std_A[i][j] * (std_B[i][j] + std_A[i][j]);
  vcl_A -= viennacl::linalg::element_prod(vcl_A, vcl_B + vcl_A);

  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  ///////
  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] = (std_A[i][j] + std_B[i][j]) * (std_B[i][j] + std_A[i][j]);
  vcl_A = viennacl::linalg::element_prod(vcl_A + vcl_B, vcl_B + vcl_A);

  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] += (std_A[i][j] + std_B[i][j]) * (std_B[i][j] + std_A[i][j]);
  vcl_A += viennacl::linalg::element_prod(vcl_A + vcl_B, vcl_B + vcl_A);

  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] -= (std_A[i][j] + std_B[i][j]) * (std_B[i][j] + std_A[i][j]);
  vcl_A -= viennacl::linalg::element_prod(vcl_A + vcl_B, vcl_B + vcl_A);

  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;


  std_B = std::vector<std::vector<cpu_value_type> >(std_B.size(), std::vector<cpu_value_type>(std_B[0].size(), cpu_value_type(1.4142)));
  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] = cpu_value_type(3.1415) * std_B[i][j];
  viennacl::copy(std_A, vcl_A);
  viennacl::copy(std_B, vcl_B);

  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] = std_A[i][j] / std_B[i][j];
  vcl_A = viennacl::linalg::element_div(vcl_A, vcl_B);

  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] += std_A[i][j] / std_B[i][j];
  vcl_A += viennacl::linalg::element_div(vcl_A, vcl_B);

  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] -= std_A[i][j] / std_B[i][j];
  vcl_A -= viennacl::linalg::element_div(vcl_A, vcl_B);

  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  ///////
  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] = (std_A[i][j] + std_B[i][j]) / std_B[i][j];
  vcl_A = viennacl::linalg::element_div(vcl_A + vcl_B, vcl_B);

  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] += (std_A[i][j] + std_B[i][j]) / std_B[i][j];
  vcl_A += viennacl::linalg::element_div(vcl_A + vcl_B, vcl_B);

  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] -= (std_A[i][j] + std_B[i][j]) / std_B[i][j];
  vcl_A -= viennacl::linalg::element_div(vcl_A + vcl_B, vcl_B);

  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  ///////
  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] = std_A[i][j] / (std_B[i][j] + std_A[i][j]);
  vcl_A = viennacl::linalg::element_div(vcl_A, vcl_B + vcl_A);

  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] += std_A[i][j] / (std_B[i][j] + std_A[i][j]);
  vcl_A += viennacl::linalg::element_div(vcl_A, vcl_B + vcl_A);

  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] -= std_A[i][j] / (std_B[i][j] + std_A[i][j]);
  vcl_A -= viennacl::linalg::element_div(vcl_A, vcl_B + vcl_A);

  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  ///////
  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] = (std_A[i][j] + std_B[i][j]) / (std_B[i][j] + std_A[i][j]);
  vcl_A = viennacl::linalg::element_div(vcl_A + vcl_B, vcl_B + vcl_A);

  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] += (std_A[i][j] + std_B[i][j]) / (std_B[i][j] + std_A[i][j]);
  vcl_A += viennacl::linalg::element_div(vcl_A + vcl_B, vcl_B + vcl_A);

  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] -= (std_A[i][j] + std_B[i][j]) / (std_B[i][j] + std_A[i][j]);
  vcl_A -= viennacl::linalg::element_div(vcl_A + vcl_B, vcl_B + vcl_A);

  if (!check_for_equality(std_A, vcl_A, epsilon))
    return EXIT_FAILURE;

  // element_pow
  std::cout << "Testing unary element_pow()..." << std::endl;

  std_B = std::vector<std::vector<cpu_value_type> >(std_B.size(), std::vector<cpu_value_type>(std_B[0].size(), cpu_value_type(1.4142)));
  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] = cpu_value_type(3.1415) * std_B[i][j];
  viennacl::copy(std_A, vcl_A);
  viennacl::copy(std_B, vcl_B);

  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] = std::pow(std_A[i][j], std_B[i][j]);
  vcl_C = viennacl::linalg::element_pow(vcl_A, vcl_B);

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] += std::pow(std_A[i][j], std_B[i][j]);
  vcl_C += viennacl::linalg::element_pow(vcl_A, vcl_B);

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] -= std::pow(std_A[i][j], std_B[i][j]);
  vcl_C -= viennacl::linalg::element_pow(vcl_A, vcl_B);

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  ///////
  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] = std::pow(std_A[i][j] + std_B[i][j], std_B[i][j]);
  vcl_C = viennacl::linalg::element_pow(vcl_A + vcl_B, vcl_B);

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] += std::pow(std_A[i][j] + std_B[i][j], std_B[i][j]);
  vcl_C += viennacl::linalg::element_pow(vcl_A + vcl_B, vcl_B);

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] -= std::pow(std_A[i][j] + std_B[i][j], std_B[i][j]);
  vcl_C -= viennacl::linalg::element_pow(vcl_A + vcl_B, vcl_B);

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  ///////
  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] = std::pow(std_A[i][j], std_B[i][j] + std_A[i][j]);
  vcl_C = viennacl::linalg::element_pow(vcl_A, vcl_B + vcl_A);

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] += std::pow(std_A[i][j], std_B[i][j] + std_A[i][j]);
  vcl_C += viennacl::linalg::element_pow(vcl_A, vcl_B + vcl_A);

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] -= std::pow(std_A[i][j], std_B[i][j] + std_A[i][j]);
  vcl_C -= viennacl::linalg::element_pow(vcl_A, vcl_B + vcl_A);

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  ///////
  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] = std::pow(std_A[i][j] + std_B[i][j], std_B[i][j] + std_A[i][j]);
  vcl_C = viennacl::linalg::element_pow(vcl_A + vcl_B, vcl_B + vcl_A);

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] += std::pow(std_A[i][j] + std_B[i][j], std_B[i][j] + std_A[i][j]);
  vcl_C += viennacl::linalg::element_pow(vcl_A + vcl_B, vcl_B + vcl_A);

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] -= std::pow(std_A[i][j] + std_B[i][j], std_B[i][j] + std_A[i][j]);
  vcl_C -= viennacl::linalg::element_pow(vcl_A + vcl_B, vcl_B + vcl_A);

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  std::cout << "Testing unary element_pow() with alpha on the right..." << std::endl;

  std_B = std::vector<std::vector<cpu_value_type> >(std_B.size(), std::vector<cpu_value_type>(std_B[0].size(), cpu_value_type(0.14142)));
  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] = cpu_value_type(3.1415) * std_B[i][j];
  viennacl::copy(std_A, vcl_A);
  viennacl::copy(std_B, vcl_B);

  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] = std::pow(std_A[i][j], alpha);
  vcl_C = viennacl::linalg::element_pow(vcl_A, alpha);

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] += std::pow(std_A[i][j], alpha);
  vcl_C += viennacl::linalg::element_pow(vcl_A, alpha);

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] -= std::pow(std_A[i][j], alpha);
  vcl_C -= viennacl::linalg::element_pow(vcl_A, alpha);

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  ///////
  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] = std::pow(std_A[i][j] + std_B[i][j], alpha);
  vcl_C = viennacl::linalg::element_pow(vcl_A + vcl_B, alpha);

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] += std::pow(std_A[i][j] + std_B[i][j], alpha);
  vcl_C += viennacl::linalg::element_pow(vcl_A + vcl_B, alpha);

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] -= std::pow(std_A[i][j] + std_B[i][j], alpha);
  vcl_C -= viennacl::linalg::element_pow(vcl_A + vcl_B, alpha);

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  std::cout << "Testing unary element_pow() with alpha on the left..." << std::endl;

  std_B = std::vector<std::vector<cpu_value_type> >(std_B.size(), std::vector<cpu_value_type>(std_B[0].size(), cpu_value_type(0.14142)));
  for (std::size_t i=0; i<std_A.size(); ++i)
    for (std::size_t j=0; j<std_A[i].size(); ++j)
      std_A[i][j] = cpu_value_type(3.1415) * std_B[i][j];
  viennacl::copy(std_A, vcl_A);
  viennacl::copy(std_B, vcl_B);

  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] = std::pow(alpha, std_A[i][j]);
  vcl_C = viennacl::linalg::element_pow(alpha, vcl_A);

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] += std::pow(alpha, std_A[i][j]);
  vcl_C += viennacl::linalg::element_pow(alpha, vcl_A);

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] -= std::pow(alpha, std_A[i][j]);
  vcl_C -= viennacl::linalg::element_pow(alpha, vcl_A);

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  ///////
  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] = std::pow(alpha, std_B[i][j] + std_A[i][j]);
  vcl_C = viennacl::linalg::element_pow(alpha, vcl_B + vcl_A);

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] += std::pow(alpha, std_B[i][j] + std_A[i][j]);
  vcl_C += viennacl::linalg::element_pow(alpha, vcl_B + vcl_A);

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_C[i][j] -= std::pow(alpha, std_B[i][j] + std_A[i][j]);
  vcl_C -= viennacl::linalg::element_pow(alpha, vcl_B + vcl_A);

  if (!check_for_equality(std_C, vcl_C, epsilon))
    return EXIT_FAILURE;

  std::cout << "Testing unary elementwise operations..." << std::endl;

#define GENERATE_UNARY_OP_TEST(FUNCNAME) \
  std_B = std::vector<std::vector<cpu_value_type> >(std_B.size(), std::vector<cpu_value_type>(std_B[0].size(), cpu_value_type(0.04142))); \
  for (std::size_t i=0; i<std_C.size(); ++i) \
    for (std::size_t j=0; j<std_C[i].size(); ++j) \
      std_A[i][j] = cpu_value_type(3.1415) * std_B[i][j]; \
  for (std::size_t i=0; i<std_C.size(); ++i) \
    for (std::size_t j=0; j<std_C[i].size(); ++j) \
      std_C[i][j] = cpu_value_type(2.7172) * std_A[i][j]; \
  viennacl::copy(std_A, vcl_A); \
  viennacl::copy(std_B, vcl_B); \
  viennacl::copy(std_C, vcl_C); \
  viennacl::copy(std_B, vcl_B); \
  \
  for (std::size_t i=0; i<std_C.size(); ++i) \
    for (std::size_t j=0; j<std_C[i].size(); ++j) \
      std_C[i][j] = std::FUNCNAME(std_A[i][j]); \
  vcl_C = viennacl::linalg::element_##FUNCNAME(vcl_A); \
 \
  if (!check_for_equality(std_C, vcl_C, epsilon)) \
  { \
    std::cout << "Failure at C = " << #FUNCNAME << "(A)" << std::endl; \
    return EXIT_FAILURE; \
  } \
 \
  for (std::size_t i=0; i<std_C.size(); ++i) \
    for (std::size_t j=0; j<std_C[i].size(); ++j) \
      std_C[i][j] = std::FUNCNAME(std_A[i][j] + std_B[i][j]); \
  vcl_C = viennacl::linalg::element_##FUNCNAME(vcl_A + vcl_B); \
 \
  if (!check_for_equality(std_C, vcl_C, epsilon)) \
  { \
    std::cout << "Failure at C = " << #FUNCNAME << "(A + B)" << std::endl; \
    return EXIT_FAILURE; \
  } \
 \
  for (std::size_t i=0; i<std_C.size(); ++i) \
    for (std::size_t j=0; j<std_C[i].size(); ++j) \
      std_C[i][j] += std::FUNCNAME(std_A[i][j]); \
  vcl_C += viennacl::linalg::element_##FUNCNAME(vcl_A); \
 \
  if (!check_for_equality(std_C, vcl_C, epsilon)) \
  { \
    std::cout << "Failure at C += " << #FUNCNAME << "(A)" << std::endl; \
    return EXIT_FAILURE; \
  } \
 \
  for (std::size_t i=0; i<std_C.size(); ++i) \
    for (std::size_t j=0; j<std_C[i].size(); ++j) \
      std_C[i][j] += std::FUNCNAME(std_A[i][j] + std_B[i][j]); \
  vcl_C += viennacl::linalg::element_##FUNCNAME(vcl_A + vcl_B); \
 \
  if (!check_for_equality(std_C, vcl_C, epsilon)) \
  { \
    std::cout << "Failure at C += " << #FUNCNAME << "(A + B)" << std::endl; \
    return EXIT_FAILURE; \
  } \
 \
  for (std::size_t i=0; i<std_C.size(); ++i) \
    for (std::size_t j=0; j<std_C[i].size(); ++j) \
      std_C[i][j] -= std::FUNCNAME(std_A[i][j]); \
  vcl_C -= viennacl::linalg::element_##FUNCNAME(vcl_A); \
 \
  if (!check_for_equality(std_C, vcl_C, epsilon)) \
  { \
    std::cout << "Failure at C -= " << #FUNCNAME << "(A)" << std::endl; \
    return EXIT_FAILURE; \
  } \
 \
  for (std::size_t i=0; i<std_C.size(); ++i) \
    for (std::size_t j=0; j<std_C[i].size(); ++j) \
      std_C[i][j] -= std::FUNCNAME(std_A[i][j] + std_B[i][j]); \
  vcl_C -= viennacl::linalg::element_##FUNCNAME(vcl_A + vcl_B); \
 \
  if (!check_for_equality(std_C, vcl_C, epsilon)) \
  { \
    std::cout << "Failure at C -= " << #FUNCNAME << "(A + B)" << std::endl; \
    return EXIT_FAILURE; \
  } \
 \

  GENERATE_UNARY_OP_TEST(acos);
  GENERATE_UNARY_OP_TEST(asin);
  GENERATE_UNARY_OP_TEST(atan);
  GENERATE_UNARY_OP_TEST(cos);
  GENERATE_UNARY_OP_TEST(cosh);
  GENERATE_UNARY_OP_TEST(exp);
  GENERATE_UNARY_OP_TEST(floor);
  GENERATE_UNARY_OP_TEST(fabs);
  GENERATE_UNARY_OP_TEST(log);
  GENERATE_UNARY_OP_TEST(log10);
  GENERATE_UNARY_OP_TEST(sin);
  GENERATE_UNARY_OP_TEST(sinh);
  GENERATE_UNARY_OP_TEST(fabs);
  //GENERATE_UNARY_OP_TEST(abs); //OpenCL allows abs on integers only
  GENERATE_UNARY_OP_TEST(sqrt);
  GENERATE_UNARY_OP_TEST(tan);
  GENERATE_UNARY_OP_TEST(tanh);

#if __cplusplus > 199711L
  GENERATE_UNARY_OP_TEST(acosh);
  GENERATE_UNARY_OP_TEST(asinh);
  GENERATE_UNARY_OP_TEST(atanh);
  GENERATE_UNARY_OP_TEST(erf);
  GENERATE_UNARY_OP_TEST(erfc);
  GENERATE_UNARY_OP_TEST(exp2);
  GENERATE_UNARY_OP_TEST(exp10);
  GENERATE_UNARY_OP_TEST(log2);
  GENERATE_UNARY_OP_TEST(round);
  GENERATE_UNARY_OP_TEST(rsqrt);
  GENERATE_UNARY_OP_TEST(sign);
  GENERATE_UNARY_OP_TEST(trunc);
#endif

  std::cout << "Complicated expressions: ";
  //std::cout << "std_A: " << std_A << std::endl;
  //std::cout << "std_B: " << std_B << std::endl;
  //std::cout << "std_C: " << std_C << std::endl;
  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_B[i][j] +=     alpha * (- std_A[i][j] - beta * std_C[i][j] + std_A[i][j]);
  vcl_B += gpu_alpha * (-   vcl_A - vcl_C * beta   +   vcl_A);

  if (!check_for_equality(std_B, vcl_B, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_B[i][j] += (- std_A[i][j] - beta * std_C[i][j] + std_A[i][j] * beta) / gpu_alpha;
  vcl_B += (-   vcl_A - vcl_C * beta + gpu_beta * vcl_A) / gpu_alpha;

  if (!check_for_equality(std_B, vcl_B, epsilon))
    return EXIT_FAILURE;


  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_B[i][j] -=     alpha * (- std_A[i][j] - beta * std_C[i][j] - std_A[i][j]);
  vcl_B -= gpu_alpha * (-   vcl_A - vcl_C * beta - vcl_A);

  if (!check_for_equality(std_B, vcl_B, epsilon))
    return EXIT_FAILURE;

  for (std::size_t i=0; i<std_C.size(); ++i)
    for (std::size_t j=0; j<std_C[i].size(); ++j)
      std_B[i][j] -= (- std_A[i][j] - beta * std_C[i][j] - std_A[i][j] * beta) / alpha;
  vcl_B   -= (-   vcl_A - vcl_C * beta - gpu_beta * vcl_A) / gpu_alpha;

  if (!check_for_equality(std_B, vcl_B, epsilon))
    return EXIT_FAILURE;

  std::cout << std::endl;
  std::cout << "----------------------------------------------" << std::endl;
  std::cout << std::endl;


  return EXIT_SUCCESS;
}




template<typename T, typename ScalarType>
int run_test(double epsilon)
{

    typedef viennacl::matrix<ScalarType, T>    VCLMatrixType;

    std::size_t dim_rows = 131;
    std::size_t dim_cols = 33;
    //std::size_t dim_rows = 5;
    //std::size_t dim_cols = 3;

    //setup std objects:
    std::vector<std::vector<ScalarType> > std_A(dim_rows, std::vector<ScalarType>(dim_cols));
    std::vector<std::vector<ScalarType> > std_B(dim_rows, std::vector<ScalarType>(dim_cols));
    std::vector<std::vector<ScalarType> > std_C(dim_rows, std::vector<ScalarType>(dim_cols));

    for (std::size_t i=0; i<std_A.size(); ++i)
      for (std::size_t j=0; j<std_A[i].size(); ++j)
      {
        std_A[i][j] = ScalarType((i+2) + (j+1)*(i+2));
        std_B[i][j] = ScalarType((j+2) + (j+1)*(j+2));
        std_C[i][j] = ScalarType((i+1) + (i+1)*(i+2));
      }

    std::vector<std::vector<ScalarType> > std_A_large(4 * dim_rows, std::vector<ScalarType>(4 * dim_cols));
    for (std::size_t i=0; i<std_A_large.size(); ++i)
      for (std::size_t j=0; j<std_A_large[i].size(); ++j)
        std_A_large[i][j] = ScalarType(i * std_A_large[i].size() + j);

    //Setup ViennaCL objects
    VCLMatrixType vcl_A_full(4 * dim_rows, 4 * dim_cols);
    VCLMatrixType vcl_B_full(4 * dim_rows, 4 * dim_cols);
    VCLMatrixType vcl_C_full(4 * dim_rows, 4 * dim_cols);

    viennacl::copy(std_A_large, vcl_A_full);
    viennacl::copy(std_A_large, vcl_B_full);
    viennacl::copy(std_A_large, vcl_C_full);

    //
    // Create A
    //
    VCLMatrixType vcl_A(dim_rows, dim_cols);

    viennacl::range vcl_A_r1(2 * dim_rows, 3 * dim_rows);
    viennacl::range vcl_A_r2(dim_cols, 2 * dim_cols);
    viennacl::matrix_range<VCLMatrixType>   vcl_range_A(vcl_A_full, vcl_A_r1, vcl_A_r2);

    viennacl::slice vcl_A_s1(2, 3, dim_rows);
    viennacl::slice vcl_A_s2(2 * dim_cols, 2, dim_cols);
    viennacl::matrix_slice<VCLMatrixType>   vcl_slice_A(vcl_A_full, vcl_A_s1, vcl_A_s2);


    //
    // Create B
    //
    VCLMatrixType vcl_B(dim_rows, dim_cols);

    viennacl::range vcl_B_r1(dim_rows, 2 * dim_rows);
    viennacl::range vcl_B_r2(2 * dim_cols, 3 * dim_cols);
    viennacl::matrix_range<VCLMatrixType>   vcl_range_B(vcl_B_full, vcl_B_r1, vcl_B_r2);

    viennacl::slice vcl_B_s1(2 * dim_rows, 2, dim_rows);
    viennacl::slice vcl_B_s2(dim_cols, 3, dim_cols);
    viennacl::matrix_slice<VCLMatrixType>   vcl_slice_B(vcl_B_full, vcl_B_s1, vcl_B_s2);


    //
    // Create C
    //
    VCLMatrixType vcl_C(dim_rows, dim_cols);

    viennacl::range vcl_C_r1(2 * dim_rows, 3 * dim_rows);
    viennacl::range vcl_C_r2(3 * dim_cols, 4 * dim_cols);
    viennacl::matrix_range<VCLMatrixType>   vcl_range_C(vcl_C_full, vcl_C_r1, vcl_C_r2);

    viennacl::slice vcl_C_s1(dim_rows, 2, dim_rows);
    viennacl::slice vcl_C_s2(0, 3, dim_cols);
    viennacl::matrix_slice<VCLMatrixType>   vcl_slice_C(vcl_C_full, vcl_C_s1, vcl_C_s2);

    viennacl::copy(std_A, vcl_A);
    viennacl::copy(std_A, vcl_range_A);
    viennacl::copy(std_A, vcl_slice_A);

    viennacl::copy(std_B, vcl_B);
    viennacl::copy(std_B, vcl_range_B);
    viennacl::copy(std_B, vcl_slice_B);

    viennacl::copy(std_C, vcl_C);
    viennacl::copy(std_C, vcl_range_C);
    viennacl::copy(std_C, vcl_slice_C);


    std::cout << std::endl;
    std::cout << "//" << std::endl;
    std::cout << "////////// Test: Copy CTOR //////////" << std::endl;
    std::cout << "//" << std::endl;

    {
      std::cout << "Testing matrix created from range... ";
      VCLMatrixType vcl_temp = vcl_range_A;
      if (check_for_equality(std_A, vcl_temp, epsilon))
        std::cout << "PASSED!" << std::endl;
      else
      {
        std::cout << "vcl_temp: " << vcl_temp << std::endl;
        std::cout << "vcl_range_A: " << vcl_range_A << std::endl;
        std::cout << "vcl_A: " << vcl_A << std::endl;
        std::cout << std::endl << "TEST failed!" << std::endl;
        return EXIT_FAILURE;
      }

      std::cout << "Testing matrix created from slice... ";
      VCLMatrixType vcl_temp2 = vcl_range_B;
      if (check_for_equality(std_B, vcl_temp2, epsilon))
        std::cout << "PASSED!" << std::endl;
      else
      {
        std::cout << std::endl << "TEST failed!" << std::endl;
        return EXIT_FAILURE;
      }
    }

    std::cout << "//" << std::endl;
    std::cout << "////////// Test: Initializer for matrix type //////////" << std::endl;
    std::cout << "//" << std::endl;

    {
      std::vector<std::vector<ScalarType> > std_dummy1(std_A.size(), std::vector<ScalarType>(std_A.size()));
      for (std::size_t i=0; i<std_A.size(); ++i) std_dummy1[i][i] = ScalarType(1);
      std::vector<std::vector<ScalarType> > std_dummy2(std_A.size(), std::vector<ScalarType>(std_A.size(), ScalarType(3)));
      std::vector<std::vector<ScalarType> > std_dummy3(std_A.size(), std::vector<ScalarType>(std_A.size()));

      viennacl::matrix<ScalarType> vcl_dummy1 = viennacl::identity_matrix<ScalarType>(std_A.size());
      viennacl::matrix<ScalarType> vcl_dummy2 = viennacl::scalar_matrix<ScalarType>(std_A.size(), std_A.size(), 3.0);
      viennacl::matrix<ScalarType> vcl_dummy3 = viennacl::zero_matrix<ScalarType>(std_A.size(), std_A.size());

      std::cout << "Testing initializer CTOR... ";
      if (   check_for_equality(std_dummy1, vcl_dummy1, epsilon)
          && check_for_equality(std_dummy2, vcl_dummy2, epsilon)
          && check_for_equality(std_dummy3, vcl_dummy3, epsilon)
         )
        std::cout << "PASSED!" << std::endl;
      else
      {
        std::cout << std::endl << "TEST failed!" << std::endl;
        return EXIT_FAILURE;
      }

      std_dummy1 = std::vector<std::vector<ScalarType> >(std_A.size(), std::vector<ScalarType>(std_A.size()));
      std_dummy2 = std::vector<std::vector<ScalarType> >(std_A.size(), std::vector<ScalarType>(std_A.size()));
      for (std::size_t i=0; i<std_A.size(); ++i) std_dummy2[i][i] = ScalarType(1);
      std_dummy3 = std::vector<std::vector<ScalarType> >(std_A.size(), std::vector<ScalarType>(std_A.size(), 3));

      vcl_dummy1 = viennacl::zero_matrix<ScalarType>(std_A.size(), std_A.size());
      vcl_dummy2 = viennacl::identity_matrix<ScalarType>(std_A.size());
      vcl_dummy3 = viennacl::scalar_matrix<ScalarType>(std_A.size(), std_A.size(), 3.0);

      std::cout << "Testing initializer assignment... ";
      if (   check_for_equality(std_dummy1, vcl_dummy1, epsilon)
          && check_for_equality(std_dummy2, vcl_dummy2, epsilon)
          && check_for_equality(std_dummy3, vcl_dummy3, epsilon)
         )
        std::cout << "PASSED!" << std::endl;
      else
      {
        std::cout << std::endl << "TEST failed!" << std::endl;
        return EXIT_FAILURE;
      }
    }

    std::cout << "//" << std::endl;
    std::cout << "////////// Test: Norms //////////" << std::endl;
    std::cout << "//" << std::endl;

    /*ScalarType std_norm_1 = viennacl::linalg::norm_1(std_C);
    ScalarType   vcl_norm_1 = viennacl::linalg::norm_1(vcl_C);
    if ( std::fabs(std_norm_1 - vcl_norm_1) / std_norm_1 > epsilon)
    {
      std::cerr << "Failure at norm_1(): " << std::fabs(std_norm_1 - vcl_norm_1) / std_norm_1  << std::endl;
      return EXIT_FAILURE;
    }

    ScalarType std_norm_inf = boost::numeric::std::norm_inf(std_C);
    ScalarType   vcl_norm_inf = viennacl::linalg::norm_inf(vcl_C);
    if ( std::fabs(std_norm_inf - vcl_norm_inf) / std_norm_inf > epsilon)
    {
      std::cerr << "Failure at norm_inf(): " << std::fabs(std_norm_inf - vcl_norm_inf) / std_norm_inf << std::endl;
      return EXIT_FAILURE;
    }*/

    ScalarType std_norm_frobenius = 0;
    for (std::size_t i=0; i<std_C.size(); ++i)
      for (std::size_t j=0; j<std_C[i].size(); ++j)
        std_norm_frobenius += std_C[i][j] * std_C[i][j];
    std_norm_frobenius = std::sqrt(std_norm_frobenius);
    ScalarType   vcl_norm_frobenius = viennacl::linalg::norm_frobenius(vcl_C);
    if ( std::fabs(std_norm_frobenius - vcl_norm_frobenius) / std_norm_frobenius > epsilon)
    {
      std::cerr << "Failure at norm_frobenius()" << std::endl;
      return EXIT_FAILURE;
    }

    viennacl::scalar<ScalarType> device_std_norm_frobenius = std_norm_frobenius;
    viennacl::scalar<ScalarType> device_vcl_norm_frobenius = viennacl::linalg::norm_frobenius(vcl_C);
    if ( std::fabs(device_std_norm_frobenius - device_vcl_norm_frobenius) / device_std_norm_frobenius > epsilon)
    {
      std::cerr << "Failure at norm_frobenius()" << std::endl;
      return EXIT_FAILURE;
    }

    vcl_norm_frobenius = viennacl::linalg::norm_frobenius(vcl_range_C);
    if ( std::fabs(std_norm_frobenius - vcl_norm_frobenius) / std_norm_frobenius > epsilon)
    {
      std::cerr << "Failure at norm_frobenius() with range" << std::endl;
      return EXIT_FAILURE;
    }

    device_vcl_norm_frobenius = viennacl::linalg::norm_frobenius(vcl_range_C);
    if ( std::fabs(device_std_norm_frobenius - device_vcl_norm_frobenius) / device_std_norm_frobenius > epsilon)
    {
      std::cerr << "Failure at norm_frobenius() with range" << std::endl;
      return EXIT_FAILURE;
    }

    vcl_norm_frobenius = viennacl::linalg::norm_frobenius(vcl_slice_C);
    if ( std::fabs(std_norm_frobenius - vcl_norm_frobenius) / std_norm_frobenius > epsilon)
    {
      std::cerr << "Failure at norm_frobenius() with slice" << std::endl;
      return EXIT_FAILURE;
    }

    device_vcl_norm_frobenius = viennacl::linalg::norm_frobenius(vcl_slice_C);
    if ( std::fabs(device_std_norm_frobenius - device_vcl_norm_frobenius) / device_std_norm_frobenius > epsilon)
    {
      std::cerr << "Failure at norm_frobenius() with slice" << std::endl;
      return EXIT_FAILURE;
    }

    std::cout << "PASSED!" << std::endl;


    //
    // run operation tests:
    //

    std::cout << "//" << std::endl;
    std::cout << "////////// Test: Operations //////////" << std::endl;
    std::cout << "//" << std::endl;


    /////// A=matrix:
    std::cout << "Testing A=matrix, B=matrix, C=matrix ..." << std::endl;
    viennacl::copy(std_A, vcl_A);
    viennacl::copy(std_B, vcl_B);
    viennacl::copy(std_C, vcl_C);
    if (run_test(epsilon,
                 std_A, std_B, std_C,
                 vcl_A, vcl_B, vcl_C) != EXIT_SUCCESS)
    {
      return EXIT_FAILURE;
    }

    std::cout << "Testing A=matrix, B=matrix, C=range ..." << std::endl;
    viennacl::copy(std_A, vcl_A);
    viennacl::copy(std_B, vcl_B);
    viennacl::copy(std_C, vcl_range_C);
    if (run_test(epsilon,
                 std_A, std_B, std_C,
                 vcl_A, vcl_B, vcl_range_C) != EXIT_SUCCESS)
    {
      return EXIT_FAILURE;
    }

    std::cout << "Testing A=matrix, B=matrix, C=slice ..." << std::endl;
    viennacl::copy(std_A, vcl_A);
    viennacl::copy(std_B, vcl_B);
    viennacl::copy(std_C, vcl_slice_C);
    if (run_test(epsilon,
                 std_A, std_B, std_C,
                 vcl_A, vcl_B, vcl_slice_C) != EXIT_SUCCESS)
    {
      return EXIT_FAILURE;
    }

    std::cout << "Testing A=matrix, B=range, C=matrix ..." << std::endl;
    viennacl::copy(std_A, vcl_A);
    viennacl::copy(std_B, vcl_range_B);
    viennacl::copy(std_C, vcl_C);
    if (run_test(epsilon,
                 std_A, std_B, std_C,
                 vcl_A, vcl_range_B, vcl_C) != EXIT_SUCCESS)
    {
      return EXIT_FAILURE;
    }

    std::cout << "Testing A=matrix, B=range, C=range ..." << std::endl;
    viennacl::copy(std_A, vcl_A);
    viennacl::copy(std_B, vcl_range_B);
    viennacl::copy(std_C, vcl_range_C);
    if (run_test(epsilon,
                 std_A, std_B, std_C,
                 vcl_A, vcl_range_B, vcl_range_C) != EXIT_SUCCESS)
    {
      return EXIT_FAILURE;
    }

    std::cout << "Testing A=matrix, B=range, C=slice ..." << std::endl;
    viennacl::copy(std_A, vcl_A);
    viennacl::copy(std_B, vcl_range_B);
    viennacl::copy(std_C, vcl_slice_C);
    if (run_test(epsilon,
                 std_A, std_B, std_C,
                 vcl_A, vcl_range_B, vcl_slice_C) != EXIT_SUCCESS)
    {
      return EXIT_FAILURE;
    }


    std::cout << "Testing A=matrix, B=slice, C=matrix ..." << std::endl;
    viennacl::copy(std_A, vcl_A);
    viennacl::copy(std_B, vcl_slice_B);
    viennacl::copy(std_C, vcl_C);
    if (run_test(epsilon,
                 std_A, std_B, std_C,
                 vcl_A, vcl_slice_B, vcl_C) != EXIT_SUCCESS)
    {
      return EXIT_FAILURE;
    }

    std::cout << "Testing A=matrix, B=slice, C=range ..." << std::endl;
    viennacl::copy(std_A, vcl_A);
    viennacl::copy(std_B, vcl_slice_B);
    viennacl::copy(std_C, vcl_range_C);
    if (run_test(epsilon,
                 std_A, std_B, std_C,
                 vcl_A, vcl_slice_B, vcl_range_C) != EXIT_SUCCESS)
    {
      return EXIT_FAILURE;
    }

    std::cout << "Testing A=matrix, B=slice, C=slice ..." << std::endl;
    viennacl::copy(std_A, vcl_A);
    viennacl::copy(std_B, vcl_slice_B);
    viennacl::copy(std_C, vcl_slice_C);
    if (run_test(epsilon,
                 std_A, std_B, std_C,
                 vcl_A, vcl_slice_B, vcl_slice_C) != EXIT_SUCCESS)
    {
      return EXIT_FAILURE;
    }



    /////// A=range:
    std::cout << "Testing A=range, B=matrix, C=matrix ..." << std::endl;
    viennacl::copy(std_A, vcl_range_A);
    viennacl::copy(std_B, vcl_B);
    viennacl::copy(std_C, vcl_C);
    if (run_test(epsilon,
                 std_A, std_B, std_C,
                 vcl_range_A, vcl_B, vcl_C) != EXIT_SUCCESS)
    {
      return EXIT_FAILURE;
    }

    std::cout << "Testing A=range, B=matrix, C=range ..." << std::endl;
    viennacl::copy(std_A, vcl_range_A);
    viennacl::copy(std_B, vcl_B);
    viennacl::copy(std_C, vcl_range_C);
    if (run_test(epsilon,
                 std_A, std_B, std_C,
                 vcl_range_A, vcl_B, vcl_range_C) != EXIT_SUCCESS)
    {
      return EXIT_FAILURE;
    }

    std::cout << "Testing A=range, B=matrix, C=slice ..." << std::endl;
    viennacl::copy(std_A, vcl_range_A);
    viennacl::copy(std_B, vcl_B);
    viennacl::copy(std_C, vcl_slice_C);
    if (run_test(epsilon,
                 std_A, std_B, std_C,
                 vcl_range_A, vcl_B, vcl_slice_C) != EXIT_SUCCESS)
    {
      return EXIT_FAILURE;
    }



    std::cout << "Testing A=range, B=range, C=matrix ..." << std::endl;
    viennacl::copy(std_A, vcl_range_A);
    viennacl::copy(std_B, vcl_range_B);
    viennacl::copy(std_C, vcl_C);
    if (run_test(epsilon,
                 std_A, std_B, std_C,
                 vcl_range_A, vcl_range_B, vcl_C) != EXIT_SUCCESS)
    {
      return EXIT_FAILURE;
    }

    std::cout << "Testing A=range, B=range, C=range ..." << std::endl;
    viennacl::copy(std_A, vcl_range_A);
    viennacl::copy(std_B, vcl_range_B);
    viennacl::copy(std_C, vcl_range_C);
    if (run_test(epsilon,
                 std_A, std_B, std_C,
                 vcl_range_A, vcl_range_B, vcl_range_C) != EXIT_SUCCESS)
    {
      return EXIT_FAILURE;
    }

    std::cout << "Testing A=range, B=range, C=slice ..." << std::endl;
    viennacl::copy(std_A, vcl_range_A);
    viennacl::copy(std_B, vcl_range_B);
    viennacl::copy(std_C, vcl_slice_C);
    if (run_test(epsilon,
                 std_A, std_B, std_C,
                 vcl_range_A, vcl_range_B, vcl_slice_C) != EXIT_SUCCESS)
    {
      return EXIT_FAILURE;
    }



    std::cout << "Testing A=range, B=slice, C=matrix ..." << std::endl;
    viennacl::copy(std_A, vcl_range_A);
    viennacl::copy(std_B, vcl_slice_B);
    viennacl::copy(std_C, vcl_C);
    if (run_test(epsilon,
                 std_A, std_B, std_C,
                 vcl_range_A, vcl_slice_B, vcl_C) != EXIT_SUCCESS)
    {
      return EXIT_FAILURE;
    }

    std::cout << "Testing A=range, B=slice, C=range ..." << std::endl;
    viennacl::copy(std_A, vcl_range_A);
    viennacl::copy(std_B, vcl_slice_B);
    viennacl::copy(std_C, vcl_range_C);
    if (run_test(epsilon,
                 std_A, std_B, std_C,
                 vcl_range_A, vcl_slice_B, vcl_range_C) != EXIT_SUCCESS)
    {
      return EXIT_FAILURE;
    }

    std::cout << "Testing A=range, B=slice, C=slice ..." << std::endl;
    viennacl::copy(std_A, vcl_range_A);
    viennacl::copy(std_B, vcl_slice_B);
    viennacl::copy(std_C, vcl_slice_C);
    if (run_test(epsilon,
                 std_A, std_B, std_C,
                 vcl_range_A, vcl_slice_B, vcl_slice_C) != EXIT_SUCCESS)
    {
      return EXIT_FAILURE;
    }


    /////// A=slice:
    std::cout << "Testing A=slice, B=matrix, C=matrix ..." << std::endl;
    viennacl::copy(std_A, vcl_slice_A);
    viennacl::copy(std_B, vcl_B);
    viennacl::copy(std_C, vcl_C);
    if (run_test(epsilon,
                 std_A, std_B, std_C,
                 vcl_slice_A, vcl_B, vcl_C) != EXIT_SUCCESS)
    {
      return EXIT_FAILURE;
    }

    std::cout << "Testing A=slice, B=matrix, C=range ..." << std::endl;
    viennacl::copy(std_A, vcl_slice_A);
    viennacl::copy(std_B, vcl_B);
    viennacl::copy(std_C, vcl_range_C);
    if (run_test(epsilon,
                 std_A, std_B, std_C,
                 vcl_slice_A, vcl_B, vcl_range_C) != EXIT_SUCCESS)
    {
      return EXIT_FAILURE;
    }

    std::cout << "Testing A=slice, B=matrix, C=slice ..." << std::endl;
    viennacl::copy(std_A, vcl_slice_A);
    viennacl::copy(std_B, vcl_B);
    viennacl::copy(std_C, vcl_slice_C);
    if (run_test(epsilon,
                 std_A, std_B, std_C,
                 vcl_slice_A, vcl_B, vcl_slice_C) != EXIT_SUCCESS)
    {
      return EXIT_FAILURE;
    }



    std::cout << "Testing A=slice, B=range, C=matrix ..." << std::endl;
    viennacl::copy(std_A, vcl_slice_A);
    viennacl::copy(std_B, vcl_range_B);
    viennacl::copy(std_C, vcl_C);
    if (run_test(epsilon,
                 std_A, std_B, std_C,
                 vcl_slice_A, vcl_range_B, vcl_C) != EXIT_SUCCESS)
    {
      return EXIT_FAILURE;
    }

    std::cout << "Testing A=slice, B=range, C=range ..." << std::endl;
    viennacl::copy(std_A, vcl_slice_A);
    viennacl::copy(std_B, vcl_range_B);
    viennacl::copy(std_C, vcl_range_C);
    if (run_test(epsilon,
                 std_A, std_B, std_C,
                 vcl_slice_A, vcl_range_B, vcl_range_C) != EXIT_SUCCESS)
    {
      return EXIT_FAILURE;
    }

    std::cout << "Testing A=slice, B=range, C=slice ..." << std::endl;
    viennacl::copy(std_A, vcl_slice_A);
    viennacl::copy(std_B, vcl_range_B);
    viennacl::copy(std_C, vcl_slice_C);
    if (run_test(epsilon,
                 std_A, std_B, std_C,
                 vcl_slice_A, vcl_range_B, vcl_slice_C) != EXIT_SUCCESS)
    {
      return EXIT_FAILURE;
    }



    std::cout << "Testing A=slice, B=slice, C=matrix ..." << std::endl;
    viennacl::copy(std_A, vcl_slice_A);
    viennacl::copy(std_B, vcl_slice_B);
    viennacl::copy(std_C, vcl_C);
    if (run_test(epsilon,
                 std_A, std_B, std_C,
                 vcl_slice_A, vcl_slice_B, vcl_C) != EXIT_SUCCESS)
    {
      return EXIT_FAILURE;
    }

    std::cout << "Testing A=slice, B=slice, C=range ..." << std::endl;
    viennacl::copy(std_A, vcl_slice_A);
    viennacl::copy(std_B, vcl_slice_B);
    viennacl::copy(std_C, vcl_range_C);
    if (run_test(epsilon,
                 std_A, std_B, std_C,
                 vcl_slice_A, vcl_slice_B, vcl_range_C) != EXIT_SUCCESS)
    {
      return EXIT_FAILURE;
    }

    std::cout << "Testing A=slice, B=slice, C=slice ..." << std::endl;
    viennacl::copy(std_A, vcl_slice_A);
    viennacl::copy(std_B, vcl_slice_B);
    viennacl::copy(std_C, vcl_slice_C);
    if (run_test(epsilon,
                 std_A, std_B, std_C,
                 vcl_slice_A, vcl_slice_B, vcl_slice_C) != EXIT_SUCCESS)
    {
      return EXIT_FAILURE;
    }


    return EXIT_SUCCESS;
}


