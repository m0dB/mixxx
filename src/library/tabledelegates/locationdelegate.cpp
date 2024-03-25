#include "library/tabledelegates/locationdelegate.h"

#include <QPainter>
#include <QTableView>

#include "moc_locationdelegate.cpp"

LocationDelegate::LocationDelegate(QTableView* pTableView)
        : TableItemDelegate(pTableView) {
}

void LocationDelegate::paintItem(
        QPainter* painter,
        const QStyleOptionViewItem& option,
        const QModelIndex& index) const {
    paintItemBackground(painter, option, index);

    if (option.state & QStyle::State_Selected) {
        // This uses selection-color from stylesheet for the text pen:
        // #LibraryContainer QTableView {
        //   selection-color: #fff;
        // }
        painter->setPen(QPen(option.palette.highlightedText().color()));
    }
    QTableView* tableView = qobject_cast<QTableView*>(parent());
    const auto elide = tableView->textElideMode();
    if (elide == Qt::ElideNone) {
        painter->drawText(option.rect, Qt::AlignVCenter, index.data().toString());
    } else {
        QString elidedText = option.fontMetrics.elidedText(
                index.data().toString(),
                elide,
                columnWidth(index));
        painter->drawText(option.rect, Qt::AlignVCenter, elidedText);
    }
}
