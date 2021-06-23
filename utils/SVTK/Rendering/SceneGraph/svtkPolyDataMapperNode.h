/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyDataMapperNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPolyDataMapperNode
 * @brief   svtkViewNode specialized for svtkPolyDataMappers
 *
 * State storage and graph traversal for svtkPolyDataMapper/PolyDataMapper and Property
 * Made a choice to merge PolyDataMapper, PolyDataMapper and property together. If there
 * is a compelling reason to separate them we can.
 */

#ifndef svtkPolyDataMapperNode_h
#define svtkPolyDataMapperNode_h

#include "svtkMapperNode.h"
#include "svtkRenderingSceneGraphModule.h" // For export macro

#include <vector> //for results

class svtkActor;
class svtkPolyDataMapper;
class svtkPolyData;

class SVTKRENDERINGSCENEGRAPH_EXPORT svtkPolyDataMapperNode : public svtkMapperNode
{
public:
  static svtkPolyDataMapperNode* New();
  svtkTypeMacro(svtkPolyDataMapperNode, svtkMapperNode);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  typedef struct
  {
    std::vector<unsigned int> vertex_index;
    std::vector<unsigned int> vertex_reverse;
    std::vector<unsigned int> line_index;
    std::vector<unsigned int> line_reverse;
    std::vector<unsigned int> triangle_index;
    std::vector<unsigned int> triangle_reverse;
    std::vector<unsigned int> strip_index;
    std::vector<unsigned int> strip_reverse;
  } svtkPDConnectivity;

protected:
  svtkPolyDataMapperNode();
  ~svtkPolyDataMapperNode() override;

  // Utilities for children
  /**
   * Makes a cleaned up version of the polydata's geometry in which NaN are removed
   * (substituted with neighbor) and the PolyDataMapper's transformation matrix is applied.
   */
  static void TransformPoints(svtkActor* act, svtkPolyData* poly, std::vector<double>& vertices);

  /**
   * Homogenizes the entire polydata using internal CreateXIndexBuffer functions.
   * They flatten the input polydata's Points, Lines, Polys, and Strips contents into
   * the output arrays. The output "index" arrays contain indices into the Points. The
   * output "reverse" array contains indices into the original CellArray.
   */
  static void MakeConnectivity(svtkPolyData* poly, int representation, svtkPDConnectivity& conn);

private:
  svtkPolyDataMapperNode(const svtkPolyDataMapperNode&) = delete;
  void operator=(const svtkPolyDataMapperNode&) = delete;
};

#endif
