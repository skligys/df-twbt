DFHACK_PLUGIN("twbt");

DFhackCExport command_result plugin_init ( color_ostream &out, vector <PluginCommand> &commands)
{
    out2 = &out;

    *out2 << "DFHack: version " << DFHACK_VERSION << ", TWBT: version " << TWBT_VER << std::endl;

    int mode = get_mode();
    if (!mode)
    {
        *out2 << COLOR_RED << "TWBT: set PRINT_MODE to TWBT in data/init/init.txt to activate the plugin" << std::endl;
        *out2 << COLOR_RESET;
        return CR_OK;        
    }
    if (mode == 1)
    {
#ifdef LEGACY_MODE_ONLY
        *out2 << COLOR_RED << "TWBT: falling back to legacy mode with this version of DF" << std::endl;
        *out2 << COLOR_RESET;
        mode = -1;
#else        
    #ifdef NO_RENDERING_PATCH
        *out2 << "TWBT: no rendering patch (not an error)" << std::endl;
    #endif
    #ifdef NO_DISPLAY_PATCH
        *out2 << "TWBT: no display patch (not an error)" << std::endl;
    #endif
#endif
    }
    legacy_mode = (mode == -1);

    #ifdef WIN32
        _load_multi_pdim = (LOAD_MULTI_PDIM) (A_LOAD_MULTI_PDIM + Core::getInstance().vinfo->getRebaseDelta());
        _render_map = (RENDER_MAP) (A_RENDER_MAP + Core::getInstance().vinfo->getRebaseDelta());
        _render_updown = (RENDER_UPDOWN) (A_RENDER_UPDOWN + Core::getInstance().vinfo->getRebaseDelta());
    #elif defined(__APPLE__)
        _load_multi_pdim = (LOAD_MULTI_PDIM) A_LOAD_MULTI_PDIM;    
        _render_map = (RENDER_MAP) A_RENDER_MAP;
        _render_updown = (RENDER_UPDOWN) A_RENDER_UPDOWN;
    #else
        _load_multi_pdim = (LOAD_MULTI_PDIM) dlsym(RTLD_DEFAULT, "_ZN8textures15load_multi_pdimERKSsPlllbS2_S2_");
        _render_map = (RENDER_MAP) A_RENDER_MAP;
        _render_updown = (RENDER_UPDOWN) A_RENDER_UPDOWN;
    #endif

    bad_item_flags.whole = 0;
    bad_item_flags.bits.in_building = true;
    bad_item_flags.bits.garbage_collect = true;
    bad_item_flags.bits.removed = true;
    bad_item_flags.bits.dead_dwarf = true;
    bad_item_flags.bits.murder = true;
    bad_item_flags.bits.construction = true;
    bad_item_flags.bits.in_inventory = true;
    bad_item_flags.bits.in_chest = true;

    // Used only if rendering patch is not available
    skytile = d_init->sky_tile;
    chasmtile = d_init->chasm_tile;

    struct stat buf;    

    if (stat("data/art/white1px.png", &buf) == 0)
    {
        long dx, dy;        
        load_tileset("data/art/white1px.png", &white_texpos, 1, 1, &dx, &dy);
    }
    else
    {
        *out2 << COLOR_RED << "TWBT: data/art/white1px.png not found, can not continue" << std::endl;
        *out2 << COLOR_RESET;

        return CR_FAILURE;
    }

    if (stat("data/art/transparent1px.png", &buf) == 0)
    {
        long dx, dy;        
        load_tileset("data/art/transparent1px.png", &transparent_texpos, 1, 1, &dx, &dy);    
    }
    else
    {
        *out2 << COLOR_RED << "TWBT: data/art/transparent1px.png not found, can not continue" << std::endl;
        *out2 << COLOR_RESET;

        return CR_FAILURE;
    }

    if (init->display.flag.is_set(init_display_flags::USE_GRAPHICS))
    {
        // Graphics is enabled. Map tileset is already loaded, so use it. Then load text tileset.

        // Existing map tileset - accessible at index 0
        struct tileset ts;
        memcpy(ts.small_texpos, init->font.small_font_texpos, sizeof(ts.small_texpos));
        tilesets.push_back(ts);

        // We will replace init->font with text font, so let's save graphics tile size
        small_map_dispx = init->font.small_font_dispx, small_map_dispy = init->font.small_font_dispy;

        has_textfont = load_text_font();
        if (!has_textfont)
            tilesets.push_back(tilesets[0]);
    }
    else
    {
        // Graphics is disabled. Text tileset is already loaded. Load map tileset.

        has_textfont = load_map_font();

        // Existing text tileset - accessible at index 1
        if (has_textfont)
        {
            struct tileset ts;
            memcpy(ts.small_texpos, init->font.small_font_texpos, sizeof(ts.small_texpos));
            tilesets.push_back(ts);
        }
        else
            tilesets.push_back(tilesets[0]);
    }

    init_text_tileset_layers();
    
    if (!has_textfont)
    {
        *out2 << COLOR_YELLOW << "TWBT: FONT and GRAPHICS_FONT are the same" << std::endl;
        *out2 << COLOR_RESET;
    }

    has_overrides = load_overrides();    

    // Load shadows
    if (stat("data/art/shadows.png", &buf) == 0)
    {
        long dx, dy;        
        load_tileset("data/art/shadows.png", shadow_texpos, 8, 1, &dx, &dy);
        shadowsloaded = true;
    }
    else
    {
        *out2 << COLOR_RED << "TWBT: data/art/shadows.png not found, shadows will not be rendered" << std::endl;
        *out2 << COLOR_RESET;
    }

    map_texpos = tilesets[0].small_texpos;
    text_texpos = tilesets[1].small_texpos;

    commands.push_back(PluginCommand(
        "mapshot", "Mapshot!",
        mapshot_cmd, true,
        ""
    ));        
    commands.push_back(PluginCommand(
        "multilevel", "Multi-level rendering",
        multilevel_cmd, false,
        ""
    ));       
    commands.push_back(PluginCommand(
        "colormap", "Colomap manipulation",
        colormap_cmd, false,
        ""
    ));
    commands.push_back(PluginCommand(
        "twbt", "Text Will Be Text",
        twbt_cmd, false,
        ""
    ));       

    if (!legacy_mode)
    {
        replace_renderer();

        INTERPOSE_HOOK(dwarfmode_hook, render).apply(true);
        INTERPOSE_HOOK(dwarfmode_hook, logic).apply(true);
        INTERPOSE_HOOK(dwarfmode_hook, feed).apply(true);

        INTERPOSE_HOOK(viewscreen_unitlistst_zoomfix, feed).apply(true);
        INTERPOSE_HOOK(viewscreen_buildinglistst_zoomfix, feed).apply(true);        
        INTERPOSE_HOOK(viewscreen_layer_unit_relationshipst_zoomfix, feed).apply(true);        

        INTERPOSE_HOOK(dungeonmode_hook, render).apply(true);
        INTERPOSE_HOOK(dungeonmode_hook, logic).apply(true);
        INTERPOSE_HOOK(dungeonmode_hook, feed).apply(true); 

        enable_building_hooks();       
        enable_item_hooks();
        enable_unit_hooks();
    }
    else
    {
        hook_legacy();
        INTERPOSE_HOOK(dwarfmode_hook_legacy, render).apply(true);
    }

    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    if (event == SC_WORLD_LOADED)
    {
        update_custom_building_overrides();
        gmenu_w = -1;
        block_index_size = 0;
    }
    else if (event == SC_VIEWSCREEN_CHANGED)
    {
        gps->force_full_display_count = 2;
        screen_map_type = 0;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    /*if (legacy_mode)
    {
        INTERPOSE_HOOK(dwarfmode_hook_legacy, render).apply(false);
        unhook_legacy();
    }*/
    
    return CR_FAILURE;

    /*if (enabled)
        restore_renderer();

    return CR_OK;*/
}