#pragma once

#include <QObject>
#include <vector>
#include "PDF_Downloader.h"

class ExcelWorker : public QObject
{
    Q_OBJECT

public:
    explicit ExcelWorker(const QString& filePath, const QString& brNumColName, const QString& pdfurlColName): m_filePath(filePath), m_brNumColName(brNumColName), m_pdfurlColName(pdfurlColName) {}

signals:
    void tasksReady(std::vector<PDF_Downloader::DownloadTask> tasks);
    void errorOccurred(QString message);
    void finished();

public slots:
    void process();

private: 
    std::vector<PDF_Downloader::DownloadTask> loadTasks(std::string excelFile, std::string brNumColName, std::string pdfColName);

private:
    QString m_filePath;
    QString m_brNumColName;
    QString m_pdfurlColName;
    QString m_errorMessage;
};