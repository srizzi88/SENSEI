/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCoincidentPoints.h

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
/**
 * @class   svtkCoincidentPoints
 * @brief   contains an octree of labels
 *
 *
 * This class provides a collection of points that is organized such that
 * each coordinate is stored with a set of point id's of points that are
 * all coincident.
 */

#ifndef svtkCoincidentPoints_h
#define svtkCoincidentPoints_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkObject.h"

class svtkIdList;
class svtkPoints;

class SVTKFILTERSGENERAL_EXPORT svtkCoincidentPoints : public svtkObject
{
public:
  static svtkCoincidentPoints* New();
  svtkTypeMacro(svtkCoincidentPoints, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Accumulates a set of Ids in a map where the point coordinate
   * is the key. All Ids in a given map entry are thus coincident.
   * @param Id - a unique Id for the given \a point that will be stored in an svtkIdList.
   * @param[in] point - the point coordinate that we will store in the map to test if any other
   * points are coincident with it.
   */
  void AddPoint(svtkIdType Id, const double point[3]);

  /**
   * Retrieve the list of point Ids that are coincident with the given \a point.
   * @param[in] point - the coordinate of coincident points we want to retrieve.
   */
  svtkIdList* GetCoincidentPointIds(const double point[3]);

  /**
   * Used to iterate the sets of coincident points within the map.
   * InitTraversal must be called first or nullptr will always be returned.
   */
  svtkIdList* GetNextCoincidentPointIds();

  /**
   * Initialize iteration to the beginning of the coincident point map.
   */
  void InitTraversal();

  /**
   * Iterate through all added points and remove any entries that have
   * no coincident points (only a single point Id).
   */
  void RemoveNonCoincidentPoints();

  /**
   * Clear the maps for reuse. This should be called if the caller
   * might reuse this class (another executive pass for instance).
   */
  void Clear();

  class implementation;
  implementation* GetImplementation() { return this->Implementation; }

  /**
   * Calculate \a num points, at a regular interval, along a parametric
   * spiral. Note this spiral is only in two dimensions having a constant
   * z value.
   */
  static void SpiralPoints(svtkIdType num, svtkPoints* offsets);

protected:
  svtkCoincidentPoints();
  ~svtkCoincidentPoints() override;

private:
  svtkCoincidentPoints(const svtkCoincidentPoints&) = delete;
  void operator=(const svtkCoincidentPoints&) = delete;

  implementation* Implementation;

  friend class implementation;
};

#endif // svtkCoincidentPoints_h
