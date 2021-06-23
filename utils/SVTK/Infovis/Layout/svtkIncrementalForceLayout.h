/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkScatterPlotMatrix.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkIncrementalForceLayout
 * @brief   incremental force-directed layout.
 *
 *
 * Performs an incremental force-directed layout of a graph. Set the graph
 * then iteratively execute UpdatePositions() to update the vertex positions.
 * Note that this directly modifies the vertex locations in the graph.
 *
 * This layout is modeled after D3's force layout described at
 * https://github.com/mbostock/d3/wiki/Force-Layout
 */

#ifndef svtkIncrementalForceLayout_h
#define svtkIncrementalForceLayout_h

#include "svtkInfovisLayoutModule.h" // For export macro
#include "svtkObject.h"

#include "svtkVector.h" // For vector ivars

class svtkGraph;

class SVTKINFOVISLAYOUT_EXPORT svtkIncrementalForceLayout : public svtkObject
{
public:
  svtkTypeMacro(svtkIncrementalForceLayout, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkIncrementalForceLayout* New();

  //@{
  /**
   * Set the graph to be positioned.
   */
  virtual void SetGraph(svtkGraph* g);
  svtkGetObjectMacro(Graph, svtkGraph);
  //@}

  //@{
  /**
   * Set the id of the vertex that will not move during the simulation.
   * Set to -1 to allow all the vertices to move.
   */
  virtual void SetFixed(svtkIdType fixed);
  svtkGetMacro(Fixed, svtkIdType);
  //@}

  //@{
  /**
   * Set the level of activity in the simulation. Default is 0.1.
   */
  svtkSetMacro(Alpha, float);
  svtkGetMacro(Alpha, float);
  //@}

  //@{
  /**
   * Set the Barnes-Hut threshold for the simulation. Higher values
   * will speed the simulation at the expense of some accuracy.
   * Default is 0.8.
   */
  svtkSetMacro(Theta, float);
  svtkGetMacro(Theta, float);
  //@}

  //@{
  /**
   * Set the charge of each vertex. Higher negative values will repel vertices
   * from each other more strongly. Default is -30.
   */
  svtkSetMacro(Charge, float);
  svtkGetMacro(Charge, float);
  //@}

  //@{
  /**
   * Set the rigitity of links in the simulation. Default is 2.
   */
  svtkSetMacro(Strength, float);
  svtkGetMacro(Strength, float);
  //@}

  //@{
  /**
   * Set the resting distance of each link in scene units, which is equal to
   * pixels when there is no scene scaling. Default is 20.
   */
  svtkSetMacro(Distance, float);
  svtkGetMacro(Distance, float);
  //@}

  //@{
  /**
   * Set the amount of gravitational pull toward the gravity point.
   * Default is 0.01.
   */
  svtkSetMacro(Gravity, float);
  svtkGetMacro(Gravity, float);
  //@}

  //@{
  /**
   * Set the multiplier for scaling down velocity in the simulation, where values closer to 1
   * are more frictionless. Default is 0.95.
   */
  svtkSetMacro(Friction, float);
  svtkGetMacro(Friction, float);
  //@}

  /**
   * Set the gravity point where all vertices will migrate. Generally this
   * should be set to the location in the center of the scene.
   * Default location is (200, 200).
   */
  virtual void SetGravityPoint(const svtkVector2f& point) { this->GravityPoint = point; }
  virtual svtkVector2f GetGravityPoint() { return this->GravityPoint; }

  /**
   * Perform one iteration of the force-directed layout.
   */
  void UpdatePositions();

protected:
  svtkIncrementalForceLayout();
  ~svtkIncrementalForceLayout() override;

  svtkGraph* Graph;
  class Implementation;
  Implementation* Impl;
  svtkIdType Fixed;
  svtkVector2f GravityPoint;
  float Alpha;
  float Theta;
  float Charge;
  float Strength;
  float Distance;
  float Gravity;
  float Friction;

private:
  svtkIncrementalForceLayout(const svtkIncrementalForceLayout&) = delete;
  void operator=(const svtkIncrementalForceLayout&) = delete;
};
#endif
