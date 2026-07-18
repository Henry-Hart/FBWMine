function p.__get_is_solid()
    return true
end

function p.__get_tex()
    return "block_dan_house_thick_wood_floor"
end

function p.__main()
    set_default_block("XAR_EMPTY_BORING")

    create_rect("XAR_DAN_HOUSE_BLOCK_THICK_WOOD_FLOOR", 0, 0, 0, 15, 15, 4)
    create_rect("XAR_DAN_HOUSE_BLOCK_THICK_WOOD_FLOOR", 2, 2, 5, 13, 13, 5)
    create_rect("XAR_DAN_HOUSE_BLOCK_THICK_WOOD_FLOOR", 6, 6, 0, 9, 9, 15)

    local str = {"W","O","R","L","D"," ","G","L","O","B","E"}

    for i = 1, #str do
        if str[i] ~= " " then
            set_pos(0, 15-i, 3, "XAR_SOLID_BORING_LETTER_" .. str[i])
            set_pos(i, 0, 3, "XAR_SOLID_BORING_LETTER_" .. str[i])
            set_pos(15, i, 3, "XAR_SOLID_BORING_LETTER_" .. str[i])
            set_pos(15-i, 15, 3, "XAR_SOLID_BORING_LETTER_" .. str[i])
        end
    end
    
    create_rect("XAR_EMPTY_BORING", 1, 1, 1, 14, 14, 3)
    create_rect("XAR_EMPTY_BORING", 7, 7, 1, 8, 8, 15)

    for i = 1, 11 do
        set_pos(2, 15-i, 1, "XAR_SOLID_BORING")
        set_pos(i, 2, 1, "XAR_SOLID_BORING")
        set_pos(13, i, 1, "XAR_SOLID_BORING")
        set_pos(15-i, 13, 1, "XAR_SOLID_BORING")
    end

    for i = 6, 12 do
        set_pos(4, 15-i, 1, "XAR_SOLID_BORING_GATO_GREEN")
        set_pos(i, 4, 1, "XAR_SOLID_BORING_GATO_LIGHT_BLUE")
        set_pos(11, i, 1, "XAR_SOLID_BORING_GATO_PURPLE")
        set_pos(15-i, 11, 1, "XAR_SOLID_BORING_GATO_YELLOW")
    end

    for i = 5, 7 do
        set_pos(6, 15-i, 1, "XAR_SOLID_BORING_GATO_YELLOW")
        set_pos(i, 6, 1, "XAR_SOLID_BORING_GATO_GREEN")
        set_pos(9, i, 1, "XAR_SOLID_BORING_GATO_LIGHT_BLUE")
        set_pos(15-i, 9, 1, "XAR_SOLID_BORING_GATO_PURPLE")
    end

    create_rect("XAR_GLASS", 1, 1, 2, 14, 14, 2)
    create_rect("XAR_SOLID_BORING_BLACK", 1, 1, 3, 14, 14, 3)
    create_rect("XAR_EMPTY_BORING", 3, 3, 3, 12, 12, 3)
    create_rect("XAR_SOLID_BORING_CONCRETE_WHITE_BORDER", 3, 3, 4, 12, 12, 4)

    create_rect("XAR_DAN_HOUSE_BLOCK_THICK_WOOD_FLOOR", 6, 6, 2, 9, 9, 4)
    create_rect("XAR_EMPTY_BORING", 7, 7, 1, 8, 8, 15)

    set_pos(1, 14, 1, "XAR_STOPPER")
    add_bent(1, 15, 1, "bent_base_ring_green")
    set_pos(1, 15, 1, "XAR_EMPTY_BORING")
    set_pos(6, 7, 1, "XAR_ASCEND_SIMPLE")

    set_pos(1, 1, 1, "XAR_STOPPER")
    add_bent(0, 1, 1, "bent_base_ring_green")
    set_pos(0, 1, 1, "XAR_EMPTY_BORING")
    set_pos(8, 6, 1, "XAR_ASCEND_SIMPLE")

    set_pos(14, 1, 1, "XAR_STOPPER")
    add_bent(14, 0, 1, "bent_base_ring_green")
    set_pos(14, 0, 1, "XAR_EMPTY_BORING")
    set_pos(9, 8, 1, "XAR_ASCEND_SIMPLE")

    set_pos(14, 14, 1, "XAR_STOPPER")
    add_bent(15, 14, 1, "bent_base_ring_green")
    set_pos(15, 14, 1, "XAR_EMPTY_BORING")
    set_pos(7, 9, 1, "XAR_ASCEND_SIMPLE")

    --add_bent(5, 5, 1, "bent_env_var_open_door")

    create_rect("XAR_ANTI_PLUG", 7, 7, 1, 8, 8, 1)

    --create_rect("XAR_TOLL_DOOR_SOLID", 7, 7, 15, 8, 8, 15)

    -- load spawn regions
    -- this HAS to be done here NOT the globe
    bridge.comm_preload()
end