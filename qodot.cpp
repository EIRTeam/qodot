#include "qodot.h"

#include "geo_generator.h"
#include "map_data.h"
#include "map_parser.h"
#include "surface_gatherer.h"
#include "scene/resources/mesh.h"

void Qodot::load_map(const String &map_file_str) {
	CharString map_file = map_file_str.utf8();
	map_parser.map_parser_load(map_file);
}

PackedStringArray Qodot::get_texture_list() {
	PackedStringArray g_textures;
	int tex_count = map_data->map_data_get_texture_count();
	LMTextureData *textures = map_data->map_data_get_textures();

	g_textures.resize(tex_count);

	for (int i = 0; i < tex_count; i++) {
		LMTextureData *texture = &textures[i];
		String g_name;
		g_name.parse_utf8(texture->name);
		g_textures.set(i, g_name);
	}

	return g_textures;
}

void Qodot::set_entity_definitions(Dictionary p_entity_defs) {
	for (int i = 0; i < p_entity_defs.size(); i++) {
		String key = p_entity_defs.get_key_at_index(i);
		int value = p_entity_defs.get_value_at_index(i).get("spawn_type");
		CharString key_cs = key.utf8();
		map_parser.map_data->map_data_set_spawn_type_by_classname(key_cs, (ENTITY_SPAWN_TYPE)value);
	}
}

void Qodot::set_worldspawn_layers(Array p_worldspawn_layers) {
	for (int i = 0; i < p_worldspawn_layers.size(); i++) {
		Dictionary worldspawn_layer = p_worldspawn_layers.get(i);

		bool build_visuals = false;
		CharString texture = NULL;

		for (int k = 0; k < worldspawn_layer.size(); k++) {
			String key = worldspawn_layer.get_key_at_index(k);
			if (key == "texture") {
				String value = worldspawn_layer.get_value_at_index(k);
				texture = value.utf8();
			} else if (key == "build_visuals") {
				build_visuals = worldspawn_layer.get_value_at_index(k);
			}
		}
		map_data->map_data_register_worldspawn_layer(texture, build_visuals);
	}
}

void Qodot::generate_geometry(Dictionary p_texture_dict) {
	Array texture_keys = p_texture_dict.keys();
	for (int i = 0; i < texture_keys.size(); i++) {
		String texture_key = texture_keys.get(i);
		Vector2 value = p_texture_dict.get_value_at_index(i);

		int width = value.x;
		int height = value.y;

		CharString texture_key_cs = texture_key.utf8();

		map_data->map_data_set_texture_size(texture_key_cs, width, height);
	}
	geo_generator.geo_generator_run();
}

Array Qodot::get_entity_dicts() {
	int ent_count = map_data->map_data_get_entity_count();
	const LMEntity *ents = map_data->map_data_get_entities();

	Array ent_dicts;

	for (int i = 0; i < ent_count; i++) {
		const LMEntity *ent = &ents[i];
		Dictionary entity_dict;

		// Brush count
		entity_dict["brush_count"] = ent->brush_count;

		// Brush indices
		PackedInt64Array brush_indices;

		for (int b = 0; b < ent->brush_count; b++) {
			const LMBrush *brush = &ent->brushes[b];
			bool is_worldspawn_layer_brush = false;

			for (int f = 0; f < brush->face_count; f++) {
				const face *face = &brush->faces[f];

				if (map_data->map_data_find_worldspawn_layer(face->texture_idx) != -1) {
					is_worldspawn_layer_brush = true;
					break;
				}
			}
			if (!is_worldspawn_layer_brush) {
				brush_indices.append(b);
			}
		}

		entity_dict["brush_indices"] = brush_indices;

		entity_dict["center"] = Vector3(ent->center.y, ent->center.z, ent->center.x);

		Dictionary entity_properties;

		for (int p = 0; p < ent->property_count; p++) {
			LMProperty *prop = &ent->properties[p];
			entity_properties[String::utf8(prop->key)] = String::utf8(prop->value);
		}

		entity_dict["properties"] = entity_properties;

		ent_dicts.append(entity_dict);
	}

	return ent_dicts;
}

Array Qodot::get_worldspawn_layer_dicts() {
	const LMEntity *ents = map_data->map_data_get_entities();
	const LMEntity *worldspawn_entity = &ents[0];

	Array worldspawn_layer_dicts;

	if (worldspawn_entity == NULL) {
		return worldspawn_layer_dicts;
	}

	int layer_count = map_data->map_data_get_worldspawn_layer_count();
	const LMWorldspawnLayer *layers = map_data->map_data_get_worldspawn_layers();

	for (int l = 0; l < layer_count; l++) {
		const LMWorldspawnLayer *worldspawn_layer = &layers[l];

		Dictionary layer_dict;

		LMTextureData *tex_data = map_data->map_data_get_texture(worldspawn_layer->texture_idx);
		if (tex_data == NULL) {
			continue;
		}

		layer_dict["texture"] = String::utf8(tex_data->name);

		PackedInt64Array brush_indices;

		for (int b = 0; b < worldspawn_entity->brush_count; ++b) {
			const LMBrush *brush = &worldspawn_entity->brushes[b];
			bool is_layer_brush = false;

			for (int f = 0; f < brush->face_count; ++f) {
				const face *face = &brush->faces[f];

				if (face->texture_idx == worldspawn_layer->texture_idx) {
					is_layer_brush = true;
					break;
				}
			}

			if (is_layer_brush) {
				brush_indices.append(b);
			}
		}

		layer_dict["brush_indices"] = brush_indices;

		worldspawn_layer_dicts.append(layer_dict);
	}

	return worldspawn_layer_dicts;
}

void Qodot::gather_texture_surfaces(const String p_texture_name, const String p_brush_filter_texture, const String p_face_filter_texture) {
	gather_texture_surfaces_internal(p_texture_name, p_brush_filter_texture, p_face_filter_texture, true);
}

void Qodot::gather_worldspawn_layer_surfaces(const String p_texture_name, const String p_brush_filter_texture, const String p_face_filter_texture) {
	gather_texture_surfaces_internal(p_texture_name, p_brush_filter_texture, p_face_filter_texture, false);
}

void Qodot::gather_texture_surfaces_internal(const String p_texture_name, const String p_brush_filter_texture, const String p_face_filter_texture, bool p_filter_layers) {
	CharString texture_name = p_texture_name.utf8();
	CharString brush_filter_texture = p_brush_filter_texture.utf8();
	CharString face_filter_texture = p_face_filter_texture.utf8();

	surface_gatherer.surface_gatherer_reset_params();
	surface_gatherer.surface_gatherer_set_split_type(SST_ENTITY);
	surface_gatherer.surface_gatherer_set_texture_filter(texture_name);
	surface_gatherer.surface_gatherer_set_brush_filter_texture(brush_filter_texture);
	surface_gatherer.surface_gatherer_set_face_filter_texture(face_filter_texture);
	surface_gatherer.surface_gatherer_set_worldspawn_layer_filter(p_filter_layers);

	surface_gatherer.surface_gatherer_run();
}

void Qodot::gather_entity_convex_collision_surfaces(int64_t p_entity_idx) {
	gather_convex_collision_surfaces(p_entity_idx, true);
}

void Qodot::gather_entity_concave_collision_surfaces(int64_t p_entity_idx) {
	gather_concave_collision_surfaces(p_entity_idx, true);
}

void Qodot::gather_worldspawn_layer_collision_surfaces(int64_t p_entity_idx) {
	gather_convex_collision_surfaces(p_entity_idx, false);
}

void Qodot::gather_convex_collision_surfaces(int64_t p_entity_idx, bool p_filter_layers) {
	surface_gatherer.surface_gatherer_reset_params();
	surface_gatherer.surface_gatherer_set_split_type(SST_BRUSH);
	surface_gatherer.surface_gatherer_set_entity_index_filter((int)p_entity_idx);
	surface_gatherer.surface_gatherer_set_worldspawn_layer_filter(p_filter_layers);

	surface_gatherer.surface_gatherer_run();
}

void Qodot::gather_concave_collision_surfaces(int64_t p_entity_idx, bool p_filter_layers) {
	surface_gatherer.surface_gatherer_reset_params();
	surface_gatherer.surface_gatherer_set_split_type(SST_NONE);
	surface_gatherer.surface_gatherer_set_entity_index_filter((int)p_entity_idx);
	surface_gatherer.surface_gatherer_set_worldspawn_layer_filter(p_filter_layers);

	surface_gatherer.surface_gatherer_run();
}

Array Qodot::fetch_surfaces(double p_inverse_scale_factor) {
	const LMSurfaces *surfs = surface_gatherer.surface_gatherer_fetch();

	Array surf_array;
	Variant v_nil;

	Vector3 gv3;
	Vector2 gv2;

	for (int s = 0; s < surfs->surface_count; ++s) {
		LMSurface *surf = &surfs->surfaces[s];

		if (surf->vertex_count == 0) {
			surf_array.append(v_nil);
			continue;
		}

		// Create vertex array

		PackedVector3Array vertices;

		for (int v = 0; v < surf->vertex_count; ++v) {
			gv3 = Vector3(surf->vertices[v].vertex.y, surf->vertices[v].vertex.z, surf->vertices[v].vertex.x);
			gv3 = gv3 / p_inverse_scale_factor;
			vertices.append(gv3);
		}

		// Create normal array
		PackedVector3Array normals;

		for (int v = 0; v < surf->vertex_count; ++v) {
			gv3 = Vector3(surf->vertices[v].normal.y, surf->vertices[v].normal.z, surf->vertices[v].normal.x);
			normals.append(gv3);
		}

		// Create tangent array
		PackedFloat64Array tangents;

		for (int v = 0; v < surf->vertex_count; v++) {
			tangents.append(surf->vertices[v].tangent.y);
			tangents.append(surf->vertices[v].tangent.z);
			tangents.append(surf->vertices[v].tangent.x);
			tangents.append(surf->vertices[v].tangent.w);
		}

		// Create UV array
		PackedVector2Array uvs;

		for (int v = 0; v < surf->vertex_count; v++) {
			gv2 = Vector2(surf->vertices[v].uv.u, surf->vertices[v].uv.v);
			uvs.append(gv2);
		}

		// Create indices array

		PackedInt32Array indices;

		for (int i = 0; i < surf->index_count; i++) {
			indices.append(surf->indices[i]);
		}

		Array brush_array;

		brush_array.resize(Mesh::ArrayType::ARRAY_MAX);
		brush_array.fill(v_nil);

		brush_array[Mesh::ArrayType::ARRAY_VERTEX] = Variant(vertices);
		brush_array[Mesh::ArrayType::ARRAY_NORMAL] = Variant(normals);
		brush_array[Mesh::ArrayType::ARRAY_TANGENT] = Variant(tangents);
		brush_array[Mesh::ArrayType::ARRAY_TEX_UV] = Variant(uvs);
		brush_array[Mesh::ArrayType::ARRAY_INDEX] = Variant(indices);

		surf_array.append(brush_array);
	}
	return surf_array;
}

void Qodot::_bind_methods() {
	ClassDB::bind_method(D_METHOD("load_map", "map_file"), &Qodot::load_map);
	ClassDB::bind_method(D_METHOD("get_texture_list"), &Qodot::get_texture_list);
	ClassDB::bind_method(D_METHOD("set_entity_definitions", "entity_defs"), &Qodot::set_entity_definitions);
	ClassDB::bind_method(D_METHOD("set_worldspawn_layers", "worldspawn_layers"), &Qodot::set_worldspawn_layers);
	ClassDB::bind_method(D_METHOD("generate_geometry", "texture_size_dict"), &Qodot::generate_geometry);
	ClassDB::bind_method(D_METHOD("get_entity_dicts"), &Qodot::get_entity_dicts);
	ClassDB::bind_method(D_METHOD("get_worldspawn_layer_dicts"), &Qodot::get_worldspawn_layer_dicts);
	ClassDB::bind_method(D_METHOD("gather_texture_surfaces", "texture_name", "brush_filter_texture", "face_filter_texture"), &Qodot::gather_texture_surfaces);
	ClassDB::bind_method(D_METHOD("gather_worldspawn_layer_surfaces", "texture_name", "brush_filter_texture", "face_filter_texture"), &Qodot::gather_worldspawn_layer_surfaces);
	ClassDB::bind_method(D_METHOD("gather_entity_convex_collision_surfaces", "entity_idx"), &Qodot::gather_entity_convex_collision_surfaces);
	ClassDB::bind_method(D_METHOD("gather_entity_concave_collision_surfaces", "entity_idx"), &Qodot::gather_entity_concave_collision_surfaces);
	ClassDB::bind_method(D_METHOD("gather_worldspawn_layer_collision_surfaces", "entity_idx"), &Qodot::gather_worldspawn_layer_collision_surfaces);
	ClassDB::bind_method(D_METHOD("fetch_surfaces", "inverse_scale_factor"), &Qodot::fetch_surfaces);
}