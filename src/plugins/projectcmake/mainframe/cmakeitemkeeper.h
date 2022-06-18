#ifndef CMAKEITEMKEEPER_H
#define CMAKEITEMKEEPER_H

#include <QObject>
#include <QStandardItem>

class CmakeItemKeeperPrivate;
class CmakeItemKeeper : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(CmakeItemKeeper)
    CmakeItemKeeper();
    CmakeItemKeeperPrivate *const d;

public:
    static CmakeItemKeeper *instance();
    virtual ~CmakeItemKeeper();

signals:
    void cmakeFileNodeNotify(QStandardItem *rootItem, const QPair<QString, QStringList> &files);

public slots:
    void addCmakeRootFile(QStandardItem *root, const QString rootPath);
    void addCmakeSubFiles(QStandardItem *root, const QStringList subPaths);
    void delCmakeFileNode(QStandardItem *rootItem);

private:
    void notifyFromWatcher(const QString &filePath);
};

#endif // CMAKEITEMKEEPER_H