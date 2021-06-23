// -*- c++ -*-

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkUnstructuredGridVolumeRayCastIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkUnstructuredGridVolumeRayCastIterator
 *
 *
 *
 * svtkUnstructuredGridVolumeRayCastIterator is a superclass for iterating
 * over the intersections of a viewing ray with a group of unstructured
 * cells.  These iterators are created with a
 * svtkUnstructuredGridVolumeRayCastFunction.
 *
 * @sa
 * svtkUnstructuredGridVolumeRayCastFunction
 *
 */

#ifndef svtkUnstructuredGridVolumeRayCastIterator_h
#define svtkUnstructuredGridVolumeRayCastIterator_h

#include "svtkObject.h"
#include "svtkRenderingVolumeModule.h" // For export macro

class svtkIdList;
class svtkDoubleArray;
class svtkDataArray;

class SVTKRENDERINGVOLUME_EXPORT svtkUnstructuredGridVolumeRayCastIterator : public svtkObject
{
public:
  svtkTypeMacro(svtkUnstructuredGridVolumeRayCastIterator, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Initializes the iteration to the start of the ray at the given screen
   * coordinates.
   */
  virtual void Initialize(int x, int y) = 0;

  /**
   * Get the intersections of the next several cells.  The cell ids are
   * stored in \c intersectedCells and the length of each ray segment
   * within the cell is stored in \c intersectionLengths.  The point
   * scalars \c scalars are interpolated and stored in \c nearIntersections
   * and \c farIntersections.  \c intersectedCells, \c intersectionLengths,
   * or \c scalars may be \c NULL to suppress passing the associated
   * information.  The number of intersections actually encountered is
   * returned.  0 is returned if and only if no more intersections are to
   * be found.
   */
  virtual svtkIdType GetNextIntersections(svtkIdList* intersectedCells,
    svtkDoubleArray* intersectionLengths, svtkDataArray* scalars, svtkDataArray* nearIntersections,
    svtkDataArray* farIntersections) = 0;

  //@{
  /**
   * Set/get the bounds of the cast ray (in viewing coordinates).  By
   * default the range is [0,1].
   */
  svtkSetVector2Macro(Bounds, double);
  svtkGetVector2Macro(Bounds, double);
  //@}

  // Descrption:
  // Set/get the maximum number of intersections returned with a call to
  // GetNextIntersections.  Set to 32 by default.
  svtkSetMacro(MaxNumberOfIntersections, svtkIdType);
  svtkGetMacro(MaxNumberOfIntersections, svtkIdType);

protected:
  svtkUnstructuredGridVolumeRayCastIterator();
  ~svtkUnstructuredGridVolumeRayCastIterator() override;

  double Bounds[2];

  svtkIdType MaxNumberOfIntersections;

private:
  svtkUnstructuredGridVolumeRayCastIterator(
    const svtkUnstructuredGridVolumeRayCastIterator&) = delete;
  void operator=(const svtkUnstructuredGridVolumeRayCastIterator&) = delete;
};

#endif // svtkUnstructuredGridRayCastIterator_h
