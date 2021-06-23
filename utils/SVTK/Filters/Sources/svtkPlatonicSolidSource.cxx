/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlatonicSolidSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPlatonicSolidSource.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"

svtkStandardNewMacro(svtkPlatonicSolidSource);

// Wrapping this in namespaces because the short names (a, b, c, etc) are
// throwing warnings on MSVC when inlined methods in svtkGenericDataArray are
// being used ('warning C4459: declaration of 'c' hides global declaration')
namespace
{
namespace svtkPlatonicSolidSourceDetail
{
// The geometry and topology of each solid. Solids are centered at
// the origin with radius 1.0.
// The golden ration phi = (1+sqrt(5))/2=1.61803398875 enters into many
// of these values.
static double TetraPoints[] = {
  1.0, 1.0, 1.0,   //
  -1.0, 1.0, -1.0, //
  1.0, -1.0, -1.0, //
  -1.0, -1.0, 1.0  //
};
static svtkIdType TetraVerts[] = {
  0, 1, 2, //
  1, 3, 2, //
  0, 2, 3, //
  0, 3, 1  //
};

static double CubePoints[] = {
  -1.0, -1.0, -1.0, //
  1.0, -1.0, -1.0,  //
  1.0, 1.0, -1.0,   //
  -1.0, 1.0, -1.0,  //
  -1.0, -1.0, 1.0,  //
  1.0, -1.0, 1.0,   //
  1.0, 1.0, 1.0,    //
  -1.0, 1.0, 1.0    //
};
static svtkIdType CubeVerts[] = {
  0, 1, 5, 4, //
  0, 4, 7, 3, //
  4, 5, 6, 7, //
  3, 7, 6, 2, //
  1, 2, 6, 5, //
  0, 3, 2, 1  //
};

static double OctPoints[] = {
  -1.0, -1.0, 0.0,            //
  1.0, -1.0, 0.0,             //
  1.0, 1.0, 0.0,              //
  -1.0, 1.0, 0.0,             //
  0.0, 0.0, -1.4142135623731, //
  0.0, 0.0, 1.4142135623731   //
};
static svtkIdType OctVerts[] = {
  4, 1, 0, //
  4, 2, 1, //
  4, 3, 2, //
  4, 0, 3, //
  0, 1, 5, //
  1, 2, 5, //
  2, 3, 5, //
  3, 0, 5  //
};

static double a_0 = 0.61803398875;
static double b = 0.381966011250;
static double DodePoints[] = {
  b, 0, 1,          //
  -b, 0, 1,         //
  b, 0, -1,         //
  -b, 0, -1,        //
  0, 1, -b,         //
  0, 1, b,          //
  0, -1, -b,        //
  0, -1, b,         //
  1, b, 0,          //
  1, -b, 0,         //
  -1, b, 0,         //
  -1, -b, 0,        //
  -a_0, a_0, a_0,   //
  a_0, -a_0, a_0,   //
  -a_0, -a_0, -a_0, //
  a_0, a_0, -a_0,   //
  a_0, a_0, a_0,    //
  -a_0, a_0, -a_0,  //
  -a_0, -a_0, a_0,  //
  a_0, -a_0, -a_0   //
};
static svtkIdType DodeVerts[] = {
  0, 16, 5, 12, 1,   //
  1, 18, 7, 13, 0,   //
  2, 19, 6, 14, 3,   //
  3, 17, 4, 15, 2,   //
  4, 5, 16, 8, 15,   //
  5, 4, 17, 10, 12,  //
  6, 7, 18, 11, 14,  //
  7, 6, 19, 9, 13,   //
  8, 16, 0, 13, 9,   //
  9, 19, 2, 15, 8,   //
  10, 17, 3, 14, 11, //
  11, 18, 1, 12, 10  //
};

static double c = 0.5;
static double d = 0.30901699;
static double IcosaPoints[] = {
  0.0, d, -c,  //
  0.0, d, c,   //
  0.0, -d, c,  //
  -d, c, 0.0,  //
  -d, -c, 0.0, //
  d, c, 0.0,   //
  d, -c, 0.0,  //
  0.0, -d, -c, //
  c, 0.0, d,   //
  -c, 0.0, d,  //
  -c, 0.0, -d, //
  c, 0.0, -d   //
};
static svtkIdType IcosaVerts[] = {
  0, 5, 3,  //
  1, 3, 5,  //
  1, 2, 9,  //
  1, 8, 2,  //
  0, 7, 11, //
  0, 10, 7, //
  2, 6, 4,  //
  7, 4, 6,  //
  3, 9, 10, //
  4, 10, 9, //
  5, 11, 8, //
  6, 8, 11, //
  1, 9, 3,  //
  1, 5, 8,  //
  0, 3, 10, //
  0, 11, 5, //
  7, 10, 4, //
  7, 6, 11, //
  2, 4, 9,  //
  2, 8, 6   //
};
} // end namespace detail
} // end anon namespace

svtkPlatonicSolidSource::svtkPlatonicSolidSource()
{
  this->SolidType = SVTK_SOLID_TETRAHEDRON;
  this->OutputPointsPrecision = SINGLE_PRECISION;
  this->SetNumberOfInputPorts(0);
}

int svtkPlatonicSolidSource::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // get the info object
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  int i;
  double *pptr, *solidPoints = nullptr, solidScale = 1.0;
  svtkIdType *cptr, numPts = 0, numCells = 0, cellSize = 0, *solidVerts = nullptr;

  svtkDebugMacro(<< "Creating Platonic solid");

  // Based on type, select correct connectivity and point arrays
  //
  switch (this->SolidType)
  {
    case SVTK_SOLID_TETRAHEDRON:
      numPts = 4;
      cellSize = 3;
      numCells = 4;
      solidPoints = svtkPlatonicSolidSourceDetail::TetraPoints;
      solidVerts = svtkPlatonicSolidSourceDetail::TetraVerts;
      solidScale = 1.0 / sqrt(3.0);
      break;

    case SVTK_SOLID_CUBE:
      numPts = 8;
      cellSize = 4;
      numCells = 6;
      solidPoints = svtkPlatonicSolidSourceDetail::CubePoints;
      solidVerts = svtkPlatonicSolidSourceDetail::CubeVerts;
      solidScale = 1.0 / sqrt(3.0);
      break;

    case SVTK_SOLID_OCTAHEDRON:
      numPts = 6;
      cellSize = 3;
      numCells = 8;
      solidPoints = svtkPlatonicSolidSourceDetail::OctPoints;
      solidVerts = svtkPlatonicSolidSourceDetail::OctVerts;
      solidScale = 1.0 / sqrt(2.0);
      break;

    case SVTK_SOLID_ICOSAHEDRON:
      numPts = 12;
      cellSize = 3;
      numCells = 20;
      solidPoints = svtkPlatonicSolidSourceDetail::IcosaPoints;
      solidVerts = svtkPlatonicSolidSourceDetail::IcosaVerts;
      solidScale = 1.0 / 0.58778524999243;
      break;

    case SVTK_SOLID_DODECAHEDRON:
      numPts = 20;
      cellSize = 5;
      numCells = 12;
      solidPoints = svtkPlatonicSolidSourceDetail::DodePoints;
      solidVerts = svtkPlatonicSolidSourceDetail::DodeVerts;
      solidScale = 1.0 / 1.070466269319;
      break;
  }

  // Create the solids
  //
  svtkPoints* pts = svtkPoints::New();

  // Set the desired precision for the points in the output.
  if (this->OutputPointsPrecision == svtkAlgorithm::DOUBLE_PRECISION)
  {
    pts->SetDataType(SVTK_DOUBLE);
  }
  else
  {
    pts->SetDataType(SVTK_FLOAT);
  }

  pts->SetNumberOfPoints(numPts);
  svtkCellArray* polys = svtkCellArray::New();
  polys->AllocateEstimate(numCells, cellSize);
  svtkIntArray* colors = svtkIntArray::New();
  colors->SetNumberOfComponents(1);
  colors->SetNumberOfTuples(numCells);

  // Points
  for (i = 0, pptr = solidPoints; i < numPts; i++, pptr += 3)
  {
    pts->SetPoint(i, solidScale * (pptr[0]), solidScale * (pptr[1]), solidScale * (pptr[2]));
  }

  // Cells
  for (i = 0, cptr = solidVerts; i < numCells; i++, cptr += cellSize)
  {
    polys->InsertNextCell(cellSize, cptr);
    colors->SetTuple1(i, i);
  }

  // Assemble the output
  output->SetPoints(pts);
  output->SetPolys(polys);
  int idx = output->GetCellData()->AddArray(colors);
  output->GetCellData()->SetActiveAttribute(idx, svtkDataSetAttributes::SCALARS);

  pts->Delete();
  polys->Delete();
  colors->Delete();

  return 1;
}

void svtkPlatonicSolidSource::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Solid Type: "
     << "\n";
  if (this->SolidType == SVTK_SOLID_TETRAHEDRON)
  {
    os << "Tetrahedron\n";
  }
  else if (this->SolidType == SVTK_SOLID_CUBE)
  {
    os << "Cube\n";
  }
  else if (this->SolidType == SVTK_SOLID_OCTAHEDRON)
  {
    os << "Octahedron\n";
  }
  else if (this->SolidType == SVTK_SOLID_ICOSAHEDRON)
  {
    os << "Icosahedron\n";
  }
  else // if ( this->SolidType == SVTK_SOLID_DODECAHEDRON )
  {
    os << "Dodecahedron\n";
  }

  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
