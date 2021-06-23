/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQtDebugLeaksModel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkQtDebugLeaksModel.h"
#include "svtkDebugLeaks.h"

#include <QCoreApplication>
#include <QDebug>
#include <QList>
#include <QPointer>
#include <QTextStream>
#include <QTimer>

//-----------------------------------------------------------------------------
class svtkQtDebugLeaksModel::qObserver : public svtkDebugLeaksObserver
{
public:
  qObserver(svtkQtDebugLeaksModel& model)
    : Model(model)
  {
    svtkDebugLeaks::SetDebugLeaksObserver(this);
  }

  ~qObserver() override { svtkDebugLeaks::SetDebugLeaksObserver(nullptr); }

  void ConstructingObject(svtkObjectBase* object) override { this->Model.addObject(object); }

  void DestructingObject(svtkObjectBase* object) override { this->Model.removeObject(object); }

  svtkQtDebugLeaksModel& Model;

private:
  qObserver(const qObserver&) = delete;
  void operator=(const qObserver&) = delete;
};

//-----------------------------------------------------------------------------
class SVTKClassInfo
{
public:
  SVTKClassInfo(const QString& className)
    : Count(0)
    , Name(className)
  {
  }
  int Count;
  QString Name;
  QList<svtkObjectBase*> Objects;
};

//-----------------------------------------------------------------------------
class svtkQtDebugLeaksModel::qInternal
{
public:
  qInternal()
    : ProcessPending(false)
  {
  }

  bool ProcessPending;
  QList<QString> Classes;
  QList<SVTKClassInfo> ClassInfo;
  QList<svtkObjectBase*> ObjectsToProcess;
  QHash<svtkObjectBase*, SVTKClassInfo*> ObjectMap;
  QHash<QString, QPointer<ReferenceCountModel> > ReferenceModels;
};

//-----------------------------------------------------------------------------
svtkQtDebugLeaksModel::svtkQtDebugLeaksModel(QObject* p)
  : QStandardItemModel(0, 2, p)
{
  this->Internal = new qInternal;
  this->Observer = new qObserver(*this);

  this->setHeaderData(0, Qt::Horizontal, QObject::tr("Class Name"));
  this->setHeaderData(1, Qt::Horizontal, QObject::tr("Class Count"));

  this->connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), SLOT(onAboutToQuit()));
}

//-----------------------------------------------------------------------------
svtkQtDebugLeaksModel::~svtkQtDebugLeaksModel()
{
  delete this->Observer;
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void svtkQtDebugLeaksModel::onAboutToQuit()
{
  delete this->Observer;
  this->Observer = nullptr;
}

//----------------------------------------------------------------------------
void svtkQtDebugLeaksModel::addObject(svtkObjectBase* object)
{
  this->Internal->ObjectsToProcess.append(object);
  if (!this->Internal->ProcessPending)
  {
    this->Internal->ProcessPending = true;
    QTimer::singleShot(0, this, SLOT(processPendingObjects()));
  }
}

//----------------------------------------------------------------------------
void svtkQtDebugLeaksModel::processPendingObjects()
{
  this->Internal->ProcessPending = false;
  foreach (svtkObjectBase* object, this->Internal->ObjectsToProcess)
  {
    this->registerObject(object);
  }
  this->Internal->ObjectsToProcess.clear();
}

//----------------------------------------------------------------------------
void svtkQtDebugLeaksModel::registerObject(svtkObjectBase* object)
{
  QString className = object->GetClassName();
  int indexOf = this->Internal->Classes.indexOf(className);
  if (indexOf < 0)
  {
    this->Internal->Classes.append(className);
    this->Internal->ClassInfo.append(SVTKClassInfo(className));

    indexOf = this->rowCount();
    this->insertRow(indexOf);
    this->setData(this->index(indexOf, 0), className);
    this->setData(this->index(indexOf, 1), 0);
  }

  SVTKClassInfo& classInfo = this->Internal->ClassInfo[indexOf];
  classInfo.Count += 1;
  classInfo.Objects.append(object);
  this->Internal->ObjectMap[object] = &classInfo;
  this->setData(this->index(indexOf, 1), classInfo.Count);

  ReferenceCountModel* model = this->Internal->ReferenceModels.value(className, nullptr);
  if (model)
  {
    model->addObject(object);
  }
}

//----------------------------------------------------------------------------
void svtkQtDebugLeaksModel::removeObject(svtkObjectBase* object)
{
  SVTKClassInfo* classInfo = this->Internal->ObjectMap.value(object, 0);
  if (classInfo)
  {
    QString className = classInfo->Name;
    int row = this->Internal->Classes.indexOf(className);
    classInfo->Count -= 1;
    classInfo->Objects.removeOne(object);
    this->Internal->ObjectMap.remove(object);

    if (classInfo->Count <= 0)
    {
      this->Internal->Classes.removeAt(row);
      this->Internal->ClassInfo.removeAt(row);
      this->removeRow(row);
    }
    else
    {
      this->setData(this->index(row, 1), classInfo->Count);
    }

    ReferenceCountModel* model = this->Internal->ReferenceModels.value(className, nullptr);
    if (model)
    {
      model->removeObject(object);
    }
  }
  else
  {
    this->Internal->ObjectsToProcess.removeOne(object);
  }
}

//-----------------------------------------------------------------------------
QList<svtkObjectBase*> svtkQtDebugLeaksModel::getObjects(const QString& className)
{
  int indexOf = this->Internal->Classes.indexOf(className);
  if (indexOf < 0)
  {
    qWarning() << "svtkQtDebugLeaksModel::getObjects: bad class name:" << className;
    return QList<svtkObjectBase*>();
  }

  SVTKClassInfo& classInfo = this->Internal->ClassInfo[indexOf];
  return classInfo.Objects;
}

//----------------------------------------------------------------------------
QStandardItemModel* svtkQtDebugLeaksModel::referenceCountModel(const QString& className)
{
  ReferenceCountModel* model = this->Internal->ReferenceModels.value(className, nullptr);
  if (!model)
  {
    model = new ReferenceCountModel(this);
    this->Internal->ReferenceModels[className] = model;
    foreach (svtkObjectBase* obj, this->getObjects(className))
    {
      model->addObject(obj);
    }
  }

  return model;
}

//-----------------------------------------------------------------------------
Qt::ItemFlags svtkQtDebugLeaksModel::flags(const QModelIndex& modelIndex) const
{
  Q_UNUSED(modelIndex);
  return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

//-----------------------------------------------------------------------------
Q_DECLARE_METATYPE(svtkObjectBase*);

//-----------------------------------------------------------------------------
ReferenceCountModel::ReferenceCountModel(QObject* p)
  : QStandardItemModel(0, 2, p)
{
  this->setHeaderData(0, Qt::Horizontal, QObject::tr("Pointer"));
  this->setHeaderData(1, Qt::Horizontal, QObject::tr("Reference Count"));
  QTimer::singleShot(100, this, SLOT(updateReferenceCounts()));
}

//-----------------------------------------------------------------------------
ReferenceCountModel::~ReferenceCountModel() {}

//-----------------------------------------------------------------------------
QString ReferenceCountModel::pointerAsString(void* ptr)
{
  QString ptrStr;
  QTextStream stream(&ptrStr);
  stream << ptr;
  return ptrStr;
}

//-----------------------------------------------------------------------------
void ReferenceCountModel::addObject(svtkObjectBase* obj)
{
  int row = this->rowCount();
  this->insertRow(row);
  this->setData(this->index(row, 0), this->pointerAsString(obj));
  this->setData(this->index(row, 0), QVariant::fromValue(obj), Qt::UserRole);
  this->setData(this->index(row, 1), obj->GetReferenceCount());
}

//-----------------------------------------------------------------------------
void ReferenceCountModel::removeObject(svtkObjectBase* obj)
{
  QString pointerString = this->pointerAsString(obj);

  for (int i = 0; i < this->rowCount(); ++i)
  {
    if (this->data(this->index(i, 0)) == pointerString)
    {
      this->removeRow(i);
      return;
    }
  }
}

//-----------------------------------------------------------------------------
void ReferenceCountModel::updateReferenceCounts()
{
  for (int row = 0; row < this->rowCount(); ++row)
  {
    QVariant pointerVariant = this->data(this->index(row, 0), Qt::UserRole);
    svtkObjectBase* obj = pointerVariant.value<svtkObjectBase*>();
    this->setData(this->index(row, 1), obj->GetReferenceCount());
  }

  QTimer::singleShot(100, this, SLOT(updateReferenceCounts()));
}

//-----------------------------------------------------------------------------
Qt::ItemFlags ReferenceCountModel::flags(const QModelIndex& modelIndex) const
{
  Q_UNUSED(modelIndex);
  return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}
