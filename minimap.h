#pragma once

/*
A minimap object represents the collection of pixels that uniquely represent an image.
This collection should be stored and accessed only by the one and only collect_minimap object.
These pixels are stored in BGRA format in the objects source array.
It also supports other properties like the images add order and alias, if assigned.
Also supports access to the buffer and will handle its own resizing if called to drop a pixel due to collission by the collect_minimap manager.
*/

#ifndef MINIMAP_H
#define MINIMAP_H
#include <string>
#include <Windows.h>

class minimap
{
public:
	minimap::minimap(int*, int, int, int, std::wstring);
	std::string minimap::unit_data() const;
	void minimap::compact_map();
	void minimap::mark_pixel(int mark);
	int minimap::access_id() const;
	~minimap();
private:
	int indiv_buffer_size; //the size of the minimaps BYTE buffer
	int indiv_pix; //number of pixels in the BYTE buffer
	int object_id; //added ID
	int major_group; //the group ID number
	std::wstring identifier; //the alias name of the object
	int* source; //pointer to the allocated source array
	friend class collect_minimap;
};

#endif // !MINIMAP_H