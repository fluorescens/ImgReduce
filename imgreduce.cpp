/*
A general implementation of the image hashing algorithm used by Crunchypack. 
Takes a collection of 256-bitmap images and reduces each image down to the pixels that are unique to the image among the set. 
Produces two tables: A tripart hash:imageID table and a more complex table with 256-rows and a list of objects that share that value for a given channel. 
*/

//NOTE: Table is written in BGR order. 
#include "stdafx.h"
#include <string>
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <memory>
#include <fstream>
#include <sstream>
#include <time.h>
#include <assert.h>
#include <deque>

#include <windows.h>
#include <tchar.h> 
#include <stdio.h>
#include <strsafe.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// The one and only application object

CWinApp theApp;

const int DEBUG_MODE = 0; //global debug state. 1 for debug enabled. 


int main()
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // initialize MFC and print and error on failure
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
			wprintf(L"A fatal error: MFC initialization failed : has occured.\n");
            nRetCode = 1;
        }
        else
        {
			std::wstring crunchy_path; //path to image folder containing bitmaps
			std::wstringstream buffer; //for crunchypack data reading
			std::wstring source_data; //stores stringstream buffer once read-in is complete
			std::wstring setup_choice; //stand alone files or part of a prebuilt pack?
			int setup_state = -1; //0: standalone, 1: prebuilt
			std::vector<std::wstring> direct_name_buffer; //names of files in folder containing a '.bmp' extension
			std::wstring standalone_folder; //the path to the folder from root
			std::wstring standalone_name; //user-enetred name for the pack file
			std::wcout << "Welcome to CrunchyPack Generator: IMGReduce 1.0!" << '\n';
			std::wcout << "Please ensure all images are 256 bitmaps.\nThere are different types of bitmap(.bmp) formats and only 256 bitmaps should be used. \n"
				<< "To check if a file is a 256 bitmap, right click the image, click properties and under the details tab check bit depth. \n"
				<< "This value should be 8; a 4 or 24 are the wrong formats (16 and 24 bitmap). Resave the file in the correct format in a program like Paint. \n"
				<< "If you sure are the images are in the correct format, proceed.\n\n";
			while (true) {
				bool quit = false; //when quit true, exit the user loop

				bool set_state = false; 
				while (set_state == false) {
					//choose the builder state: directly on a collection of images or as part of a prebuilt crunchypack file
					if (setup_state != 0 || setup_state != 1) {
						std::wcout << "Standalone images(i) or build from a crunchypack template(p)? (enter character):";
						std::getline(std::wcin, setup_choice);
						switch (setup_choice[0]) {
						case('i') :
							//standalone images
							setup_state = 0;
							set_state = true;
							break;
						case('p') :
							//build from pack
							setup_state = 1;
							set_state = true; 
							break;
						default:
							std::wcout << "No such option is available. Options include i or p" << '\n';
							break; 
						}
					}
				}


				bool set_path = false; 
				while (set_path == false) {
					switch (setup_state) {
						//standalone file needs location, 
					case 0: {
						std::wcout << "Enter full filepath from root to 256-bitmap containing folder: ";
						if (DEBUG_MODE == 1) {
							crunchy_path = L"C:\\Users\\JamesH\\Documents\\NetBeansProjects\\CleanStencilUI\\src\\cleanstencilui\\testfolder";
						}
						else {
							std::getline(std::wcin, crunchy_path);
						}
						/*
						Pack filename processing and exclusion would have been done by the java crunchypack setup.
						Without that, file finding and verification must be done here.

						Windows routine for extracting filenames from a folder. Automatically excludes any files NOT containing a .bmp extension. 
						However, .bmp folders of the wrong bit depth may still be included. Need to open the file to check the header. 
						*/
						WIN32_FIND_DATA w32_data_finding;
						LARGE_INTEGER filesize;
						TCHAR szDir[MAX_PATH];
						size_t length_of_arg;
						HANDLE hFind = INVALID_HANDLE_VALUE;
						DWORD dwError = 0;

						STRSAFE_LPCWSTR crunchy_to_lpwstr = crunchy_path.c_str(); //convert file wstring to a LPCWSTR for windows methods. 

						StringCchCopy(szDir, MAX_PATH, crunchy_to_lpwstr);
						StringCchCat(szDir, MAX_PATH, TEXT("\\*"));


						hFind = FindFirstFile(szDir, &w32_data_finding);

						if (INVALID_HANDLE_VALUE == hFind)
						{
							std::wcout << "The program could not find that folder.\n";
							continue;
						}
						else {
							//the folder could be opened. Save the path. 
							standalone_folder = crunchy_path;
						}

						do
						{
							//enumerate all files and folders inside the folder
							if (w32_data_finding.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
							{

							}
							else
							{
								//add any file with the right extension to the name buffer vector
								filesize.LowPart = w32_data_finding.nFileSizeLow;
								filesize.HighPart = w32_data_finding.nFileSizeHigh;
								std::wstring cmp = w32_data_finding.cFileName;
								if (cmp.find(L".bmp") == std::wstring::npos) {
									//file did not contain correct extension
									std::wcout << " File " << w32_data_finding.cFileName << " is not a bitmap. Excluded from import." << '\n';
								}
								else {
									//file is a .bmp
									direct_name_buffer.push_back(w32_data_finding.cFileName);
									//as a convenience, prints all included and excluded files to console
									_tprintf(TEXT("  %s   %ld bytes\n"), w32_data_finding.cFileName, filesize.QuadPart);
								}
							}
						} while (FindNextFile(hFind, &w32_data_finding) != 0);

						FindClose(hFind); //close handle
						set_path = true; 
						quit = true;
						break;
					}
					case 1: {
						//build from crunchypack: already given a file location and filtered images in the crunchypack file itself
						std::wcout << "Enter full path to crunchypack setup file, including name and extension: ";
						if (DEBUG_MODE == 1) {
							crunchy_path = L"C:\\Users\\JamesH\\Documents\\NetBeansProjects\\CleanStencilUI\\cpack_fpath_condense_spack.txt";
						}
						else {
							std::getline(std::wcin, crunchy_path);
						}
						std::wifstream datafile(crunchy_path, std::ios::in);
						if (!datafile.is_open()) {
							std::wcout << "The program could not find the crunchypack setup file.\nRemember to include the full file name and .txt extension.";
							continue;
						}
						else {
							//read the file into a buffer and save the string for extraction. 
							buffer << datafile.rdbuf();
							source_data = buffer.str();
							set_path = true;
							quit = true;
							datafile.close(); //close the file. 
						}
						break;
					}
					default:
						break;
					}
				}
				

				if (quit = true) {
					//one of the above build porcesses compleated successfully. 
					if (setup_state == 0) {
						//in standalone, user needs to choose a package name. 
						std::wcout << "Choose a name for the package: ";
						if (DEBUG_MODE == 1) {
							standalone_name = L"Debug";
						}
						else {
							std::getline(std::wcin, standalone_name);
						}

					}
					std::wcout << "Setup success! Processing..." << std::endl;
					break;
				}
			}
        }
    }
    else
    {
        wprintf(L"A Fatal Error: GetModuleHandle failed : has occured.\n");
        nRetCode = 1;
    }

    return nRetCode;
}
