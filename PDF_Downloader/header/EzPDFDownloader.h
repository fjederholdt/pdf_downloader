#pragma once

#include "PDF_Downloader.h"

#include <QDialog>
#include <QProgressBar>
#include <QFuture>

class QPushButton;
class QLineEdit;
class QComboBox;
class QCheckBox;
class QHBoxLayout;
class QLabel;

class EzPDFDownloader : public QDialog
{
	Q_OBJECT

public:
	EzPDFDownloader(QWidget* parent = nullptr);

signals: 
	void progressUpdated(int percentage);

private slots:
	void browseExcel();
	void browseOutput();
	void startDownload();
	void onProgressUpdate(int percentage);
	void onTasksReady(std::vector<PDF_Downloader::DownloadTask> tasks);
	void onWorkerError(QString message);

private:
	void startExcelWorker();
	std::unordered_set<std::string> getExistingPDFNames();
	std::vector<PDF_Downloader::DownloadTask> filterAlreadyDownloaded();
	void writeResultsToCsv();
	std::string escapeCsv(const std::string& input);

private:
	QProgressBar* pProgressBar;
	QLineEdit* excelPathEdit;
	QLineEdit* outputPathEdit;
	QLineEdit* brColumnLineEdit;
	QLineEdit* urlColumnLineEdit;
	QPushButton* browseExcelBtn;
	QPushButton* browseOutputBtn;
	QPushButton* startBtn;
	QCheckBox* retryCheckBox;
	QLabel* retryLabel;
	QHBoxLayout* retryLayout;
	std::vector<PDF_Downloader::DownloadTask> m_Tasks;
	std::unordered_set<std::string> m_ExistingPDFs;
	QVector<QFuture<std::pair<std::string, PDF_Downloader::DownloadStatus>>> m_Futures;
	bool newTasksReady = false;
	std::atomic<int> completed = 0;
	int total = 0;
};

