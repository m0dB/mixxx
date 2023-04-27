#pragma once

#include "util/class.h"
#include "waveform/renderers/qopengl/waveformrendererabstract.h"
#include "waveform/renderers/waveformrenderersignalbase.h"

class WaveformWidgetRenderer;

namespace qopengl {
class WaveformRendererSignalBase;
}

class qopengl::WaveformRendererSignalBase : public ::WaveformRendererSignalBase,
                                            public qopengl::WaveformRendererAbstract {
  public:
    explicit WaveformRendererSignalBase(WaveformWidgetRenderer* waveformWidget);
    ~WaveformRendererSignalBase() override = default;

    void draw(QPainter* painter, QPaintEvent* event) override {
    }

    qopengl::WaveformRendererAbstract* qopenglWaveformRenderer() override {
        return this;
    }

    DISALLOW_COPY_AND_ASSIGN(WaveformRendererSignalBase);
};
