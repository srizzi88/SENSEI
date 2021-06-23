/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQtDebugLeaksModel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkQtDebugLeaksModel
 * @brief   model class that observes the svtkDebugLeaks singleton
 *
 *
 * This class is used internally by the svtkQtDebugLeaksView.  It installs an
 * observer on the svtkDebugLeaks singleton and uses the observer to maintain
 * a model of all svtkObjectBase derived objects that are alive in memory.
 */

#ifndef svtkQtDebugLeaksModel_h
#define svtkQtDebugLeaksModel_h

#include "svtkGUISupportQtModule.h" // For export macro
#include <QStandardItemModel>

class svtkObjectBase;

class SVTKGUISUPPORTQT_EXPORT svtkQtDebugLeaksModel : public QStandardItemModel
{
  Q_OBJECT

public:
  svtkQtDebugLeaksModel(QObject* p = nullptr);
  ~svtkQtDebugLeaksModel() override;

  /**
   * Get the list of objects in the model that have the given class name
   */
  QList<svtkObjectBase*> getObjects(const QString& className);

  /**
   * Return an item model that contains only objects with the given class name.
   * The model has two columns: object address (string), object reference count (integer)
   * The caller is allowed to reparent or delete the returned model.
   */
  QStandardItemModel* referenceCountModel(const QString& className);

protected slots:

  void addObject(svtkObjectBase* object);
  void removeObject(svtkObjectBase* object);
  void registerObject(svtkObjectBase* object);
  void processPendingObjects();
  void onAboutToQuit();

  // Inherited method from QAbstractItemModel
  Qt::ItemFlags flags(const QModelIndex& index) const override;

private:
  class qInternal;
  qInternal* Internal;

  class qObserver;
  qObserver* Observer;

  Q_DISABLE_COPY(svtkQtDebugLeaksModel);
};

// TODO - move to private
//-----------------------------------------------------------------------------
class ReferenceCountModel : public QStandardItemModel
{
  Q_OBJECT

public:
  ReferenceCountModel(QObject* p = nullptr);
  ~ReferenceCountModel() override;
  void addObject(svtkObjectBase* obj);
  void removeObject(svtkObjectBase* obj);
  QString pointerAsString(void* ptr);

  // Inherited method from QAbstractItemModel
  Qt::ItemFlags flags(const QModelIndex& index) const override;

protected slots:
  void updateReferenceCounts();
};

#endif
// SVTK-HeaderTest-Exclude: svtkQtDebugLeaksModel.h
