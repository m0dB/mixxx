#pragma once

#include <QColor>

#include "rendergraph/opengl/openglnode.h"
#include "shaders/rgbashader.h"
#include "shaders/textureshader.h"
#include "util/opengltexture2d.h"
#include "waveform/renderers/waveformrendermarkbase.h"

class QDomNode;
class SkinContext;

namespace rendergraph {
class GeometryNode;
}

namespace allshader {
class DigitsRenderNode;
class WaveformRenderMark;
}

class allshader::WaveformRenderMark : public ::WaveformRenderMarkBase,
                                      public rendergraph::OpenGLNode {
  public:
    explicit WaveformRenderMark(WaveformWidgetRenderer* waveformWidget,
            ::WaveformRendererAbstract::PositionSource type =
                    ::WaveformRendererAbstract::Play);

    void draw(QPainter* painter, QPaintEvent* event) override {
        Q_UNUSED(painter);
        Q_UNUSED(event);
    }

    bool init() override;

    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

  private:
    void updateMarkImage(WaveformMarkPointer pMark) override;

    void updatePlayPosMarkTexture();

    void drawTriangle(QPainter* painter,
            const QBrush& fillColor,
            QPointF p1,
            QPointF p2,
            QPointF p3);

    void drawMark(const QMatrix4x4& matrix, const QRectF& rect, QColor color);
    void drawTexture(const QMatrix4x4& matrix, float x, float y, QOpenGLTexture* texture);
    void updateUntilMark(double playPosition, double markerPosition);
    void drawUntilMark(const QMatrix4x4& matrix, float x);
    float getMaxHeightForText() const;

    mixxx::RGBAShader m_rgbaShader;
    mixxx::TextureShader m_textureShader;
    int m_beatsUntilMark;
    double m_timeUntilMark;
    double m_currentBeatPosition;
    double m_nextBeatPosition;
    std::unique_ptr<ControlProxy> m_pTimeRemainingControl;

    bool m_isSlipRenderer;

    DigitsRenderNode* m_pDigitsRenderNode;
    rendergraph::GeometryNode* m_pPlayPosNode;

    DISALLOW_COPY_AND_ASSIGN(WaveformRenderMark);
};
