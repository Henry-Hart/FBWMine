function p.__get_is_solid()
    return true
end

function p.__get_tex()
    return "block_minecraft_world"
end

function p.__main()
    set_default_block("XAR_EMPTY_BORING")

    base_blue_tele.set_blue_type_down(8, 8, 8)

    -- shell
    create_rect("XAR_SOLID_BORING_BLUE_BORDER", 1, 1, 1, 14, 14, 14)
    local centers = {{1,1,1}, {14, 13, 1}, {5, 1, 13}, {1, 10, 13},
                     {12, 12, 14}, {11, 1, 13}, {2, 6, 1}, {14, 1, 1}}               
    for i = 1,#centers do
        if i % 2 == 0 then 
        create_rect("XAR_SOLID_BORING_BRIGHT_GREEN_BORDER",
            centers[i][1]-1, centers[i][2]-1, centers[i][3]-1,
            centers[i][1]+1, centers[i][2]+1, centers[i][3]+1)
        else
            create_rect("XAR_SOLID_BORING_DARK_GREEN_BORDER",
            centers[i][1]-1, centers[i][2]-1, centers[i][3]-1,
            centers[i][1]+1, centers[i][2]+1, centers[i][3]+1)
        end
    end

    local big_centers = {{14,6,8}, {1,7,8}, {9,1,7}, {8,14,9}}            
    for i = 1,#big_centers do
        create_rect("XAR_SOLID_BORING_DARK_GREEN_X",
            big_centers[i][1]-2, big_centers[i][2]-2, big_centers[i][3]-1-(i%2),
            big_centers[i][1]+(i%2), big_centers[i][2]+2, big_centers[i][3]+2)
    end
    std.create_shell("XAR_GLASS")

    create_rect("XAR_SOLID_BORING_DARK_GREEN_X", 7, 7, 14, 9, 9, 14)

    -- hollow out inside
    create_rect("XAR_EMPTY_BORING", 2, 2, 2, 13, 13, 13)

    -- bottom attachment
    create_rect("XAR_DAN_HOUSE_BLOCK_THICK_WOOD_FLOOR", 6, 6, 0, 9, 9, 0)
    create_rect("XAR_EMPTY_BORING", 7, 7, 0, 8, 8, 1)

    add_bent_s(13, 3, 12, "bent_base_txt", 
        "This ^x0000ffBlue Ring^! will take you deep inside the " ..
        "^xffff00Inner Globe^! in the center of this room.\n\n" ..
        "To exit the ^xffff00Inner Globe^!, fly up to find ^xff00ffPink Rings^! when inside.\n\n" ..
        "If you get ^xff0000stuck^!, restart ^xffffffFBW^! without starting ^xffffffFBWMine^!."
    )
    add_bent(13, 3, 11, "bent_base_ring_blue")

    set_pos(8, 8, 8, "minecraft_globe_inner")

    -- so the player can exit
    add_bent(2, 13, 2, "bent_base_waypoint_out_only")

    base_blue_tele.set_blue_type_down(8,8,8)
end