#include "stdafx.h"
#include "minimap.h"
#include <string>

minimap::minimap(BYTE* insource, int sourcesize, int assign_id, int group_assign, std::wstring objname)
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
Upon being notified of a collission by the manager, the location of the pixel to remove is sent to the minimap object
*/
void minimap::remove_pixel(int remove)
{
	//removes a single pixel from this BYTE source array
	//need to copy over the before and after 
	int new_buffer_size = indiv_buffer_size - 4;
	if (new_buffer_size == 0) {
		//break out of the function, the object has no more unique points (nullptr source) and should be excluded from further pack builds. 
		std::wcout << "OBJ: " << object_id << " has no unique pixels." << '\n';
		indiv_buffer_size = 0; //update the size 
		source = nullptr;
		delete[] source;
		return;
	}
	//Resizing cases: pixel removed is at beginning, last, or middle. 
	//Allocate an array of the size minus a pixel and copy over the before and after portions of source to the new buffer. 
	BYTE* newsource = new BYTE[new_buffer_size];
	if (newsource == nullptr) {
		//Abort condition: OS failed to allocate enough memory. 
		int msgboxID = MessageBox(
			NULL,
			(LPCWSTR)L"The system could not allocate enough memory for a critical operation. Program Aborted.",
			(LPCWSTR)L"ERROR code 1x01",
			MB_OK
			);
		std::abort();
	}
	//copy the portion before the delete, skip the removed pixel, then copy the after delete. 
	int index_before = remove * 4; //0
	int index_after = indiv_buffer_size - index_before;
	for (int i = 0; i < index_before; ++i) {
		newsource[i] = source[i];
	}
	for (int i = index_before; i < indiv_buffer_size - 4; ++i) {
		newsource[i] = source[i + 4];
	}

	indiv_buffer_size = new_buffer_size;
	BYTE* freethis = source;
	this->source = newsource; //update source pointer to new buffer. 
	delete[] freethis; //free old source memory
}


void minimap::search_source_for_pxl(int red, int green, int blue) {
	//since the hashmap can't tell me the Y of the parent pixel to be cleared, we'll have to get RBG from child and search manually 
}

int minimap::access_id() const
{
	return object_id;
}

minimap::~minimap()
{
	delete[] source; 
}
