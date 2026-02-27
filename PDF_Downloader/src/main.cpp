#include "EzPDFDownloader.h"

#include <QApplication>

int main(int argc, char * argv[]) 
{
    QApplication app(argc, argv);
    EzPDFDownloader EzDownloader;
    EzDownloader.show();
    return app.exec();
}