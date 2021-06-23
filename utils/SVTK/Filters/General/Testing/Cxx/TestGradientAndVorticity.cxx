/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGradientAndVorticity.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkCellType.h"
#include "svtkCellTypeSource.h"
#include "svtkDoubleArray.h"
#include "svtkElevationFilter.h"
#include "svtkFloatArray.h"
#include "svtkGeneralTransform.h"
#include "svtkGradientFilter.h"
#include "svtkHigherOrderHexahedron.h"
#include "svtkHigherOrderQuadrilateral.h"
#include "svtkHigherOrderWedge.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"
#include "svtkStdString.h"
#include "svtkStructuredGrid.h"
#include "svtkStructuredGridReader.h"
#include "svtkTransformFilter.h"
#include "svtkUnstructuredGrid.h"
#include "svtkUnstructuredGridReader.h"

#include <vector>

#define SVTK_CREATE(type, var) svtkSmartPointer<type> var = svtkSmartPointer<type>::New()

// The 3D cell with the maximum number of points is SVTK_LAGRANGE_HEXAHEDRON.
// We support up to 6th order hexahedra.
#define SVTK_MAXIMUM_NUMBER_OF_POINTS 216

namespace
{
double Tolerance = 0.00001;

bool ArePointsWithinTolerance(double v1, double v2)
{
  if (v1 == v2 || fabs(v1) + fabs(v2) < Tolerance)
  {
    return true;
  }

  if (v1 == 0.0)
  {
    if (fabs(v2) < Tolerance)
    {
      return true;
    }
    std::cout << fabs(v2) << " (fabs(v2)) should be less than " << Tolerance << std::endl;
    return false;
  }
  if (fabs(1. - v1 / v2) < Tolerance)
  {
    return true;
  }
  std::cout << fabs(1. - v1 / v2) << " (fabs(1 - v1/v2)) should be less than " << Tolerance
            << std::endl;
  return false;
}

//-----------------------------------------------------------------------------
void CreateCellData(svtkDataSet* grid, int numberOfComponents, int offset, const char* arrayName)
{
  svtkIdType numberOfCells = grid->GetNumberOfCells();
  SVTK_CREATE(svtkDoubleArray, array);
  array->SetNumberOfComponents(numberOfComponents);
  array->SetNumberOfTuples(numberOfCells);
  std::vector<double> tupleValues(numberOfComponents);
  double point[3], parametricCenter[3], weights[SVTK_MAXIMUM_NUMBER_OF_POINTS];
  for (svtkIdType i = 0; i < numberOfCells; i++)
  {
    svtkCell* cell = grid->GetCell(i);
    cell->GetParametricCenter(parametricCenter);
    int subId = 0;
    cell->EvaluateLocation(subId, parametricCenter, point, weights);
    for (int j = 0; j < numberOfComponents; j++)
    { // +offset makes the curl/vorticity nonzero
      tupleValues[j] = point[(j + offset) % 3];
    }
    array->SetTypedTuple(i, &tupleValues[0]);
  }
  array->SetName(arrayName);
  grid->GetCellData()->AddArray(array);
}

//-----------------------------------------------------------------------------
void CreatePointData(svtkDataSet* grid, int numberOfComponents, int offset, const char* arrayName)
{
  svtkIdType numberOfPoints = grid->GetNumberOfPoints();
  SVTK_CREATE(svtkDoubleArray, array);
  array->SetNumberOfComponents(numberOfComponents);
  array->SetNumberOfTuples(numberOfPoints);
  std::vector<double> tupleValues(numberOfComponents);
  double point[3];
  for (svtkIdType i = 0; i < numberOfPoints; i++)
  {
    grid->GetPoint(i, point);
    for (int j = 0; j < numberOfComponents; j++)
    { // +offset makes the curl/vorticity nonzero
      tupleValues[j] = point[(j + offset) % 3];
    }
    array->SetTypedTuple(i, &tupleValues[0]);
  }
  array->SetName(arrayName);
  grid->GetPointData()->AddArray(array);
}

//-----------------------------------------------------------------------------
int IsGradientCorrect(svtkDoubleArray* gradients, int offset)
{
  int numberOfComponents = gradients->GetNumberOfComponents();
  for (svtkIdType i = 0; i < gradients->GetNumberOfTuples(); i++)
  {
    double* values = gradients->GetTuple(i);
    for (int origComp = 0; origComp < numberOfComponents / 3; origComp++)
    {
      for (int gradDir = 0; gradDir < 3; gradDir++)
      {
        if ((origComp - gradDir + offset) % 3 == 0)
        {
          if (fabs(values[origComp * 3 + gradDir] - 1.) > Tolerance)
          {
            svtkGenericWarningMacro(
              "Gradient value should be one but is " << values[origComp * 3 + gradDir]);
            return 0;
          }
        }
        else if (fabs(values[origComp * 3 + gradDir]) > Tolerance)
        {
          svtkGenericWarningMacro(
            "Gradient value should be zero but is " << values[origComp * 3 + gradDir]);
          return 0;
        }
      }
    }
  }
  return 1;
}

//-----------------------------------------------------------------------------
// we assume that the gradients are correct and so we can compute the "real"
// vorticity from it
int IsVorticityCorrect(svtkDoubleArray* gradients, svtkDoubleArray* vorticity)
{
  if (gradients->GetNumberOfComponents() != 9 || vorticity->GetNumberOfComponents() != 3)
  {
    svtkGenericWarningMacro("Bad number of components.");
    return 0;
  }
  for (svtkIdType i = 0; i < gradients->GetNumberOfTuples(); i++)
  {
    double* g = gradients->GetTuple(i);
    double* v = vorticity->GetTuple(i);
    if (!ArePointsWithinTolerance(v[0], g[7] - g[5]))
    {
      svtkGenericWarningMacro("Bad vorticity[0] value "
        << v[0] << " " << g[7] - g[5] << " difference is " << (v[0] - g[7] + g[5]));
      return 0;
    }
    else if (!ArePointsWithinTolerance(v[1], g[2] - g[6]))
    {
      svtkGenericWarningMacro("Bad vorticity[1] value "
        << v[1] << " " << g[2] - g[6] << " difference is " << (v[1] - g[2] + g[6]));
      return 0;
    }
    else if (!ArePointsWithinTolerance(v[2], g[3] - g[1]))
    {
      svtkGenericWarningMacro("Bad vorticity[2] value "
        << v[2] << " " << g[3] - g[1] << " difference is " << (v[2] - g[3] + g[1]));
      return 0;
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
// we assume that the gradients are correct and so we can compute the "real"
// Q criterion from it
int IsQCriterionCorrect(svtkDoubleArray* gradients, svtkDoubleArray* qCriterion)
{
  if (gradients->GetNumberOfComponents() != 9 || qCriterion->GetNumberOfComponents() != 1)
  {
    svtkGenericWarningMacro("Bad number of components.");
    return 0;
  }
  for (svtkIdType i = 0; i < gradients->GetNumberOfTuples(); i++)
  {
    double* g = gradients->GetTuple(i);
    double qc = qCriterion->GetValue(i);

    double t1 = .25 *
      ((g[7] - g[5]) * (g[7] - g[5]) + (g[3] - g[1]) * (g[3] - g[1]) +
        (g[2] - g[6]) * (g[2] - g[6]));
    double t2 = .5 *
      (g[0] * g[0] + g[4] * g[4] + g[8] * g[8] +
        .5 *
          ((g[3] + g[1]) * (g[3] + g[1]) + (g[6] + g[2]) * (g[6] + g[2]) +
            (g[7] + g[5]) * (g[7] + g[5])));

    if (!ArePointsWithinTolerance(qc, t1 - t2))
    {
      svtkGenericWarningMacro(
        "Bad Q-criterion value " << qc << " " << t1 - t2 << " difference is " << (qc - t1 + t2));
      return 0;
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
// we assume that the gradients are correct and so we can compute the "real"
// divergence from it
int IsDivergenceCorrect(svtkDoubleArray* gradients, svtkDoubleArray* divergence)
{
  if (gradients->GetNumberOfComponents() != 9 || divergence->GetNumberOfComponents() != 1)
  {
    svtkGenericWarningMacro("Bad number of components.");
    return 0;
  }
  for (svtkIdType i = 0; i < gradients->GetNumberOfTuples(); i++)
  {
    double* g = gradients->GetTuple(i);
    double div = divergence->GetValue(i);
    double gValue = g[0] + g[4] + g[8];

    if (!ArePointsWithinTolerance(div, gValue))
    {
      svtkGenericWarningMacro(
        "Bad divergence value " << div << " " << gValue << " difference is " << (div - gValue));
      return 0;
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
int PerformTest(svtkDataSet* grid)
{
  // Cleaning out the existing field data so that I can replace it with
  // an analytic function that I know the gradient of
  grid->GetPointData()->Initialize();
  grid->GetCellData()->Initialize();
  const char fieldName[] = "LinearField";
  int offset = 1;
  const int numberOfComponents = 3;
  CreateCellData(grid, numberOfComponents, offset, fieldName);
  CreatePointData(grid, numberOfComponents, offset, fieldName);

  SVTK_CREATE(svtkGradientFilter, cellGradients);
  cellGradients->SetInputData(grid);
  cellGradients->SetInputScalars(svtkDataObject::FIELD_ASSOCIATION_CELLS, fieldName);
  const char resultName[] = "Result";
  cellGradients->SetResultArrayName(resultName);

  SVTK_CREATE(svtkGradientFilter, pointGradients);
  pointGradients->SetInputData(grid);
  pointGradients->SetInputScalars(svtkDataObject::FIELD_ASSOCIATION_POINTS, fieldName);
  pointGradients->SetResultArrayName(resultName);

  // if we have an unstructured grid we also want to test out the options
  // for which cells contribute to the gradient computation so we loop
  // over them here.
  int gradientOptions = grid->IsA("svtkUnstructuredGrid") ? 2 : 0;
  for (int option = 0; option <= gradientOptions; option++)
  {
    cellGradients->SetContributingCellOption(option);
    pointGradients->SetContributingCellOption(option);
    cellGradients->Update();
    pointGradients->Update();

    svtkDoubleArray* gradCellArray = svtkArrayDownCast<svtkDoubleArray>(
      svtkDataSet::SafeDownCast(cellGradients->GetOutput())->GetCellData()->GetArray(resultName));

    if (!grid->IsA("svtkUnstructuredGrid"))
    {
      // ignore cell gradients if this is an unstructured grid
      // because the accuracy is so lousy
      if (!IsGradientCorrect(gradCellArray, offset))
      {
        return EXIT_FAILURE;
      }
    }

    svtkDoubleArray* gradPointArray = svtkArrayDownCast<svtkDoubleArray>(
      svtkDataSet::SafeDownCast(pointGradients->GetOutput())->GetPointData()->GetArray(resultName));

    if (!IsGradientCorrect(gradPointArray, offset))
    {
      return EXIT_FAILURE;
    }

    // now check on the vorticity calculations
    SVTK_CREATE(svtkGradientFilter, cellVorticity);
    cellVorticity->SetInputData(grid);
    cellVorticity->SetInputScalars(svtkDataObject::FIELD_ASSOCIATION_CELLS, fieldName);
    cellVorticity->SetResultArrayName(resultName);
    cellVorticity->SetComputeVorticity(1);
    cellVorticity->SetContributingCellOption(option);
    cellVorticity->Update();

    SVTK_CREATE(svtkGradientFilter, pointVorticity);
    pointVorticity->SetInputData(grid);
    pointVorticity->SetInputScalars(svtkDataObject::FIELD_ASSOCIATION_POINTS, fieldName);
    pointVorticity->SetResultArrayName(resultName);
    pointVorticity->SetComputeVorticity(1);
    pointVorticity->SetComputeQCriterion(1);
    pointVorticity->SetComputeDivergence(1);
    pointVorticity->SetContributingCellOption(option);
    pointVorticity->Update();

    // cell stuff
    svtkDoubleArray* vorticityCellArray = svtkArrayDownCast<svtkDoubleArray>(
      svtkDataSet::SafeDownCast(cellVorticity->GetOutput())->GetCellData()->GetArray("Vorticity"));

    if (!IsVorticityCorrect(gradCellArray, vorticityCellArray))
    {
      return EXIT_FAILURE;
    }

    // point stuff
    svtkDoubleArray* vorticityPointArray = svtkArrayDownCast<svtkDoubleArray>(
      svtkDataSet::SafeDownCast(pointVorticity->GetOutput())->GetPointData()->GetArray("Vorticity"));

    if (!IsVorticityCorrect(gradPointArray, vorticityPointArray))
    {
      return EXIT_FAILURE;
    }
    svtkDoubleArray* divergencePointArray =
      svtkArrayDownCast<svtkDoubleArray>(svtkDataSet::SafeDownCast(pointVorticity->GetOutput())
                                         ->GetPointData()
                                         ->GetArray("Divergence"));

    if (!IsDivergenceCorrect(gradPointArray, divergencePointArray))
    {
      return EXIT_FAILURE;
    }
    svtkDoubleArray* qCriterionPointArray =
      svtkArrayDownCast<svtkDoubleArray>(svtkDataSet::SafeDownCast(pointVorticity->GetOutput())
                                         ->GetPointData()
                                         ->GetArray("Q-criterion"));
    if (!IsQCriterionCorrect(gradPointArray, qCriterionPointArray))
    {
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
} // end local namespace

//-----------------------------------------------------------------------------
int TestGradientAndVorticity(int argc, char* argv[])
{
  int i;
  // Need to get the data root.
  const char* data_root = nullptr;
  for (i = 0; i < argc - 1; i++)
  {
    if (strcmp("-D", argv[i]) == 0)
    {
      data_root = argv[i + 1];
      break;
    }
  }
  if (!data_root)
  {
    svtkGenericWarningMacro("Need to specify the directory to SVTK_DATA_ROOT with -D <dir>.");
    return EXIT_FAILURE;
  }

  svtkStdString filename;
  filename = data_root;
  filename += "/Data/SampleStructGrid.svtk";
  SVTK_CREATE(svtkStructuredGridReader, structuredGridReader);
  structuredGridReader->SetFileName(filename.c_str());
  structuredGridReader->Update();
  svtkDataSet* grid = svtkDataSet::SafeDownCast(structuredGridReader->GetOutput());

  if (PerformTest(grid))
  {
    return EXIT_FAILURE;
  }

  // convert the structured grid to an unstructured grid
  SVTK_CREATE(svtkUnstructuredGrid, ug);
  ug->SetPoints(svtkStructuredGrid::SafeDownCast(grid)->GetPoints());
  ug->Allocate(grid->GetNumberOfCells());
  for (svtkIdType id = 0; id < grid->GetNumberOfCells(); id++)
  {
    svtkCell* cell = grid->GetCell(id);
    ug->InsertNextCell(cell->GetCellType(), cell->GetPointIds());
  }

  if (PerformTest(ug))
  {
    return EXIT_FAILURE;
  }

  // now test gradient of a variety of cell types using the cell type source. we scale
  // and rotate the grid to make sure that we don't have the cells conveniently
  // set up to their parametric coordinate system and then compare to an analytic
  // function (f=x) such that the gradient is (1, 0, 0).
  svtkNew<svtkCellTypeSource> cellTypeSource;
  cellTypeSource->SetBlocksDimensions(3, 3, 3); // make sure we have an interior cell
  cellTypeSource->SetCellOrder(3);
  svtkNew<svtkTransformFilter> transformFilter;
  transformFilter->SetInputConnection(cellTypeSource->GetOutputPort());
  svtkNew<svtkGeneralTransform> generalTransform;
  generalTransform->Scale(2, 3, 4);
  transformFilter->SetTransform(generalTransform);
  svtkNew<svtkElevationFilter> elevationFilter;
  elevationFilter->SetLowPoint(0, 0, 0);
  elevationFilter->SetHighPoint(1, 0, 0);
  elevationFilter->SetScalarRange(0, 1);
  elevationFilter->SetInputConnection(transformFilter->GetOutputPort());
  svtkNew<svtkGradientFilter> gradientFilter;
  gradientFilter->SetInputConnection(elevationFilter->GetOutputPort());
  gradientFilter->SetInputScalars(svtkDataObject::FIELD_ASSOCIATION_POINTS, "Elevation");
  int oneDCells[] = {
    SVTK_LINE,
    // SVTK_QUADRATIC_EDGE, Derivatives() not implemented
    SVTK_CUBIC_LINE, SVTK_LAGRANGE_CURVE,
    -1 // mark as end
  };
  transformFilter->Update();
  svtkDataSet* output = transformFilter->GetOutput();
  double bounds[6];
  output->GetBounds(bounds);
  elevationFilter->SetLowPoint(bounds[0], 0, 0);
  elevationFilter->SetHighPoint(bounds[1], 0, 0);
  elevationFilter->SetScalarRange(bounds[0], bounds[1]);
  for (i = 0; oneDCells[i + 1] != -1; i++)
  {
    cellTypeSource->SetCellType(oneDCells[i]);
    gradientFilter->Update();
    svtkFloatArray* result = svtkFloatArray::SafeDownCast(
      gradientFilter->GetOutput()->GetPointData()->GetArray("Gradients"));
    double range[2];
    result->GetRange(range, 0);
    if (range[0] < .99 || range[1] > 1.01)
    {
      svtkGenericWarningMacro("Incorrect gradient for cell type " << oneDCells[i]);
      return EXIT_FAILURE;
    }
    for (int j = 1; j < 3; j++)
    {
      result->GetRange(range, j);
      if (range[0] < -.01 || range[1] > .01)
      {
        svtkGenericWarningMacro("Incorrect gradient for cell type " << oneDCells[i]);
        return EXIT_FAILURE;
      }
    }
  }
  int twoDCells[] = {
    SVTK_TRIANGLE, SVTK_QUAD, SVTK_QUADRATIC_TRIANGLE, SVTK_QUADRATIC_QUAD, SVTK_LAGRANGE_TRIANGLE,
    SVTK_LAGRANGE_QUADRILATERAL,
    -1 // mark as end
  };
  cellTypeSource->SetCellType(twoDCells[0]);
  generalTransform->RotateZ(30);
  transformFilter->Update();
  output = transformFilter->GetOutput();
  output->GetBounds(bounds);
  elevationFilter->SetLowPoint(bounds[0], 0, 0);
  elevationFilter->SetHighPoint(bounds[1], 0, 0);
  elevationFilter->SetScalarRange(bounds[0], bounds[1]);
  for (i = 0; twoDCells[i + 1] != -1; i++)
  {
    cellTypeSource->SetCellType(twoDCells[i]);
    gradientFilter->Update();
    svtkFloatArray* result = svtkFloatArray::SafeDownCast(
      gradientFilter->GetOutput()->GetPointData()->GetArray("Gradients"));
    double range[2];
    result->GetRange(range, 0);
    if (range[0] < .99 || range[1] > 1.01)
    {
      svtkGenericWarningMacro("Incorrect gradient for cell type " << twoDCells[i]);
      return EXIT_FAILURE;
    }
    for (int j = 1; j < 3; j++)
    {
      result->GetRange(range, j);
      if (range[0] < -.01 || range[1] > .01)
      {
        svtkGenericWarningMacro("Incorrect gradient for cell type " << twoDCells[i]);
        return EXIT_FAILURE;
      }
    }
  }
  int threeDCells[] = {
    SVTK_TETRA, SVTK_HEXAHEDRON, SVTK_WEDGE, SVTK_PYRAMID, SVTK_QUADRATIC_TETRA,
    SVTK_QUADRATIC_HEXAHEDRON, SVTK_QUADRATIC_WEDGE,
    // SVTK_QUADRATIC_PYRAMID,
    SVTK_LAGRANGE_TETRAHEDRON, SVTK_LAGRANGE_HEXAHEDRON, SVTK_LAGRANGE_WEDGE,
    -1 // mark as end
  };
  cellTypeSource->SetCellType(threeDCells[0]);
  generalTransform->RotateX(20);
  generalTransform->RotateY(40);
  transformFilter->Update();
  output = transformFilter->GetOutput();
  output->GetBounds(bounds);
  elevationFilter->SetLowPoint(bounds[0], 0, 0);
  elevationFilter->SetHighPoint(bounds[1], 0, 0);
  elevationFilter->SetScalarRange(bounds[0], bounds[1]);
  for (i = 0; threeDCells[i] != -1; i++)
  {
    cellTypeSource->SetCellType(threeDCells[i]);
    gradientFilter->Update();
    svtkFloatArray* result = svtkFloatArray::SafeDownCast(
      gradientFilter->GetOutput()->GetPointData()->GetArray("Gradients"));
    double range[2];
    result->GetRange(range, 0);
    if (range[0] < .99 || range[1] > 1.01)
    {
      svtkGenericWarningMacro("Incorrect gradient for cell type " << threeDCells[i]);
      return EXIT_FAILURE;
    }
    for (int j = 1; j < 3; j++)
    {
      result->GetRange(range, j);
      if (range[0] < -.01 || range[1] > .01)
      {
        svtkGenericWarningMacro("Incorrect gradient for cell type " << threeDCells[i]);
        return EXIT_FAILURE;
      }
    }
  }

  return EXIT_SUCCESS;
}
