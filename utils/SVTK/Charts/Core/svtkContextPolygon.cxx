/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContextPolygon.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkContextPolygon.h"

#include <algorithm>
#include <vector>

#include "svtkTransform2D.h"

//-----------------------------------------------------------------------------
class svtkContextPolygonPrivate
{
public:
  std::vector<svtkVector2f> points;
};

//-----------------------------------------------------------------------------
svtkContextPolygon::svtkContextPolygon()
  : d(new svtkContextPolygonPrivate)
{
}

//-----------------------------------------------------------------------------
svtkContextPolygon::svtkContextPolygon(const svtkContextPolygon& polygon)
  : d(new svtkContextPolygonPrivate)
{
  d->points = polygon.d->points;
}

//-----------------------------------------------------------------------------
svtkContextPolygon::~svtkContextPolygon()
{
  delete d;
}

//-----------------------------------------------------------------------------
void svtkContextPolygon::AddPoint(const svtkVector2f& point)
{
  d->points.push_back(point);
}

//-----------------------------------------------------------------------------
void svtkContextPolygon::AddPoint(float x, float y)
{
  this->AddPoint(svtkVector2f(x, y));
}

//-----------------------------------------------------------------------------
svtkVector2f svtkContextPolygon::GetPoint(svtkIdType index) const
{
  return d->points[index];
}

//-----------------------------------------------------------------------------
svtkIdType svtkContextPolygon::GetNumberOfPoints() const
{
  return static_cast<svtkIdType>(d->points.size());
}

//-----------------------------------------------------------------------------
void svtkContextPolygon::Clear()
{
  d->points.clear();
}

//-----------------------------------------------------------------------------
bool svtkContextPolygon::Contains(const svtkVector2f& point) const
{
  float x = point.GetX();
  float y = point.GetY();

  // http://en.wikipedia.org/wiki/Point_in_polygon RayCasting method
  // shooting the ray along the x axis
  bool inside = false;
  float xintersection;
  for (size_t i = 0; i < d->points.size(); i++)
  {
    const svtkVector2f& p1 = d->points[i];
    const svtkVector2f& p2 = d->points[(i + 1) % d->points.size()];

    if (y > std::min(p1.GetY(), p2.GetY()) && y <= std::max(p1.GetY(), p2.GetY()) &&
      p1.GetY() != p2.GetY())
    {
      if (x <= std::max(p1.GetX(), p2.GetX()))
      {
        xintersection =
          (y - p1.GetY()) * (p2.GetX() - p1.GetX()) / (p2.GetY() - p1.GetY()) + p1.GetX();
        if (p1.GetX() == p2.GetX() || x <= xintersection)
        {
          // each time we intersect we switch if we are in side or not
          inside = !inside;
        }
      }
    }
  }

  return inside;
}

//-----------------------------------------------------------------------------
svtkContextPolygon svtkContextPolygon::Transformed(svtkTransform2D* transform) const
{
  svtkContextPolygon transformed;
  transformed.d->points.resize(d->points.size());
  transform->TransformPoints(reinterpret_cast<float*>(&d->points[0]),
    reinterpret_cast<float*>(&transformed.d->points[0]), static_cast<int>(d->points.size()));
  return transformed;
}

//-----------------------------------------------------------------------------
svtkContextPolygon& svtkContextPolygon::operator=(const svtkContextPolygon& other)
{
  if (this != &other)
  {
    d->points = other.d->points;
  }

  return *this;
}
