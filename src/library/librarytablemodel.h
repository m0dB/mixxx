#pragma once

#include "library/basesqltablemodel.h"

class LibraryTableModel : public BaseSqlTableModel {
    Q_OBJECT
  public:
    LibraryTableModel(QObject* parent, TrackCollectionManager* pTrackCollectionManager,
                      const char* settingsNamespace);
    ~LibraryTableModel() override;

    void setTableModel();

    bool isColumnInternal(int column) final;
    TrackModel::Capabilities getCapabilities() const final;
};
