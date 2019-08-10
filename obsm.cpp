#include "obsm.h"

obsm::obsm(QHash<QString, QString> opts)
{
    QString dir = "";
    bool daemon = false;
    QHashIterator<QString, QString> i(opts);
    while (i.hasNext()) {
        i.next();
        if (i.key() == "d" || i.key() == "dir") {
            dir = i.value();
        } else if (i.key() == "f" || i.key() == "file") {
            _bookmarkFilePath = i.value();
        } else if (i.key() == "b" || i.key() == "background") {
            daemon = i.value().toInt();
        } else if (i.key() == "g" || i.key() == "git") {
            _use_git_m = i.value().toInt();
        } else if (i.key() == "r" || i.key() == "remote") {
            _use_rGit_m = i.value().toInt();
        }
    }
    QDir dDir(dir);
    if (!dDir.exists(dir) || !dDir.isReadable()) {
        qCritical() << "Directory specified in `--dir` option does not exist or permission issue.";
        return;
    }
    if (!_readBookmarkFile()) {
        return;
    }
    QString dFile = dir;
    if (!dFile.endsWith("/") || !dFile.endsWith("\\")) {
        dFile.append("/");
    }
    dFile = QDir::toNativeSeparators(dFile);
    if (_use_git_m) {
        QString gDir = dFile;
        gDir.append(".git");
        dDir.setPath(gDir);
        if (!dDir.exists() || !dDir.isReadable()) {
            qCritical() << "The `--git` option is set but the `--dir` folder does not have a `.git` folder or the folder is not accesible.";
            return;
        }
        if (_use_rGit_m) {
            QProcess::execute("git", QStringList({"pull", "origin", "master"}));
        }
    }
    _db_m = QSqlDatabase::addDatabase("QSQLITE");
    dFile.append("obsm.db");
    _newDB = !QFile(dFile).exists();
    _db_m.setDatabaseName(dFile);
    if (!_db_m.open()) {
        qCritical() << "Unable to open bookmark database file.";
        return;
    }
    _query_p = new QSqlQuery;
    _foldersTName = _defFoldersTName;
    _bookmarksTName = _defBookmarksTName;
    if (_newDB) {
        if (!_initDBTables()) {
            return;
        }
        _process();
        _newDB = false;
        _foldersTName.append("_temp");
        _bookmarksTName.append("_temp");
        qDebug() << "newdb";
    } else {
        _foldersTName.append("_temp");
        _bookmarksTName.append("_temp");
        _process();
        qDebug() << "existingdb";
    }
    if (daemon) {
        QFileSystemWatcher watcher;
        watcher.addPath(_bookmarkFilePath);
        connect(&watcher, SIGNAL(fileChanged(QString)), this, SLOT(_process()));
    }
}

obsm::~obsm()
{
    if (_lBookmarkFile_m.isOpen()) {
        _lBookmarkFile_m.close();
    }
    delete _query_p;
}

// private

bool obsm::_initDBTables()
{
    for (int i = 0; i < 2; i++) {
        _query_p->prepare(QString("CREATE TABLE %1%2 (id INTEGER PRIMARY KEY, parent INTEGER, name TEXT NOT NULL, added INTEGER NOT NULL, modified INTEGER NOT NULL)").arg(_foldersTName).arg(i ? "_temp" : ""));
        if (!_execSqlQuery()) {
            qCritical() << "Failed creating folders table.";
            return false;
        }
        _query_p->prepare(QString("CREATE TABLE %1%2 (id INTEGER PRIMARY KEY, folderid INTEGER NOT NULL, name TEXT NOT NULL, url TEXT NOT NULL, added INTEGER NOT NULL, modified INTEGER NOT NULL)").arg(_bookmarksTName).arg(i ? "_temp" : ""));
        if (!_execSqlQuery()) {
            qCritical() << "Failed creating bookmarks table.";
            return false;
        }
        _query_p->prepare(QString("INSERT INTO %1%2 (parent, name, added, modified) VALUES(0, 'roots', :utime, 0)").arg(_foldersTName).arg(i ? "_temp" : ""));
        _query_p->bindValue(":utime", uTime());
        if (!_execSqlQuery()) {
            qCritical() << "Unable to insert data into database file.";
            return false;
        }
    }
    return true;
}

bool obsm::_readBookmarkFile()
{
    _lBookmarkFile_m.setFileName(_bookmarkFilePath);
    if (!_lBookmarkFile_m.exists() || !_lBookmarkFile_m.open(QIODevice::ReadOnly)) {
        qCritical() << "Bookmark file specified in `--file` option can not be opened.";
         return false;
    }
    QJsonParseError jsonErr;
    QJsonDocument qjdoc = QJsonDocument::fromJson(_lBookmarkFile_m.readAll(), &jsonErr);
    _lBookmarkFile_m.close();
    if (jsonErr.error) {
        qCritical() << "Bookmark file specified in `--file` option can not be parsed.";
        return false;
    }
    _lBookmarkData_m = qjdoc.object().value("roots").toObject();
    return true;
}

unsigned long obsm::chromeTimeToUTime(unsigned long ctime)
{
    return (ctime - 11644473600000000) / 1000000;
}

unsigned long obsm::uTimeToChromeTime(unsigned long utime)
{
    return (utime * 1000000) + 11644473600000000;
}

QString obsm::uTime()
{
    QDateTime t(QDateTime::currentDateTime());
    return QString::number(t.toTime_t());
}

void obsm::_printSqlErr()
{
    QSqlError err = _query_p->lastError();
    qInfo() << err.text() << _query_p->lastQuery() << _query_p->boundValues();
}

bool obsm::_execSqlQuery()
{
    if (!_query_p->exec()) {
        _printSqlErr();
        return false;
    }
    return true;
}

void obsm::_processJson(QJsonObject object)
{
    bool nest;
    foreach (const QString& key, object.keys()) {
        nest = false;
        if (key == "meta_info") {
            continue;
        }
        if (key == "children") {
            _processFolder(&object);
            nest = true;
        }
        QJsonValue value = object.value(key);
        if (value.type() == QJsonValue::Object) {
            _processJson(value.toObject());
        } else if (value.type() == QJsonValue::Array) {
            _processJson(value.toArray());
        } else if (value.type() == QJsonValue::String) {
            if (key == "date_added") {
                _bookmark_m["date_added"] = value.toString();
                _bookmarkIter_m++;
            } else if (key == "name") {
                _bookmark_m["name"] = value.toString();
                _bookmarkIter_m++;
            } else if (key == "type") {
                if (value.toString() == "folder") {
                    _bookmark_m.clear();
                    _bookmarkIter_m = 0;
                    continue;
                }
                _bookmarkIter_m++;
            } else if (key == "url") {
                _bookmark_m["url"] = value.toString();
                _bookmarkIter_m++;
            }
            if (_bookmarkIter_m >= 4) {
                _processBookmark();
                _bookmarkIter_m = 0;
                _bookmark_m.clear();
            }
        }
        if (nest) {
            _curNest_m.removeLast();
            _cur_folder_m = _curNest_m.last();
        }
    }
}

void obsm::_processJson(QJsonArray array)
{
    foreach (const QJsonValue & value, array) {
        QJsonObject obj = value.toObject();
        _processJson(obj);
    }
}

void obsm::_processBookmark()
{
    _query_p->prepare(QString("SELECT id, name, url FROM %1 WHERE folderid = :id AND (name = :name OR url = :url)").arg(_bookmarksTName));
    _query_p->bindValue(":id", _cur_folder_m);
    _query_p->bindValue(":name", _bookmark_m["name"]);
    _query_p->bindValue(":url", _bookmark_m["url"]);
    _execSqlQuery();
    if (_query_p->next()) {
        qDebug() << _query_p->value(0) << _query_p->value(1) << _query_p->value(2) << " NEED TO UPDATE DB";
        //TODO UPDATE BOOKMARK
    } else {
        _query_p->prepare(QString("INSERT INTO %1 (name, url, folderid, added, modified) VALUES(:name, :url, :id, :added, :modified)").arg(_bookmarksTName));
        _query_p->bindValue(":name", _bookmark_m["name"]);
        _query_p->bindValue(":url", _bookmark_m["url"]);
        _query_p->bindValue(":id", _cur_folder_m);
        _query_p->bindValue(":added", uTime());
        _query_p->bindValue(":modified", uTime());
        _execSqlQuery();
    }
}

void obsm::_processFolder(QJsonObject *object)
{
    _query_p->prepare(QString("SELECT id FROM %1 WHERE name = :name AND parent = :parent").arg(_foldersTName));
    _query_p->bindValue(":name", object->value("name").toString());
    _query_p->bindValue(":parent", _cur_folder_m);
    _execSqlQuery();
    if (!_query_p->next()) {
        _query_p->prepare(QString("INSERT INTO %1 (parent, name, added, modified) VALUES(:parent, :name, :added, :modified)").arg(_foldersTName));
        _query_p->bindValue(":parent", _cur_folder_m);
        _query_p->bindValue(":name", object->value("name").toString());
        _query_p->bindValue(":added", uTime());
        _query_p->bindValue(":modified", uTime());
        _execSqlQuery();
        _cur_folder_m = _query_p->lastInsertId().toString();
        _folders_list_m << _cur_folder_m;
        _curNest_m << _cur_folder_m;
    } else {
        _query_p->first();
        _cur_folder_m = _query_p->value(0).toString();
        _folders_list_m << _cur_folder_m;
        _curNest_m << _cur_folder_m;
        //TODO UPDATE FOLDER
    }
}

// private slots

void obsm::_process()
{
    _folders_list_m.clear();
    _query_p->prepare(QString("SELECT id FROM %1 WHERE name = 'roots'").arg(_foldersTName));
    _execSqlQuery();
    _query_p->first();
    _cur_folder_m = _query_p->value(0).toString();
    _folders_list_m << _cur_folder_m;
    _curNest_m << _cur_folder_m;
    foreach(const QString& key, _lBookmarkData_m.keys()) {
        if (key == QString("bookmark_bar") || key == QString("other")) {
            // TODO CHECK IF FOLDER IS NEWER (modified) THAN DB
            QJsonValue value = _lBookmarkData_m.value(key);
            if (value.type() != QJsonValue::Object) {
                continue;
            }
            _processJson(value.toObject());
        }
    }
    if (!_newDB) {
        QString ids = _folders_list_m.join(",");
        _query_p->prepare(QString("DELETE FROM bookmarks WHERE folderid NOT IN (%1)").arg(ids));
        _execSqlQuery();
        _query_p->prepare(QString("DELETE FROM folders WHERE id NOT IN (%1)").arg(ids));
        _execSqlQuery();
    }
}
