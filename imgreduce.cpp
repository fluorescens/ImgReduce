/*
A general implementation of the image hashing algorithm used by Crunchypack. 
Takes a collection of 24-bitmap images and reduces each image down to the pixels that are unique to the image among the set. 
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

const int DEBUG_MODE = 1; //global debug state. 1 for debug enabled. 

/*
Forward declarations list. Defined in definition. 
*/
std::vector<std::vector<std::wstring>> extract_fpath_from_file(const std::wstring&);
std::vector<std::wstring> extract_group_from_file(const std::vector<std::vector<std::wstring>>&);
std::vector<std::vector<std::wstring>> extract_filedata_from_file(const std::vector<std::vector<std::wstring>>&);
int pkg_table_to_file(const std::wstring&,
	const std::vector<std::vector<int>>&,
	const std::vector<std::vector<int>>&,
	const std::vector<std::vector<int>>&,
	const std::vector<std::wstring>&,
	const std::vector<std::vector<std::wstring>>&);

int pkg_map_to_file(std::unordered_map<long int, int>&,
	const std::wstring&,
	const std::vector<std::wstring>&,
	const std::vector<std::vector<std::wstring>>&);

void colission_removal_in_source(std::vector<std::vector<int>>&, std::vector<std::vector<int>>&, collect_minimap&);

int main()
{
	int nRetCode = 0;
	HMODULE hModule = ::GetModuleHandle(nullptr);
	if (hModule != nullptr)
	{
		// initialize MFC and print and error on failure
		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
		{
			wprintf(L"Fatal Error: MFC initialization failed\n");
			nRetCode = 1;
		}
		else
		{
			collect_minimap cmapref; //the one and only minimap collection manager object 


			//set up initial selection dialogue
			std::wstring crunchy_path; //path to image folder
			std::wstringstream buffer; //read in crunchypack setup file, if chosen
			std::wstring source_data; //string of crunchypack data
			std::wstring setup_choice; //dialogue choice of standalone or prebuilt pack
			int setup_state = -1; //initialize the state to not 0 or 1
			std::vector<std::wstring> direct_name_buffer; //list of image file names from a standalone choice
			std::wstring standalone_folder; //path to image folder
			std::wstring standalone_name; //name of the file to be output
			std::wcout << "Welcome to CrunchyPack Generator: IMGReduce 1.0!" << '\n';
			std::wcout << "For screen adapter compatibility ensure all images are 24-bitmaps.\nThere are different types of bitmap(.bmp) formats and only 24-bitmaps should be used for this purpose. \n"
				<< "To check if a file is a 24 bitmap, right click the image, click properties and under the details tab check bit depth. \n"
				<< "This value should be 24; a 4 or 16 are not screen-compatible formats. Recrop and save the desired images from the source as 24-bitmaps. \n"
				<< "This program ONLY accepts bitmap formats. If you sure are the images are in the correct format for your purpose, proceed.\n\n";
			while (true) {
				bool quit = false;
				if (setup_state != 0 || setup_state != 1) {
					std::wcout << "Standalone images(i) or build from a crunchypack template(p)? (enter character):";
					std::getline(std::wcin, setup_choice);
					switch (setup_choice[0]) {
					case('i'):
						//standalone images
						setup_state = 0;
						break;
					case('p'):
						//build from pack
						setup_state = 1;
						break;
					default:
						std::wcout << "No such option is available. Options include i or p" << '\n';
						continue;
					}
				}

				switch (setup_state) {
					//case 0: create a table from a collection of standalone images
				case 0: {
					std::wcout << "Enter full filepath from root to 24-bitmap containing folder: ";
					if (DEBUG_MODE == 1) {
						crunchy_path = L"C:\\Users\\JamesH\\Pictures\\Screenshots\\minecraft_test_icons";
					}
					else {
						std::getline(std::wcin, crunchy_path);
					}
					/*
					Pack filename processing and exclusion would have been done by the java crunchypack setup.
					Without that, file finding and verification must be done here.

					Windows specific code for testing if folder exists and enumerating the files of a folder. 
					*/
					WIN32_FIND_DATA w32_data_finding;
					LARGE_INTEGER filesize;
					TCHAR szDir[MAX_PATH];
					size_t length_of_arg;
					HANDLE hFind = INVALID_HANDLE_VALUE;
					DWORD dwError = 0;

					STRSAFE_LPCWSTR crunchy_to_lpwstr = crunchy_path.c_str();

					StringCchCopy(szDir, MAX_PATH, crunchy_to_lpwstr);
					StringCchCat(szDir, MAX_PATH, TEXT("\\*"));


					hFind = FindFirstFile(szDir, &w32_data_finding);
					//failed to find the provided folder. abort current enumeration
					if (INVALID_HANDLE_VALUE == hFind)
					{
						std::wcout << "The program could not find that folder.\n";
						continue;
					}
					else {
						standalone_folder = crunchy_path;
					}

					//enumerate all .bmp extended files in a valid folder. 
					do
					{
						if (w32_data_finding.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
						{

						}
						else
						{
							//filters files by selecting only those with a bitmap extension. 
							filesize.LowPart = w32_data_finding.nFileSizeLow;
							filesize.HighPart = w32_data_finding.nFileSizeHigh;
							std::wstring cmp = w32_data_finding.cFileName;
							if (cmp.find(L".bmp") == std::wstring::npos) {
								//notified of fail-to-import due to matching
								std::wcout << " File " << w32_data_finding.cFileName << " is not a bitmap. Excluded from import." << '\n';
							}
							else {
								direct_name_buffer.push_back(w32_data_finding.cFileName);
								_tprintf(TEXT("  %s   %ld bytes\n"), w32_data_finding.cFileName, filesize.QuadPart);
							}
						}
					} while (FindNextFile(hFind, &w32_data_finding) != 0);

					FindClose(hFind);
					quit = true;
					break;
				}
				//case 1: use the files already listed/verified from a prebuilt crunchypack 
				case 1: {
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
						buffer << datafile.rdbuf();
						source_data = buffer.str();
						quit = true;
					}
					break;
				}
				default:
					break;
				}

				if (quit = true) {
					if (setup_state == 0) {
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

			//processes the data in the vector buffers depending on standalone(0) or prebuilt(1) import state
			switch (setup_state) {
			case 0: {
				//if the escaped \ at the end of the file path was not supplied, append it
				if (standalone_folder[standalone_folder.size() - 1] != '\\') {
					standalone_folder += L"\\";
				}
				master_filepath = standalone_folder;
				chosen_packname = standalone_name;
				group_names.push_back(L"none");
				//real file name -alias name -group assignment (unassigned by default in this mode. all images in same group)
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
				//prebuilt pack is processed 
				master_tokenize = extract_fpath_from_file(source_data);
				master_filepath = master_tokenize[0][0];
				chosen_packname = master_tokenize[1][0];
				group_names = extract_group_from_file(master_tokenize);
				file_names = extract_filedata_from_file(master_tokenize);
				break;
			}



			//debug print of the imported files
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


			//rough estimate of workload completion
			const int perfile_progress = (100 / file_names.size()) / 3;
			int progress = 0;

			//for each file, assemble the filename from the master filepath and each image vector name
			for (int i = 0; i < file_names.size(); ++i) {
				std::wstring wstr_to_file = master_filepath + file_names[i][0];
				LPCWSTR path_to_file = wstr_to_file.c_str();
				int group_property = std::stoi(file_names[i][2]);
				std::wstring obj_alias = file_names[i][1];
				std::wcout << path_to_file;
				if (DEBUG_MODE == 1) {
					std::wcout << "\n\n" << "Processing : " << file_names[i][0] << '\n';
				}
				//pass each image path to the hash-against-self function
				LoadInitialImage(path_to_file, cmapref, group_property, obj_alias);
				progress += perfile_progress;
				std::wcout << "Progress: " << progress << "%" << '\n';
			}

			std::vector<std::vector<int>> gcollide_handling_firstocc; //arrays for holding multiple-colission values, first colission event
			std::vector<std::vector<int>> gcollide_handling_secondocc; //second colission events

			//hash-against-collection function
			//fills the collision vectors with the object ID and pixel number of collisions 
			std::unordered_map<long int, int> tripart_hashmap = delete_global_duplicates(cmapref, gcollide_handling_firstocc, gcollide_handling_secondocc);

			//performs the removal of marked pixels and resizing of minimap buffers to the unique collection size
			colission_removal_in_source(gcollide_handling_firstocc, gcollide_handling_secondocc, cmapref);

			//clear bad object names from source. 
			std::vector<std::vector<std::wstring>> images_with_unique;



			/*
			Table feature disabled. Not supported for building stencilpacks. 
			*/
			//Builds [Color, like 125 red, matches....] ObjectID1, ObjectID2...
			//this is the vectors from which the table is built
			//Vector of 256 empty vectors, one per color. 
			std::vector<std::vector<int>> redbucket(256); //the BGR vectors
			std::vector<std::vector<int>> greenbucket(256);
			std::vector<std::vector<int>> bluebucket(256);

			//O(1) time access for an image...directly. 
			//load red, with img ID: EXAMPLE 112: 7, 9, 5
			/*
			for (int i = 0; i < cmapref.vec_size(); ++i) {
				if (cmapref.access_size_source(i) != 0) {
					//if the object has nothing in its buffer (entirely duplicate) it should be purged from the images table along with its name.
					//this is a useless object for identification
					images_with_unique.push_back(file_names[i]);

					int* csource = cmapref.access_minimap_source(i);

					for (int r = 0; r < cmapref.access_size_source(i); r += 4) {
						int bbucket = csource[r];
						int gbucket = csource[r + 1];
						int rbucket = csource[r + 2];

						if (bbucket == 48) {
							std::cout << "SS " << i << " " << bbucket << " " << gbucket << " " << rbucket << '\n';
						}

						if (gbucket == 48) {
							std::cout << "GB " << i << " " << bbucket << " " << gbucket << " " << rbucket << '\n';
						}


						if (bluebucket[(int)csource[r]].empty() == true) {
							bluebucket[(int)csource[r]].push_back(i);
						}
						else {
							if (bluebucket[(int)csource[r]].back() == i) {
								//Do nothing, don't double-add. 
							}
							else {
								int b = 4;
								bluebucket[(int)csource[r]].push_back(i);
							}
						}


						if (greenbucket[(int)csource[r + 1]].empty() == true) {
							greenbucket[(int)csource[r + 1]].push_back(i);
						}
						else {
							if (greenbucket[(int)csource[r + 1]].back() == i) {
								//Do nothing, don't double-add. 
							}
							else {
								int b = 4;
								greenbucket[(int)csource[r + 1]].push_back(i);
							}

						}


						if (redbucket[(int)csource[r + 2]].empty() == true) {
							redbucket[(int)csource[r + 2]].push_back(i); //fill an empty bucket
						}
						else {
							if (redbucket[(int)csource[r + 2]].back() == i) {
								//Avoids 122: 0, 0, 0. if P has 3 unique pixels all starting with 122. Only need to look up 122 once. 
							}
							else {
								int b = 4;
								redbucket[(int)csource[r + 2]].push_back(i); //add to a full bucket
							}
						}

					}
				}
				else {
					//Object has no unique pixels. Exclude from collection. 
				}
			}
			*/
			
			//int write_status = pkg_table_to_file(chosen_packname, redbucket, greenbucket, bluebucket, group_names, images_with_unique); //writes the composite table to file. 
			
			
			
			
			//Write the map to the output file: only map output supported at this time (1.0 build) for stencilpack building. 
			int write_map_status = pkg_map_to_file(tripart_hashmap, chosen_packname, group_names, images_with_unique);
			if (write_map_status == 0) {
				std::wcout << "Crunchypack compression table file successfully created!" << '\n';
			}

		}
	}
	else
	{
		// Module failed to initialize. 
		wprintf(L"Fatal Error: GetModuleHandle failed\n");
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

	//load the image from source into memory
	HBITMAP hbmScreen = NULL;
	BITMAP bmpScreen;
	HDC hdcWindow;
	hbmScreen = (HBITMAP)LoadImage(NULL, filename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	if (!hbmScreen)
	{
		//if the system cannot allocate enough ram, trigger a hard abort. the operation can never be successfully compleated. 
		MessageBox(NULL, L"Failed to acquire bitmap. Program aborted.", L"Error 0x02", MB_OK);
		std::abort();
	}
	else {
		//MessageBox(NULL, L"Successful acquisition", L"Success", MB_OK);
	}


	GetObject(hbmScreen, sizeof(BITMAP), &bmpScreen); //get bitmap object dimensions for buffer calculations
	DWORD dwBmpSize = bmpScreen.bmWidth * 4 * bmpScreen.bmHeight;
	BYTE* lpbitmap = new BYTE[dwBmpSize];
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
	for (int i = 0; i < (dwBmpSize); i += 4) {
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

	//reduce the bitmap buffer to contain only the minimum uniques and return it. 
	//reduce lpbitmap to unique-only; 

	DWORD compact_bmap_size = 4 * uniqueplx.size();
	//HANDLE h_compact_mem = GlobalAlloc(GHND, compact_bmap_size);
	//BYTE* compact_bmap = (BYTE *)GlobalLock(h_compact_mem);
	int* compact_bmap = new int[compact_bmap_size];
	if (compact_bmap == NULL) {
		//could not allocate enough memory for the operation. hard abort: cannot complete operation
		MessageBox(NULL, L"System could not allocate sufficient memory. Program aborted.", L"Error 0x03", MB_OK);
		std::abort();
	}
	int tk = 0;
	for (inter_vec = uniqueplx.begin(); inter_vec != uniqueplx.end(); ++inter_vec) { 
		//uniquepxl is the locations of all the 4-part unique structures
		//copy the unique values into the new buffer pixel by pixel 
		compact_bmap[tk] = (int)lpbitmap[(*inter_vec)];
		compact_bmap[tk + 1] = (int)lpbitmap[(*inter_vec + 1)];
		compact_bmap[tk + 2] = (int)lpbitmap[(*inter_vec + 2)];
		compact_bmap[tk + 3] = (int)lpbitmap[(*inter_vec + 3)];
		tk += 4;
	}

	//print the entire contents of minimap buffer. 
	if (DEBUG_MODE == 1) {
		std::wcout << "Minimaps compact pixels CB: " << '\n';
		for (int t = 0; t < compact_bmap_size; t += 4) {
			std::cout << compact_bmap[t] << " " << compact_bmap[t + 1] << " " << compact_bmap[t + 2] << " " << compact_bmap[t + 3];
			if (compact_bmap[t] == 47 && compact_bmap[t + 1] == 47 && compact_bmap[t + 2] == 47) {
				std::cout << "					MGMPOP";
			}
			if (compact_bmap[t] == 17 && compact_bmap[t + 1] == 48 && compact_bmap[t + 2] == 45) {
				std::cout << "					CPOP";
			}
			std::cout << '\n';
		}
	}

	//add the image-unique bmap array to the collection 
	minimap_collection.add_map(compact_bmap, compact_bmap_size, object_group, object_name); 
																							
	delete[] lpbitmap;
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



/*
Builds a map of all unique tripart color objects using a rolling hash.
Backtracks and marks single and multi collision points for deletion from the image minimap buffers.  
*/
std::unordered_map<long int, int> delete_global_duplicates(collect_minimap& cmapref, std::vector<std::vector<int>>& global_collide_handling_firstocc, std::vector<std::vector<int>>& global_collide_handling_secondocc) {
	//handle global duplicates on their first collision by clearing from both the previous and current image 
	global_collide_handling_firstocc.resize(cmapref.vec_size()); //has as many vectors as images
																 //0: 4, 32 
																 //BUCKET collides with OBJECT at OBJECT_PIXEK(Y): Remove Y from both bucket and object

	//since the map is already marked at this value (known to have had previous colissions) only remove this pixel from the current object
	global_collide_handling_secondocc.resize(cmapref.vec_size());
	//FORMAT: Bucket: Pixel
	//PIXEL is a well-known colission point. Remove PIXEL from BUCKET



	std::unordered_map<long int, int> tripart_obj;
	const int hash_badunique = cmapref.vec_size() + 1; //a value larger than the largest item ID
	for (int i = 0; i < cmapref.vec_size(); ++i) {
		//request: the 4-part elements, need access to minimap source buffer
		int* dstart = cmapref.access_minimap_source(i);
		int bufsize = cmapref.access_size_source(i); //number of pixels in object minimap
		for (int y = 0; y < bufsize; y += 4) {
			//rolling hash each BGR pixel
			long int h1 = (dstart[y] * 257 + dstart[y + 1]) % 1000000007; //b-g	
			long int h2 = (h1 * 257 + dstart[y + 2]) % 1000000007; //bg-r	
			long int h3 = (h2 * 257 + dstart[y]) % 1000000007; //bgr-b		


			if (tripart_obj.find(h3) == tripart_obj.end()) {
				//object is currently unique
				tripart_obj[h3] = i; //in the hash, the item ID is loaded as owning this hash of the pixel
				if (DEBUG_MODE == 1) {
					std::cout << "Add at" << std::to_string((long int)h3) << " " << std::to_string(i) << "(" << (int)dstart[y] << "," << (int)dstart[y + 1] << "," << (int)dstart[y + 2] << ")" << '\n';
				}
			}
			else if (tripart_obj[h3] < hash_badunique) {
				//the pixel was previously unique (objectID in range, < hash_badunique) but just got hit again
				int remove_previous = tripart_obj[h3]; //the object at that hash was originally unique must have its source array cleared
													   //need to find parent value as well. Must retrieve by searching

													   //index is the parent, i is the child, and y is the pixel in the child
													   //global[previously_owned_by] {collides with, at}
				global_collide_handling_firstocc[remove_previous].push_back(i); //push back objectID
				global_collide_handling_firstocc[remove_previous].push_back(y / 4); //push back pixel
				tripart_obj[h3] = hash_badunique; //mark this pixel as a multi-hit point now so on second clear we ONLY clear from child. 
												  //Don't double-delete
				if (DEBUG_MODE == 1) {
					std::cout << "WARNING: object first CL" << '\n';
					std::cout << "First order colission: Object " << remove_previous << " with Object " << i << "(" << dstart[y] << " " << dstart[y + 1] << " " << dstart[y + 2] << ")" << '\n';
				}
			}
			else if (tripart_obj[h3] == hash_badunique) {
				//tripart is ea known multi-hit value
				//do a simple clear since at this point, the parent has been cleared already
				global_collide_handling_secondocc[i].push_back(y / 4); //ObjectID: Pixel
				if (DEBUG_MODE == 1) {
					std::cout << "WARNING: object bad CL" << '\n';
					std::cout << "Second order colission: Object " << i << "(" << dstart[y] << " " << dstart[y + 1] << " " << dstart[y + 2] << ")" << '\n';
				}
			}
		}
	}

	//marks entries with a multi-colission value to be removed from map to save space on the output
	//do not delete from the map while parsing it (resizing isues), temporarily store values for later
	std::deque<long int> clear_keys;
	for (std::unordered_map<long int, int>::iterator iter = tripart_obj.begin(); iter != tripart_obj.end(); ++iter) {
		if (iter->second == cmapref.vec_size() + 1) {
			clear_keys.push_back(iter->first);
		}
		else {

		}
	}

	//clears bad entries from the map
	std::unordered_map<long int, int>::iterator map_iter;
	for (std::deque<long int>::const_iterator iter = clear_keys.begin(); iter != clear_keys.end(); ++iter) {
		map_iter = tripart_obj.find(*iter);
		tripart_obj.erase(map_iter);
	}
	clear_keys.clear();

	return tripart_obj;
}


/*
Backtracks and deletes collision points out of participating minimap buffers.
Reduces minimaps down to a collection of triparts that are unique among all all objects in the collection after execution.
*/
void colission_removal_in_source(std::vector<std::vector<int>>& global_collide_handling_firstocc, std::vector<std::vector<int>>& global_collide_handling_secondocc, collect_minimap& cmapref) {
	//handles the global colission arrays and removes the conflict pixels from the object source. 
	std::cout << '\n' << "Collide handling: ";
	std::vector<int>::const_iterator iter_source;
	//iterates through the parent (first owned unique) and child (tried to own unique) vectors and marks the collision pixels
	for (int j = 0; j < global_collide_handling_firstocc.size(); ++j) {
		if (global_collide_handling_firstocc[j].empty()) {
			//This object experieneced no colissions. Its source pixels are all unique. 
		}
		else {
			for (iter_source = global_collide_handling_firstocc[j].begin(); iter_source != global_collide_handling_firstocc[j].end(); ++iter_source) {
				//each one of these represents a colission
				//first vector guarenteed to only every have one child per tripart, then it'll be marked bad_hash and put into second vector
				//[parent] child_obj, child_pixel
				int parentobj = j;
				int childobj = (*iter_source); //the vector in j
				++iter_source;
				int childpixel = (*iter_source);
				if (DEBUG_MODE == 1) {
					std::cout << std::to_string(j) << " " << std::to_string(childobj) << " " << std::to_string(childpixel) << '\n';
				}
				else {

				}


				//mark from child, returning rbg, then BACKTRACK to delete from parent. 
				//access the bmap source of child, navigate to the pixel, copy the RBG and then call delete on the pixel. 
				//need to handle the optential source of minimap = nullpointer case. 
				int* childsource = cmapref.access_minimap_source(childobj);
				int oset = 0;
				for (int i = 0; i < childpixel; ++i) {
					oset += 4;
				}

				int redmatch = childsource[oset];
				int greenmatch = childsource[oset + 1];
				int bluematch = childsource[oset + 2]; 
				cmapref.mark_ambiguous_pixel(childobj, childpixel);


				//Now mark the parent, need to find the RBG of the parent using the colornatch temps above. 
				int* parentsource = cmapref.access_minimap_source(parentobj);
				int parent_pixel_start = 0;
				for (int i = 0; i < cmapref.access_size_source(parentobj); i += 4) {
					//find the rbg point and return the i, O(n) but very rarely executed 
					if (parentsource[i] == redmatch  &&  parentsource[i + 1] == greenmatch   &&   parentsource[i + 2] == bluematch) {
						parent_pixel_start = i;
						break;
					}
					else {
						//Should never occure. should always be match. 
					}
				} 
				cmapref.mark_ambiguous_pixel(parentobj, parent_pixel_start / 4);
			}
		}
	}

	//iterates through child array only (parent already cleared) marking [objectID] pixel. 
	for (int i = 0; i < global_collide_handling_secondocc.size(); ++i) {
		for (int m = 0; m < global_collide_handling_secondocc[i].size(); ++m) {
			cmapref.mark_ambiguous_pixel(i, m);
		}
	}


	//ALL MARKINGS COMPLETE. 
	cmapref.compact_maps();
}


/*
Prints the file with hashmap style output (Feature compatable with stencilbuilder as of current build.)
*/
int pkg_map_to_file(std::unordered_map<long int, int>& tripart_map,
	const std::wstring& chosen_packname,
	const std::vector<std::wstring>& group_names,
	const std::vector<std::vector<std::wstring>>& file_names) {
	//add functionality to name the pack
	//package the master tables to the binary data file

	std::wstring data_ext = L"Crunchypack_map_";
	std::wstring full_name = data_ext + chosen_packname + L".txt";

	std::wstring alt_name;

	//force the user to select a unique name for the file
	while (true) {
		std::ifstream outloc(full_name, std::ios::in);
		if (outloc.is_open()) {
			outloc.close();
			std::wcout << "A file already exists using the name you proposed.\nAlternate name: ";
			std::getline(std::wcin, alt_name);
			full_name = data_ext + alt_name + L".txt";
		}
		else {
			break;
		}
	}

	//output the file to local directory
	std::wofstream data_out(full_name, std::ios::out);

	//Write Groupnames in a single long line
	//write imagenames in a single long line
	for (int i = 0; i < group_names.size(); ++i) {
		data_out << group_names[i] << ",";
	}
	data_out << '\n';
	for (int i = 0; i < file_names.size(); ++i) {
		for (int k = 0; k < file_names[i].size(); ++k) {
			data_out << file_names[i][k] << ",";
		}
		data_out << '\t';
	}
	data_out << '\n';


	//O(1) time for direct matches to the tripart rolling hash
	for (std::unordered_map<long int, int>::const_iterator iter = tripart_map.begin(); iter != tripart_map.end(); ++iter) {
		data_out << std::to_wstring(iter->first) << " " << std::to_wstring(iter->second) << '\n';
	}

	data_out.close();
	return 0;
}



/*
Prints the file with table style output. 
NOT SUPPORTED for stencilpack builds as of 1.0. 
*/
int pkg_table_to_file(const std::wstring& chosen_packname,
	const std::vector<std::vector<int>>& redbucket,
	const std::vector<std::vector<int>>& greenbucket,
	const std::vector<std::vector<int>>& bluebucket,
	const std::vector<std::wstring>& group_names,
	const std::vector<std::vector<std::wstring>>& file_names) {
	std::wstring data_ext = L"Crunchypack_table_";
	std::wstring full_name = data_ext + chosen_packname + L".txt";

	//test if a similarly named file exists already. If it does, force manual name selection. 

	std::wstring alt_name;

	while (true) {
		std::ifstream outloc(full_name, std::ios::in);
		if (outloc.is_open()) {
			outloc.close();
			std::wcout << "A file already exists using the name you proposed.\nAlternate name: ";
			std::getline(std::wcin, alt_name);
			full_name = data_ext + alt_name + L".txt";
		}
		else {
			break;
		}
	}


	std::wofstream data_out(full_name, std::ios::out);

	//Write Groupnames in a single long line
	//write imagenames in a single long line
	for (int i = 0; i < group_names.size(); ++i) {
		data_out << group_names[i] << ",";
	}
	data_out << '\n';
	for (int i = 0; i < file_names.size(); ++i) {
		for (int k = 0; k < file_names[i].size(); ++k) {
			data_out << file_names[i][k] << ",";
		}
		data_out << '\t';
	}
	data_out << '\n';


	//O(1) time access for an image...directly. 
	//load red, with img ID: EXAMPLE 112: 7, 9, 5
	for (int k = 0; k < 256; ++k) {
		data_out << k << " ";
		if (bluebucket[k].size() > 0) {
			for (int m = 0; m < bluebucket[k].size(); ++m) {
				data_out << bluebucket[k][m] << ",";
			}
			data_out << "\t";
		}
		else {
			data_out << "\t";
		}
		if (greenbucket[k].size() > 0) {
			for (int m = 0; m < greenbucket[k].size(); ++m) {
				data_out << greenbucket[k][m] << ",";
			}
			data_out << "\t";
		}
		else {
			data_out << "\t";
		}
		if (redbucket[k].size() > 0) {
			for (int m = 0; m < redbucket[k].size(); ++m) {
				data_out << redbucket[k][m] << ",";
			}
			data_out << "\t";
		}
		else {
			data_out << "\t";
		}
		data_out << "\n";
	}
	data_out.close();

	//upon successful close, delete the temporary package file from java--------------------------------------------------

	return 0;
}