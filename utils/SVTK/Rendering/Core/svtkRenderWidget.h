/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef svtkRenderWidget_h
#define svtkRenderWidget_h

#include "svtkNew.h" // For member variables.
#include "svtkObject.h"
#include "svtkRenderingCoreModule.h" // For export macro
#include "svtkVector.h"              // For member variables.
#include <string>                   // For member variables.

class svtkAbstractInteractionDevice;
class svtkAbstractRenderDevice;

class SVTKRENDERINGCORE_EXPORT svtkRenderWidget : public svtkObject
{
public:
  svtkTypeMacro(svtkRenderWidget, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkRenderWidget* New();

  /**
   * @brief Set the widget position in screen coordinates.
   * @param pos The position of the widget in screen coordinates.
   */
  void SetPosition(const svtkVector2i& pos);

  /**
   * @brief Get the widget position in screen coordinates.
   * @return The position of the widget in screen coordinates, default of 0, 0.
   */
  svtkVector2i GetPosition() const { return this->Position; }

  /**
   * @brief Set the widget size in screen coordinates.
   * @param size The width and height of the widget in screen coordinates
   */
  void SetSize(const svtkVector2i& size);

  /**
   * @brief Get the widget size in screen coordinates.
   * @return The width and height of the widget in screen coordinates, default
   * of 300x300.
   */
  svtkVector2i GetSize() const { return this->Size; }

  /**
   * @brief Set the name of the widget.
   * @param name The name to set to the window.
   */
  void SetName(const std::string& name);

  /**
   * @brief Get the name of the widget.
   * @return The current name of the widget.
   */
  std::string GetName() const { return this->Name; }

  /**
   * @brief Render everything in the current widget.
   */
  virtual void Render();

  /**
   * @brief Make the widget's context current, this will defer to the OS
   * specific methods, and calls should be kept to a minimum as they are quite
   * expensive.
   */
  virtual void MakeCurrent();

  void Initialize();
  void Start();

protected:
  svtkRenderWidget();
  ~svtkRenderWidget() override;

  svtkVector2i Position; // Position of the widget in screen coordinates.
  svtkVector2i Size;     // Position of the widget in screen coordinates.
  std::string Name;     // The name of the widget.

  svtkNew<svtkAbstractInteractionDevice> InteractionDevice; // Interaction device.
  svtkNew<svtkAbstractRenderDevice> RenderDevice;           // Render device target.

private:
  svtkRenderWidget(const svtkRenderWidget&) = delete;
  void operator=(const svtkRenderWidget&) = delete;
};

#endif
