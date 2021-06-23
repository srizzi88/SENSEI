/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkEllipsoidTensorProbeRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkEllipsoidTensorProbeRepresentation
 * @brief   A concrete implementation of svtkTensorProbeRepresentation that renders tensors as
 * ellipoids.
 *
 * svtkEllipsoidTensorProbeRepresentation is a concrete implementation of
 * svtkTensorProbeRepresentation. It renders tensors as ellipsoids. Locations
 * between two points when probed have the tensors linearly interpolated
 * from the neighboring locations on the polyline.
 *
 * @sa
 * svtkTensorProbeWidget
 */

#ifndef svtkEllipsoidTensorProbeRepresentation_h
#define svtkEllipsoidTensorProbeRepresentation_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkTensorProbeRepresentation.h"

class svtkCellPicker;
class svtkTensorGlyph;
class svtkPolyDataNormals;

class SVTKINTERACTIONWIDGETS_EXPORT svtkEllipsoidTensorProbeRepresentation
  : public svtkTensorProbeRepresentation
{
public:
  static svtkEllipsoidTensorProbeRepresentation* New();

  //@{
  /**
   * Standard methods for instances of this class.
   */
  svtkTypeMacro(svtkEllipsoidTensorProbeRepresentation, svtkTensorProbeRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  void BuildRepresentation() override;
  int RenderOpaqueGeometry(svtkViewport*) override;

  /**
   * Can we pick the tensor glyph at the current cursor pos
   */
  int SelectProbe(int pos[2]) override;

  //@{
  /**
   * See svtkProp for details.
   */
  void GetActors(svtkPropCollection*) override;
  void ReleaseGraphicsResources(svtkWindow*) override;
  //@}

  /*
   * Register internal Pickers within PickingManager
   */
  void RegisterPickers() override;

protected:
  svtkEllipsoidTensorProbeRepresentation();
  ~svtkEllipsoidTensorProbeRepresentation() override;

  // Get the interpolated tensor at the current position
  void EvaluateTensor(double t[9]);

  svtkActor* EllipsoidActor;
  svtkPolyDataMapper* EllipsoidMapper;
  svtkPolyData* TensorSource;
  svtkTensorGlyph* TensorGlypher;
  svtkCellPicker* CellPicker;
  svtkPolyDataNormals* PolyDataNormals;

private:
  svtkEllipsoidTensorProbeRepresentation(const svtkEllipsoidTensorProbeRepresentation&) = delete;
  void operator=(const svtkEllipsoidTensorProbeRepresentation&) = delete;
};

#endif
