#ifndef OBSM_H
#define OBSM_H

#include <QObject>
#include <QDebug>
#include <QProcess>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileSystemWatcher>
#include <QDateTime>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

class obsm : public QObject
{
    Q_OBJECT
public:
    obsm(QHash<QString, QString>);
    ~obsm();

private:
    const QString _defFoldersTName = "folders";
    const QString _defBookmarksTName = "bookmarks";
    QFile _lBookmarkFile_m;
    QProcess _git_m;
    QJsonObject _lBookmarkData_m;
    QStringList _curNest_m;
    QStringList _folders_list_m;
    QString _cur_folder_m;
    QString _bookmarkFilePath;
    QString _foldersTName;
    QString _bookmarksTName;
    QSqlDatabase _db_m;
    QSqlQuery *_query_p;
    QMap<QString, QString> _bookmark_m;
    bool _newDB = false;
    bool _use_git_m;
    bool _use_rGit_m;
    int _bookmarkIter_m;

    void _processJson(QJsonObject object);
    void _processJson(QJsonArray array);
    void _processBookmark();
    void _processFolder(QJsonObject *object);
    void _printSqlErr();
    bool _execSqlQuery();
    bool _initDBTables();
    bool _readBookmarkFile();
    unsigned long chromeTimeToUTime(unsigned long ctime);
    unsigned long uTimeToChromeTime(unsigned long utime);
    QString uTime();

private slots:
    void _process();
};

#endif // OBSM_H
