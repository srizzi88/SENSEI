/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExporter
 * @brief   abstract class to write a scene to a file
 *
 * svtkExporter is an abstract class that exports a scene to a file. It
 * is very similar to svtkWriter except that a writer only writes out
 * the geometric and topological data for an object, where an exporter
 * can write out material properties, lighting, camera parameters etc.
 * The concrete subclasses of this class may not write out all of this
 * information. For example svtkOBJExporter writes out Wavefront obj files
 * which do not include support for camera parameters.
 *
 * svtkExporter provides the convenience methods StartWrite() and EndWrite().
 * These methods are executed before and after execution of the Write()
 * method. You can also specify arguments to these methods.
 * This class defines SetInput and GetInput methods which take or return
 * a svtkRenderWindow.
 * @warning
 * Every subclass of svtkExporter must implement a WriteData() method.
 *
 * @sa
 * svtkOBJExporter svtkRenderWindow svtkWriter
 */

#ifndef svtkExporter_h
#define svtkExporter_h

#include "svtkIOExportModule.h" // For export macro
#include "svtkObject.h"
class svtkRenderWindow;
class svtkRenderer;

class SVTKIOEXPORT_EXPORT svtkExporter : public svtkObject
{
public:
  svtkTypeMacro(svtkExporter, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Write data to output. Method executes subclasses WriteData() method, as
   * well as StartWrite() and EndWrite() methods.
   */
  virtual void Write();

  /**
   * Convenient alias for Write() method.
   */
  void Update();

  //@{
  /**
   * Set/Get the rendering window that contains the scene to be written.
   */
  virtual void SetRenderWindow(svtkRenderWindow*);
  svtkGetObjectMacro(RenderWindow, svtkRenderWindow);
  //@}

  //@{
  /**
   * Set/Get the renderer that contains actors to be written.
   * If it is set to nullptr (by default), then in most subclasses
   * the behavior is to only export actors of the first renderer.
   * In some subclasses, if ActiveRenderer is nullptr then
   * actors of all renderers will be exported.
   * The renderer must be in the renderer collection of the specified
   * RenderWindow.
   * \sa SetRenderWindow()
   */
  virtual void SetActiveRenderer(svtkRenderer*);
  svtkGetObjectMacro(ActiveRenderer, svtkRenderer);
  //@}

  //@{
  /**
   * These methods are provided for backward compatibility. Will disappear
   * soon.
   */
  void SetInput(svtkRenderWindow* renWin) { this->SetRenderWindow(renWin); }
  svtkRenderWindow* GetInput() { return this->GetRenderWindow(); }
  //@}

  /**
   * Specify a function to be called before data is written.  Function will
   * be called with argument provided.
   */
  void SetStartWrite(void (*f)(void*), void* arg);

  /**
   * Specify a function to be called after data is written.
   * Function will be called with argument provided.
   */
  void SetEndWrite(void (*f)(void*), void* arg);

  /**
   * Set the arg delete method. This is used to free user memory.
   */
  void SetStartWriteArgDelete(void (*f)(void*));

  /**
   * Set the arg delete method. This is used to free user memory.
   */
  void SetEndWriteArgDelete(void (*f)(void*));

  /**
   * Returns the MTime also considering the RenderWindow.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkExporter();
  ~svtkExporter() override;

  svtkRenderWindow* RenderWindow;
  svtkRenderer* ActiveRenderer;
  virtual void WriteData() = 0;

  void (*StartWrite)(void*);
  void (*StartWriteArgDelete)(void*);
  void* StartWriteArg;
  void (*EndWrite)(void*);
  void (*EndWriteArgDelete)(void*);
  void* EndWriteArg;

private:
  svtkExporter(const svtkExporter&) = delete;
  void operator=(const svtkExporter&) = delete;
};

#endif
