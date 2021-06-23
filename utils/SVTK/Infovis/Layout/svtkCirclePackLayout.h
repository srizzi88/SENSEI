/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkCirclePackLayout.h

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
 * @class   svtkCirclePackLayout
 * @brief   layout a svtkTree as a circle packing.
 *
 *
 * svtkCirclePackLayout assigns circle shaped regions to each vertex
 * in the tree, creating a circle packing layout.  The data is added
 * as a data array with three components per tuple representing the
 * center and radius of the circle using the format (Xcenter, Ycenter, Radius).
 *
 * This algorithm relies on a helper class to perform the actual layout.
 * This helper class is a subclass of svtkCirclePackLayoutStrategy.
 *
 * An array by default called "size" can be attached to the input tree
 * that specifies the size of each leaf node in the tree.  The filter will
 * calculate the sizes of all interior nodes in the tree based on the sum
 * of the leaf node sizes.  If no "size" array is given in the input svtkTree,
 * a size of 1 is used for all leaf nodes to find the size of the interior nodes.
 *
 * @par Thanks:
 * Thanks to Thomas Otahal from Sandia National Laboratories
 * for help developing this class.
 *
 */

#ifndef svtkCirclePackLayout_h
#define svtkCirclePackLayout_h

#include "svtkInfovisLayoutModule.h" // For export macro
#include "svtkTreeAlgorithm.h"

class svtkCirclePackLayoutStrategy;
class svtkDoubleArray;
class svtkDataArray;
class svtkTree;

class SVTKINFOVISLAYOUT_EXPORT svtkCirclePackLayout : public svtkTreeAlgorithm
{
public:
  static svtkCirclePackLayout* New();

  svtkTypeMacro(svtkCirclePackLayout, svtkTreeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The field name to use for storing the circles for each vertex.
   * The rectangles are stored in a triple float array
   * (Xcenter, Ycenter, Radius).
   * Default name is "circles"
   */
  svtkGetStringMacro(CirclesFieldName);
  svtkSetStringMacro(CirclesFieldName);
  //@}

  /**
   * The array to use for the size of each vertex.
   * Default name is "size".
   */
  virtual void SetSizeArrayName(const char* name)
  {
    this->SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_VERTICES, name);
  }

  //@{
  /**
   * The strategy to use when laying out the tree map.
   */
  svtkGetObjectMacro(LayoutStrategy, svtkCirclePackLayoutStrategy);
  void SetLayoutStrategy(svtkCirclePackLayoutStrategy* strategy);
  //@}

  /**
   * Returns the vertex id that contains pnt (or -1 if no one contains it)
   * pnt[0] is x, and pnt[1] is y.
   * If cinfo[3] is provided, then (Xcenter, Ycenter, Radius) of the circle
   * containing pnt[2] will be returned.
   */
  svtkIdType FindVertex(double pnt[2], double* cinfo = nullptr);

  /**
   * Return the Xcenter, Ycenter, and Radius of the
   * vertex's bounding circle
   */
  void GetBoundingCircle(svtkIdType id, double* cinfo);

  /**
   * Get the modification time of the layout algorithm.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkCirclePackLayout();
  ~svtkCirclePackLayout() override;

  char* CirclesFieldName;
  svtkCirclePackLayoutStrategy* LayoutStrategy;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkCirclePackLayout(const svtkCirclePackLayout&) = delete;
  void operator=(const svtkCirclePackLayout&) = delete;
  void prepareSizeArray(svtkDoubleArray* mySizeArray, svtkTree* tree);
};

#endif
