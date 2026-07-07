function p.__get_is_solid()
    return false
end

function p.__get_tex()
    return ""
end

function p.__main()
    set_default_block("XAR_EMPTY_BORING")


    spawn_path = fbwmine_data.spawn_path

    local level = get_level()
    local PATH = get_input_path()
    local good = true
    local total_x = 0
    local total_y = 0
    local total_z = 0
    local coeff = 16
    for i = 0, 5 do
        pos = PATH[level-i-1]

        -- calculate coords
        total_x = total_x + pos.x * coeff
        total_y = total_y + pos.y * coeff
        total_z = total_z + (pos.z - 1) * coeff
        coeff = coeff * 16

        -- try to speed this up slightly
        if (not good) or pos.x ~= spawn_path[6-i][1] or pos.y ~= spawn_path[6-i][2] or pos.z ~= spawn_path[6-i][3] then
            good = false
        end
    end

    -- obtain correct coords
    total_x = total_x - 134217728
    total_y = total_y - 134217728
    total_z = total_z - 48
    
    -- world border check
    if -29999983 > total_x or total_x > 29999983 or -29999983 > total_y or total_y > 29999983 then
        create_rect("block_minecraft_world_border", 0, 0, 0, 15, 15, 15)
    elseif bridge.comm(total_x//16, total_y//16, total_z//16 + 4) then

        -- if we actually sent some data, load in a chunk
        bridge.chunk_from_data(total_x, total_z, total_y)
    end

    if good then -- we are in the right subchunk
        base_blue_tele.set_blue_type_terminal(spawn_path[7][1], spawn_path[7][2], spawn_path[7][3])
    else
        base_blue_tele.set_blue_type_up()
    end

    -- this can be used for debugging
    --add_bent_s(0,0,0,"bent_base_txt", "This text box is at position " .. tostring(total_x) .. "," .. tostring(total_z) .. "," .. tostring(total_y)
    --.. "\nChunk pos " .. tostring(total_x//16) .. "," .. tostring(total_z//16) .. "," .. tostring(total_y//16)
    --)
end