#include "waveform/widgets/qopengl/waveformwidget.h"

#include "waveform/renderers/qopengl/waveformrendererabstract.h"
#include "widget/wwaveformviewer.h"

using namespace qopengl;

WaveformWidget::WaveformWidget(const QString& group, QWidget* parent)
        : WGLWidget(parent), WaveformWidgetAbstract(group) {
}

WaveformWidget::~WaveformWidget() {
    makeCurrentIfNeeded();
    for (int i = 0; i < m_rendererStack.size(); ++i) {
        delete m_rendererStack[i];
    }
    m_rendererStack.clear();
    doneCurrent();
}

mixxx::Duration WaveformWidget::render() {
    makeCurrentIfNeeded();
    renderGL();
    doneCurrent();
    // not used, here for API compatibility
    return mixxx::Duration();
}

void WaveformWidget::renderGL() {
    if (shouldOnlyDrawBackground()) {
        if (!m_rendererStack.empty()) {
            m_rendererStack[0]->qopenglWaveformRenderer()->renderGL();
        }
    } else {
        for (int i = 0; i < m_rendererStack.size(); ++i) {
            m_rendererStack[i]->qopenglWaveformRenderer()->renderGL();
        }
    }
}

void WaveformWidget::initializeGL() {
    for (int i = 0; i < m_rendererStack.size(); ++i) {
        m_rendererStack[i]->qopenglWaveformRenderer()->initializeOpenGLFunctions();
        m_rendererStack[i]->qopenglWaveformRenderer()->initializeGL();
    }
}

void WaveformWidget::resizeGL(int w, int h) {
    makeCurrentIfNeeded();
    for (int i = 0; i < m_rendererStack.size(); ++i) {
        m_rendererStack[i]->qopenglWaveformRenderer()->resizeGL(w, h);
    }
    doneCurrent();
}

void WaveformWidget::handleEventFromWindow(QEvent* ev) {
    auto viewer = dynamic_cast<WWaveformViewer*>(parent());
    if (viewer) {
        viewer->handleEventFromWindow(ev);
    }
}
