#include "waveform/renderers/allshader/waveformrendererrgb.h"

#include <chrono>
#include <iostream>
#include <thread>

#include "dmx512usb.h"
#include "track/track.h"
#include "util/math.h"
#include "waveform/renderers/allshader/matrixforwidgetgeometry.h"
#include "waveform/renderers/waveformwidgetrenderer.h"
#include "waveform/waveform.h"

class DMXController {
  public:
    std::atomic<int> m_numClients{};
    std::chrono::steady_clock::time_point m_start;
    DMX512USB m_dmx;
    std::thread m_thread;
    allshader::WaveformRendererRGB* m_pClients[4];
    int m_prevms{};
    bool m_last{};
    std::atomic<int> m_tick{};
    std::atomic<uint32_t> m_rgb[4]{};
    DMXController()
            : m_start(std::chrono::steady_clock::now()),
              m_dmx(0),
              m_thread(&DMXController::run, this) {
    }
    ~DMXController() {
        m_thread.join();
    }
    bool tick() {
        int ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - m_start)
                         .count();
        if (ms - m_prevms < 4) {
            m_prevms = ms;
            return m_last;
        }
        if (ms - m_tick > 24) {
            m_tick = ms;
        }
        m_last = ms - m_tick > 8;
        m_prevms = ms;
        return m_last;
    }
    void run() {
        try {
            runInner();
        } catch (...) {
            qWarning() << "DMXController exception";
        }
    }
    void runInner() {
        int standard = 1000 / 30;
        int interval = 1000 / 30;
        int tick = 0;
        auto t = std::chrono::steady_clock::now();
        std::array<uint32_t, 4> rf{};
        std::array<uint32_t, 4> gf{};
        std::array<uint32_t, 4> bf{};
        while (m_numClients != -1) {
            int curtick = m_tick;
            if (curtick > tick + standard * 2 / 3) {
                tick = curtick;
            } else {
                tick += standard;
            }
            int ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - m_start)
                             .count();
            int d = (tick + standard) - ms;

            if (d > standard)
                interval = std::min(interval + 1, standard + 1);
            else if (d < standard)
                interval = std::max(interval - 1, standard - 1);

            t += std::chrono::milliseconds(interval);
            std::this_thread::sleep_until(t);

            uint32_t rt{}, gt{}, bt{};
            uint32_t n = 0;
            for (int i = 0; i < 4; i++) {
                uint32_t rgbCopy = m_rgb[i].load();
                uint32_t r = (rgbCopy >> 16) & 255;
                uint32_t g = (rgbCopy >> 8) & 255;
                uint32_t b = (rgbCopy) & 255;

                uint32_t m = std::max(r, std::max(g, b));
                if (r != m)
                    r /= 2;
                if (g != m)
                    g /= 2;
                if (b != m)
                    b /= 2;

                rf[i] = r > rf[i] + 32 ? (rf[i] * 7 + r) / 8 : (rf[i] + r) / 2;
                gf[i] = g > gf[i] + 32 ? (gf[i] * 7 + g) / 8 : (gf[i] + g) / 2;
                bf[i] = b > bf[i] + 32 ? (bf[i] * 7 + b) / 8 : (bf[i] + b) / 2;

                if (rf[i] != 0 || gf[i] != 0 || bf[i] != 0) {
                    rt += rf[i];
                    gt += gf[i];
                    bt += bf[i];
                    n++;
                }
            }
            if (n != 0) {
                rt /= n;
                gt /= n;
                bt /= n;
            }
            m_dmx.clear();
            m_dmx.set(10, 255);
            m_dmx.set(11, rt);
            m_dmx.set(12, gt);
            m_dmx.set(13, bt);
            m_dmx.send();
        }
    }
    int addClient(allshader::WaveformRendererRGB* pClient) {
        for (int i = 0; i < 4; i++) {
            if (m_pClients[i] == nullptr) {
                m_pClients[i] = pClient;
                m_numClients++;
                return i;
            }
        }
        return -1;
    }
    void removeClient(allshader::WaveformRendererRGB* pClient) {
        for (int i = 0; i < 4; i++) {
            if (m_pClients[i] == pClient) {
                m_pClients[i] = nullptr;
                m_numClients--;
                if (m_numClients == 0) {
                    m_numClients = -1;
                    delete this;
                }
            }
        }
    }
    static DMXController* instance() {
        static DMXController* pInstance = nullptr;
        if (!pInstance) {
            if (DMX512USB::getDeviceCount() != 0) {
                pInstance = new DMXController;
            }
        }
        return pInstance;
    }
};

namespace allshader {

namespace {
inline float math_pow2(float x) {
    return x * x;
}
} // namespace

WaveformRendererRGB::WaveformRendererRGB(WaveformWidgetRenderer* waveformWidget,
        ::WaveformRendererAbstract::PositionSource type)
        : WaveformRendererSignalBase(waveformWidget),
          m_isSlipRenderer(type == ::WaveformRendererAbstract::Slip) {
    m_clientId = DMXController::instance() ? DMXController::instance()->addClient(this) : -1;
}

WaveformRendererRGB::~WaveformRendererRGB() {
    if (m_clientId != -1) {
        DMXController::instance()->removeClient(this);
    }
}

void WaveformRendererRGB::onSetup(const QDomNode& node) {
    Q_UNUSED(node);
}

void WaveformRendererRGB::initializeGL() {
    WaveformRendererSignalBase::initializeGL();
    m_shader.init();
}

void WaveformRendererRGB::paintGL() {
    TrackPointer pTrack = m_waveformRenderer->getTrackInfo();
    if (!pTrack || (m_isSlipRenderer && !m_waveformRenderer->isSlipActive())) {
        return;
    }

    auto positionType = m_isSlipRenderer ? ::WaveformRendererAbstract::Slip
                                         : ::WaveformRendererAbstract::Play;

    ConstWaveformPointer waveform = pTrack->getWaveform();
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

    const float devicePixelRatio = m_waveformRenderer->getDevicePixelRatio();
    const int length = static_cast<int>(m_waveformRenderer->getLength() * devicePixelRatio);

    // See waveformrenderersimple.cpp for a detailed explanation of the frame and index calculation
    const int visualFramesSize = dataSize / 2;
    const double firstVisualFrame =
            m_waveformRenderer->getFirstDisplayedPosition(positionType) * visualFramesSize;
    const double lastVisualFrame =
            m_waveformRenderer->getLastDisplayedPosition(positionType) * visualFramesSize;

    // Represents the # of visual frames per horizontal pixel.
    const double visualIncrementPerPixel =
            (lastVisualFrame - firstVisualFrame) / static_cast<double>(length);

    // Per-band gain from the EQ knobs.
    float allGain(1.0), lowGain(1.0), midGain(1.0), highGain(1.0);
    // applyCompensation = false, as we scale to match filtered.all
    getGains(&allGain, false, &lowGain, &midGain, &highGain);
    float volume{};
    getVolume(&volume);

    const float breadth = static_cast<float>(m_waveformRenderer->getBreadth()) * devicePixelRatio;
    const float halfBreadth = breadth / 2.0f;

    const float heightFactor = allGain * halfBreadth / m_maxValue;

    const float low_r = static_cast<float>(m_rgbLowColor_r);
    const float mid_r = static_cast<float>(m_rgbMidColor_r);
    const float high_r = static_cast<float>(m_rgbHighColor_r);
    const float low_g = static_cast<float>(m_rgbLowColor_g);
    const float mid_g = static_cast<float>(m_rgbMidColor_g);
    const float high_g = static_cast<float>(m_rgbHighColor_g);
    const float low_b = static_cast<float>(m_rgbLowColor_b);
    const float mid_b = static_cast<float>(m_rgbMidColor_b);
    const float high_b = static_cast<float>(m_rgbHighColor_b);

    // Effective visual frame for x
    double xVisualFrame = qRound(firstVisualFrame / visualIncrementPerPixel) *
            visualIncrementPerPixel;

    const int numVerticesPerLine = 6; // 2 triangles

    const int reserved = numVerticesPerLine * (length + 1);

    m_vertices.clear();
    m_vertices.reserve(reserved);
    m_colors.clear();
    m_colors.reserve(reserved);

    m_vertices.addRectangle(0.f,
            halfBreadth - 0.5f * devicePixelRatio,
            static_cast<float>(length),
            m_isSlipRenderer ? halfBreadth : halfBreadth + 0.5f * devicePixelRatio);
    m_colors.addForRectangle(
            static_cast<float>(m_axesColor_r),
            static_cast<float>(m_axesColor_g),
            static_cast<float>(m_axesColor_b));

    const double maxSamplingRange = visualIncrementPerPixel / 2.0;

    for (int pos = 0; pos < length; ++pos) {
        const int visualFrameStart = std::lround(xVisualFrame - maxSamplingRange);
        const int visualFrameStop = std::lround(xVisualFrame + maxSamplingRange);

        const int visualIndexStart = std::max(visualFrameStart * 2, 0);
        const int visualIndexStop =
                std::min(std::max(visualFrameStop, visualFrameStart + 1) * 2, dataSize - 1);

        const float fpos = static_cast<float>(pos);

        // Find the max values for low, mid, high and all in the waveform data.
        // - Max of left and right
        uchar u8maxLow{};
        uchar u8maxMid{};
        uchar u8maxHigh{};
        // - Per channel
        uchar u8maxAllChn[2]{};
        for (int chn = 0; chn < 2; chn++) {
            // data is interleaved left / right
            for (int i = visualIndexStart + chn; i < visualIndexStop + chn; i += 2) {
                const WaveformData& waveformData = data[i];

                u8maxLow = math_max(u8maxLow, waveformData.filtered.low);
                u8maxMid = math_max(u8maxMid, waveformData.filtered.mid);
                u8maxHigh = math_max(u8maxHigh, waveformData.filtered.high);
                u8maxAllChn[chn] = math_max(u8maxAllChn[chn], waveformData.filtered.all);
            }
        }

        // Cast to float
        float maxLow = static_cast<float>(u8maxLow);
        float maxMid = static_cast<float>(u8maxMid);
        float maxHigh = static_cast<float>(u8maxHigh);
        float maxAllChn[2]{static_cast<float>(u8maxAllChn[0]), static_cast<float>(u8maxAllChn[1])};
        // Uncomment to undo scaling with pow(value, 2.0f * 0.316f) done in analyzerwaveform.h
        // float maxAllChn[2]{unscale(u8maxAllChn[0]), unscale(u8maxAllChn[1])};

        // Calculate the squared magnitude of the maxLow, maxMid and maxHigh values.
        // We take the square root to get the magnitude below.
        const float sum = math_pow2(maxLow) + math_pow2(maxMid) + math_pow2(maxHigh);

        // Apply the gains
        maxLow *= lowGain;
        maxMid *= midGain;
        maxHigh *= highGain;

        // Calculate the squared magnitude of the gained maxLow, maxMid and maxHigh values
        // We take the square root to get the magnitude below.
        const float sumGained = math_pow2(maxLow) + math_pow2(maxMid) + math_pow2(maxHigh);

        // The maxAll values will be used to draw the amplitude. We scale them according to
        // magnitude of the gained maxLow, maxMid and maxHigh values
        if (sum != 0.f) {
            // magnitude = sqrt(sum) and magnitudeGained = sqrt(sumGained), and
            // factor = magnitudeGained / magnitude, but we can do with a single sqrt:
            const float factor = std::sqrt(sumGained / sum);
            maxAllChn[0] *= factor;
            maxAllChn[1] *= factor;
        }

        // Use the gained maxLow, maxMid and maxHigh values to calculate the color components
        float red = maxLow * low_r + maxMid * mid_r + maxHigh * high_r;
        float green = maxLow * low_g + maxMid * mid_g + maxHigh * high_g;
        float blue = maxLow * low_b + maxMid * mid_b + maxHigh * high_b;

        // Normalize the color components using the maximum of the three
        const float maxComponent = math_max3(red, green, blue);
        if (maxComponent == 0.f) {
            // Avoid division by 0
            red = 0.f;
            green = 0.f;
            blue = 0.f;
        } else {
            const float normFactor = 1.f / maxComponent;
            red *= normFactor;
            green *= normFactor;
            blue *= normFactor;
        }

        // Lines are thin rectangles
        m_vertices.addRectangle(fpos - 0.5f,
                halfBreadth - heightFactor * maxAllChn[0],
                fpos + 0.5f,
                m_isSlipRenderer ? halfBreadth : halfBreadth + heightFactor * maxAllChn[1]);
        m_colors.addForRectangle(red, green, blue);

        xVisualFrame += visualIncrementPerPixel;
    }

    if (m_clientId != -1 && DMXController::instance()->tick()) {
        const double visualFrameAtPlayPos = firstVisualFrame +
                m_waveformRenderer->getPlayMarkerPosition() *
                        (lastVisualFrame - firstVisualFrame);

        const double delta = visualFrameAtPlayPos - m_visualFrameAtPlayPos;
        if (delta > 0 && delta < 100) {
            m_smoothDelta = (m_smoothDelta + delta) / 2;
        }
        if (delta == 0)
            volume = 0.f;
        m_visualFrameAtPlayPos = visualFrameAtPlayPos;

            const int visualIndexStart = std::max<int>(std::lround(m_visualFrameAtPlayPos) * 2, 0);
            const int visualIndexStop = std::min<int>(
                    std::lround(m_visualFrameAtPlayPos * 2 + m_smoothDelta * 2),
                    dataSize - 1);

            uint32_t sumLow{};
            uint32_t sumMid{};
            uint32_t sumHigh{};
            uint8_t maxAll{};
            for (int chn = 0; chn < 2; chn++) {
                // data is interleaved left / right
                for (int i = visualIndexStart + chn; i < visualIndexStop + chn; i += 2) {
                    const WaveformData& waveformData = data[i];

                    sumLow += waveformData.filtered.low;
                    sumMid += waveformData.filtered.mid;
                    sumHigh += waveformData.filtered.high;
                    maxAll = std::max(maxAll, waveformData.filtered.all);
                }
            }

            float fSumLow = static_cast<float>(sumLow);
            float fSumMid = static_cast<float>(sumMid);
            float fSumHigh = static_cast<float>(sumHigh);

            // Apply the gains
            fSumLow *= lowGain;
            fSumMid *= midGain;
            fSumHigh *= highGain;

            // Use the gained fSumLow, fSumMid and fSumHigh values to calculate the color components
            float red = fSumLow * low_r + fSumMid * mid_r + fSumHigh * high_r;
            float green = fSumLow * low_g + fSumMid * mid_g + fSumHigh * high_g;
            float blue = fSumLow * low_b + fSumMid * mid_b + fSumHigh * high_b;

            // Normalize the color components using the maximum of the three
            const float maxComponent = math_max3(red, green, blue);
            if (maxComponent == 0.f) {
                // Avoid division by 0
                red = 0.f;
                green = 0.f;
                blue = 0.f;
            } else {
                const float normFactor = static_cast<float>(maxAll) / maxComponent;
                red *= normFactor;
                green *= normFactor;
                blue *= normFactor;
            }
            /*
        // Find the max values for low, mid, high and all in the waveform data.
        // - Max of left and right
        uchar u8maxLow{};
        uchar u8maxMid{};
        uchar u8maxHigh{};
        // - Per channel
        uchar u8maxAllChn[2]{};
        for (int chn = 0; chn < 2; chn++) {
            // data is interleaved left / right
            for (int i = visualIndexStart + chn; i < visualIndexStop + chn; i += 2) {
                const WaveformData& waveformData = data[i];

                u8maxLow = math_max(u8maxLow, waveformData.filtered.low);
                u8maxMid = math_max(u8maxMid, waveformData.filtered.mid);
                u8maxHigh = math_max(u8maxHigh, waveformData.filtered.high);
                u8maxAllChn[chn] = math_max(u8maxAllChn[chn], waveformData.filtered.all);
            }
        }

        // Cast to float
        float maxLow = static_cast<float>(u8maxLow);
        float maxMid = static_cast<float>(u8maxMid);
        float maxHigh = static_cast<float>(u8maxHigh);
        float maxAllChn[2]{static_cast<float>(u8maxAllChn[0]), static_cast<float>(u8maxAllChn[1])};
        // Uncomment to undo scaling with pow(value, 2.0f * 0.316f) done in analyzerwaveform.h
        // float maxAllChn[2]{unscale(u8maxAllChn[0]), unscale(u8maxAllChn[1])};

        // Calculate the squared magnitude of the maxLow, maxMid and maxHigh values.
        // We take the square root to get the magnitude below.
        const float sum = math_pow2(maxLow) + math_pow2(maxMid) + math_pow2(maxHigh);

        // Apply the gains
        maxLow *= lowGain;
        maxMid *= midGain;
        maxHigh *= highGain;

        // Calculate the squared magnitude of the gained maxLow, maxMid and maxHigh values
        // We take the square root to get the magnitude below.
        const float sumGained = math_pow2(maxLow) + math_pow2(maxMid) + math_pow2(maxHigh);

        // The maxAll values will be used to draw the amplitude. We scale them according to
        // magnitude of the gained maxLow, maxMid and maxHigh values
        if (sum != 0.f) {
            // magnitude = sqrt(sum) and magnitudeGained = sqrt(sumGained), and
            // factor = magnitudeGained / magnitude, but we can do with a single sqrt:
            const float factor = std::sqrt(sumGained / sum);
            maxAllChn[0] *= factor;
            maxAllChn[1] *= factor;
        }

        // Use the gained maxLow, maxMid and maxHigh values to calculate the color components
        float red = maxLow * low_r + maxMid * mid_r + maxHigh * high_r;
        float green = maxLow * low_g + maxMid * mid_g + maxHigh * high_g;
        float blue = maxLow * low_b + maxMid * mid_b + maxHigh * high_b;

        // Normalize the color components using the maximum of the three
        const float maxComponent = math_max3(red, green, blue);
        if (maxComponent == 0.f) {
            // Avoid division by 0
            red = 0.f;
            green = 0.f;
            blue = 0.f;
        } else {
            const float normFactor = 255.f / maxComponent;
            red *= normFactor;
            green *= normFactor;
            blue *= normFactor;
        }
        */

            uint32_t ured = std::lround(std::clamp(red * volume, 0.f, 255.f));
            uint32_t ugreen = std::lround(std::clamp(green * volume, 0.f, 255.f));
            uint32_t ublue = std::lround(std::clamp(blue * volume, 0.f, 255.f));

            DMXController::instance()->m_rgb[m_clientId].store(
                    (ured << 16) | (ugreen << 8) | ublue);
    }

    DEBUG_ASSERT(reserved == m_vertices.size());
    DEBUG_ASSERT(reserved == m_colors.size());

    const QMatrix4x4 matrix = matrixForWidgetGeometry(m_waveformRenderer, true);

    const int matrixLocation = m_shader.matrixLocation();
    const int positionLocation = m_shader.positionLocation();
    const int colorLocation = m_shader.colorLocation();

    m_shader.bind();
    m_shader.enableAttributeArray(positionLocation);
    m_shader.enableAttributeArray(colorLocation);

    m_shader.setUniformValue(matrixLocation, matrix);

    m_shader.setAttributeArray(
            positionLocation, GL_FLOAT, m_vertices.constData(), 2);
    m_shader.setAttributeArray(
            colorLocation, GL_FLOAT, m_colors.constData(), 3);

    glDrawArrays(GL_TRIANGLES, 0, m_vertices.size());

    m_shader.disableAttributeArray(positionLocation);
    m_shader.disableAttributeArray(colorLocation);
    m_shader.release();
}

} // namespace allshader
