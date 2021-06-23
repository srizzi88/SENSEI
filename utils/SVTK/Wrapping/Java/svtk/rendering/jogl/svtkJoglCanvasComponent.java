package svtk.rendering.jogl;

import com.jogamp.opengl.GLCapabilities;
import com.jogamp.opengl.GLProfile;
import com.jogamp.opengl.awt.GLCanvas;

import svtk.svtkGenericOpenGLRenderWindow;
import svtk.svtkRenderWindow;

public class svtkJoglCanvasComponent extends svtkAbstractJoglComponent<GLCanvas> {

  public svtkJoglCanvasComponent() {
    this(new svtkGenericOpenGLRenderWindow());
  }

  public svtkJoglCanvasComponent(svtkRenderWindow renderWindow) {
    this(renderWindow, new GLCapabilities(GLProfile.getDefault()));
  }

  public svtkJoglCanvasComponent(svtkRenderWindow renderWindow, GLCapabilities capabilities) {
    super(renderWindow, new GLCanvas(capabilities));
    this.getComponent().addGLEventListener(this.glEventListener);
  }
}
