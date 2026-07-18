function p.__get_is_solid()
    return true
end

function p.__get_tex()
    return "block_dark_green_border"
end

function p.__main()
    set_default_block("XAR_EMPTY_BORING")

    create_xar_chunk("XAR_SMALL_YELLOW_FLOWER_LASER_SECRET_2")

    set_pos(4,9,9, "block_minecraft_globe_room")
end