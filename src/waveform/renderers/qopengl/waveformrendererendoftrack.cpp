#include "waveform/renderers/qopengl/waveformrendererendoftrack.h"

#include <QDomNode>
#include <QPaintEvent>
#include <QPainter>

#include "control/controlobject.h"
#include "control/controlproxy.h"
#include "util/timer.h"
#include "waveform/waveformwidgetfactory.h"
#include "waveform/widgets/qopengl/waveformwidget.h"
#include "widget/wskincolor.h"
#include "widget/wwidget.h"

namespace {

constexpr int kBlinkingPeriodMillis = 1000;

} // anonymous namespace

using namespace qopengl;

WaveformRendererEndOfTrack::WaveformRendererEndOfTrack(
        WaveformWidgetRenderer* waveformWidget)
        : WaveformRenderer(waveformWidget),
          m_pEndOfTrackControl(nullptr),
          m_pTimeRemainingControl(nullptr) {
}

WaveformRendererEndOfTrack::~WaveformRendererEndOfTrack() {
    delete m_pEndOfTrackControl;
    delete m_pTimeRemainingControl;
}

bool WaveformRendererEndOfTrack::init() {
    m_timer.restart();

    m_pEndOfTrackControl = new ControlProxy(
            m_waveformRenderer->getGroup(), "end_of_track");
    m_pTimeRemainingControl = new ControlProxy(
            m_waveformRenderer->getGroup(), "time_remaining");
    return true;
}

void WaveformRendererEndOfTrack::setup(const QDomNode& node, const SkinContext& context) {
    m_color = QColor(200, 25, 20);
    const QString endOfTrackColorName = context.selectString(node, "EndOfTrackColor");
    if (!endOfTrackColorName.isNull()) {
        m_color.setNamedColor(endOfTrackColorName);
        m_color = WSkinColor::getCorrectColor(m_color);
    }
    //m_pen = QPen(QBrush(m_color), 2.5 * scaleFactor());
}

void WaveformRendererEndOfTrack::initializeGL() {
    QString vertexShaderCode =
            "\
uniform mat4 matrix;\n\
attribute vec4 position;\n\
varying vec2 vposition;\n\
void main()\n\
{\n\
    vposition = position.xy;\n\
    gl_Position = position;\n\
}\n";

    QString fragmentShaderCode =
            "\
uniform vec4 color;\n\
varying vec2 vposition;\n\
void main()\n\
{\n\
    gl_FragColor = vec4(color.x, color.y, color.z, color.w * (0.5 + 0.33 * max(0.,vposition.x)));\n\
}\n";

    if (!m_shaderProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderCode)) {
        return;
    }

    if (!m_shaderProgram.addShaderFromSourceCode(
                QOpenGLShader::Fragment, fragmentShaderCode)) {
        return;
    }

    if (!m_shaderProgram.link()) {
        return;
    }

    if (!m_shaderProgram.bind()) {
        return;
    }
}

void WaveformRendererEndOfTrack::fillWithGradient(QColor color) {
    const float posarray[] = {-1.f, -1.f, 1.f, -1.f, -1.f, 1.f, 1.f, 1.f};

    m_shaderProgram.bind();

    int colorLocation = m_shaderProgram.uniformLocation("color");
    int positionLocation = m_shaderProgram.attributeLocation("position");

    m_shaderProgram.setUniformValue(colorLocation, color);

    m_shaderProgram.enableAttributeArray(positionLocation);
    m_shaderProgram.setAttributeArray(
            positionLocation, GL_FLOAT, posarray, 2);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void WaveformRendererEndOfTrack::renderGL() {
    if (!m_pEndOfTrackControl->toBool()) {
        return;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const int elapsed = m_timer.elapsed().toIntegerMillis() % kBlinkingPeriodMillis;

    const double blinkIntensity = (double)(2 * abs(elapsed - kBlinkingPeriodMillis / 2)) /
            kBlinkingPeriodMillis;

    const double remainingTime = m_pTimeRemainingControl->get();
    const double remainingTimeTriggerSeconds =
            WaveformWidgetFactory::instance()->getEndOfTrackWarningTime();
    const double criticalIntensity = (remainingTimeTriggerSeconds - remainingTime) /
            remainingTimeTriggerSeconds;

    QColor color = m_color;
    color.setAlphaF(criticalIntensity * blinkIntensity);

    fillWithGradient(color);
}
