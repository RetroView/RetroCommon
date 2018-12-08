#include "DeafBabe.hpp"
#include "hecl/Blender/Connection.hpp"

namespace DataSpec::DNAMP1 {

void DeafBabe::BlenderInit(hecl::blender::PyOutStream& os) {
  os << "TYPE_COLORS = {'NoSFX':(0.0, 0.0, 0.0),\n"
        "               'Stone':(1.0, 0.43, 0.15),\n"
        "               'Metal':(0.5, 0.5, 0.5),\n"
        "               'Grass':(0.0, 0.42, 0.01),"
        "               'Ice':(0.0, 0.1, 0.1),\n"
        "               'Metal Grating':(0.09, 0.09, 0.09),\n"
        "               'Phazon':(0.24, 0.0, 0.21),\n"
        "               'Dirt':(0.1, 0.07, 0.05),\n"
        "               'Stone':(0.12, 0.12, 0.12),\n"
        "               'Lava':(0.8, 0.15, 0.0),\n"
        "               'Stone/Rock':(0.06, 0.05, 0.03),\n"
        "               'Snow':(0.9, 1.0, 1.0),\n"
        "               'Mud (Slow)':(0.12, 0.06, 0.02),\n"
        "               'Mud':(0.12, 0.06, 0.02),\n"
        "               'Glass':(0.27, 0.38, 0.9),\n"
        "               'Shield':(1.0, 0.6, 0.0),\n"
        "               'Sand':(0.53, 0.44, 0.21),\n"
        "               'Wood':(0.30, 0.15, 0.03),\n"
        "               'Organic':(0.19, 0.45, 0.2)}\n"
        "\n"
        "# Diffuse Color Maker\n"
        "def make_color(index, mat_type, name):\n"
        "    new_mat = bpy.data.materials.new(name)\n"
        "    if mat_type in TYPE_COLORS:\n"
        "        new_mat.diffuse_color = TYPE_COLORS[mat_type]\n"
        "    else:\n"
        "        new_mat.diffuse_color.hsv = ((index / 6.0) % 1.0, 1.0-((index // 6) / 6.0), 1)\n"
        "    return new_mat\n"
        "\n"
        "bpy.types.Material.retro_unknown = bpy.props.BoolProperty(name='Retro: Unknown (U)')\n"
        "bpy.types.Material.retro_surface_stone = bpy.props.BoolProperty(name='Retro Surface: Stone')\n"
        "bpy.types.Material.retro_surface_metal = bpy.props.BoolProperty(name='Retro Surface: Metal')\n"
        "bpy.types.Material.retro_surface_grass = bpy.props.BoolProperty(name='Retro Surface: Grass')\n"
        "bpy.types.Material.retro_surface_ice = bpy.props.BoolProperty(name='Retro Surface: Ice')\n"
        "bpy.types.Material.retro_pillar = bpy.props.BoolProperty(name='Retro Pillar (I)')\n"
        "bpy.types.Material.retro_surface_metal_grating = bpy.props.BoolProperty(name='Retro Surface: Metal Grating')\n"
        "bpy.types.Material.retro_surface_phazon = bpy.props.BoolProperty(name='Retro Surface: Phazon')\n"
        "bpy.types.Material.retro_surface_dirt = bpy.props.BoolProperty(name='Retro Surface: Rock')\n"
        "bpy.types.Material.retro_surface_lava = bpy.props.BoolProperty(name='Retro Surface: Lava')\n"
        "bpy.types.Material.retro_surface_lava_stone = bpy.props.BoolProperty(name='Retro Surface: Lava Stone')\n"
        "bpy.types.Material.retro_surface_snow = bpy.props.BoolProperty(name='Retro Surface: Snow')\n"
        "bpy.types.Material.retro_surface_mud_slow = bpy.props.BoolProperty(name='Retro Surface: Mud (Slow)')\n"
        "bpy.types.Material.retro_half_pipe = bpy.props.BoolProperty(name='Retro: Half Pipe (H)')\n"
        "bpy.types.Material.retro_surface_mud = bpy.props.BoolProperty(name='Retro Surface: Mud')\n"
        "bpy.types.Material.retro_surface_glass = bpy.props.BoolProperty(name='Retro Surface: Glass')\n"
        "bpy.types.Material.retro_surface_shield = bpy.props.BoolProperty(name='Retro Surface: Shield')\n"
        "bpy.types.Material.retro_surface_sand = bpy.props.BoolProperty(name='Retro Surface: Sand')\n"
        "bpy.types.Material.retro_projectile_passthrough = bpy.props.BoolProperty(name='Retro: Projectile Passthrough "
        "(P)')\n"
        "bpy.types.Material.retro_solid = bpy.props.BoolProperty(name='Retro: Solid (K)')\n"
        "bpy.types.Material.retro_no_platform_collision = bpy.props.BoolProperty(name='Retro: No Platform Collision')\n"
        "bpy.types.Material.retro_camera_passthrough = bpy.props.BoolProperty(name='Retro: Camera Passthrough (O)')\n"
        "bpy.types.Material.retro_surface_wood = bpy.props.BoolProperty(name='Retro Surface: Wood')\n"
        "bpy.types.Material.retro_surface_organic = bpy.props.BoolProperty(name='Retro Surface: Organic')\n"
        "bpy.types.Material.retro_no_edge_collision = bpy.props.BoolProperty(name='Retro: No Edge Collision')\n"
        "bpy.types.Material.retro_see_through = bpy.props.BoolProperty(name='Retro: See Through')\n"
        "bpy.types.Material.retro_scan_passthrough = bpy.props.BoolProperty(name='Retro: Scan Passthrough (S)')\n"
        "bpy.types.Material.retro_ai_passthrough = bpy.props.BoolProperty(name='Retro: AI Passthrough (A)')\n"
        "bpy.types.Material.retro_ceiling = bpy.props.BoolProperty(name='Retro: Ceiling (C)')\n"
        "bpy.types.Material.retro_wall = bpy.props.BoolProperty(name='Retro: Wall (W)')\n"
        "bpy.types.Material.retro_floor = bpy.props.BoolProperty(name='Retro: Floor (F)')\n"
        "\n"
        "material_dict = {}\n"
        "material_index = []\n"
        "def get_type_id(data):\n"
        "\n"
        "    ret = 0\n"
        "    for i in range(1, 24):\n"
        "        if i == 5 or i == 13 or i in range(18, 22):\n"
        "            continue\n"
        "        if ((data >> i) & 1):\n"
        "            ret = i\n"
        "    return ret\n"
        "\n"
        "def select_material(data):\n"
        "\n"
        "    type_id = get_type_id(data)\n"
        "    mat_type = str(type_id)\n"
        "    if type_id == 0:\n"
        "        mat_type = 'NoSFX'\n"
        "    if type_id == 1:\n"
        "        mat_type = 'Stone'\n"
        "    elif type_id == 2:\n"
        "        mat_type = 'Metal'\n"
        "    elif type_id == 3:\n"
        "        mat_type = 'Grass'\n"
        "    elif type_id == 4:\n"
        "        mat_type = 'Ice'\n"
        "    elif type_id == 6:\n"
        "        mat_type = 'Metal Grating'\n"
        "    elif type_id == 7:\n"
        "        mat_type = 'Phazon'\n"
        "    elif type_id == 8:\n"
        "        mat_type = 'Dirt'\n"
        "    elif type_id == 9:\n"
        "        mat_type = 'Lava'\n"
        "    elif type_id == 10:\n"
        "        mat_type = 'Stone/Rock'\n"
        "    elif type_id == 11:\n"
        "        mat_type = 'Snow'\n"
        "    elif type_id == 12:\n"
        "        mat_type = 'Mud (Slow)'\n"
        "    elif type_id == 14:\n"
        "        mat_type = 'Mud'\n"
        "    elif type_id == 15:\n"
        "        mat_type = 'Glass'\n"
        "    elif type_id == 16:\n"
        "        mat_type = 'Shield'\n"
        "    elif type_id == 17:\n"
        "        mat_type = 'Sand'\n"
        "    elif type_id == 22:\n"
        "        mat_type = 'Wood'\n"
        "    elif type_id == 23:\n"
        "        mat_type = 'Organic'\n"
        "\n"
        "    mat_flags = ''\n"
        "    if ((data >> 0) & 1):\n"
        "        mat_flags += 'U'\n"
        "    else:\n"
        "        mat_flags += 'x'\n"
        "    if ((data >> 5) & 1):\n"
        "        mat_flags += 'I'\n"
        "    else:\n"
        "        mat_flags += 'x'\n"
        "    if ((data >> 13) & 1):\n"
        "        mat_flags += 'H'\n"
        "    else:\n"
        "        mat_flags += 'x'\n"
        "    if ((data >> 18) & 1):\n"
        "        mat_flags += 'P'\n"
        "    else:\n"
        "        mat_flags += 'x'\n"
        "    if ((data >> 19) & 1):\n"
        "        mat_flags += 'K'\n"
        "    else:\n"
        "        mat_flags += 'x'\n"
        "    if ((data >> 20) & 1):\n"
        "        mat_flags += 'u'\n"
        "    else:\n"
        "        mat_flags += 'x'\n"
        "    if ((data >> 21) & 1):\n"
        "        mat_flags += 'O'\n"
        "    else:\n"
        "        mat_flags += 'x'\n"
        "    if ((data >> 24) & 1):\n"
        "        mat_flags += 'u'\n"
        "    else:\n"
        "        mat_flags += 'x'\n"
        "    if ((data >> 26) & 1):\n"
        "        mat_flags += 'T'\n"
        "    else:\n"
        "        mat_flags += 'x'\n"
        "    if ((data >> 27) & 1):\n"
        "        mat_flags += 'S'\n"
        "    else:\n"
        "        mat_flags += 'x'\n"
        "    if ((data >> 28) & 1):\n"
        "        mat_flags += 'A'\n"
        "    else:\n"
        "        mat_flags += 'x'\n"
        "    if ((data >> 29) & 1):\n"
        "        mat_flags += 'C'\n"
        "    else:\n"
        "        mat_flags += 'x'\n"
        "    if ((data >> 30) & 1):\n"
        "        mat_flags += 'W'\n"
        "    else:\n"
        "        mat_flags += 'x'\n"
        "    if ((data >> 31) & 1):\n"
        "        mat_flags += 'F'\n"
        "    else:\n"
        "        mat_flags += 'x'\n"
        "\n"
        "    if len(mat_flags) > 0:\n"
        "        mat_flags = ' ' + mat_flags\n"
        "\n"
        "    mat_name = mat_type + mat_flags\n"
        "\n"
        "    if mat_name in material_index:\n"
        "        return material_index.index(mat_name)\n"
        "    elif mat_name in material_dict:\n"
        "        material_index.append(mat_name)\n"
        "        return len(material_index)-1\n"
        "    else:\n"
        "        mat = make_color(len(material_dict), mat_type, mat_name)\n"
        "        mat.diffuse_intensity = 1.0\n"
        "        mat.specular_intensity = 0.0\n"
        "        mat.retro_unknown = ((data >> 0) & 1)\n"
        "        mat.retro_surface_stone = ((data >> 1) & 1)\n"
        "        mat.retro_surface_metal = ((data >> 2) & 1)\n"
        "        mat.retro_surface_grass = ((data >> 3) & 1) \n"
        "        mat.retro_surface_ice = ((data >> 4) & 1)\n"
        "        mat.retro_pillar = ((data >> 5) & 1)\n"
        "        mat.retro_surface_metal_grating = ((data >> 6) & 1)\n"
        "        mat.retro_surface_phazon = ((data >> 7) & 1)\n"
        "        mat.retro_surface_dirt = ((data >> 8) & 1)\n"
        "        mat.retro_surface_lava = ((data >> 9) & 1)\n"
        "        mat.retro_surface_lava_stone = ((data >> 10) & 1)\n"
        "        mat.retro_surface_snow = ((data >> 11) & 1)\n"
        "        mat.retro_surface_mud_slow = ((data >> 12) & 1)\n"
        "        mat.retro_half_pipe = ((data >> 13) & 1)\n"
        "        mat.retro_surface_mud = ((data >> 14) & 1)\n"
        "        mat.retro_surface_glass = ((data >> 15) & 1)\n"
        "        mat.retro_surface_shield = ((data >> 16) & 1)\n"
        "        mat.retro_surface_sand = ((data >> 17) & 1)\n"
        "        mat.retro_projectile_passthrough = ((data >> 18) & 1)\n"
        "        mat.retro_solid = ((data >> 19) & 1)\n"
        "        mat.retro_no_platform_collision = ((data >> 20) & 1)\n"
        "        mat.retro_camera_passthrough = ((data >> 21) & 1)\n"
        "        mat.retro_surface_wood = ((data >> 22) & 1)\n"
        "        mat.retro_surface_organic = ((data >> 23) & 1)\n"
        "        mat.retro_no_edge_collision = ((data >> 24) & 1)\n"
        "        mat.retro_see_through = ((data >> 26) & 1)\n"
        "        mat.retro_scan_passthrough = ((data >> 27) & 1)\n"
        "        mat.retro_ai_passthrough = ((data >> 28) & 1)\n"
        "        mat.retro_ceiling = ((data >> 29) & 1)\n"
        "        mat.retro_wall= ((data >> 30) & 1)\n"
        "        mat.retro_floor = ((data >> 31) & 1)\n"
        "        material_dict[mat_name] = mat\n"
        "        material_index.append(mat_name)\n"
        "        return len(material_index)-1\n"
        "\n"
        "\n";
}

} // namespace DataSpec::DNAMP1
