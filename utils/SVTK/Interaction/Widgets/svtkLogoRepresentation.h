/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLogoRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLogoRepresentation
 * @brief   represent the svtkLogoWidget
 *
 *
 * This class provides support for interactively positioning a logo. A logo
 * is defined by an instance of svtkImage. The properties of the image,
 * including transparency, can be set with an instance of svtkProperty2D. To
 * position the logo, use the superclass's Position and Position2 coordinates.
 *
 * @sa
 * svtkLogoWidget
 */

#ifndef svtkLogoRepresentation_h
#define svtkLogoRepresentation_h

#include "svtkBorderRepresentation.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkImageData;
class svtkImageProperty;
class svtkTexture;
class svtkPolyData;
class svtkPoionts;
class svtkPolyDataMapper2D;
class svtkTexturedActor2D;
class svtkProperty2D;

class SVTKINTERACTIONWIDGETS_EXPORT svtkLogoRepresentation : public svtkBorderRepresentation
{
public:
  /**
   * Instantiate this class.
   */
  static svtkLogoRepresentation* New();

  //@{
  /**
   * Standard SVTK class methods.
   */
  svtkTypeMacro(svtkLogoRepresentation, svtkBorderRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify/retrieve the image to display in the balloon.
   */
  virtual void SetImage(svtkImageData* img);
  svtkGetObjectMacro(Image, svtkImageData);
  //@}

  //@{
  /**
   * Set/get the image property (relevant only if an image is shown).
   */
  virtual void SetImageProperty(svtkProperty2D* p);
  svtkGetObjectMacro(ImageProperty, svtkProperty2D);
  //@}

  /**
   * Satisfy the superclasses' API.
   */
  void BuildRepresentation() override;

  //@{
  /**
   * These methods are necessary to make this representation behave as
   * a svtkProp.
   */
  void GetActors2D(svtkPropCollection* pc) override;
  void ReleaseGraphicsResources(svtkWindow*) override;
  int RenderOverlay(svtkViewport*) override;
  //@}

protected:
  svtkLogoRepresentation();
  ~svtkLogoRepresentation() override;

  // data members
  svtkImageData* Image;
  svtkProperty2D* ImageProperty;

  // Represent the image
  svtkTexture* Texture;
  svtkPoints* TexturePoints;
  svtkPolyData* TexturePolyData;
  svtkPolyDataMapper2D* TextureMapper;
  svtkTexturedActor2D* TextureActor;

  // Helper methods
  virtual void AdjustImageSize(double o[2], double borderSize[2], double imageSize[2]);

private:
  svtkLogoRepresentation(const svtkLogoRepresentation&) = delete;
  void operator=(const svtkLogoRepresentation&) = delete;
};

#endif
