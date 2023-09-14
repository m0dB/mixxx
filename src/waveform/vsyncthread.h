#pragma once

#include <QPair>
#include <QSemaphore>
#include <QThread>
#include <QTime>

#include "util/performancetimer.h"
#include "widget/wglwidget.h"

// NOTE note a thread anymore, more a VSyncTimer
class VSyncThread : public QObject {
    Q_OBJECT
  public:
    enum VSyncMode {
        ST_TIMER = 0,
        ST_MESA_VBLANK_MODE_1,
        ST_SGI_VIDEO_SYNC,
        ST_OML_SYNC_CONTROL,
        ST_FREE,
        ST_COUNT // Dummy Type at last, counting possible types
    };

    VSyncThread(QObject* pParent);
    ~VSyncThread();

    bool waitForVideoSync(WGLWidget* glw);
    int elapsed();
    int toNextSyncMicros();
    void setSyncIntervalTimeMicros(int usSyncTimer);
    void setVSyncType(int mode);
    int droppedFrames();
    void setSwapWait(int sw);
    int fromTimerToNextSyncMicros(const PerformanceTimer& timer);
    bool shouldSwap(bool maySleep);
    void swapped();
    void getAvailableVSyncTypes(QList<QPair<int, QString>>* list);
    void setupSync(WGLWidget* glw, int index);
    void waitUntilSwap(WGLWidget* glw);
    mixxx::Duration sinceLastSwap() const;
    int getSyncIntervalTimeMicros() const {
        return m_syncIntervalTimeMicros;
    }

  private:
    bool m_bDoRendering;
    bool m_vSyncTypeChanged;
    int m_syncIntervalTimeMicros;
    int m_waitToSwapMicros;
    int m_remainingForSwapMicros;
    enum VSyncMode m_vSyncMode;
    bool m_syncOk;
    int m_droppedFrames;
    int m_swapWait;
    PerformanceTimer m_timer;
    double m_displayFrameRate;
    int m_vSyncPerRendering;
    mixxx::Duration m_sinceLastSwap;
};
