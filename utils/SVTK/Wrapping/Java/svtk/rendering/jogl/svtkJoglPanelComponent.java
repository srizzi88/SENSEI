package svtk.rendering.jogl;

import com.jogamp.opengl.GLCapabilities;
import com.jogamp.opengl.GLProfile;
import com.jogamp.opengl.awt.GLJPanel;

import svtk.svtkGenericOpenGLRenderWindow;
import svtk.svtkRenderWindow;

public class svtkJoglPanelComponent extends svtkAbstractJoglComponent<GLJPanel> {

  public svtkJoglPanelComponent() {
    this(new svtkGenericOpenGLRenderWindow());
  }

  public svtkJoglPanelComponent(svtkRenderWindow renderWindow) {
    this(renderWindow, new GLCapabilities(GLProfile.getDefault()));
  }

  public svtkJoglPanelComponent(svtkRenderWindow renderWindow, GLCapabilities capabilities) {
    super(renderWindow, new GLJPanel(capabilities));
    this.getComponent().addGLEventListener(this.glEventListener);
  }
}
