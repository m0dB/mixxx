#include "vsyncthread.h"

#include <QThread>
#include <QTime>
#include <QtDebug>

#include "moc_vsyncthread.cpp"
#include "util/math.h"
#include "util/performancetimer.h"
#include "waveform/guitick.h"
#include "waveform/waveformwidgetfactory.h"

VSyncThread::VSyncThread(QObject* pParent)
        : m_bDoRendering(true),
          m_vSyncTypeChanged(false),
          m_syncIntervalTimeMicros(33333), // 30 FPS
          m_waitToSwapMicros(0),
          m_vSyncMode(ST_TIMER),
          m_syncOk(false),
          m_droppedFrames(0),
          m_swapWait(0),
          m_displayFrameRate(60.0),
          m_vSyncPerRendering(1) {
}

VSyncThread::~VSyncThread() {
    m_bDoRendering = false;
    //delete m_glw;
}


int VSyncThread::elapsed() {
    return static_cast<int>(m_timer.elapsed().toIntegerMicros());
}

void VSyncThread::setSyncIntervalTimeMicros(int syncTime) {
    m_syncIntervalTimeMicros = syncTime;
    m_vSyncPerRendering = static_cast<int>(
            round(m_displayFrameRate * m_syncIntervalTimeMicros / 1000));
}

void VSyncThread::setVSyncType(int type) {
    if (type >= (int)VSyncThread::ST_COUNT) {
        type = VSyncThread::ST_TIMER;
    }
    m_vSyncMode = (enum VSyncMode)type;
    m_droppedFrames = 0;
    m_vSyncTypeChanged = true;
}

int VSyncThread::toNextSyncMicros() {
    int rest = m_waitToSwapMicros - static_cast<int>(m_timer.elapsed().toIntegerMicros());
    // int math is fine here, because we do not expect times > 4.2 s
    if (rest < 0) {
        rest %= m_syncIntervalTimeMicros;
        rest += m_syncIntervalTimeMicros;
    }
    return rest;
}

int VSyncThread::fromTimerToNextSyncMicros(const PerformanceTimer& timer) {
    int difference = static_cast<int>(m_timer.difference(timer).toIntegerMicros());
    // int math is fine here, because we do not expect times > 4.2 s
    return difference + m_waitToSwapMicros;
}

int VSyncThread::droppedFrames() {
    return m_droppedFrames;
}

bool VSyncThread::shouldSwap(bool maySleep) {
    if (m_waitToSwapMicros == 0) {
        m_waitToSwapMicros = m_syncIntervalTimeMicros;
        m_timer.start();
    }

    m_remainingForSwapMicros = m_waitToSwapMicros -
            static_cast<int>(m_timer.elapsed().toIntegerMicros());

    if (m_remainingForSwapMicros > 1000) {
        if (!maySleep) {
            return false;
        }
        if (m_remainingForSwapMicros > 4000) {
            QThread::usleep(1000);
            return false;
        }
        QThread::usleep(m_remainingForSwapMicros);
    }
    return true;
}

void VSyncThread::swapped() {
    m_sinceLastSwap = m_timer.restart();
    int lastSwapTime = static_cast<int>(m_sinceLastSwap.toIntegerMicros());
    if (m_remainingForSwapMicros < 0) {
        // Our swapping call was already delayed
        // The real swap might happens on the following VSync, depending on driver settings
        m_droppedFrames++; // Count as Real Time Error
    }
    m_waitToSwapMicros = m_syncIntervalTimeMicros +
            ((m_waitToSwapMicros - lastSwapTime) % m_syncIntervalTimeMicros);
}

void VSyncThread::getAvailableVSyncTypes(QList<QPair<int, QString>>* pList) {
    for (int i = (int)VSyncThread::ST_TIMER; i < (int)VSyncThread::ST_COUNT; i++) {
        //if (isAvailable(type))  // TODO
        {
            enum VSyncMode mode = (enum VSyncMode)i;

            QString name;
            switch (mode) {
            case VSyncThread::ST_TIMER:
                name = tr("Timer (Fallback)");
                break;
            case VSyncThread::ST_MESA_VBLANK_MODE_1:
                name = tr("MESA vblank_mode = 1");
                break;
            case VSyncThread::ST_SGI_VIDEO_SYNC:
                name = tr("Wait for Video sync");
                break;
            case VSyncThread::ST_OML_SYNC_CONTROL:
                name = tr("Sync Control");
                break;
            case VSyncThread::ST_FREE:
                name = tr("Free + 1 ms (for benchmark only)");
                break;
            default:
                break;
            }
            QPair<int, QString > pair = QPair<int, QString >(i, name);
            pList->append(pair);
        }
    }
}

mixxx::Duration VSyncThread::sinceLastSwap() const {
    return m_sinceLastSwap;
}
