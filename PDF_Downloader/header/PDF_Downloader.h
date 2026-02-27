#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <curl/curl.h>

namespace fs = std::filesystem;

namespace PDF_Downloader
{
	struct DownloadStatus
	{
		std::string errorMessage;
		int httpCode;
		bool success;
	};

	struct DownloadTask
	{
		std::string brNumber;
		std::string url;
		DownloadStatus status;
	};

	// libcurl write callback
	static size_t write_callback(void* ptr, size_t size, size_t nmemb, void* stream)
	{
		std::ofstream* out = static_cast<std::ofstream*>(stream);
		out->write(static_cast<char*>(ptr), size * nmemb);
		return size * nmemb;
	}

	static DownloadStatus download_pdf(const std::string& url, const std::string& filename)
	{
		CURL* curl = curl_easy_init();
		if (!curl)
		{
			return { "Failed to init curl", 0, false};
		};

		std::string tempPath = filename + ".tmp";

		std::ofstream file(tempPath, std::ios::binary);
		if (!file.is_open())
		{
			curl_easy_cleanup(curl);
			return { "Failed to write to " + filename, 0, false};
		}
		
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/98.0.4758.102 Safari/537.36");
		curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1L);
		curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "");
		curl_easy_setopt(curl, CURLOPT_COOKIEJAR, "cookies.txt");
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

		CURLcode res = curl_easy_perform(curl);
		long httpCode = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

		file.close();
		curl_easy_cleanup(curl);

		// If curl failed OR HTTP not OK → delete temp file
		if (res != CURLE_OK || httpCode != 200)
		{
			if (fs::exists(tempPath))
				fs::remove(tempPath);

			return { std::string(curl_easy_strerror(res)), httpCode, false };
		}

		std::ifstream check(tempPath, std::ios::binary);
		char header[5] = {};
		check.read(header, 5);
		check.close();

		if (std::string(header, 5) != "%PDF-")
		{
			if (fs::exists(tempPath))
				fs::remove(tempPath);

			return { url + " is not a PDF", 0, false };
		}
		// Rename temp file to final .pdf
		fs::rename(tempPath, filename);

		return { "Success", httpCode, true };
	}

	class CurlDownloader
	{
	public:
		DownloadStatus download(const DownloadTask& task, std::string outputPath, bool retryThreeTimes)
		{
			std::string pathAndFilename = outputPath + "/" + task.brNumber + ".pdf";
			DownloadStatus status = PDF_Downloader::download_pdf(task.url, pathAndFilename);
			if (status.success == true)
			{
				return status;
			}
			else if ((retryThreeTimes == true) && (status.httpCode == 200))
			{
				for (size_t i = 0; i < 2; i++)
				{
					status = PDF_Downloader::download_pdf(task.url, pathAndFilename);
					if (status.success == true)
					{
						return status;
					}
				}
			}
			//std::cout << "Downloading: " << task.url << " -> " << pathAndFilename << "\n";
			return status;
		};
	};

	static DownloadStatus downloadTaskWorker(const DownloadTask& task, std::string outputPath, bool retryThreeTimes)
	{
		CurlDownloader downloader;
		if (task.status.success == false)
		{
			return { "BR number: " + task.brNumber + " Failed downloading because " + task.status.errorMessage, task.status.httpCode, false };
		}
		return downloader.download(task, outputPath, retryThreeTimes);
	}
}