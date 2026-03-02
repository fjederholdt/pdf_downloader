Introduction

PDF Downloader is a Qt-based desktop application that downloads PDF files in bulk using data from an Excel file.

The application:

Loads tasks from an Excel sheet

Extracts BR numbers and PDF URLs

Downloads PDFs concurrently using a thread pool

Tracks progress with a GUI progress bar

Logs download results (success/error) to a CSV file

It is built with:

Qt (GUI + concurrency)

CMake (build system)

vcpkg (dependency management)

libcurl (HTTP downloads)

Setup
1. Clone the Repository
git clone https://github.com/your-username/pdf_downloader.git
cd pdf_downloader/PDF_Downloader
2. Install Dependencies

This project uses vcpkg for dependency management.

Make sure you have:

CMake

Qt (Qt 6 recommended)

vcpkg

Install required libraries via vcpkg (if not using manifest mode):

vcpkg install curl:x64-windows

If using manifest mode (vcpkg.json), dependencies will install automatically during CMake configuration.

3. Build the Project

From inside the PDF_Downloader directory:

cmake -B build -S . ^
  -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake ^
  -DCMAKE_BUILD_TYPE=Release

cmake --build build --config Release

After building, the executable will be located in:

build/Release/
Use

Launch the application.

Click Browse Excel and select the Excel file containing:

BR number column

PDF URL column

Choose an output folder.

Configure which Excel columns correspond to:

BR Number

PDF URL

Click Start Download.

The application will:

Download PDFs concurrently

Show progress in the progress bar

Store downloaded files in the selected output folder

Generate a CSV log file containing:

BR Number

Success (true/false)

Error message (if any)
