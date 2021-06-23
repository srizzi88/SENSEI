#include "svtkCellData.h"
#include "svtkCellSizeFilter.h"
#include "svtkCellType.h"
#include "svtkCellTypeSource.h"
#include "svtkDoubleArray.h"
#include "svtkNew.h"
#include "svtkTestUtilities.h"
#include "svtkUnstructuredGrid.h"
#include "svtkUnstructuredGridReader.h"

int CellSizeFilter2(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{

  const int NumberOf1DCellTypes = 5;
  const int OneDCellTypes[NumberOf1DCellTypes] = { SVTK_LINE, SVTK_QUADRATIC_EDGE, SVTK_CUBIC_LINE,
    SVTK_LAGRANGE_CURVE, SVTK_BEZIER_CURVE };
  const int NumberOf2DCellTypes = 8;
  const int TwoDCellTypes[NumberOf2DCellTypes] = { SVTK_TRIANGLE, SVTK_QUAD, SVTK_QUADRATIC_TRIANGLE,
    SVTK_QUADRATIC_QUAD, SVTK_LAGRANGE_TRIANGLE, SVTK_LAGRANGE_QUADRILATERAL, SVTK_BEZIER_TRIANGLE,
    SVTK_BEZIER_QUADRILATERAL };
  const int NumberOf3DCellTypes = 16;
  const int ThreeDCellTypes[NumberOf3DCellTypes] = { SVTK_TETRA, SVTK_HEXAHEDRON, SVTK_WEDGE,
    SVTK_PYRAMID, SVTK_PENTAGONAL_PRISM, SVTK_HEXAGONAL_PRISM, SVTK_QUADRATIC_TETRA,
    SVTK_QUADRATIC_HEXAHEDRON, SVTK_QUADRATIC_WEDGE, SVTK_QUADRATIC_PYRAMID, SVTK_LAGRANGE_TETRAHEDRON,
    SVTK_LAGRANGE_HEXAHEDRON, SVTK_LAGRANGE_WEDGE, SVTK_BEZIER_TETRAHEDRON, SVTK_BEZIER_HEXAHEDRON,
    SVTK_BEZIER_WEDGE };

  for (int i = 0; i < NumberOf1DCellTypes; i++)
  {

    svtkNew<svtkCellTypeSource> cellTypeSource;
    cellTypeSource->SetBlocksDimensions(1, 1, 1);
    cellTypeSource->SetCellOrder(2);
    cellTypeSource->SetCellType(OneDCellTypes[i]);
    svtkNew<svtkCellSizeFilter> filter;
    filter->SetInputConnection(cellTypeSource->GetOutputPort());
    filter->ComputeSumOn();
    filter->Update();

    svtkDoubleArray* length = svtkDoubleArray::SafeDownCast(
      svtkUnstructuredGrid::SafeDownCast(filter->GetOutput())->GetFieldData()->GetArray("Length"));

    if (fabs(length->GetValue(0) - 1.0) > .0001)
    {
      svtkGenericWarningMacro("Wrong length dimension for the cell source type "
        << OneDCellTypes[i] << " supposed to be 1.0 whereas it is " << length->GetValue(0));
      return EXIT_FAILURE;
    }
  }

  for (int i = 0; i < NumberOf2DCellTypes; i++)
  {

    svtkNew<svtkCellTypeSource> cellTypeSource;
    cellTypeSource->SetBlocksDimensions(1, 1, 1);
    cellTypeSource->SetCellOrder(2);
    cellTypeSource->SetCellType(TwoDCellTypes[i]);
    svtkNew<svtkCellSizeFilter> filter;
    filter->SetInputConnection(cellTypeSource->GetOutputPort());
    filter->ComputeSumOn();
    filter->Update();

    svtkDoubleArray* area = svtkDoubleArray::SafeDownCast(
      svtkUnstructuredGrid::SafeDownCast(filter->GetOutput())->GetFieldData()->GetArray("Area"));

    if (fabs(area->GetValue(0) - 1.0) > .0001)
    {
      svtkGenericWarningMacro("Wrong area dimension for the cell source type "
        << TwoDCellTypes[i] << " supposed to be 1.0 whereas it is " << area->GetValue(0));
      return EXIT_FAILURE;
    }
  }

  for (int i = 0; i < NumberOf3DCellTypes; i++)
  {

    svtkNew<svtkCellTypeSource> cellTypeSource;
    cellTypeSource->SetBlocksDimensions(1, 1, 1);
    cellTypeSource->SetCellOrder(3);
    cellTypeSource->SetCellType(ThreeDCellTypes[i]);
    svtkNew<svtkCellSizeFilter> filter;
    filter->SetInputConnection(cellTypeSource->GetOutputPort());
    filter->ComputeSumOn();
    filter->Update();

    svtkDoubleArray* volume = svtkDoubleArray::SafeDownCast(
      svtkUnstructuredGrid::SafeDownCast(filter->GetOutput())->GetFieldData()->GetArray("Volume"));

    if (fabs(volume->GetValue(0) - 1.0) > .0001)
    {
      svtkGenericWarningMacro("Wrong volume dimension for the cell source type "
        << ThreeDCellTypes[i] << " supposed to be 1.0 whereas it is " << volume->GetValue(0));
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
