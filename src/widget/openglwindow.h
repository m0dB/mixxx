#pragma once

#include <QOpenGLWindow>

class WGLWidget;

/// Helper class used by wglwidgetqopengl

class OpenGLWindow : public QOpenGLWindow {
    Q_OBJECT

  public:
    OpenGLWindow(WGLWidget* pWidget);
    ~OpenGLWindow();

    void widgetDestroyed();

    bool isInitialized() const {
        return m_initialized;
    }

  private:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;
    bool event(QEvent* pEv) override;

    WGLWidget* m_pWidget;
    bool m_dirty{};
    bool m_initialized{};
};
