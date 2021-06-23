/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkView.h

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
 * @class   svtkView
 * @brief   The superclass for all views.
 *
 *
 * svtkView is the superclass for views.  A view is generally an area of an
 * application's canvas devoted to displaying one or more SVTK data objects.
 * Associated representations (subclasses of svtkDataRepresentation) are
 * responsible for converting the data into a displayable format.  These
 * representations are then added to the view.
 *
 * For views which display only one data object at a time you may set a
 * data object or pipeline connection directly on the view itself (e.g.
 * svtkGraphLayoutView, svtkLandscapeView, svtkTreeMapView).
 * The view will internally create a svtkDataRepresentation for the data.
 *
 * A view has the concept of linked selection.  If the same data is displayed
 * in multiple views, their selections may be linked by setting the same
 * svtkAnnotationLink on their representations (see svtkDataRepresentation).
 */

#ifndef svtkView_h
#define svtkView_h

#include "svtkObject.h"
#include "svtkViewsCoreModule.h" // For export macro

class svtkAlgorithmOutput;
class svtkCommand;
class svtkDataObject;
class svtkDataRepresentation;
class svtkSelection;
class svtkViewTheme;

class SVTKVIEWSCORE_EXPORT svtkView : public svtkObject
{
public:
  static svtkView* New();
  svtkTypeMacro(svtkView, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Adds the representation to the view.
   */
  void AddRepresentation(svtkDataRepresentation* rep);

  /**
   * Set the representation to the view.
   */
  void SetRepresentation(svtkDataRepresentation* rep);

  /**
   * Convenience method which creates a simple representation with the
   * connection and adds it to the view.
   * Returns the representation internally created.
   * NOTE: The returned representation pointer is not reference-counted,
   * so you MUST call Register() on the representation if you want to
   * keep a reference to it.
   */
  svtkDataRepresentation* AddRepresentationFromInputConnection(svtkAlgorithmOutput* conn);

  /**
   * Convenience method which sets the representation with the
   * connection and adds it to the view.
   * Returns the representation internally created.
   * NOTE: The returned representation pointer is not reference-counted,
   * so you MUST call Register() on the representation if you want to
   * keep a reference to it.
   */
  svtkDataRepresentation* SetRepresentationFromInputConnection(svtkAlgorithmOutput* conn);

  /**
   * Convenience method which creates a simple representation with the
   * specified input and adds it to the view.
   * NOTE: The returned representation pointer is not reference-counted,
   * so you MUST call Register() on the representation if you want to
   * keep a reference to it.
   */
  svtkDataRepresentation* AddRepresentationFromInput(svtkDataObject* input);

  /**
   * Convenience method which sets the representation to the
   * specified input and adds it to the view.
   * NOTE: The returned representation pointer is not reference-counted,
   * so you MUST call Register() on the representation if you want to
   * keep a reference to it.
   */
  svtkDataRepresentation* SetRepresentationFromInput(svtkDataObject* input);

  /**
   * Removes the representation from the view.
   */
  void RemoveRepresentation(svtkDataRepresentation* rep);

  /**
   * Removes any representation with this connection from the view.
   */
  void RemoveRepresentation(svtkAlgorithmOutput* rep);

  /**
   * Removes all representations from the view.
   */
  void RemoveAllRepresentations();

  /**
   * Returns the number of representations from first port(0) in this view.
   */
  int GetNumberOfRepresentations();

  /**
   * The representation at a specified index.
   */
  svtkDataRepresentation* GetRepresentation(int index = 0);

  /**
   * Check to see if a representation is present in the view.
   */
  bool IsRepresentationPresent(svtkDataRepresentation* rep);

  /**
   * Update the view.
   */
  virtual void Update();

  /**
   * Apply a theme to the view.
   */
  virtual void ApplyViewTheme(svtkViewTheme* svtkNotUsed(theme)) {}

  /**
   * Returns the observer that the subclasses can use to listen to additional
   * events. Additionally these subclasses should override
   * ProcessEvents() to handle these events.
   */
  svtkCommand* GetObserver();

  //@{
  /**
   * A ptr to an instance of ViewProgressEventCallData is provided in the call
   * data when svtkCommand::ViewProgressEvent is fired.
   */
  class ViewProgressEventCallData
  {
    const char* Message;
    double Progress;
    //@}

  public:
    ViewProgressEventCallData(const char* msg, double progress)
    {
      this->Message = msg;
      this->Progress = progress;
    }
    ~ViewProgressEventCallData() { this->Message = nullptr; }

    /**
     * Get the message.
     */
    const char* GetProgressMessage() const { return this->Message; }

    /**
     * Get the progress value in range [0.0, 1.0].
     */
    double GetProgress() const { return this->Progress; }
  };

  /**
   * Meant for use by subclasses and svtkRepresentation subclasses.
   * Call this method to register svtkObjects (generally
   * svtkAlgorithm subclasses) which fire svtkCommand::ProgressEvent with the
   * view. The view listens to svtkCommand::ProgressEvent and fires
   * ViewProgressEvent with ViewProgressEventCallData containing the message and
   * the progress amount. If message is not provided, then the class name for
   * the algorithm is used.
   */
  void RegisterProgress(svtkObject* algorithm, const char* message = nullptr);

  /**
   * Unregister objects previously registered with RegisterProgress.
   */
  void UnRegisterProgress(svtkObject* algorithm);

protected:
  svtkView();
  ~svtkView() override;

  /**
   * Called to process events.
   * The superclass processes selection changed events from its representations.
   * This may be overridden by subclasses to process additional events.
   */
  virtual void ProcessEvents(svtkObject* caller, unsigned long eventId, void* callData);

  /**
   * Create a default svtkDataRepresentation for the given svtkAlgorithmOutput.
   * View subclasses may override this method to create custom representations.
   * This method is called by Add/SetRepresentationFromInputConnection.
   * NOTE, the caller must delete the returned svtkDataRepresentation.
   */
  virtual svtkDataRepresentation* CreateDefaultRepresentation(svtkAlgorithmOutput* conn);

  /**
   * Subclass "hooks" for notifying subclasses of svtkView when representations are added
   * or removed. Override these methods to perform custom actions.
   */
  virtual void AddRepresentationInternal(svtkDataRepresentation* svtkNotUsed(rep)) {}
  virtual void RemoveRepresentationInternal(svtkDataRepresentation* svtkNotUsed(rep)) {}

  //@{
  /**
   * True if the view takes a single representation that should be reused on
   * Add/SetRepresentationFromInput(Connection) calls. Default is off.
   */
  svtkSetMacro(ReuseSingleRepresentation, bool);
  svtkGetMacro(ReuseSingleRepresentation, bool);
  svtkBooleanMacro(ReuseSingleRepresentation, bool);
  bool ReuseSingleRepresentation;
  //@}

private:
  svtkView(const svtkView&) = delete;
  void operator=(const svtkView&) = delete;

  class svtkImplementation;
  svtkImplementation* Implementation;

  class Command;
  friend class Command;
  Command* Observer;

  class svtkInternal;
  svtkInternal* Internal;
};

#endif
