#pragma once
#include <cstddef>
#include <string>

struct material_entry_t {
    const char *id;
    const char *name;
    const char *category;
    const char *description;
    const char *preview_color;
    const char *ncf;
};

const material_entry_t *material_catalog_entries(size_t &count);
const material_entry_t *material_catalog_find(const std::string &id);
std::string material_catalog_build_preview_scene(const material_entry_t &entry);
