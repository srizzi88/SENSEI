package svtk;

import java.lang.ref.WeakReference;
import java.lang.reflect.Constructor;
import java.util.HashMap;
import java.util.TreeSet;
import java.util.concurrent.locks.ReentrantLock;

/**
 * Provide a Java thread safe implementation of svtkJavaMemoryManager. This does
 * not make SVTK thread safe. This only insure that the change of reference count
 * will never happen in two concurrent thread in the Java world.
 *
 * @see svtkJavaMemoryManager
 * @author sebastien jourdain - sebastien.jourdain@kitware.com
 */
public class svtkJavaMemoryManagerImpl implements svtkJavaMemoryManager {
  private svtkJavaGarbageCollector garbageCollector;
  private ReentrantLock lock;
  private svtkReferenceInformation lastGcResult;
  private HashMap<Long, WeakReference<svtkObjectBase>> objectMap;
  private HashMap<Long, String> objectMapClassName;

  public svtkJavaMemoryManagerImpl() {
    this.lock = new ReentrantLock();
    this.objectMap = new HashMap<Long, WeakReference<svtkObjectBase>>();
    this.objectMapClassName = new HashMap<Long, String>();
    this.garbageCollector = new svtkJavaGarbageCollector();
  }

  // Thread safe
  public svtkObjectBase getJavaObject(Long svtkId) {
    // Check pre-condition
    if (svtkId == null || svtkId.longValue() == 0) {
      throw new RuntimeException("Invalid ID, can not be null or equal to 0.");
    }

    // Check inside the map if the object is already there
    WeakReference<svtkObjectBase> value = objectMap.get(svtkId);
    svtkObjectBase resultObject = (value == null) ? null : value.get();

    // If not, we have to do something
    if (value == null || resultObject == null) {
      try {
        // Make sure no concurrency could happen inside that
        this.lock.lock();

        // Now that we have the lock make sure someone else didn't
        // create the object in between, if so just return the created
        // instance
        value = objectMap.get(svtkId);
        resultObject = (value == null) ? null : value.get();
        if (resultObject != null) {
          return resultObject;
        }

        // We need to do the work of the gc
        if (value != null && resultObject == null) {
          this.unRegisterJavaObject(svtkId);
        }

        // No-one did create it, so let's do it
        if (resultObject == null) {
          try {
            String className = svtkObjectBase.SVTKGetClassNameFromReference(svtkId.longValue());
            Class<?> c = Class.forName("svtk." + className);
            Constructor<?> cons = c.getConstructor(new Class<?>[] { long.class });
            resultObject = (svtkObjectBase) cons.newInstance(new Object[] { svtkId });
          } catch (Exception e) {
            e.printStackTrace();
          }
        }
      } finally {
        this.lock.unlock();
      }
    }
    return resultObject;
  }

  // Thread safe
  public void registerJavaObject(Long id, svtkObjectBase obj) {
    try {
      this.lock.lock();
      this.objectMap.put(id, new WeakReference<svtkObjectBase>(obj));
      this.objectMapClassName.put(id, obj.GetClassName());
    } finally {
      this.lock.unlock();
    }
  }

  // Thread safe
  public void unRegisterJavaObject(Long id) {
    try {
      this.lock.lock();
      this.objectMapClassName.remove(id);
      WeakReference<svtkObjectBase> value = this.objectMap.remove(id);

      // Prevent double deletion...
      if (value != null) {
        svtkObjectBase.SVTKDeleteReference(id.longValue());
      } else {
        throw new RuntimeException("You try to delete a svtkObject that is not referenced in the Java object Map. You may have call Delete() twice.");
      }
    } finally {
      this.lock.unlock();
    }
  }

  // Thread safe
  public svtkReferenceInformation gc(boolean debug) {
    System.gc();
    try {
      this.lock.lock();
      final svtkReferenceInformation infos = new svtkReferenceInformation(debug);
      for (Long id : new TreeSet<Long>(this.objectMap.keySet())) {
        svtkObjectBase obj = this.objectMap.get(id).get();
        if (obj == null) {
          infos.addFreeObject(this.objectMapClassName.get(id));
          this.unRegisterJavaObject(id);
        } else {
          infos.addKeptObject(this.objectMapClassName.get(id));
        }
      }

      this.lastGcResult = infos;
      return infos;
    } finally {
      this.lock.unlock();
    }
  }

  public svtkJavaGarbageCollector getAutoGarbageCollector() {
    return this.garbageCollector;
  }

  // Thread safe
  public int deleteAll() {
    int size = this.objectMap.size();
    try {
      this.lock.lock();
      for (Long id : new TreeSet<Long>(this.objectMap.keySet())) {
        this.unRegisterJavaObject(id);
      }
    } finally {
      this.lock.unlock();
    }
    return size;
  }

  public int getSize() {
    return objectMap.size();
  }

  public svtkReferenceInformation getLastReferenceInformation() {
    return this.lastGcResult;
  }
}
