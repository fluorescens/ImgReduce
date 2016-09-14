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
#include "collect_minimap.h"
#include "minimap.h"

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

void LoadInitialImage(LPCWSTR filename, collect_minimap& minimap_collection, int object_group, std::wstring object_name); 
std::vector<std::vector<std::wstring>> extract_fpath_from_file(const std::wstring& sourcedat); 
std::vector<std::wstring> extract_group_from_file(const std::vector<std::vector<std::wstring>>& master_vector); 
std::vector<std::vector<std::wstring>> extract_filedata_from_file(const std::vector<std::vector<std::wstring>>& master_vector); 

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

			//file loading variables.
			std::vector<std::vector<std::wstring>> master_tokenize;
			std::wstring master_filepath; //zero-zero is always the filepath to img folder. 
			std::wstring chosen_packname; //1-0 always the name for the pack. 
			std::vector<std::wstring> group_names;
			std::vector<std::vector<std::wstring>> file_names;

			switch (setup_state) {
			case 0: {
				//if the ending \ in the file path was not provided, append it. 
				if (standalone_folder[standalone_folder.size() - 1] != '\\') {
					standalone_folder += L"\\";
				}
				master_filepath = standalone_folder;
				chosen_packname = standalone_name;
				group_names.push_back(L"none");
				//real-alias-group
				for (int i = 0; i < direct_name_buffer.size(); ++i) {
					std::vector<std::wstring> standalone_file_names;
					standalone_file_names.push_back(direct_name_buffer[i]);
					standalone_file_names.push_back(direct_name_buffer[i]);
					standalone_file_names.push_back(L"0");
					file_names.push_back(standalone_file_names);
				}
				break;
			}
			case 1:
				//same data as above but the data comes from crunchypack file
				master_tokenize = extract_fpath_from_file(source_data);
				master_filepath = master_tokenize[0][0];
				chosen_packname = master_tokenize[1][0];
				group_names = extract_group_from_file(master_tokenize);
				file_names = extract_filedata_from_file(master_tokenize);
				break;
			}


			if (DEBUG_MODE == 1) {
				std::wcout << "SRC: " << source_data << "\n\n";
				std::wcout << "Groups: " << '\n';
				for (int i = 0; i < group_names.size(); ++i) {
					std::wcout << group_names[i] << " ";
				}
				std::cout << "\n\n";
				std::cout << "File table" << '\n';
				for (int i = 0; i < file_names.size(); ++i) {
					for (int j = 0; j < file_names[i].size(); ++j) {
						std::wcout << file_names[i][j] << " ";
					}
					std::wcout << '\n';
				}
			}
			else {
				std::wcout << "Processed the data file. Building workload." << std::endl;
			}

			collect_minimap& cmapref = collect_minimap(); //the one and only minimap collection manager object 

			const int perfile_progress = (100 / file_names.size()) / 3;
			int progress = 0;


			for (int i = 0; i < file_names.size(); ++i) {
				//stage one processing of image files
				std::wstring wstr_to_file = master_filepath + file_names[i][0];
				LPCWSTR path_to_file = wstr_to_file.c_str();
				int group_property = std::stoi(file_names[i][2]);
				std::wstring obj_alias = file_names[i][1];
				std::wcout << path_to_file;
				if (DEBUG_MODE == 1) {
					std::wcout << "\n\n" << "Processing : " << file_names[i][0] << '\n';
				}
				LoadInitialImage(path_to_file, cmapref, group_property, obj_alias);
				progress += perfile_progress;
				std::wcout << "Progress: " << progress << "%" << '\n';
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



/*
FUNCTION PURPOSE: 
Opens and reads the bitmaps in the directory or named in the Crunchypack setup.
Removes duplicate pixels WITHIN an image, so only the pixels unique to the image are submitted for hash processing and global-unique candidates
This helps keep the running memory footprint of the object low (no larger than the biggest bitmap at any given time for this stage plus size of all minimap sources) 
*/
void LoadInitialImage(LPCWSTR filename, collect_minimap& minimap_collection, int object_group, std::wstring object_name) {

	HBITMAP hbmScreen = NULL;
	BITMAP bmpScreen;
	HDC hdcWindow;
	hbmScreen = (HBITMAP)LoadImage(NULL, filename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	if (!hbmScreen)
	{
		MessageBox(NULL, L"Failed to acquire bitmap. Program aborted.", L"Error 0x02", MB_OK);
		std::abort();
	}
	else {
		//MessageBox(NULL, L"Successful acquisition", L"Success", MB_OK);
	}


	GetObject(hbmScreen, sizeof(BITMAP), &bmpScreen); //get bitmap object dimensions for buffer calculations
	DWORD dwBmpSize = bmpScreen.bmWidth * 4 * bmpScreen.bmHeight;
	HANDLE hDIB = GlobalAlloc(GHND, dwBmpSize);
	BYTE* lpbitmap = (BYTE *)GlobalLock(hDIB);
	if (lpbitmap == NULL) {
		//abort the program. Failure to acquire memory in an unrecoverable error. 
		MessageBox(NULL, L"Failed to allocate bitmap memory. Program aborted.", L"Error 0x03", MB_OK);
		std::abort();
	}
	GetBitmapBits(hbmScreen, dwBmpSize, lpbitmap);
	//handle to bitmap, size of buffer, pointer to buffer. So we just need to size the buffer.. 
	if (DEBUG_MODE == 1) {
		//number of bytes allocated in RAM to the image self-hashing stage. 
		std::wcout << "Allocated bytes: " << (int)dwBmpSize << '\n';
	}




	//hashing system, image-with-self
	std::unordered_map<int, BOOL> Rreduction;
	std::unordered_map<int, BOOL> Greduction;
	std::unordered_map<int, BOOL> Breduction;
	std::vector<int> uniqueplx;
	int threepoint = 0;
	for (int i = 0; i < (dwBmpSize / 4); i += 4) {
		//iterates through every pixel and finds all the unique pixels within the image
		threepoint = 0;
		//find redunique
		if (Rreduction.find((int)lpbitmap[i + 2]) == Rreduction.end()) {
			//not in the map already
			//std::cout << " " << i << ":" << (int)lpbitmap[i] << "R "; 
			Rreduction[(int)lpbitmap[i + 2]] = true;
			++threepoint;
		}
		else {
			//found such an R key
		}

		if (Greduction.find((int)lpbitmap[i + 1]) == Greduction.end()) {
			//not in the map already
			//std::cout << " " << (i+1) << ":" << (int)lpbitmap[i + 1] << "G ";
			Greduction[(int)lpbitmap[i + 1]] = true;
			++threepoint;
		}
		else {
			//found such an G key
		}

		if (Breduction.find((int)lpbitmap[i]) == Breduction.end()) {
			//not in the map already
			//std::cout << " " << (i + 2) << ":" << (int)lpbitmap[i + 2] << "B " << '\n';
			Breduction[(int)lpbitmap[i]] = true;
			++threepoint;
		}
		else {
			//found such an B key
		}



		if (threepoint >= 1) {
			uniqueplx.push_back(i); //pushes back the pixel location within the full bmap
		}
		else {
			//the tripart structure is not unique.
		}
	}






	std::vector<int>::const_iterator inter_vec;
	if (DEBUG_MODE == 1) {
		std::wcout << "Unique pixel within large array: " << '\n';
		for (inter_vec = uniqueplx.begin(); inter_vec != uniqueplx.end(); ++inter_vec) {
			std::wcout << (int)lpbitmap[(*inter_vec)] << " " << (int)lpbitmap[(*inter_vec + 1)] << " " << (int)lpbitmap[(*inter_vec + 2)] << " " << (int)lpbitmap[(*inter_vec + 3)];
			std::wcout << '\n';
		}
	}

	//reduce the bitmap buffer to contain only the minimum uniques and return it. 
	//reduce lpbitmap to unique-only; 
	//newalloc something 

	DWORD compact_bmap_size = 4 * uniqueplx.size();
	//HANDLE h_compact_mem = GlobalAlloc(GHND, compact_bmap_size);
	//BYTE* compact_bmap = (BYTE *)GlobalLock(h_compact_mem);
	BYTE* compact_bmap = new BYTE[compact_bmap_size];
	if (compact_bmap == NULL) {
		MessageBox(NULL, L"System could not allocate sufficient memory. Program aborted.", L"Error 0x03", MB_OK);
		std::abort();
	}
	int tk = 0;
	for (inter_vec = uniqueplx.begin(); inter_vec != uniqueplx.end(); ++inter_vec) { //uniquepxl is the locations of all the 4-part unique structures
																					 //copy the unique values into the new buffer pixel by pixel 
		compact_bmap[tk] = (int)lpbitmap[(*inter_vec)];
		compact_bmap[tk + 1] = (int)lpbitmap[(*inter_vec + 1)];
		compact_bmap[tk + 2] = (int)lpbitmap[(*inter_vec + 2)];
		compact_bmap[tk + 3] = (int)lpbitmap[(*inter_vec + 3)];
		tk += 4;
	}

	if (DEBUG_MODE == 1) {
		std::wcout << "Minimaps compact pixels: " << '\n';
		for (int t = 0; t < compact_bmap_size; t += 4) {
			std::cout << (int)compact_bmap[t] << " " << (int)compact_bmap[t + 1] << " " << (int)compact_bmap[t + 2] << " " << (int)compact_bmap[t + 3];
			std::cout << '\n';
		}
	}

	minimap_collection.add_map(compact_bmap, compact_bmap_size, object_group, object_name); //add the image-unique bmap array to the collection
	GlobalFree(hDIB); //release the memory associated with the original large bitmap, no longer needed. 
}




/*
Extracts the path to the bitmap source folder from the crunchypack setup file
*/
std::vector<std::vector<std::wstring>> extract_fpath_from_file(const std::wstring& sourcedat) {
	//this file processes the produced file from the interface into usable components
	//pre-tokenize the whole file
	std::wstring substr;
	int vec_load = 0;
	const int vec_dynamic = 100; //add 5 more rows to the vector. 
	std::vector<std::vector<std::wstring>> data_parse(100);
	for (int i = 0; i < sourcedat.length(); ++i) {
		if (sourcedat[i] == '\n') {
			data_parse[vec_load].push_back(substr);
			substr.clear();
			++vec_load;
			if (vec_load % vec_dynamic == 0) {
				int resize_req = vec_load + vec_dynamic; //resize the vector to hold all components
				data_parse.resize(resize_req);
			}

		}
		else {
			substr += sourcedat[i];
		}
	}

	for (int i = 0; i < data_parse.size(); ++i) {
		if (data_parse[i].empty() == true) {

		}
		else {
			std::wcout << data_parse[i][0] << '\n';
		}
	}

	//error checking. zero-zero should only every have one entry, as should name and groups. 
	assert(data_parse[0].size() == 1);
	assert(data_parse[1].size() == 1);
	assert(data_parse[2].size() == 1);

	return data_parse;
}


/*
Extracts groups from the crunchypack setup file
*/
std::vector<std::wstring> extract_group_from_file(const std::vector<std::vector<std::wstring>>& master_vector) {
	//this procudes a vector of group names, with the end of the vector always being the default "none"
	const int defined_groups = 2;
	std::vector<std::wstring> vector_gname;

	std::wstring substr;
	for (int i = 0; i < master_vector[defined_groups][0].length(); ++i) {
		if (master_vector[defined_groups][0][i] == ',') {
			vector_gname.push_back(substr);
			substr.clear();
		}
		else {
			substr += master_vector[defined_groups][0][i];
		}
	}

	return vector_gname;
}


/*
Extracts the image bitmap file names from the crunchypack setup file. 
*/
std::vector<std::vector<std::wstring>> extract_filedata_from_file(const std::vector<std::vector<std::wstring>>& master_vector) {
	//this procudes a vector of image real name (for location purposes), its alias string, and its group name. Used in minimap construction.
	std::wstring extract_fname;
	std::vector<std::vector<std::wstring>> vector_fname;
	vector_fname.resize(master_vector.size());
	int linecounter = 0;
	const int fdata_start = 3;
	for (int i = fdata_start; i < master_vector.size(); ++i) {
		if (master_vector[i].empty() == true) {
			break; //vector capacity may exceed number of files due to dynamic resizing. But all files are guarenteed to be contiguous. 
		}
		std::wstring substr = master_vector[i][0]; //each master vector row contains one data line 
		std::wstring subfile;
		for (int k = 0; k < substr.length(); ++k) {
			if (substr[k] == ',') {
				vector_fname[linecounter].push_back(subfile);
				subfile.clear();
			}
			else {
				subfile += substr[k];
			}
		}
		++linecounter;
	}

	vector_fname.resize(linecounter);
	return vector_fname;
}