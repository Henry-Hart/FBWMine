function p.__get_is_solid()
    return true
end

function p.__get_tex()
    return "block_minecraft_globe_o1"
end

function p.__main()
    set_default_block("XAR_EMPTY_BORING")

    -- outside
    create_rect("block_minecraft_globe_outside", 0, 0, 0, 15, 15, 15)

    -- wall decoration
    create_rect("XAR_SOLID_BORING", 0, 5, 5, 0, 10, 10)
    create_rect("XAR_SOLID_BORING", 5, 0, 5, 10, 15, 10)
    create_rect("XAR_SOLID_BORING", 5, 5, 0, 10, 10, 15)

    -- hole
    create_rect("XAR_EMPTY_BORING", 15, 6, 6, 15, 9, 9)

    -- space inside
    create_rect("XAR_EMPTY_BORING", 1, 1, 1, 14, 14, 14)

    -- pillars
    create_rect("block_minecraft_globe_outside", 6, 6, 1, 6, 6, 8)
    create_rect("block_minecraft_globe_outside", 6, 9, 1, 6, 9, 7)
    create_rect("block_minecraft_globe_outside", 9, 6, 1, 9, 6, 6)
    create_rect("block_minecraft_globe_outside", 9, 9, 1, 9, 9, 5)

    -- things on top of pillars
    add_bent(9, 9, 6, "bent_base_ring_green")
    set_pos(9, 8, 5, "XAR_ASCEND_SIMPLE")
    set_pos(8, 9, 5, "XAR_ASCEND_SIMPLE")
    set_pos(9, 10, 5, "XAR_ASCEND_SIMPLE")
    set_pos(10, 9, 5, "XAR_ASCEND_SIMPLE")

    set_pos(6, 9, 9, "block_minecraft_globe")
    set_pos(6, 9, 8, "block_minecraft_holder")

    add_bent_s(6, 6, 9, "bent_base_waypoint", "World Globe Room")

    set_pos(9, 6, 7, "block_minecraft_info")
end