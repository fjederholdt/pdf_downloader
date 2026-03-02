#include "ExcelWorker.h"

#include <OpenXLSX.hpp>

using namespace OpenXLSX;

void ExcelWorker::process()
{
	try
	{
		auto tasks = loadTasks(m_filePath.toStdString(), m_brNumColName.toStdString(), m_pdfurlColName.toStdString());
		if (tasks.empty() == true)
		{
			emit errorOccurred(m_errorMessage);
		}
		else
		{
			emit tasksReady(std::move(tasks));
		}
	}
	catch (const std::exception& ex)
	{
		emit errorOccurred(ex.what());
	}

	emit finished();
}

std::vector<PDF_Downloader::DownloadTask> ExcelWorker::loadTasks(std::string excelFile, std::string brNumColName, std::string pdfColName)
{
	if (fs::exists(excelFile) == false)
	{
		m_errorMessage = QString::fromStdString("Path doesn't exist: " + excelFile);
		return {};
	}

	XLDocument doc;
	doc.open(excelFile);
	auto wks = doc.workbook().worksheet(1);

	uint64_t brnumCol = 0;
	uint64_t urlCol = 0;

	// Read header row
	auto headerRow = wks.row(1);
	for (uint64_t col = 1; col <= headerRow.cellCount(); ++col)
	{
		std::string value = headerRow.findCell(col).value().get<std::string>();

		if (value == brNumColName)
			brnumCol = col;
		else if (value == pdfColName)
			urlCol = col;
	}

	if (brnumCol == 0)
	{
		m_errorMessage = QString::fromStdString("No columns for " + brNumColName + " found.");
		return {};
	}
	if (urlCol == 0)
	{
		m_errorMessage = QString::fromStdString("No columns for " + pdfColName + " found.");
		return {};
	}

	uint64_t lastRow = wks.rowCount();

	std::vector<PDF_Downloader::DownloadTask> AllDownloadTasks;
	AllDownloadTasks.reserve(lastRow);
	for (auto& row : wks.rows())
	{
		auto brIt = row.cells().begin();
		std::advance(brIt, brnumCol - 1);

		auto urlIt = row.cells().begin();
		std::advance(urlIt, urlCol - 1);

		std::string brNumber = brIt->value().get<std::string>();
		std::string url = urlIt->value().get<std::string>();

		if (url.empty() == true)
		{
			AllDownloadTasks.push_back({ brNumber, "", { "Url is empty", 0, false } });
			continue;
		}
		else if ((url.substr(0, 4) != "http") || (url.substr(url.length() - 3) != "pdf"))
		{
			AllDownloadTasks.push_back({ brNumber, url, { "Url is not a http or .pdf", 0, false } });
			continue;
		}
		else
		{
			AllDownloadTasks.push_back({ brNumber, url, { "Success", 0, true } });
		}

	}

	doc.close();
	return AllDownloadTasks;
}