#include "waveform/renderers/qopengl/waveformrenderer.h"

#include "waveform/widgets/qopengl/waveformwidget.h"

using namespace qopengl;

WaveformRenderer::WaveformRenderer(WaveformWidgetRenderer* widget)
        : ::WaveformRendererAbstract(widget) {
}

WaveformRenderer::~WaveformRenderer() = default;

void WaveformRenderer::draw(QPainter* painter, QPaintEvent* event) {
    DEBUG_ASSERT(false);
}
