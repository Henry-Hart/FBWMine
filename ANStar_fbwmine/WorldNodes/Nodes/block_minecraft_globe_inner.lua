function p.__get_is_solid()
    return true
end

function p.__get_tex()
    return "block_minecraft_world"
end

function p.__main()
    set_default_block("XAR_SOLID_BORING")
    -- in future could make minecrafty shell

    create_rect("minecraft_container", 6, 6, 1, 9, 9, 1)
    
    spawn_path = fbwmine_data.spawn_path
    base_blue_tele.set_blue_type_down(spawn_path[1][1], spawn_path[1][2], spawn_path[1][3])
    
end