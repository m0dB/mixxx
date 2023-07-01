#include "library/trackset/basetracksetfeature.h"

#include "library/library.h"
#include "moc_basetracksetfeature.cpp"

BaseTrackSetFeature::BaseTrackSetFeature(
        Library* pLibrary,
        UserSettingsPointer pConfig,
        const QString& rootViewName,
        const QString& iconName)
        : LibraryFeature(pLibrary, pConfig, iconName),
          m_rootViewName(rootViewName),
          m_pSidebarModel(make_parented<TreeItemModel>(this)) {
}

void BaseTrackSetFeature::activate() {
    emit switchToView(m_rootViewName);
    emit disableSearch();
    emit enableCoverArtDisplay(true);
}

void BaseTrackSetFeature::copyChild(const QModelIndex& index) const {
    Q_UNUSED(index);
    qDebug() << "AAAAAA" << m_pLibrary->currentLibraryView();
}

void BaseTrackSetFeature::pasteChild(
        const QModelIndex& index) {
    Q_UNUSED(index);
    qDebug() << "AAAAAA" << m_pLibrary->currentLibraryView();
}
