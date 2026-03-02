#include "EzPDFDownloader.h"
#include "ExcelWorker.h"

#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QLabel>
#include <QtConcurrent>
#include <QThreadPool>
#include <iostream>

EzPDFDownloader::EzPDFDownloader(QWidget* parent)
    : QDialog(parent)
{
    outputPathEdit = new QLineEdit(this);
    excelPathEdit = new QLineEdit(this);

    browseOutputBtn = new QPushButton("Browse Output Folder", this);
    browseExcelBtn = new QPushButton("Browse Excel", this);

    brColumnLineEdit = new QLineEdit(this);
    urlColumnLineEdit = new QLineEdit(this);
    brColumnLineEdit->setText("BRnum");
    urlColumnLineEdit->setText("Pdf_URL");
    QFormLayout* columnLayout = new QFormLayout;
    columnLayout->addRow("BR Number column name:", brColumnLineEdit);
    columnLayout->addRow("PDF URL column name:", urlColumnLineEdit);

    QGroupBox* columnGroup = new QGroupBox("Column Mapping");
    
    columnGroup->setLayout(columnLayout);


    startBtn = new QPushButton("Start Download", this);

    pProgressBar = new QProgressBar(this);
    pProgressBar->setRange(0, 100);

    auto layout = new QVBoxLayout(this);

    retryCheckBox = new QCheckBox(this);
    retryLabel = new QLabel(this);
    retryLayout = new QHBoxLayout(layout->widget());
    
    retryCheckBox->setChecked(true);
    retryLabel->setText("Retry download three times");
    retryLayout->addWidget(retryCheckBox);
    retryLayout->addWidget(retryLabel);

    layout->addWidget(outputPathEdit);
    layout->addWidget(browseOutputBtn);

    layout->addWidget(columnGroup);

    layout->addWidget(excelPathEdit);
    layout->addWidget(browseExcelBtn);

    layout->addLayout(retryLayout);

    layout->addWidget(startBtn);
    layout->addWidget(pProgressBar);

    connect(browseExcelBtn, &QPushButton::clicked,
        this, &EzPDFDownloader::browseExcel);

    connect(browseOutputBtn, &QPushButton::clicked,
        this, &EzPDFDownloader::browseOutput);

    connect(startBtn, &QPushButton::clicked,
        this, &EzPDFDownloader::startDownload);

    connect(this, &EzPDFDownloader::progressUpdated, this, &EzPDFDownloader::onProgressUpdate);

    QThreadPool::globalInstance()->setMaxThreadCount(20);

    browseExcelBtn->setEnabled(false);
    startBtn->setEnabled(false);
}

void EzPDFDownloader::browseExcel()
{
    newTasksReady = false;
    QString file = QFileDialog::getOpenFileName(
        this,
        "Select Excel File",
        "",
        "Excel Files (*.xlsx)"
    );

    if ((file.isEmpty() == false) && (fs::exists(file.toStdU32String()) == true))
    {
        startBtn->setEnabled(false);
        excelPathEdit->setText(file);
        
        startExcelWorker();
    }
}

void EzPDFDownloader::browseOutput()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        "Select Output Directory"
    );

    if (!dir.isEmpty())
    {
        outputPathEdit->setText(dir);
        m_ExistingPDFs = getExistingPDFNames();
        browseExcelBtn->setEnabled(true);
    }
}

void EzPDFDownloader::startDownload()
{
    const std::string outputPath = outputPathEdit->text().toStdString();
    if ((outputPath.empty() == true) || (fs::exists(outputPath) == false))
    {
        QMessageBox::information(this,
            "Output path does not exist", "Invalid output path");
        return;
    }

    brColumnLineEdit->setEnabled(false);
    urlColumnLineEdit->setEnabled(false);
    retryCheckBox->setEnabled(false);
    startBtn->setEnabled(false);

    //std::vector<PDF_Downloader::DownloadTask> newTasks = PDF_Downloader::loadTasks(excelPathEdit->text().toStdString(), outputPathEdit->text().toStdString());
    std::cout << "Start download pipeline\n";

    QThreadPool* pool = QThreadPool::globalInstance();

    total = m_Tasks.size();
    completed = 0;
    emit progressUpdated(completed);
    m_Futures.clear();
    for (const auto& task : m_Tasks)
    {
        auto future = QtConcurrent::run(
            pool, [this, task, outputPath]() 
            {
                PDF_Downloader::DownloadStatus status = PDF_Downloader::downloadTaskWorker(task, outputPath, retryCheckBox->isChecked());
                
                int done = ++completed;
                int percent = (done * 100) / total;

                emit progressUpdated(percent);

                return std::make_pair(task.brNumber, status);;
            }
        );

        m_Futures.append(future);
    }
}

void EzPDFDownloader::onProgressUpdate(int percentage)
{
    pProgressBar->setValue(percentage);
    if (percentage == 100)
    {
        for (auto& future : m_Futures)
        {
            future.waitForFinished();
        }
        
        writeResultsToCsv();
        
        brColumnLineEdit->setEnabled(true);
        urlColumnLineEdit->setEnabled(true);
        retryCheckBox->setEnabled(true);
        startBtn->setEnabled(true);
    }
}

void EzPDFDownloader::startExcelWorker()
{
    QThread* thread = new QThread;
    ExcelWorker* worker =
        new ExcelWorker(excelPathEdit->text(), brColumnLineEdit->text(), urlColumnLineEdit->text());

    worker->moveToThread(thread);

    connect(thread, &QThread::started,
        worker, &ExcelWorker::process);

    connect(worker, &ExcelWorker::tasksReady,
        this, &EzPDFDownloader::onTasksReady);

    connect(worker, &ExcelWorker::errorOccurred,
        this, &EzPDFDownloader::onWorkerError);

    connect(worker, &ExcelWorker::finished,
        thread, &QThread::quit);

    connect(worker, &ExcelWorker::finished,
        worker, &QObject::deleteLater);

    connect(thread, &QThread::finished,
        thread, &QObject::deleteLater);

    thread->start();
}

std::unordered_set<std::string> EzPDFDownloader::getExistingPDFNames()
{
    std::unordered_set<std::string> existing;
    for (const auto& entry : fs::directory_iterator(outputPathEdit->text().toStdString()))
    {
        if (entry.is_regular_file() == false)
        {
            continue;
        }

        if (entry.path().extension() == ".pdf")
        {
            existing.insert(entry.path().stem().string());
        }
    }
    return existing;
}

std::vector<PDF_Downloader::DownloadTask> EzPDFDownloader::filterAlreadyDownloaded()
{
    std::vector<PDF_Downloader::DownloadTask> filtered;

    for (const auto& task : m_Tasks)
    {
        if (m_ExistingPDFs.find(task.brNumber) == m_ExistingPDFs.end())
        {
            filtered.push_back(task);
        }
    }

    return filtered;
}

void EzPDFDownloader::writeResultsToCsv()
{
    std::string pathToCsv = outputPathEdit->text().toStdString() + "/DownloadResults.csv";
    std::ofstream file(pathToCsv);

    file << "BRnum;Success;HttpCode;ErrorMessage\n";

    int timeouts = 0;
    int notFound = 0;
    int emptyOrNotPDF = 0;
    int failed = 0;
    int success = 0;

    for (const auto& future : m_Futures)
    {
        auto resultPair = future.result();
        PDF_Downloader::DownloadStatus result = resultPair.second;
        file << resultPair.first << ";"
            << (result.success ? "true" : "false") << ";"
            << result.httpCode << ";"
            << escapeCsv(result.errorMessage)
            << "\n";

        if (result.success == false)
        {
            if (result.httpCode == 200)
            {
                timeouts++;
            }
            else if (result.httpCode == 404)
            {
                notFound++;
            }
            else if (result.httpCode == 0)
            {
                emptyOrNotPDF++;
            }
            failed++;
        }
        else
        {
            success++;
        }
    }

    QMessageBox* Results = new QMessageBox(this);
    QString resultText = QString::fromStdString("PDF download succeeded: " + std::to_string(success) + "\nPDF download failed : " + std::to_string(failed) + "\nDownloads timed out: " + std::to_string(timeouts) + "\nUrl links not found: " + std::to_string(notFound) + "\nEmpty links or not a pdf link : " + std::to_string(emptyOrNotPDF));
    Results->information(this, "Results", resultText,"Ok");
}

std::string EzPDFDownloader::escapeCsv(const std::string& input)
{
    if (input.find(',') == std::string::npos &&
        input.find('"') == std::string::npos)
        return input;

    std::string escaped = input;

    size_t pos = 0;
    while ((pos = escaped.find('"', pos)) != std::string::npos)
    {
        escaped.insert(pos, "\"");
        pos += 2;
    }

    return "\"" + escaped + "\"";
}



void EzPDFDownloader::onTasksReady(std::vector<PDF_Downloader::DownloadTask> tasks)
{
    m_Tasks = std::move(tasks);
    if (m_ExistingPDFs.size() > 0)
    {
        m_Tasks = filterAlreadyDownloaded();
    }
    newTasksReady = true;
    startBtn->setEnabled(true);
}

void EzPDFDownloader::onWorkerError(QString message)
{
    QMessageBox* pErrorMessage = new QMessageBox(this);
    pErrorMessage->warning(this, "Failed loading from excel file", message, "Ok");
}
