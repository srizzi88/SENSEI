/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOSPRayActorNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOSPRayActorNode
 * @brief   links svtkActor and svtkMapper to OSPRay
 *
 * Translates svtkActor/Mapper state into OSPRay rendering calls
 */

#ifndef svtkOSPRayActorNode_h
#define svtkOSPRayActorNode_h

#include "svtkActorNode.h"
#include "svtkRenderingRayTracingModule.h" // For export macro
#include "svtkTimeStamp.h"                 //for mapper changed time
#include "svtkWeakPointer.h"               //also for mapper changed time

class svtkActor;
class svtkCompositeDataDisplayAttributes;
class svtkDataArray;
class svtkInformationDoubleKey;
class svtkInformationIntegerKey;
class svtkInformationObjectBaseKey;
class svtkInformationStringKey;
class svtkMapper;
class svtkPiecewiseFunction;
class svtkPolyData;
class svtkProperty;
class svtkTimeStamp;

class SVTKRENDERINGRAYTRACING_EXPORT svtkOSPRayActorNode : public svtkActorNode
{
public:
  static svtkOSPRayActorNode* New();
  svtkTypeMacro(svtkOSPRayActorNode, svtkActorNode);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Overridden to take into account my renderables time, including
   * mapper and data into mapper inclusive of composite input
   */
  virtual svtkMTimeType GetMTime() override;

  /**
   * When added to the mapper, enables scale array and scale function.
   */
  static svtkInformationIntegerKey* ENABLE_SCALING();

  //@{
  /**
   * Convenience method to set enabled scaling on my renderable.
   */
  static void SetEnableScaling(int value, svtkActor*);
  static int GetEnableScaling(svtkActor*);
  //@}

  /**
   * Name of a point aligned, single component wide, double valued array that,
   * when added to the mapper, will be used to scale each element in the
   * sphere and cylinder representations individually.
   * When not supplied the radius is constant across all elements and
   * is a function of the Mapper's PointSize and LineWidth.
   */
  static svtkInformationStringKey* SCALE_ARRAY_NAME();

  /**
   * Convenience method to set a scale array on my renderable.
   */
  static void SetScaleArrayName(const char* scaleArrayName, svtkActor*);

  /**
   * A piecewise function for values from the scale array that alters the resulting
   * radii arbitrarily
   */
  static svtkInformationObjectBaseKey* SCALE_FUNCTION();

  /**
   * Convenience method to set a scale function on my renderable.
   */
  static void SetScaleFunction(svtkPiecewiseFunction* scaleFunction, svtkActor*);

  /**
   * Indicates that the actor acts as a light emitting object.
   */
  static svtkInformationDoubleKey* LUMINOSITY();

  //@{
  /**
   * Convenience method to set luminosity on my renderable.
   */
  static void SetLuminosity(double value, svtkProperty*);
  static double GetLuminosity(svtkProperty*);
  //@}

protected:
  svtkOSPRayActorNode();
  ~svtkOSPRayActorNode();

private:
  svtkOSPRayActorNode(const svtkOSPRayActorNode&) = delete;
  void operator=(const svtkOSPRayActorNode&) = delete;

  svtkWeakPointer<svtkMapper> LastMapper;
  svtkTimeStamp MapperChangedTime;
};
#endif
