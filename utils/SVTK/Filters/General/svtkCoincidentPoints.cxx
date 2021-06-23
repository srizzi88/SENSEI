/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCoincidentPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "svtkCoincidentPoints.h"

#include "svtkIdList.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPoints.h"
#include "svtkSmartPointer.h"

#include <map>
#include <set>
#include <vector>

//----------------------------------------------------------------------------
// svtkCoincidentPoints::implementation

class svtkCoincidentPoints::implementation
{
public:
  implementation() { this->TraversalIterator = this->CoordMap.end(); }

  ~implementation() { this->CoordMap.clear(); }

  struct Coord
  {
    double coord[3];

    Coord()
    {
      this->coord[0] = -1.0;
      this->coord[1] = -1.0;
      this->coord[2] = -1.0;
    }
    Coord(const Coord& src)
    {
      this->coord[0] = src.coord[0];
      this->coord[1] = src.coord[1];
      this->coord[2] = src.coord[2];
    }
    Coord(const double src[3])
    {
      this->coord[0] = src[0];
      this->coord[1] = src[1];
      this->coord[2] = src[2];
    }
    Coord(const double x, const double y, const double z)
    {
      this->coord[0] = x;
      this->coord[1] = y;
      this->coord[2] = z;
    }

    inline bool operator<(const Coord& other) const
    {
      return this->coord[0] < other.coord[0] ||
        (this->coord[0] == other.coord[0] &&
          (this->coord[1] < other.coord[1] ||
            (this->coord[1] == other.coord[1] && this->coord[2] < other.coord[2])));
    }
  };

  typedef std::map<Coord, svtkSmartPointer<svtkIdList> >::iterator MapCoordIter;

  svtkCoincidentPoints* Self;

  std::map<Coord, svtkSmartPointer<svtkIdList> > CoordMap;
  std::map<svtkIdType, svtkIdType> CoincidenceMap;
  MapCoordIter TraversalIterator;
};

//----------------------------------------------------------------------------
// svtkCoincidentPoints

svtkStandardNewMacro(svtkCoincidentPoints);

svtkCoincidentPoints::svtkCoincidentPoints()
{
  this->Implementation = new implementation();
  this->Implementation->Self = this;
}

svtkCoincidentPoints::~svtkCoincidentPoints()
{
  delete this->Implementation;
}

void svtkCoincidentPoints::Clear()
{
  this->Implementation->CoordMap.clear();
  this->Implementation->CoincidenceMap.clear();
}

void svtkCoincidentPoints::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  // os << indent << "MaximumDepth: " << this->MaximumDepth << "\n";
}

void svtkCoincidentPoints::AddPoint(svtkIdType Id, const double point[3])
{
  implementation::Coord coord(point);
  implementation::MapCoordIter mapIter = this->Implementation->CoordMap.find(coord);
  if (mapIter == this->Implementation->CoordMap.end())
  {
    svtkSmartPointer<svtkIdList> idSet = svtkSmartPointer<svtkIdList>::New();
    idSet->InsertNextId(Id);
    this->Implementation->CoordMap[coord] = idSet;
  }
  else
  {
    (*mapIter).second->InsertNextId(Id);
  }
}

svtkIdList* svtkCoincidentPoints::GetCoincidentPointIds(const double point[3])
{
  implementation::Coord coord(point);
  implementation::MapCoordIter mapIter = this->Implementation->CoordMap.find(coord);
  if (mapIter == this->Implementation->CoordMap.end())
  {
    return nullptr;
  }

  if ((*mapIter).second->GetNumberOfIds() > 1)
  {
    return (*mapIter).second;
  }
  else
  {
    // No Coincident Points
    return nullptr;
  }
}

void svtkCoincidentPoints::RemoveNonCoincidentPoints()
{
  implementation::MapCoordIter mapIter = this->Implementation->CoordMap.begin();
  while (mapIter != this->Implementation->CoordMap.end())
  {
    if ((*mapIter).second->GetNumberOfIds() <= 1)
    {
      this->Implementation->CoordMap.erase(mapIter++);
    }
    else
    {
      ++mapIter;
    }
  }
}

svtkIdList* svtkCoincidentPoints::GetNextCoincidentPointIds()
{
  svtkIdList* rvalue = nullptr;
  if (this->Implementation->TraversalIterator != this->Implementation->CoordMap.end())
  {
    rvalue = (*this->Implementation->TraversalIterator).second;
    ++this->Implementation->TraversalIterator;
    return rvalue;
  }

  return rvalue;
}

void svtkCoincidentPoints::InitTraversal()
{
  this->Implementation->TraversalIterator = this->Implementation->CoordMap.begin();
}

//----------------------------------------------------------------------------
// svtkSpiralkVertices - calculate points at a regular interval along a parametric
// spiral.
void svtkCoincidentPoints::SpiralPoints(svtkIdType num, svtkPoints* offsets)
{
  int maxIter = 10;
  double pi = svtkMath::Pi();
  double a = 1 / (4 * pi * pi);
  offsets->Initialize();
  offsets->SetNumberOfPoints(num);

  for (svtkIdType i = 0; i < num; i++)
  {
    double d = 2.0 * i / sqrt(3.0);
    // We are looking for points at regular intervals along the parametric spiral
    // x = t*cos(2*pi*t)
    // y = t*sin(2*pi*t)
    // We cannot solve this equation exactly, so we use newton's method.
    // Using an Excel trendline, we find that
    // t = 0.553*d^0.502
    // is an excellent starting point./g
    double t = 0.553 * pow(d, 0.502);
    for (int iter = 0; iter < maxIter; iter++)
    {
      double r = sqrt(t * t + a * a);
      double f = pi * (t * r + a * a * log(t + r)) - d;
      double df = 2 * pi * r;
      t = t - f / df;
    }
    double x = t * cos(2 * pi * t);
    double y = t * sin(2 * pi * t);
    offsets->SetPoint(i, x, y, 0);
  }
}
