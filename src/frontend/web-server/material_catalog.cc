#include "material_catalog.h"

#include <sstream>

static const material_entry_t CATALOG[] = {

/* ── Metals ──────────────────────────────────────────────────────────────── */

{
    "gold_polished", "Gold (Polished)", "Metal",
    "Mirror-smooth yellow gold with sharp specular highlights.",
    "#C89420",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.93, 0.73, 0.24) }
        }
        scalars = {
            metallic = 1.0
            roughness = 0.06
            ior = 1.5
        }
    })"
},
{
    "gold_brushed", "Gold (Brushed)", "Metal",
    "Directionally brushed gold with anisotropic highlight stretching.",
    "#B8841A",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.93, 0.73, 0.24) }
        }
        scalars = {
            metallic = 1.0
            roughness = 0.18
            anisotropy = 0.84
            anisotropy_rotation = 0
            ior = 1.5
        }
    })"
},
{
    "gold_rough", "Gold (Rough)", "Metal",
    "Worn gold with diffuse-like broad specular response.",
    "#A07218",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.88, 0.68, 0.20) }
        }
        scalars = {
            metallic = 1.0
            roughness = 0.46
            ior = 1.5
        }
    })"
},
{
    "silver_mirror", "Silver (Mirror)", "Metal",
    "Near-perfect mirror silver with minimal roughness.",
    "#C8D0D8",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.92, 0.94, 0.96) }
        }
        scalars = {
            metallic = 1.0
            roughness = 0.03
            ior = 1.5
        }
    })"
},
{
    "silver_brushed", "Silver (Brushed)", "Metal",
    "Anisotropically brushed silver with stretched highlight.",
    "#A8B0B8",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.90, 0.92, 0.94) }
        }
        scalars = {
            metallic = 1.0
            roughness = 0.16
            anisotropy = 0.90
            anisotropy_rotation = 0
            ior = 1.5
        }
    })"
},
{
    "copper_polished", "Copper (Polished)", "Metal",
    "Warm-toned polished copper with moderate reflectivity.",
    "#B56A30",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.84, 0.46, 0.26) }
        }
        scalars = {
            metallic = 1.0
            roughness = 0.10
            ior = 1.5
        }
    })"
},
{
    "copper_rough", "Copper (Rough)", "Metal",
    "Dull copper with wide specular lobe.",
    "#966030",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.78, 0.42, 0.22) }
        }
        scalars = {
            metallic = 1.0
            roughness = 0.46
            ior = 1.5
        }
    })"
},
{
    "aluminum_polished", "Aluminum (Polished)", "Metal",
    "Bright polished aluminum, slightly cooler than silver.",
    "#B4C0C8",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.82, 0.88, 0.92) }
        }
        scalars = {
            metallic = 1.0
            roughness = 0.07
            ior = 1.5
        }
    })"
},
{
    "aluminum_brushed", "Aluminum (Brushed)", "Metal",
    "Brushed finish aluminum with strong directional anisotropy.",
    "#909CA4",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.80, 0.86, 0.90) }
        }
        scalars = {
            metallic = 1.0
            roughness = 0.20
            anisotropy = 0.92
            anisotropy_rotation = 0
            ior = 1.5
        }
    })"
},
{
    "iron", "Iron", "Metal",
    "Dark matte iron with diffuse-dominant metallic response.",
    "#4A4E52",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.28, 0.30, 0.32) }
        }
        scalars = {
            metallic = 1.0
            roughness = 0.60
            ior = 1.5
        }
    })"
},
{
    "steel_polished", "Steel (Polished)", "Metal",
    "Neutral-tone stainless steel with tight specular response.",
    "#A0AAB2",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.72, 0.76, 0.80) }
        }
        scalars = {
            metallic = 1.0
            roughness = 0.14
            ior = 1.5
        }
    })"
},
{
    "chrome", "Chrome", "Metal",
    "Ultra-reflective chrome with a near-mirror surface.",
    "#C0C8D0",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.90, 0.92, 0.95) }
        }
        scalars = {
            metallic = 1.0
            roughness = 0.04
            ior = 1.5
        }
    })"
},
{
    "titanium", "Titanium", "Metal",
    "Warm grey titanium with moderate diffuse spread.",
    "#707880",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.54, 0.58, 0.62) }
        }
        scalars = {
            metallic = 1.0
            roughness = 0.24
            ior = 1.5
        }
    })"
},
{
    "bronze", "Bronze", "Metal",
    "Classic reddish-brown bronze alloy with moderate gloss.",
    "#8C6030",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.64, 0.42, 0.20) }
        }
        scalars = {
            metallic = 1.0
            roughness = 0.22
            ior = 1.5
        }
    })"
},
{
    "brass", "Brass", "Metal",
    "Yellow-toned brass alloy with medium polish.",
    "#B09030",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.82, 0.70, 0.22) }
        }
        scalars = {
            metallic = 1.0
            roughness = 0.18
            ior = 1.5
        }
    })"
},
{
    "platinum", "Platinum", "Metal",
    "Cool-white platinum with subtle warmth and clean reflections.",
    "#D0D4D8",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.90, 0.91, 0.93) }
        }
        scalars = {
            metallic = 1.0
            roughness = 0.10
            ior = 1.5
        }
    })"
},
{
    "nickel", "Nickel", "Metal",
    "Light warm-grey nickel with semi-glossy surface.",
    "#909898",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.64, 0.68, 0.66) }
        }
        scalars = {
            metallic = 1.0
            roughness = 0.26
            ior = 1.5
        }
    })"
},
{
    "pewter", "Pewter", "Metal",
    "Dark grey pewter with matte metallic sheen.",
    "#606870",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.42, 0.46, 0.48) }
        }
        scalars = {
            metallic = 1.0
            roughness = 0.40
            ior = 1.5
        }
    })"
},
{
    "oxidized_copper", "Oxidized Copper", "Metal",
    "Patinated copper with FBM-driven green-brown color variation.",
    "#5A8E72",
    R"(type = principled
    properties = {
        samplers = {
            base_color = {
                type = fbm_marble
                a = col3(0.54, 0.30, 0.18)
                b = col3(0.25, 0.18, 0.14)
                vein = col3(0.10, 0.30, 0.24)
                scale = 4.8
                vein_frequency = 8.0
                turbulence = 3.5
                octaves = 5
                lacunarity = 2.0
                gain = 0.5
                vein_strength = 0.6
                vein_sharpness = 3.4
            }
        }
        scalars = {
            metallic = 0.88
            roughness = 0.46
            ior = 1.5
        }
    })"
},
{
    "rusted_iron", "Rusted Iron", "Metal",
    "Heavily corroded iron with FBM rust color and rough surface.",
    "#7A4020",
    R"(type = principled
    properties = {
        samplers = {
            base_color = {
                type = fbm_marble
                a = col3(0.48, 0.22, 0.08)
                b = col3(0.34, 0.14, 0.06)
                vein = col3(0.22, 0.10, 0.04)
                scale = 3.2
                vein_frequency = 6.0
                turbulence = 4.0
                octaves = 6
                lacunarity = 2.0
                gain = 0.5
                vein_strength = 0.8
                vein_sharpness = 2.5
            }
        }
        scalars = {
            metallic = 0.70
            roughness = 0.72
            ior = 1.5
        }
    })"
},
{
    "mitsuba_rough_aluminum", "Rough Aluminum (Mitsuba)", "Metal",
    "Industrial rough aluminum lifted from the Mitsuba spaceship scene.",
    "#898786",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.536965, 0.531038, 0.531677) }
        }
        scalars = {
            metallic = 1.0
            roughness = 0.10
            ior = 1.01965
        }
    })"
},
{
    "mitsuba_rough_steel", "Rough Steel (Mitsuba)", "Metal",
    "Muted rough steel based on the Mitsuba car scene trim material.",
    "#AEA5AF",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.687547, 0.649065, 0.687011) }
        }
        scalars = {
            metallic = 1.0
            roughness = 0.10
            ior = 3.13449
        }
    })"
},

/* ── Dielectrics ──────────────────────────────────────────────────────────── */

{
    "glass_clear", "Glass (Clear)", "Dielectric",
    "Perfectly smooth clear glass at IOR 1.5.",
    "#C8DDE8",
    R"(type = dielectric
    properties = {
        scalars = {
            ior = 1.5
            transparency = 1.0
        }
    })"
},
{
    "glass_frosted", "Glass (Frosted)", "Dielectric",
    "Sandblasted frosted glass with soft diffuse transmission.",
    "#DCE8EF",
    R"(type = rough_dielectric
    properties = {
        samplers = {
            transmission = { type = color, value = col3(0.96, 0.97, 1.0) }
        }
        scalars = {
            roughness = 0.22
            ior = 1.46
            transparency = 0.96
        }
    })"
},
{
    "glass_dense_frosted", "Glass (Dense Frosted)", "Dielectric",
    "Thick frosted glass with heavy diffusion like acrylic.",
    "#CDD8DF",
    R"(type = rough_dielectric
    properties = {
        samplers = {
            transmission = { type = color, value = col3(0.90, 0.92, 0.96) }
        }
        scalars = {
            roughness = 0.40
            ior = 1.46
            transparency = 0.93
        }
    })"
},
{
    "glass_blue", "Glass (Blue)", "Dielectric",
    "Blue-tinted glass with volumetric absorption.",
    "#4A80C0",
    R"(type = rough_dielectric
    properties = {
        samplers = {
            transmission = { type = color, value = col3(1.0, 1.0, 1.0) }
            absorption_color = { type = color, value = col3(0.48, 0.72, 1.0) }
        }
        scalars = {
            roughness = 0.04
            ior = 1.48
            transparency = 0.98
            absorption_distance = 1.2
        }
    })"
},
{
    "glass_green", "Glass (Green)", "Dielectric",
    "Bottle-green glass with volumetric colour absorption.",
    "#2A7A40",
    R"(type = rough_dielectric
    properties = {
        samplers = {
            transmission = { type = color, value = col3(1.0, 1.0, 1.0) }
            absorption_color = { type = color, value = col3(0.34, 0.82, 0.44) }
        }
        scalars = {
            roughness = 0.04
            ior = 1.52
            transparency = 0.97
            absorption_distance = 1.0
        }
    })"
},
{
    "glass_amber", "Glass (Amber)", "Dielectric",
    "Warm amber glass with golden volumetric colour.",
    "#C07820",
    R"(type = rough_dielectric
    properties = {
        samplers = {
            transmission = { type = color, value = col3(1.0, 1.0, 1.0) }
            absorption_color = { type = color, value = col3(1.0, 0.72, 0.18) }
        }
        scalars = {
            roughness = 0.04
            ior = 1.50
            transparency = 0.97
            absorption_distance = 0.8
        }
    })"
},
{
    "glass_rose", "Glass (Rose)", "Dielectric",
    "Soft pink-rose tinted glass.",
    "#D07090",
    R"(type = rough_dielectric
    properties = {
        samplers = {
            transmission = { type = color, value = col3(1.0, 1.0, 1.0) }
            absorption_color = { type = color, value = col3(1.0, 0.50, 0.65) }
        }
        scalars = {
            roughness = 0.04
            ior = 1.48
            transparency = 0.97
            absorption_distance = 1.0
        }
    })"
},
{
    "glass_smoked", "Glass (Smoked)", "Dielectric",
    "Dark smoked glass with strong absorption across all wavelengths.",
    "#303438",
    R"(type = rough_dielectric
    properties = {
        samplers = {
            transmission = { type = color, value = col3(1.0, 1.0, 1.0) }
            absorption_color = { type = color, value = col3(0.60, 0.62, 0.65) }
        }
        scalars = {
            roughness = 0.04
            ior = 1.50
            transparency = 0.92
            absorption_distance = 0.4
        }
    })"
},
{
    "glass_etched", "Glass (Etched)", "Dielectric",
    "Acid-etched glass with voronoi surface relief and soft transmission.",
    "#D8E4EC",
    R"(type = rough_dielectric
    properties = {
        samplers = {
            transmission = { type = color, value = col3(0.94, 0.96, 1.0) }
            roughness = {
                type = checker
                a = col3(0.12, 0.12, 0.12)
                b = col3(0.22, 0.22, 0.22)
                scale_u = 10
                scale_v = 10
            }
            normal = {
                type = voronoi_normal
                cells = 180
                max_deviation = 10
                seed = 53
            }
        }
        scalars = {
            roughness = 0.12
            ior = 1.48
            transparency = 0.97
        }
    })"
},
{
    "diamond", "Diamond", "Dielectric",
    "Diamond with high IOR 2.42 producing strong prismatic dispersion.",
    "#E8F4FF",
    R"(type = dielectric
    properties = {
        scalars = {
            ior = 2.42
            transparency = 1.0
        }
    })"
},
{
    "ice", "Ice", "Dielectric",
    "Clear ice at IOR 1.31 with barely visible surface imperfections.",
    "#D4EEF8",
    R"(type = rough_dielectric
    properties = {
        samplers = {
            transmission = { type = color, value = col3(0.97, 0.98, 1.0) }
        }
        scalars = {
            roughness = 0.06
            ior = 1.31
            transparency = 0.99
        }
    })"
},
{
    "water", "Water", "Dielectric",
    "Pure water at IOR 1.33 — very smooth surface.",
    "#A8D8F0",
    R"(type = dielectric
    properties = {
        scalars = {
            ior = 1.33
            transparency = 1.0
        }
    })"
},
{
    "sapphire", "Sapphire", "Dielectric",
    "Deep blue sapphire at IOR 1.76 with strong absorption.",
    "#1840A0",
    R"(type = rough_dielectric
    properties = {
        samplers = {
            transmission = { type = color, value = col3(1.0, 1.0, 1.0) }
            absorption_color = { type = color, value = col3(0.14, 0.34, 0.96) }
        }
        scalars = {
            roughness = 0.02
            ior = 1.76
            transparency = 0.98
            absorption_distance = 0.6
        }
    })"
},
{
    "ruby", "Ruby", "Dielectric",
    "Deep red ruby at IOR 1.76 with selective colour absorption.",
    "#C01830",
    R"(type = rough_dielectric
    properties = {
        samplers = {
            transmission = { type = color, value = col3(1.0, 1.0, 1.0) }
            absorption_color = { type = color, value = col3(0.90, 0.10, 0.16) }
        }
        scalars = {
            roughness = 0.02
            ior = 1.76
            transparency = 0.97
            absorption_distance = 0.5
        }
    })"
},
{
    "crystal", "Crystal", "Dielectric",
    "High-clarity crystal glass at IOR 1.55.",
    "#EAF4FA",
    R"(type = dielectric
    properties = {
        scalars = {
            ior = 1.55
            transparency = 1.0
        }
    })"
},
{
    "mitsuba_window_glass", "Window Glass (Mitsuba)", "Dielectric",
    "Thin automotive-style window glass taken from the Mitsuba car scenes.",
    "#D7E3EE",
    R"(type = thin_dielectric
    properties = {
        samplers = {
            transmission = { type = color, value = col3(1, 1, 1) }
        }
        scalars = {
            ior = 1.5
            transparency = 1.0
        }
    })"
},

/* ── Principled / Coated ──────────────────────────────────────────────────── */

{
    "paint_red", "Paint (Red Gloss)", "Principled",
    "High-gloss automotive red with strong clearcoat.",
    "#C41818",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.86, 0.10, 0.08) }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.10
            ior = 1.5
            clearcoat = 0.95
            clearcoat_roughness = 0.04
        }
    })"
},
{
    "paint_blue", "Paint (Blue Gloss)", "Principled",
    "Deep cobalt blue gloss paint with clearcoat.",
    "#1430C0",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.10, 0.20, 0.84) }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.10
            ior = 1.5
            clearcoat = 0.95
            clearcoat_roughness = 0.04
        }
    })"
},
{
    "paint_green", "Paint (Green Gloss)", "Principled",
    "Vivid green gloss paint with clearcoat layer.",
    "#147A28",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.08, 0.58, 0.16) }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.10
            ior = 1.5
            clearcoat = 0.95
            clearcoat_roughness = 0.04
        }
    })"
},
{
    "paint_yellow", "Paint (Yellow Gloss)", "Principled",
    "Bright yellow gloss paint.",
    "#D4C018",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.96, 0.84, 0.06) }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.10
            ior = 1.5
            clearcoat = 0.95
            clearcoat_roughness = 0.04
        }
    })"
},
{
    "paint_orange", "Paint (Orange Gloss)", "Principled",
    "Safety-orange gloss paint.",
    "#D46010",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.94, 0.42, 0.04) }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.10
            ior = 1.5
            clearcoat = 0.90
            clearcoat_roughness = 0.04
        }
    })"
},
{
    "paint_white", "Paint (White Gloss)", "Principled",
    "Pure gloss white enamel paint.",
    "#E8E8E8",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.96, 0.96, 0.96) }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.08
            ior = 1.5
            clearcoat = 1.0
            clearcoat_roughness = 0.03
        }
    })"
},
{
    "lacquer_black", "Black Lacquer", "Principled",
    "Piano-black lacquer with near-mirror reflections.",
    "#101214",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.02, 0.02, 0.02) }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.04
            ior = 1.5
            clearcoat = 1.0
            clearcoat_roughness = 0.02
        }
    })"
},
{
    "ceramic_white", "Ceramic (White)", "Principled",
    "Smooth white ceramic glaze with subtle specular sheen.",
    "#F0EDE8",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.94, 0.92, 0.88) }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.34
            ior = 1.52
            clearcoat = 0.50
            clearcoat_roughness = 0.07
        }
    })"
},
{
    "ceramic_terracotta", "Ceramic (Terracotta)", "Principled",
    "Unglazed terracotta with warm earth tones and matte finish.",
    "#B86040",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.76, 0.38, 0.22) }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.55
            ior = 1.48
            clearcoat = 0.10
            clearcoat_roughness = 0.12
        }
    })"
},
{
    "porcelain", "Porcelain", "Principled",
    "Translucent-looking porcelain with soft glaze and subtle subsurface.",
    "#EEE8E0",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.96, 0.92, 0.86) }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.20
            ior = 1.54
            clearcoat = 0.70
            clearcoat_roughness = 0.06
        }
    })"
},
{
    "enamel", "Enamel", "Principled",
    "Vitreous enamel with a hard, glassy surface.",
    "#C0D0E0",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.72, 0.82, 0.90) }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.12
            ior = 1.52
            clearcoat = 0.85
            clearcoat_roughness = 0.05
        }
    })"
},
{
    "plastic_glossy", "Plastic (Glossy)", "Principled",
    "Neutral mid-grey glossy thermoplastic.",
    "#707880",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.40, 0.42, 0.46) }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.24
            ior = 1.46
            clearcoat = 0.30
            clearcoat_roughness = 0.08
        }
    })"
},
{
    "plastic_matte", "Plastic (Matte)", "Principled",
    "Matte ABS plastic with no clearcoat.",
    "#5A6060",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.32, 0.34, 0.36) }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.60
            ior = 1.46
            clearcoat = 0.0
            clearcoat_roughness = 0.08
        }
    })"
},
{
    "rubber_black", "Rubber (Black)", "Principled",
    "Matte black rubber with slightly elevated IOR.",
    "#1A1E20",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.04, 0.04, 0.04) }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.80
            ior = 1.52
            clearcoat = 0.0
            clearcoat_roughness = 0.10
        }
    })"
},
{
    "rubber_red", "Rubber (Red)", "Principled",
    "Red vulcanised rubber with matte finish.",
    "#A01818",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.72, 0.08, 0.06) }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.76
            ior = 1.52
            clearcoat = 0.0
            clearcoat_roughness = 0.10
        }
    })"
},
{
    "silicone", "Silicone", "Principled",
    "Translucent milky silicone with soft specular response.",
    "#D8E0DC",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.88, 0.90, 0.88) }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.48
            ior = 1.41
            clearcoat = 0.15
            clearcoat_roughness = 0.10
        }
    })"
},
{
    "wax_candle", "Wax (Candle)", "Principled",
    "Off-white candle wax with waxy sheen and slight subsurface look.",
    "#F0E4C8",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.96, 0.90, 0.72) }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.44
            ior = 1.44
            clearcoat = 0.22
            clearcoat_roughness = 0.08
        }
    })"
},
{
    "paint_metallic", "Paint (Metallic)", "Principled",
    "Silver metallic car paint with medium-gloss clearcoat.",
    "#909898",
    R"(type = principled
    properties = {
        samplers = {
            base_color = {
                type = fbm_marble
                a = col3(0.72, 0.74, 0.78)
                b = col3(0.58, 0.60, 0.64)
                vein = col3(0.85, 0.86, 0.88)
                scale = 12.0
                vein_frequency = 14.0
                turbulence = 0.8
                octaves = 4
                lacunarity = 2.0
                gain = 0.5
                vein_strength = 0.4
                vein_sharpness = 5.0
            }
        }
        scalars = {
            metallic = 0.55
            roughness = 0.26
            ior = 1.5
            clearcoat = 0.60
            clearcoat_roughness = 0.06
        }
    })"
},
{
    "paint_purple", "Paint (Purple Gloss)", "Principled",
    "Rich purple gloss paint.",
    "#6028A8",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.40, 0.12, 0.74) }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.10
            ior = 1.5
            clearcoat = 0.90
            clearcoat_roughness = 0.04
        }
    })"
},
{
    "paint_teal", "Paint (Teal Gloss)", "Principled",
    "Teal-turquoise gloss paint.",
    "#107880",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.06, 0.52, 0.54) }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.10
            ior = 1.5
            clearcoat = 0.90
            clearcoat_roughness = 0.04
        }
    })"
},
{
    "mitsuba_dark_plastic", "Dark Plastic (Mitsuba)", "Principled",
    "Low-sheen black plastic from the Mitsuba spaceship interior parts.",
    "#050505",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.01, 0.01, 0.01) }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.20
            ior = 1.5
        }
    })"
},
{
    "mitsuba_dark_rubber", "Dark Rubber (Mitsuba)", "Principled",
    "Dark rubber from the Mitsuba spaceship and car scenes with a soft highlight.",
    "#090909",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.025, 0.025, 0.025) }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.40
            ior = 1.5
        }
    })"
},
{
    "mitsuba_leather_dark", "Leather (Dark Mitsuba)", "Principled",
    "Very dark brown leather from the Mitsuba spaceship upholstery set.",
    "#090403",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.034, 0.014, 0.008) }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.40
            ior = 1.5
        }
    })"
},
{
    "mitsuba_leather_red", "Leather (Red Mitsuba)", "Principled",
    "Deep oxblood leather from the Mitsuba spaceship scene.",
    "#29080A",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.163, 0.03, 0.037) }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.40
            ior = 1.5
        }
    })"
},
{
    "mitsuba_leather_pink", "Leather (Pink Mitsuba)", "Principled",
    "Bright pink leather accent material from the Mitsuba spaceship scene.",
    "#C52D43",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.772, 0.175, 0.262) }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.40
            ior = 1.5
        }
    })"
},
{
    "mitsuba_lego_yellow", "Lego Yellow (Mitsuba)", "Principled",
    "Toy-brick yellow adapted from the Mitsuba lego scene as a simple glossy plastic.",
    "#CC804C",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.799103, 0.502886, 0.029557) }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.22
            ior = 1.5
            clearcoat = 0.25
            clearcoat_roughness = 0.06
        }
    })"
},

/* ── Diffuse / Lambert ────────────────────────────────────────────────────── */

{
    "diffuse_white", "Diffuse White", "Diffuse",
    "Near-perfect Lambertian white reference surface.",
    "#E8E8E8",
    R"(type = lambert
    properties = {
        samplers = {
            diffuse = { type = color, value = col3(0.90, 0.90, 0.90) }
        }
    })"
},
{
    "diffuse_black", "Diffuse Black", "Diffuse",
    "Dark matte black Lambert surface.",
    "#141414",
    R"(type = lambert
    properties = {
        samplers = {
            diffuse = { type = color, value = col3(0.03, 0.03, 0.03) }
        }
    })"
},
{
    "diffuse_red", "Diffuse Red", "Diffuse",
    "Saturated red Lambertian diffuse.",
    "#B81818",
    R"(type = lambert
    properties = {
        samplers = {
            diffuse = { type = color, value = col3(0.78, 0.08, 0.06) }
        }
    })"
},
{
    "diffuse_blue", "Diffuse Blue", "Diffuse",
    "Medium blue Lambertian diffuse.",
    "#183898",
    R"(type = lambert
    properties = {
        samplers = {
            diffuse = { type = color, value = col3(0.10, 0.22, 0.64) }
        }
    })"
},
{
    "diffuse_green", "Diffuse Green", "Diffuse",
    "Forest green Lambertian diffuse.",
    "#186828",
    R"(type = lambert
    properties = {
        samplers = {
            diffuse = { type = color, value = col3(0.10, 0.44, 0.16) }
        }
    })"
},
{
    "clay_warm", "Clay (Warm)", "Diffuse",
    "Warm terra-cotta clay with fully diffuse response.",
    "#B06040",
    R"(type = lambert
    properties = {
        samplers = {
            diffuse = { type = color, value = col3(0.72, 0.38, 0.24) }
        }
    })"
},
{
    "plaster", "Plaster", "Diffuse",
    "Slightly textured off-white wall plaster.",
    "#E0D8CC",
    R"(type = lambert
    properties = {
        samplers = {
            diffuse = {
                type = fbm_marble
                a = col3(0.88, 0.84, 0.78)
                b = col3(0.82, 0.78, 0.72)
                vein = col3(0.78, 0.74, 0.68)
                scale = 6.0
                vein_frequency = 2.0
                turbulence = 0.5
                octaves = 3
                lacunarity = 2.0
                gain = 0.5
                vein_strength = 0.2
                vein_sharpness = 2.0
            }
        }
    })"
},
{
    "chalk", "Chalk", "Diffuse",
    "Very bright white chalk surface.",
    "#F0EEE8",
    R"(type = lambert
    properties = {
        samplers = {
            diffuse = { type = color, value = col3(0.96, 0.94, 0.90) }
        }
    })"
},
{
    "concrete", "Concrete", "Diffuse",
    "Medium grey poured concrete.",
    "#888888",
    R"(type = lambert
    properties = {
        samplers = {
            diffuse = {
                type = fbm_marble
                a = col3(0.58, 0.58, 0.58)
                b = col3(0.50, 0.50, 0.50)
                vein = col3(0.46, 0.46, 0.46)
                scale = 4.0
                vein_frequency = 3.0
                turbulence = 1.2
                octaves = 4
                lacunarity = 2.0
                gain = 0.5
                vein_strength = 0.35
                vein_sharpness = 2.5
            }
        }
    })"
},
{
    "carbon_black", "Carbon (Matte Black)", "Diffuse",
    "Extremely dark carbon black, near-zero reflectance.",
    "#0A0A0A",
    R"(type = lambert
    properties = {
        samplers = {
            diffuse = { type = color, value = col3(0.01, 0.01, 0.01) }
        }
    })"
},

/* ── Procedural ───────────────────────────────────────────────────────────── */

{
    "marble_white", "Marble (White)", "Procedural",
    "Classic white Carrara marble with dark grey veining.",
    "#E0DCD4",
    R"(type = principled
    properties = {
        samplers = {
            base_color = {
                type = fbm_marble
                a = col3(0.92, 0.90, 0.86)
                b = col3(0.80, 0.78, 0.74)
                vein = col3(0.20, 0.18, 0.16)
                scale = 3.2
                vein_frequency = 4.0
                turbulence = 0.6
                octaves = 5
                lacunarity = 2.0
                gain = 0.5
                vein_strength = 0.90
                vein_sharpness = 4.0
            }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.28
            ior = 1.55
            clearcoat = 0.55
            clearcoat_roughness = 0.06
        }
    })"
},
{
    "marble_black", "Marble (Black)", "Procedural",
    "Nero marquina black marble with white veining.",
    "#222222",
    R"(type = principled
    properties = {
        samplers = {
            base_color = {
                type = fbm_marble
                a = col3(0.08, 0.08, 0.10)
                b = col3(0.12, 0.12, 0.14)
                vein = col3(0.88, 0.86, 0.82)
                scale = 3.0
                vein_frequency = 5.0
                turbulence = 0.5
                octaves = 5
                lacunarity = 2.0
                gain = 0.5
                vein_strength = 0.85
                vein_sharpness = 5.0
            }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.22
            ior = 1.55
            clearcoat = 0.65
            clearcoat_roughness = 0.05
        }
    })"
},
{
    "marble_green", "Marble (Green)", "Procedural",
    "Verde Guatemala green marble with light veining.",
    "#2A5E38",
    R"(type = principled
    properties = {
        samplers = {
            base_color = {
                type = fbm_marble
                a = col3(0.18, 0.42, 0.26)
                b = col3(0.12, 0.34, 0.20)
                vein = col3(0.70, 0.72, 0.68)
                scale = 3.5
                vein_frequency = 5.0
                turbulence = 0.7
                octaves = 5
                lacunarity = 2.0
                gain = 0.5
                vein_strength = 0.80
                vein_sharpness = 4.5
            }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.25
            ior = 1.55
            clearcoat = 0.60
            clearcoat_roughness = 0.06
        }
    })"
},
{
    "marble_red", "Marble (Red)", "Procedural",
    "Rojo alicante red marble with cream veining.",
    "#8A2020",
    R"(type = principled
    properties = {
        samplers = {
            base_color = {
                type = fbm_marble
                a = col3(0.62, 0.14, 0.10)
                b = col3(0.52, 0.10, 0.08)
                vein = col3(0.92, 0.88, 0.80)
                scale = 3.0
                vein_frequency = 4.5
                turbulence = 0.6
                octaves = 5
                lacunarity = 2.0
                gain = 0.5
                vein_strength = 0.75
                vein_sharpness = 4.0
            }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.26
            ior = 1.55
            clearcoat = 0.55
            clearcoat_roughness = 0.06
        }
    })"
},
{
    "granite", "Granite", "Procedural",
    "Speckled dark grey granite with tight FBM grain.",
    "#484C50",
    R"(type = principled
    properties = {
        samplers = {
            base_color = {
                type = fbm_marble
                a = col3(0.32, 0.34, 0.36)
                b = col3(0.22, 0.24, 0.26)
                vein = col3(0.56, 0.54, 0.52)
                scale = 8.0
                vein_frequency = 18.0
                turbulence = 5.0
                octaves = 6
                lacunarity = 2.0
                gain = 0.5
                vein_strength = 0.55
                vein_sharpness = 6.0
            }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.50
            ior = 1.54
            clearcoat = 0.10
            clearcoat_roughness = 0.10
        }
    })"
},
{
    "granite_pink", "Granite (Pink)", "Procedural",
    "Warm pink granite with quartz-like speckle.",
    "#C09080",
    R"(type = principled
    properties = {
        samplers = {
            base_color = {
                type = fbm_marble
                a = col3(0.82, 0.62, 0.56)
                b = col3(0.66, 0.46, 0.40)
                vein = col3(0.90, 0.88, 0.84)
                scale = 8.0
                vein_frequency = 16.0
                turbulence = 4.5
                octaves = 6
                lacunarity = 2.0
                gain = 0.5
                vein_strength = 0.50
                vein_sharpness = 5.5
            }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.48
            ior = 1.54
            clearcoat = 0.10
            clearcoat_roughness = 0.10
        }
    })"
},
{
    "checker_bw", "Checker (B&W)", "Procedural",
    "Classic black-and-white checkerboard with semi-gloss finish.",
    "#888888",
    R"(type = principled
    properties = {
        samplers = {
            base_color = {
                type = checker
                a = col3(0.92, 0.92, 0.92)
                b = col3(0.06, 0.06, 0.06)
                scale_u = 6
                scale_v = 6
            }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.30
            ior = 1.48
            clearcoat = 0.20
            clearcoat_roughness = 0.10
        }
    })"
},
{
    "checker_color", "Checker (Color)", "Procedural",
    "Red and white checkerboard with gloss clearcoat.",
    "#A43030",
    R"(type = principled
    properties = {
        samplers = {
            base_color = {
                type = checker
                a = col3(0.90, 0.90, 0.90)
                b = col3(0.80, 0.10, 0.10)
                scale_u = 6
                scale_v = 6
            }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.18
            ior = 1.5
            clearcoat = 0.50
            clearcoat_roughness = 0.06
        }
    })"
},
{
    "graphpaper", "Graph Paper", "Procedural",
    "Technical graph paper texture with major and minor grid lines.",
    "#E0E8F0",
    R"(type = lambert
    properties = {
        samplers = {
            diffuse = {
                type = graphpaper
                base = col3(0.92, 0.94, 0.96)
                minor = col3(0.72, 0.76, 0.82)
                major = col3(0.44, 0.52, 0.64)
                scale = 10
                major_every = 5
                minor_width = 0.025
                major_width = 0.055
            }
        }
    })"
},
{
    "carbon_fiber", "Carbon Fiber", "Procedural",
    "Woven carbon fiber with glossy resin coat.",
    "#1A1E22",
    R"(type = principled
    properties = {
        samplers = {
            base_color = {
                type = weave
                base = col3(0.06, 0.07, 0.08)
                warp = col3(0.12, 0.14, 0.16)
                weft = col3(0.04, 0.04, 0.05)
                scale = 14
                band_width = 0.55
            }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.12
            ior = 1.5
            clearcoat = 0.85
            clearcoat_roughness = 0.04
        }
    })"
},
{
    "fabric_blue", "Fabric (Blue)", "Procedural",
    "Blue woven fabric with visible thread structure.",
    "#1840A0",
    R"(type = principled
    properties = {
        samplers = {
            base_color = {
                type = weave
                base = col3(0.10, 0.24, 0.72)
                warp = col3(0.08, 0.18, 0.56)
                weft = col3(0.14, 0.30, 0.82)
                scale = 18
                band_width = 0.62
            }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.70
            ior = 1.46
            clearcoat = 0.0
            clearcoat_roughness = 0.10
        }
    })"
},
{
    "denim", "Denim", "Procedural",
    "Classic indigo denim with woven texture.",
    "#2A4060",
    R"(type = principled
    properties = {
        samplers = {
            base_color = {
                type = weave
                base = col3(0.18, 0.28, 0.50)
                warp = col3(0.22, 0.34, 0.60)
                weft = col3(0.86, 0.88, 0.90)
                scale = 22
                band_width = 0.58
            }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.74
            ior = 1.46
            clearcoat = 0.0
            clearcoat_roughness = 0.10
        }
    })"
},
{
    "wood_light", "Wood (Light Oak)", "Procedural",
    "Light oak wood grain with subtle FBM variation.",
    "#C09050",
    R"(type = principled
    properties = {
        samplers = {
            base_color = {
                type = fbm_marble
                a = col3(0.82, 0.60, 0.30)
                b = col3(0.70, 0.48, 0.22)
                vein = col3(0.58, 0.36, 0.14)
                scale = 2.0
                vein_frequency = 12.0
                turbulence = 0.8
                octaves = 4
                lacunarity = 2.0
                gain = 0.5
                vein_strength = 0.60
                vein_sharpness = 3.0
            }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.36
            ior = 1.50
            clearcoat = 0.30
            clearcoat_roughness = 0.08
        }
    })"
},
{
    "wood_dark", "Wood (Dark Walnut)", "Procedural",
    "Rich dark walnut with tight wood-grain FBM.",
    "#402010",
    R"(type = principled
    properties = {
        samplers = {
            base_color = {
                type = fbm_marble
                a = col3(0.32, 0.18, 0.08)
                b = col3(0.24, 0.12, 0.04)
                vein = col3(0.46, 0.28, 0.12)
                scale = 2.0
                vein_frequency = 14.0
                turbulence = 0.9
                octaves = 4
                lacunarity = 2.0
                gain = 0.5
                vein_strength = 0.65
                vein_sharpness = 3.5
            }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.28
            ior = 1.50
            clearcoat = 0.50
            clearcoat_roughness = 0.07
        }
    })"
},
{
    "slate", "Slate", "Procedural",
    "Dark grey slate with voronoi surface relief and matte finish.",
    "#384044",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.22, 0.26, 0.28) }
            normal = {
                type = voronoi_normal
                cells = 120
                max_deviation = 14
                seed = 17
            }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.72
            ior = 1.52
            clearcoat = 0.0
            clearcoat_roughness = 0.10
        }
    })"
},
{
    "stone", "Stone", "Procedural",
    "Natural stone surface with FBM colour variation.",
    "#787060",
    R"(type = lambert
    properties = {
        samplers = {
            diffuse = {
                type = fbm_marble
                a = col3(0.58, 0.54, 0.48)
                b = col3(0.46, 0.42, 0.38)
                vein = col3(0.38, 0.34, 0.30)
                scale = 5.0
                vein_frequency = 4.0
                turbulence = 2.0
                octaves = 5
                lacunarity = 2.0
                gain = 0.5
                vein_strength = 0.45
                vein_sharpness = 2.5
            }
        }
    })"
},
{
    "terrazzo", "Terrazzo", "Procedural",
    "Polished terrazzo floor with checker-pattern aggregate.",
    "#B8A898",
    R"(type = principled
    properties = {
        samplers = {
            base_color = {
                type = checker
                a = col3(0.78, 0.72, 0.66)
                b = col3(0.90, 0.86, 0.80)
                scale_u = 20
                scale_v = 20
            }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.22
            ior = 1.54
            clearcoat = 0.40
            clearcoat_roughness = 0.07
        }
    })"
},
{
    "voronoi_relief", "Voronoi Relief", "Procedural",
    "Smooth surface with strong voronoi normal-map displacement.",
    "#8090A0",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.52, 0.60, 0.68) }
            normal = {
                type = voronoi_normal
                cells = 240
                max_deviation = 18
                seed = 41
            }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.30
            ior = 1.5
            clearcoat = 0.25
            clearcoat_roughness = 0.08
        }
    })"
},

/* ── Subsurface ───────────────────────────────────────────────────────────── */

{
    "sss_skin", "Skin", "Subsurface",
    "Human skin with warm subsurface scattering and fine normal detail.",
    "#E08868",
    R"(type = subsurface
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.86, 0.56, 0.42) }
            subsurface_color = { type = color, value = col3(1.0, 0.42, 0.28) }
            subsurface_radius = { type = color, value = col3(0.38, 0.72, 1.18) }
            normal = {
                type = voronoi_normal
                cells = 28
                max_deviation = 5
                seed = 5
            }
        }
        scalars = {
            subsurface = 0.45
            thickness = 0.22
            roughness = 0.50
            ior = 1.4
        }
    })"
},
{
    "sss_wax", "Wax (SSS)", "Subsurface",
    "Translucent candle wax with warm subsurface scattering.",
    "#F0D880",
    R"(type = subsurface
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.96, 0.88, 0.60) }
            subsurface_color = { type = color, value = col3(1.0, 0.80, 0.30) }
            subsurface_radius = { type = color, value = col3(0.80, 0.60, 0.30) }
        }
        scalars = {
            subsurface = 0.65
            thickness = 0.30
            roughness = 0.42
            ior = 1.44
        }
    })"
},
{
    "sss_jade", "Jade", "Subsurface",
    "Translucent green jade with strong internal scattering.",
    "#387858",
    R"(type = subsurface
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.28, 0.56, 0.40) }
            subsurface_color = { type = color, value = col3(0.40, 0.80, 0.55) }
            subsurface_radius = { type = color, value = col3(0.55, 0.90, 0.65) }
        }
        scalars = {
            subsurface = 0.72
            thickness = 0.45
            roughness = 0.28
            ior = 1.66
        }
    })"
},
{
    "sss_marble", "Marble (SSS)", "Subsurface",
    "White marble with subsurface scattering for translucent thin edges.",
    "#E8E2D8",
    R"(type = subsurface
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.92, 0.88, 0.82) }
            subsurface_color = { type = color, value = col3(0.98, 0.94, 0.88) }
            subsurface_radius = { type = color, value = col3(1.0, 0.95, 0.85) }
        }
        scalars = {
            subsurface = 0.35
            thickness = 0.18
            roughness = 0.30
            ior = 1.55
        }
    })"
},

/* ── Emissive ─────────────────────────────────────────────────────────────── */

{
    "emissive_warm", "Warm Light", "Emissive",
    "Warm tungsten-toned emissive for incandescent light sources.",
    "#FF9040",
    R"(type = emissive
    properties = {
        samplers = {
            emissive = { type = color, value = col3(6.0, 3.8, 1.8) }
        }
    })"
},
{
    "emissive_cool", "Cool Light", "Emissive",
    "Cool daylight-spectrum emissive for overcast sky or LED sources.",
    "#8AAEE0",
    R"(type = emissive
    properties = {
        samplers = {
            emissive = { type = color, value = col3(2.4, 3.6, 6.0) }
        }
    })"
},
{
    "emissive_neon_red", "Neon Red", "Emissive",
    "Saturated neon red glow.",
    "#FF1040",
    R"(type = emissive
    properties = {
        samplers = {
            emissive = { type = color, value = col3(8.0, 0.4, 0.8) }
        }
    })"
},
{
    "emissive_neon_green", "Neon Green", "Emissive",
    "Vivid neon green emissive.",
    "#20FF50",
    R"(type = emissive
    properties = {
        samplers = {
            emissive = { type = color, value = col3(0.5, 8.0, 1.2) }
        }
    })"
},
{
    "emissive_neon_blue", "Neon Blue", "Emissive",
    "Intense neon blue glow.",
    "#2060FF",
    R"(type = emissive
    properties = {
        samplers = {
            emissive = { type = color, value = col3(0.4, 1.6, 10.0) }
        }
    })"
},
{
    "emissive_neon_orange", "Neon Orange", "Emissive",
    "Bright neon orange — sodium-lamp orange-yellow.",
    "#FF7010",
    R"(type = emissive
    properties = {
        samplers = {
            emissive = { type = color, value = col3(9.0, 3.6, 0.5) }
        }
    })"
},
{
    "emissive_neon_purple", "Neon Purple", "Emissive",
    "Electric violet-purple neon.",
    "#9020F0",
    R"(type = emissive
    properties = {
        samplers = {
            emissive = { type = color, value = col3(4.0, 0.4, 9.0) }
        }
    })"
},
{
    "emissive_white", "White Light", "Emissive",
    "Neutral balanced white light source.",
    "#FFFFFF",
    R"(type = emissive
    properties = {
        samplers = {
            emissive = { type = color, value = col3(5.0, 5.0, 5.0) }
        }
    })"
},

{
    "stainless_steel_satin", "Steel (Satin)", "Metal",
    "Satin-finish stainless steel with low anisotropy.",
    "#888E94",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.66, 0.70, 0.74) }
        }
        scalars = {
            metallic = 1.0
            roughness = 0.30
            anisotropy = 0.50
            anisotropy_rotation = 0
            ior = 1.5
        }
    })"
},
{
    "paint_pink", "Paint (Pink Gloss)", "Principled",
    "Soft rose-pink gloss paint.",
    "#D8607A",
    R"(type = principled
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.90, 0.34, 0.48) }
        }
        scalars = {
            metallic = 0.0
            roughness = 0.10
            ior = 1.5
            clearcoat = 0.90
            clearcoat_roughness = 0.04
        }
    })"
},
{
    "mossy_stone", "Mossy Stone", "Procedural",
    "Stone surface overgrown with green FBM moss.",
    "#506040",
    R"(type = lambert
    properties = {
        samplers = {
            diffuse = {
                type = fbm_marble
                a = col3(0.32, 0.44, 0.22)
                b = col3(0.44, 0.36, 0.24)
                vein = col3(0.24, 0.30, 0.16)
                scale = 4.5
                vein_frequency = 6.0
                turbulence = 2.5
                octaves = 5
                lacunarity = 2.0
                gain = 0.5
                vein_strength = 0.55
                vein_sharpness = 2.8
            }
        }
    })"
},
{
    "thin_glass", "Thin Glass", "Dielectric",
    "Thin glass pane — one-sided dielectric with minimal thickness effect.",
    "#D4EAF4",
    R"(type = thin_dielectric
    properties = {
        scalars = {
            ior = 1.5
            transparency = 1.0
        }
    })"
},
{
    "sss_milk", "Milk", "Subsurface",
    "Opaque white milk with dense shallow subsurface scattering.",
    "#F0EEE8",
    R"(type = subsurface
    properties = {
        samplers = {
            base_color = { type = color, value = col3(0.96, 0.94, 0.90) }
            subsurface_color = { type = color, value = col3(1.0, 0.98, 0.94) }
            subsurface_radius = { type = color, value = col3(0.80, 0.76, 0.68) }
        }
        scalars = {
            subsurface = 0.90
            thickness = 0.08
            roughness = 0.20
            ior = 1.35
        }
    })"
},

}; /* end CATALOG */

const material_entry_t *material_catalog_entries(size_t &count) {
    count = sizeof(CATALOG) / sizeof(CATALOG[0]);
    return CATALOG;
}

const material_entry_t *material_catalog_find(const std::string &id)
{
    size_t count = 0;
    const material_entry_t *entries = material_catalog_entries(count);
    for (size_t i = 0; i < count; ++i) {
        if (id == entries[i].id) return &entries[i];
    }
    return nullptr;
}

std::string material_catalog_build_preview_scene(const material_entry_t &entry)
{
    std::ostringstream ss;
    ss << "title = Material Preview\n"
       << "version = 1.0\n"
       << "default_camera = matcam\n\n"
       << "environment = {\n"
       << "    type = gradient\n"
       << "    config = {\n"
       << "        a = col3(0.86, 0.90, 0.96)\n"
       << "        b = col3(0.12, 0.16, 0.24)\n"
       << "    }\n"
       << "}\n\n"
       << "camera = {\n"
       << "    matcam = {\n"
       << "        type = thin-lens\n"
       << "        fov = 33\n"
       << "        position = vec3(-1.8, 0.8, -4.6)\n"
       << "        target = vec3(0.12, -0.5, 0.0)\n"
       << "        up = vec3(0, 1, 0)\n"
       << "        flength = 50\n"
       << "        aperture = 0\n"
       << "    }\n"
       << "}\n\n"
       << "geometry = {\n"
       << "    ball = {\n"
       << "        type = mesh\n"
       << "        source = gen(icosphere)\n"
       << "        resolution = 50\n"
       << "        modifiers = {\n"
       << "            scale = vec3(1.06, 1.06, 1.06)\n"
       << "            translation = vec3(0, -0.18, 0)\n"
       << "        }\n"
       << "    }\n"
       << "    companion = {\n"
       << "        type = csg\n"
       << "        op   = intersection\n"
       << "        left = {\n"
       << "            type     = sphere\n"
       << "            position = vec3(1.46, -1.60, 0.0)\n"
       << "            radius   = 0.36\n"
       << "        }\n"
       << "        right = {\n"
       << "            type       = sierpinski_tetrahedron\n"
       << "            position   = vec3(1.46, -1.60, 0.0)\n"
       << "            radius     = 0.44\n"
       << "            resolution = 4\n"
       << "        }\n"
       << "    }\n"
       << "    seat_ring = {\n"
       << "        type = mesh\n"
       << "        source = gen(rounded_ring)\n"
       << "        resolution = 72\n"
       << "        radius = 0.84\n"
       << "        height = 0.10\n"
       << "        thickness = 0.08\n"
       << "        profile_resolution = 12\n"
       << "        modifiers = {\n"
       << "            translation = vec3(0, -1.02, 0)\n"
       << "        }\n"
       << "    }\n"
       << "    stand_post = {\n"
       << "        type = mesh\n"
       << "        source = gen(capped_cylinder)\n"
       << "        resolution = 48\n"
       << "        modifiers = {\n"
       << "            scale = vec3(0.26, 0.72, 0.26)\n"
       << "            translation = vec3(0, -1.28, 0)\n"
       << "        }\n"
       << "    }\n"
       << "    stand_base = {\n"
       << "        type = mesh\n"
       << "        source = gen(capped_cylinder)\n"
       << "        resolution = 64\n"
       << "        modifiers = {\n"
       << "            scale = vec3(0.92, 0.12, 0.92)\n"
       << "            translation = vec3(0, -1.84, 0)\n"
       << "        }\n"
       << "    }\n"
       << "    floor = {\n"
       << "        type = mesh\n"
       << "        source = gen(plane)\n"
       << "        resolution = 1\n"
       << "        modifiers = {\n"
       << "            scale = vec3(12.0, 1.0, 12.0)\n"
       << "            translation = vec3(0, -1.96, 0)\n"
       << "        }\n"
       << "    }\n"
       << "    key_light_geo = {\n"
       << "        type   = sphere\n"
       << "        position = vec3(-3.8, 5.4, -0.8)\n"
       << "        radius = 0.65\n"
       << "    }\n"
       << "    rim_light_geo = {\n"
       << "        type   = sphere\n"
       << "        position = vec3(3.2, 3.6, 2.0)\n"
       << "        radius = 0.40\n"
       << "    }\n"
       << "}\n\n"
       << "material = {\n"
       << "    stand_mat = {\n"
       << "        type = principled\n"
       << "        properties = {\n"
       << "            samplers = {\n"
       << "                base_color = { type = color, value = col3(0.20, 0.20, 0.20) }\n"
       << "            }\n"
       << "            scalars = {\n"
       << "                roughness = 0.48\n"
       << "                metallic = 0.0\n"
       << "                clearcoat = 0.06\n"
       << "                clearcoat_roughness = 0.24\n"
       << "            }\n"
       << "        }\n"
       << "    }\n"
       << "    floor_mat = {\n"
       << "        type = lambert\n"
       << "        properties = {\n"
       << "            samplers = {\n"
       << "                diffuse = {\n"
       << "                    type     = checker\n"
       << "                    a        = col3(0.725, 0.710, 0.680)\n"
       << "                    b        = col3(0.325, 0.310, 0.250)\n"
       << "                    scale_u  = 3.2\n"
       << "                    scale_v  = 3.2\n"
       << "                }\n"
       << "            }\n"
       << "        }\n"
       << "    }\n"
       << "    secondary_mat = {\n"
       << "        type = principled\n"
       << "        properties = {\n"
       << "            samplers = {\n"
       << "                base_color = { type = color, value = col3(0.82, 0.76, 0.58) }\n"
       << "            }\n"
       << "            scalars = {\n"
       << "                roughness = 0.28\n"
       << "                metallic  = 0.88\n"
       << "                ior       = 1.5\n"
       << "            }\n"
       << "        }\n"
       << "    }\n"
       << "    key_light_mat = {\n"
       << "        type = emissive\n"
       << "        properties = {\n"
       << "            samplers = {\n"
       << "                emissive = { type = color, value = col3(42, 36, 26) }\n"
       << "            }\n"
       << "        }\n"
       << "    }\n"
       << "    rim_light_mat = {\n"
       << "        type = emissive\n"
       << "        properties = {\n"
       << "            samplers = {\n"
       << "                emissive = { type = color, value = col3(7, 12, 20) }\n"
       << "            }\n"
       << "        }\n"
       << "    }\n"
       << "    preview_mat = {\n";

    {
        std::istringstream ncf(entry.ncf ? entry.ncf : "");
        std::string line;
        while (std::getline(ncf, line)) {
            ss << "        " << line << "\n";
        }
    }

    ss << "    }\n"
       << "}\n\n"
       << "object = {\n"
       << "    ball       = { geometry = ball,          material = preview_mat   }\n"
       << "    companion  = { geometry = companion,     material = secondary_mat }\n"
       << "    seat_ring  = { geometry = seat_ring,     material = stand_mat     }\n"
       << "    stand_post = { geometry = stand_post,    material = stand_mat     }\n"
       << "    stand_base = { geometry = stand_base,    material = stand_mat     }\n"
       << "    floor      = { geometry = floor,         material = floor_mat     }\n"
       << "    key_light  = { geometry = key_light_geo, material = key_light_mat }\n"
       << "    rim_light  = { geometry = rim_light_geo, material = rim_light_mat }\n"
       << "}\n";
    return ss.str();
}
