#include "widget/openglwindow.h"

#include <QResizeEvent>

#include "widget/wglwidget.h"

OpenGLWindow::OpenGLWindow(WGLWidget* widget)
        : m_pWidget(widget) {
}

OpenGLWindow::~OpenGLWindow() {
}

void OpenGLWindow::initializeGL() {
    if (m_pWidget) {
        m_pWidget->initializeGL();
    }
}

void OpenGLWindow::paintGL() {
}

void OpenGLWindow::resizeGL(int w, int h) {
}

void OpenGLWindow::widgetDestroyed() {
    m_pWidget = nullptr;
}

void OpenGLWindow::hideEvent(QHideEvent* e) {
    // override to avoid drawing in the main thread, after moving to the vsync thread
    Q_UNUSED(e);
}

void OpenGLWindow::exposeEvent(QExposeEvent* e) {
    // override to avoid drawing in the main thread, after moving to the vsync thread
    Q_UNUSED(e);
}

bool OpenGLWindow::event(QEvent* ev) {
    bool result = QOpenGLWindow::event(ev);

    if (m_pWidget) {
        const auto t = ev->type();
        if (t == QEvent::Expose) {
            m_pWidget->exposed();
        }
        // Forward the following events to the WGLWidget
        else if (t == QEvent::MouseButtonDblClick || t == QEvent::MouseButtonPress ||
                t == QEvent::MouseButtonRelease || t == QEvent::MouseMove ||
                t == QEvent::DragEnter || t == QEvent::DragLeave ||
                t == QEvent::DragMove || t == QEvent::Drop || t == QEvent::Wheel) {
            m_pWidget->handleEventFromWindow(ev);
        }
    }

    return result;
}
