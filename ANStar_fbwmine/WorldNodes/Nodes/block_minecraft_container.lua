function p.__get_is_solid()
    return false
end

function p.__get_tex()
    return ""
end

function p.__main()
    set_default_block("XAR_SOLID_BORING")

    spawn_path = fbwmine_data.spawn_path
    
    local depth = 0
    local level = get_level()
    local PATH = get_input_path()
    for i = 1, 4 do -- bad, but the only thing that works
        if get_input_path_bt(level-i) == "block_minecraft_container" then
            depth = depth + 1
        else
            break
        end
    end

    -- make blue rings channel to spawnpoint
    local good = true
    for i = 0, depth do
        pos = PATH[level-i-1]
        if pos.x ~= spawn_path[depth-i+1][1] or pos.y ~= spawn_path[depth-i+1][2] or pos.z ~= spawn_path[depth-i+1][3] then
            good = false
            break
        end
    end

    if good then
        next_path = spawn_path[depth+2]
        base_blue_tele.set_blue_type_down(next_path[1], next_path[2], next_path[3])
    else
        base_blue_tele.set_blue_type_up()
    end

    -- TODO: keep track of full path for world border
    if depth < 3 then
        create_rect("minecraft_container", 0, 0, 1, 15, 15, 1)
    elseif depth == 3 then
        create_rect("minecraft_container", 0, 0, 1, 15, 15, 2)
    else
        create_rect("minecraft_chunk", 0, 0, 0, 15, 15, 15)
    end
end