#include "stdafx.h"
#include "collect_minimap.h"

#include<vector>
#include <iostream>
#include <string>


collect_minimap::collect_minimap()
{
	assign_id = 0;
}

/*
Adds a map to the collection, assigning the sequential ID number it arrives in as the objectID.
*/
void collect_minimap::add_map(BYTE* bsource, int sourcesize, int group_assign, std::wstring objname)
{
	minimap* newminimap = new minimap(bsource, sourcesize, assign_id, group_assign, objname);
	index_minimaps.push_back(newminimap);
	index_minimaps[0]->unit_data();
	++assign_id;
}

/*
Returns the size of the minimap source array
*/
int collect_minimap::access_size_source(int object) const
{
	if (object > index_minimaps.size() - 1 || object < 0) {
		return 0;
	}

	return index_minimaps[object]->indiv_buffer_size;
}

/*
Provides access to the minimap source array
*/ 
BYTE * collect_minimap::access_minimap_source(int object) const
{
	if (object > index_minimaps.size() - 1 || object < 0) {
		return nullptr;
	}
	BYTE* datsource = index_minimaps[object]->source;

	return datsource;
}


/*
Returns the size of the current minimap containing vector of the manager
*/
int collect_minimap::vec_size() const
{
	return index_minimaps.size();
}

/*
Returns the objectID of the minimap object
*/
int collect_minimap::access_obj_id(int object) const
{
	int group_id = index_minimaps[object]->access_id();
	return group_id;
}

/*
Removes from vector index number minimap the pixel_number(th) pixel
*/
void collect_minimap::remove_ambiguous_pixel(int object_number, int pixel_number)
{
	index_minimaps[object_number]->remove_pixel(pixel_number);
}

/*
Printing method for whole minimap containg vector
*/
std::string collect_minimap::data_string()
{
	std::string pkg;

	for (iter_cminmaps = index_minimaps.begin(); iter_cminmaps != index_minimaps.end(); ++iter_cminmaps) {
		pkg += (*iter_cminmaps)->unit_data() + " ";
	}

	return pkg;
}

/*
Delete the minimap objects in the vector manager. 
*/
collect_minimap::~collect_minimap()
{
	for (iter_cminmaps = index_minimaps.begin(); iter_cminmaps != index_minimaps.end(); ++iter_cminmaps) {
		delete (*iter_cminmaps);
	}
}
