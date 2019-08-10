#include <QCoreApplication>
#include <QCommandLineParser>
#include <signal.h>
#include "obsm.h"

static obsm *OBSM;
static QCoreApplication *a;

static void quit(int)
{
    if (OBSM != nullptr) {
        OBSM->~obsm();
    }
    a->exit();
}

int main(int argc, char *argv[])
{
    a = new QCoreApplication(argc, argv);
    signal(SIGINT, quit);
    signal(SIGTERM, quit);
    QCoreApplication::setApplicationName("Open Bookmark Sync & Merger");
    QCoreApplication::setApplicationVersion("0.1");
    QCommandLineParser parser;
    parser.setApplicationDescription("Passing a browser's bookmark file to this program will merge with information in a database, the resulting new bookmark file is sent back to the browser.");
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption dir(QStringList() << "d" << "dir",
        QCoreApplication::translate("main", "Folder to store the database file."), "~/.obsmdir"
    );
    parser.addOption(dir);
    QCommandLineOption file(QStringList() << "f" << "file",
        QCoreApplication::translate("main", "Bookmark file location"), "~/.config/chromium/Default/Bookmarks"
    );
    parser.addOption(file);
    QCommandLineOption daemon(QStringList() << "b" << "background",
        QCoreApplication::translate("main", "By default the program will run once and exit, with this option it will keep running, monitoring the bookmark file for changes.")
    );
    parser.addOption(daemon);
    QCommandLineOption git(QStringList() << "g" << "git",
        QCoreApplication::translate("main", "Use git for versioning of the database file, if this option is used, the folder specified in the `--dir` option must be a git repository.")
    );
    parser.addOption(git);
    QCommandLineOption remote(QStringList() << "r" << "remote",
        QCoreApplication::translate("main", "Use `git push` / `git pull` if `--git` option is used, `--dir` must have a remote git respository tracked.")
    );
    parser.addOption(remote);
    parser.process(*a);
    if (!parser.isSet("dir")) {
        qWarning() << "dir parameter required.";
        parser.showHelp();
    }
    if (!parser.isSet("file")) {
        qWarning() << "file parameter required.";
        parser.showHelp();
    }
    QHash<QString, QString> opts;
    QString optName;
    for (int i = 0; i < parser.optionNames().size(); i++) {
        optName = parser.optionNames().at(i);
        opts.insert(optName, parser.value(optName));
    }
    OBSM = new obsm(opts);
    return a->exec();
}
