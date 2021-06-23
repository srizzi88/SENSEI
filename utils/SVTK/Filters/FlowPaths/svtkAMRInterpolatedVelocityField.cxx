#include "svtkAMRInterpolatedVelocityField.h"
#include "svtkObjectFactory.h"
#include "svtkOverlappingAMR.h"
#include "svtkUniformGrid.h"
#include <cassert>
//----------------------------------------------------------------------------
namespace
{
bool Inside(double q[3], double gbounds[6])
{
  if ((q[0] < gbounds[0]) || (q[0] > gbounds[1]) || (q[1] < gbounds[2]) || (q[1] > gbounds[3]) ||
    (q[2] < gbounds[4]) || (q[2] > gbounds[5]))
  {
    return false;
  }
  else
  {
    return true;
  }
}

bool FindInLevel(double q[3], svtkOverlappingAMR* amrds, int level, unsigned int& gridId)
{
  for (unsigned int i = 0; i < amrds->GetNumberOfDataSets(level); i++)
  {
    double gbounds[6];
    amrds->GetBounds(level, i, gbounds);
    bool inside = Inside(q, gbounds);
    if (inside)
    {
      gridId = i;
      return true;
    }
  }
  return false;
}

};

bool svtkAMRInterpolatedVelocityField::FindGrid(
  double q[3], svtkOverlappingAMR* amrds, unsigned int& level, unsigned int& gridId)
{
  if (!FindInLevel(q, amrds, 0, gridId))
  {
    return false;
  }

  unsigned int maxLevels = amrds->GetNumberOfLevels();
  for (level = 0; level < maxLevels; level++)
  {
    unsigned int n;
    unsigned int* children = amrds->GetChildren(level, gridId, n);
    if (children == nullptr)
    {
      break;
    }
    unsigned int i;
    for (i = 0; i < n; i++)
    {
      double bb[6];
      amrds->GetBounds(level + 1, children[i], bb);
      if (Inside(q, bb))
      {
        gridId = children[i];
        break;
      }
    }
    if (i >= n)
    {
      break;
    }
  }
  return true;
}
svtkStandardNewMacro(svtkAMRInterpolatedVelocityField);

svtkAMRInterpolatedVelocityField::svtkAMRInterpolatedVelocityField()
{
  this->Weights = new double[8];
  this->AmrDataSet = nullptr;
  this->LastLevel = this->LastId = -1;
}

svtkAMRInterpolatedVelocityField::~svtkAMRInterpolatedVelocityField()
{
  delete[] this->Weights;
  this->Weights = nullptr;
}

void svtkAMRInterpolatedVelocityField::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void svtkAMRInterpolatedVelocityField::SetAMRData(svtkOverlappingAMR* amrds)
{
  this->AmrDataSet = amrds;
}

int svtkAMRInterpolatedVelocityField::FunctionValues(double* x, double* f)
{
  if (this->LastDataSet && this->FunctionValues(this->LastDataSet, x, f))
  {
    return 1;
  }

  // Either we do not know which data set it is, or existing LastDataSet does not contain x
  // In any case, set LastDataSet to nullptr and try to find a new one
  this->LastDataSet = nullptr;
  this->LastCellId = -1;

  this->LastLevel = -1;
  this->LastId = -1;
  unsigned int level, gridId;
  if (!FindGrid(x, this->AmrDataSet, level, gridId))
  {
    return 0;
  }
  this->LastLevel = level;
  this->LastId = gridId;

  svtkDataSet* ds = this->AmrDataSet->GetDataSet(level, gridId);
  if (!ds)
  {
    return 0;
  }
  if (!this->FunctionValues(ds, x, f))
  {
    return 0;
  }

  this->LastDataSet = ds;
  return 1;
}

bool svtkAMRInterpolatedVelocityField::SetLastDataSet(int level, int id)
{
  this->LastLevel = level;
  this->LastId = id;
  this->LastDataSet = this->AmrDataSet->GetDataSet(level, id);
  return this->LastDataSet != nullptr;
}

void svtkAMRInterpolatedVelocityField::SetLastCellId(svtkIdType, int)
{
  svtkWarningMacro("Calling SetLastCellId has no effect");
}

bool svtkAMRInterpolatedVelocityField::GetLastDataSetLocation(unsigned int& level, unsigned int& id)
{
  if (this->LastLevel < 0)
  {
    return false;
  }

  level = static_cast<unsigned int>(this->LastLevel);
  id = static_cast<unsigned int>(this->LastId);
  return true;
}
