#pragma once

/*Minimap collection manager class.
This single object manages the collection minimap objects that represent the set
of unique pixels from each image. 
*/
#ifndef COLLECT_MINIMAP_H
#define COLLECT_MINIMAP_H

#include <vector>
#include <string>
#include "minimap.h"
#include <string>

class collect_minimap
{
public:
	collect_minimap();
	void collect_minimap::add_map(int*, int, int, std::wstring);
	int collect_minimap::access_size_source(int object) const;
	int* collect_minimap::access_minimap_source(int object) const;
	int collect_minimap::vec_size() const;
	int collect_minimap::access_obj_id(int object) const;
	void collect_minimap::compact_maps();
	void collect_minimap::mark_ambiguous_pixel(int object_number, int pixel_number);
	std::string collect_minimap::data_string();
	~collect_minimap();
private:
	/*
	Vector of all current minimap objects representing an image collection & a provided iterator. 
	*/
	std::vector<minimap*> index_minimaps;
	std::vector<minimap* >::const_iterator iter_cminmaps;
	int assign_id;
};

#endif // !COLLECT_MINIMAP_H