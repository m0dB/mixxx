#pragma once

#include <QPointer>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSGNode>
#include <QtQml>

#include "qml/qmlplayerproxy.h"
#include "qml/qmlwaveformrenderer.h"
#include "track/track.h"
#include "util/performancetimer.h"
#include "waveform/isynctimeprovider.h"
#include "waveform/renderers/waveformwidgetrenderer.h"

class WaveformRendererAbstract;

namespace allshader {
class WaveformWidget;
class WaveformRenderMark;
class WaveformRenderMarkRange;
} // namespace allshader
namespace rendergraph {
class Node;
class OpacityNode;
class Engine;
class TreeNode;
} // namespace rendergraph

namespace mixxx {
namespace qml {

class QmlPlayerProxy;

class QmlWaveformDisplay : public QQuickItem, ISyncTimeProvider, public WaveformWidgetRenderer {
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QmlPlayerProxy* player READ getPlayer WRITE setPlayer
                    NOTIFY playerChanged REQUIRED)
    Q_PROPERTY(QString group READ getGroup WRITE setGroup NOTIFY groupChanged REQUIRED)
    Q_PROPERTY(QQmlListProperty<QmlWaveformRendererFactory> renderers READ renderers)
    Q_CLASSINFO("DefaultProperty", "renderers")
    QML_NAMED_ELEMENT(WaveformDisplay)

  public:
    QmlWaveformDisplay(QQuickItem* parent = nullptr);
    ~QmlWaveformDisplay() override;

    void setPlayer(QmlPlayerProxy* player);
    QmlPlayerProxy* getPlayer() const;

    void setGroup(const QString& group) override;

    int fromTimerToNextSyncMicros(const PerformanceTimer& timer) override;
    int getSyncIntervalTimeMicros() const override {
        return m_syncIntervalTimeMicros;
    }

    virtual void componentComplete() override;

    QQmlListProperty<QmlWaveformRendererFactory> renderers();

  protected:
    QSGNode* updatePaintNode(QSGNode* old, QQuickItem::UpdatePaintNodeData*) override;
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;
  private slots:
    void slotTrackLoaded(TrackPointer pLoadedTrack);
    void slotTrackLoading(TrackPointer pNewTrack, TrackPointer pOldTrack);
    void slotTrackUnloaded();
    void slotWaveformUpdated();

    void slotFrameSwapped();
    void slotWindowChanged(QQuickWindow* window);
  signals:
    void playerChanged();
    void groupChanged(const QString& group);

  private:
    void setCurrentTrack(TrackPointer pTrack);

    QPointer<QmlPlayerProxy> m_pPlayer;
    TrackPointer m_pCurrentTrack;

    PerformanceTimer m_timer;

    int m_syncIntervalTimeMicros{1000000 / 60}; // TODO don't hardcode

    bool m_geometryChanged{false};
    std::unique_ptr<rendergraph::Engine> m_pEngine;
    QList<QmlWaveformRendererFactory*> m_waveformRenderers;
};

} // namespace qml
} // namespace mixxx
