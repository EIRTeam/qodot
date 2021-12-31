#ifndef QODOT_H
#define QODOT_H
#include "core/variant/dictionary.h"
#include "core/object/ref_counted.h"
#include "core/string/ustring.h"
#include "geo_generator.h"
#include "map_parser.h"
#include "surface_gatherer.h"

class Qodot : public RefCounted {
	GDCLASS(Qodot, RefCounted);
	std::shared_ptr<LMMapData> map_data = std::make_shared<LMMapData>();
	LMMapParser map_parser = LMMapParser(map_data);
	LMGeoGenerator geo_generator = LMGeoGenerator(map_data);
	LMSurfaceGatherer surface_gatherer = LMSurfaceGatherer(map_data);

public:
	void load_map(const String &map_file_str);
	PackedStringArray get_texture_list();
	void set_entity_definitions(Dictionary p_entity_defs);
	void set_worldspawn_layers(Array p_worldspawn_layers);
	void generate_geometry(Dictionary p_texture_dict);
	Array get_entity_dicts();
	Array get_worldspawn_layer_dicts();
	void gather_texture_surfaces(const String p_texture_name, const String p_brush_filter_texture, const String p_face_filter_texture);
	void gather_worldspawn_layer_surfaces(const String p_texture_name, const String p_brush_filter_texture, const String p_face_filter_texture);
	void gather_entity_convex_collision_surfaces(int64_t p_entity_idx);
	void gather_entity_concave_collision_surfaces(int64_t p_entity_idx);
	void gather_worldspawn_layer_collision_surfaces(int64_t p_entity_idx);
	Array fetch_surfaces(double p_inverse_scale_factor);

protected:
	static void _bind_methods();

private:
	void gather_texture_surfaces_internal(const String p_texture_name, const String p_brush_filter_texture, const String p_face_filter_texture, bool p_filter_layers);
	void gather_convex_collision_surfaces(int64_t p_entity_idx, bool p_filter_layers);
	void gather_concave_collision_surfaces(int64_t p_entity_idx, bool p_filter_layers);
};
#endif