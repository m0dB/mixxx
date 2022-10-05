#pragma once

#include "waveformwidgetabstract.h"
#include "widget/wglwidget.h"

class QtRGBWaveformWidget : public WGLWidget, public WaveformWidgetAbstract {
    Q_OBJECT
  public:
    virtual ~QtRGBWaveformWidget();

    virtual WaveformWidgetType::Type getType() const { return WaveformWidgetType::QtRGBWaveform; }

    static inline QString getWaveformWidgetName() { return tr("RGB") + " - Qt"; }
    static inline bool useOpenGl() { return true; }
    static inline bool useOpenGles() { return true; }
    static inline bool useOpenGLShaders() { return false; }
    static inline bool developerOnly() { return false; }

  protected:
    virtual void castToQWidget();
    virtual void paintEvent(QPaintEvent* event);
    virtual mixxx::Duration render();

#ifndef MIXXX_USE_QGLWIDGET
    // overrides for WGLWidget
    virtual void renderGL(OpenGLWindow* w) {
        QPainter painter(w);
        draw(&painter, nullptr);
    }

    virtual void preRenderGL(OpenGLWindow* w) {
        preRender(w->getTimer(), w->getMicrosUntilSwap());
    }
#endif

  private:
    QtRGBWaveformWidget(const QString& group, QWidget* parent);
    friend class WaveformWidgetFactory;
};
