function p.__get_is_solid()
    return true
end

function p.__get_tex()
    return "block_minecraft_globe_o1"
end

function p.__main()
    set_default_block("XAR_EMPTY_BORING")
    
    std.create_shell("block_minecraft_globe_outside")

    -- hole
    for x = 6,9 do
    for y = 6,9 do
        set_pos(x,y,15, "block_e")
    end
    end

    add_bent(8,8,9,"bent_base_ring_green")
    set_pos(8, 8, 8, "block_minecraft_globe")
    set_pos(8, 8, 7, "block_minecraft_holder")
end