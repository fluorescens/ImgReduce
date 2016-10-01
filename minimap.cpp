#include "stdafx.h"
#include "minimap.h"
#include <string>

minimap::minimap(int* insource, int sourcesize, int assign_id, int group_assign, std::wstring objname)
{
	identifier = objname;
	major_group = group_assign;
	object_id = assign_id;
	source = insource;
	indiv_buffer_size = sourcesize;
	indiv_pix = indiv_buffer_size / 4;
}

std::string minimap::unit_data() const
{
	//provides string representation of object id paired with the current number of unique pixels in the source array
	std::string line;
	line = std::to_string(object_id) + " " + std::to_string(indiv_pix);
	return line;
}


/*
Allocates a new source array composed of all unmarked pixels, copies the pixels, then releases the original marked array. 
*/
void minimap::compact_map()
{
	int dead_blocks = 0;
	for (int i = 0; i < indiv_buffer_size; i += 4) {
		if (source[i] == -1) {
			++dead_blocks;
		}
	}

	int* new_source;
	if (dead_blocks == indiv_buffer_size / 4) {
		new_source = nullptr;
	}
	else {
		int external_ctr = 0;
		new_source = new int[(indiv_buffer_size - dead_blocks * 4)];
		if (new_source == nullptr) {
			int msgboxID = MessageBox(
				NULL,
				(LPCWSTR)L"The system could not allocate enough memory for a critical operation. Program Aborted.",
				(LPCWSTR)L"ERROR code 1x01",
				MB_OK
			);
			std::abort();
		}
		for (int i = 0; i < indiv_buffer_size; ++i) {
			if (source[i] == -1) {
				i += 3;
			}
			else {
				new_source[external_ctr] = source[i];
				++external_ctr;
			}
		}
	}

	indiv_buffer_size = (indiv_buffer_size - dead_blocks * 4);
	int* freethis = source;
	source = new_source;
	delete[] freethis;

	std::cout << "IDP: " << object_id << " :";
	for (int i = 0; i < indiv_buffer_size; ++i) {
		std::cout << (int)source[i] << " ";
	}
	std::cout << '\n';
}

/*
Markes a pixel that has experienced a collision with a -1 on all of its components.  
*/
void minimap::mark_pixel(int mark)
{
	int index_before = mark * 4; //gets to first pixel in set. 
	for (int i = index_before; i < (index_before + 4); ++i) {
		source[i] = -1;
	}
}


int minimap::access_id() const
{
	return object_id;
}

minimap::~minimap()
{
	delete[] source; 
}
