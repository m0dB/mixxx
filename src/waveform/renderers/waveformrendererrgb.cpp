#include "waveformrendererrgb.h"

#include "waveformwidgetrenderer.h"
#include "waveform/waveform.h"
#include "waveform/waveformwidgetfactory.h"
#include "widget/wskincolor.h"
#include "track/track.h"
#include "widget/wwidget.h"
#include "util/math.h"
#include "util/painterscope.h"

WaveformRendererRGB::WaveformRendererRGB(
        WaveformWidgetRenderer* waveformWidgetRenderer)
        : WaveformRendererSignalBase(waveformWidgetRenderer) {
}

WaveformRendererRGB::~WaveformRendererRGB() {
}

void WaveformRendererRGB::onSetup(const QDomNode& /* node */) {
}

inline float math_pow2(float x) {
    return x * x;
}

void WaveformRendererRGB::draw(QPainter* painter,
                                          QPaintEvent* /*event*/) {
    static float float256table[256];
    static float sqrtftable[255 * 255 * 3 + 1];
    static bool float256tableInitialized = false;
    if (!float256tableInitialized) {
        for (int i = 0; i < 256; i++) {
            float256table[i] = static_cast<float>(i);
        }
        for (int i = 0; i < 255 * 255 * 3 + 1; i++) {
            sqrtftable[i] = sqrtf(static_cast<float>(i));
        }
        float256tableInitialized = true;
    }

    const TrackPointer trackInfo = m_waveformRenderer->getTrackInfo();
    if (!trackInfo) {
        return;
    }

    ConstWaveformPointer waveform = trackInfo->getWaveform();
    if (waveform.isNull()) {
        return;
    }

    const int dataSize = waveform->getDataSize();
    if (dataSize <= 1) {
        return;
    }

    const WaveformData* data = waveform->data();
    if (data == nullptr) {
        return;
    }

    PainterScope PainterScope(painter);

    painter->setRenderHints(QPainter::Antialiasing, false);
    painter->setRenderHints(QPainter::SmoothPixmapTransform, false);
    painter->setWorldMatrixEnabled(false);
    painter->resetTransform();

    // Rotate if drawing vertical waveforms
    if (m_waveformRenderer->getOrientation() == Qt::Vertical) {
        painter->setTransform(QTransform(0, 1, 1, 0, 0, 0));
    }

    const double firstVisualIndex = m_waveformRenderer->getFirstDisplayedPosition() * dataSize;
    const double lastVisualIndex = m_waveformRenderer->getLastDisplayedPosition() * dataSize;

    const double offset = firstVisualIndex;

    // Represents the # of waveform data points per horizontal pixel.
    const double gain = (lastVisualIndex - firstVisualIndex) /
            (double)m_waveformRenderer->getLength();

    // Per-band gain from the EQ knobs.
    float allGain(1.0), lowGain(1.0), midGain(1.0), highGain(1.0);
    getGains(&allGain, &lowGain, &midGain, &highGain);

    QColor color;

    QPen pen;
    pen.setCapStyle(Qt::FlatCap);
    pen.setWidthF(math_max(1.0, 1.0 / m_waveformRenderer->getVisualSamplePerPixel()));

    const int breadth = m_waveformRenderer->getBreadth();
    const float halfBreadth = static_cast<float>(breadth) / 2.0f;

    const float heightFactor = allGain * halfBreadth / sqrtf(255 * 255 * 3);

    // Draw reference line
    painter->setPen(m_pColors->getAxesColor());
    painter->drawLine(QLineF(0, halfBreadth, m_waveformRenderer->getLength(), halfBreadth));

    double xVisualSampleIndex = offset;

    const double maxSamplingRange = gain / 2.0;
    const int n = m_waveformRenderer->getLength();
    for (int x = 0; x < n; ++x) {
        // Our current pixel (x) corresponds to a number of visual samples
        // (visualSamplerPerPixel) in our waveform object. We take the max of
        // all the data points on either side of xVisualSampleIndex within a
        // window of 'maxSamplingRange' visual samples to measure the maximum
        // data point contained by this pixel.

        // Since xVisualSampleIndex is in visual-samples (e.g. R,L,R,L) we want
        // to check +/- maxSamplingRange frames, not samples. To do this, divide
        // xVisualSampleIndex by 2. Since frames indices are integers, we round
        // to the nearest integer by adding 0.5 before casting to int.
        int visualFrameStart = int(xVisualSampleIndex / 2.0 - maxSamplingRange + 0.5);
        int visualFrameStop = int(xVisualSampleIndex / 2.0 + maxSamplingRange + 0.5);
        const int lastVisualFrame = dataSize / 2 - 1;

        // We now know that some subset of [visualFrameStart, visualFrameStop]
        // lies within the valid range of visual frames. Clamp
        // visualFrameStart/Stop to within [0, lastVisualFrame].
        visualFrameStart = math_clamp(visualFrameStart, 0, lastVisualFrame);
        visualFrameStop = math_clamp(visualFrameStop, 0, lastVisualFrame);

        int visualIndexStart = visualFrameStart * 2;
        int visualIndexStop  = visualFrameStop * 2;

        unsigned char maxLow  = 0;
        unsigned char maxMid  = 0;
        unsigned char maxHigh = 0;
        float maxAll = 0.;
        float maxAllNext = 0.;

        for (int i = visualIndexStart;
             i >= 0 && i + 1 < dataSize && i + 1 <= visualIndexStop; i += 2) {
            const WaveformData& waveformData = data[i];
            const WaveformData& waveformDataNext = data[i + 1];

            maxLow  = math_max3(maxLow,  waveformData.filtered.low,  waveformDataNext.filtered.low);
            maxMid  = math_max3(maxMid,  waveformData.filtered.mid,  waveformDataNext.filtered.mid);
            maxHigh = math_max3(maxHigh, waveformData.filtered.high, waveformDataNext.filtered.high);
            float all = math_pow2(float256table[waveformData.filtered.low] * lowGain) +
                    math_pow2(float256table[waveformData.filtered.mid] * midGain) +
                    math_pow2(float256table[waveformData.filtered.high] * highGain);
            maxAll = math_max(maxAll, all);
            float allNext = math_pow2(float256table[waveformDataNext.filtered.low] * lowGain) +
                    math_pow2(float256table[waveformDataNext.filtered.mid] * midGain) +
                    math_pow2(float256table[waveformDataNext.filtered.high] * highGain);
            maxAllNext = math_max(maxAllNext, allNext);
        }

        float maxLowF = float256table[maxLow] * lowGain;
        float maxMidF = float256table[maxMid] * midGain;
        float maxHighF = float256table[maxHigh] * highGain;

        float red = maxLowF * m_rgbLowColor_r + maxMidF * m_rgbMidColor_r +
                maxHighF * m_rgbHighColor_r;
        float green = maxLowF * m_rgbLowColor_g + maxMidF * m_rgbMidColor_g +
                maxHighF * m_rgbHighColor_g;
        float blue = maxLowF * m_rgbLowColor_b + maxMidF * m_rgbMidColor_b +
                maxHighF * m_rgbHighColor_b;

        // Compute maximum (needed for value normalization)
        float max = math_max3(red, green, blue);

        // Prevent division by zero
        if (max > 0.0f) {
            const float invmax = 1.0 / max;
            // Set color
            color.setRgbF(red * invmax, green * invmax, blue * invmax);

            // pen.setColor(color);

            // painter->setPen(pen);
            switch (m_alignment) {
                case Qt::AlignBottom:
                case Qt::AlignRight:
                    painter->drawLine(x,
                            breadth,
                            x,
                            breadth -
                                    (int)(heightFactor *
                                            sqrtftable[(int)math_max(
                                                    maxAll, maxAllNext)]));
                    break;
                case Qt::AlignTop:
                case Qt::AlignLeft:
                    painter->drawLine(x,
                            0,
                            x,
                            (int)(heightFactor *
                                    sqrtftable[(int)math_max(
                                            maxAll, maxAllNext)]));
                    break;
                default: {
                    int y1 = (int)(halfBreadth - heightFactor * sqrtftable[(int)maxAll]);
                    int y2 = (int)(halfBreadth + heightFactor * sqrtftable[(int)maxAllNext]);
                    painter->fillRect(x, y1, 1, y2 - y1, color);
                }
                    /*
                    painter->drawLine(x,
                            (int)(halfBreadth -
                                    heightFactor * sqrtftable[(int)maxAll]),
                            x,
                            (int)(halfBreadth +
                                    heightFactor *
                                            sqrtftable[(int)maxAllNext]));
                                            */
            }
        }

        xVisualSampleIndex += gain;
    }
}
