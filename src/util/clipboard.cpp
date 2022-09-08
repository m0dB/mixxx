#include "clipboard.h"

#include <QApplication>
#include <QClipboard>

QString& Clipboard::text() {
    static QString s_text;
    return s_text;
}

void Clipboard::begin() {
    text() = "";
}

void Clipboard::add(const QUrl& url) {
    text().append(url.toString() + "\n");
}

void Clipboard::end() {
    QApplication::clipboard()->setText(text());
}

QList<QUrl> Clipboard::urls() {
    const QString text = QApplication::clipboard()->text();
    QStringList strings = text.split("\n", QString::SkipEmptyParts);
    QList<QUrl> result;
    for (QString string : strings) {
        result.append(QUrl(string));
    }
    return result;
}
