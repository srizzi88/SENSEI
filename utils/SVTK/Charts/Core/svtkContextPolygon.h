/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContextPolygon.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkContextPolygon_h
#define svtkContextPolygon_h

#include "svtkChartsCoreModule.h"
#include "svtkType.h"   // For svtkIdType
#include "svtkVector.h" // For svtkVector2f

class svtkTransform2D;
class svtkContextPolygonPrivate;

class SVTKCHARTSCORE_EXPORT svtkContextPolygon
{
public:
  // Description:
  // Creates a new, empty polygon.
  svtkContextPolygon();

  // Description:
  // Creates a new copy of \p polygon.
  svtkContextPolygon(const svtkContextPolygon& polygon);

  // Description:
  // Destroys the polygon.
  ~svtkContextPolygon();

  // Description:
  // Adds a point to the polygon.
  void AddPoint(const svtkVector2f& point);

  // Description:
  // Adds a point to the polygon.
  void AddPoint(float x, float y);

  // Description:
  // Returns the point at index.
  svtkVector2f GetPoint(svtkIdType index) const;

  // Description:
  // Returns the number of points in the polygon.
  svtkIdType GetNumberOfPoints() const;

  // Description:
  // Clears all the points from the polygon.
  void Clear();

  // Description:
  // Returns \c true if the polygon contains \p point.
  bool Contains(const svtkVector2f& point) const;

  // Description:
  // Returns a new polygon with each point transformed by \p transform.
  svtkContextPolygon Transformed(svtkTransform2D* transform) const;

  // Description:
  // Copies the values from \p other to this polygon.
  svtkContextPolygon& operator=(const svtkContextPolygon& other);

private:
  svtkContextPolygonPrivate* const d;
};

#endif // svtkContextPolygon_h
// SVTK-HeaderTest-Exclude: svtkContextPolygon.h
