#include "clipboardtext.h"

QList<QUrl> clipboardTextToUrls(const QString& text)
{    
    QStringList strings = text.split("\n", QString::SkipEmptyParts);
    QList<QUrl> result;
    for (QString string: strings)
    {
        result.append(QUrl(string));
    }
    return result;
}

