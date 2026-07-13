
-- communication string
local t = string.rep("\0", 4096)

-- for an optimisation
local block_ids = {}
local block_ids_set = {}

-- to sync with FBWMine version
local LUA_VERSION = 0

function p.comm(o1, o2, o3)

    height = o3
    if height > 23 then

        -- generate a pink / blue ring instead

        if (randf() < 0.3) then
            local x = randi(0,15)
            local y = randi(0,15)
            local z = randi(0,15)
            if (randf() < 0.3) then
                --add_bent(x,y,z, "bent_base_ring_blue")
            else
                add_bent(x,y,z, "bent_base_ring_pink_source")
            end
        end

        return false
    end

    set_blue_type_up(t, o1, height, o2, 0, 0)
    return true
end

-- generate a chunk from t
function p.chunk_from_data(msg_x, msg_y, msg_z)

    -- error handling
    if string.byte(t, 1) == 0 then
        
        local e = string.byte(t, 2)
        local wanted_lua_version = string.byte(t, 3)
        if wanted_lua_version < LUA_VERSION then
            add_bent_s(8,8,8,"bent_base_txt", 
                "^xffffffYour FBWMine.exe needs updating^!"
            )
            create_rect("XAR_RAINBOW_FLOWER_SOLID", 7, 7, 7, 9, 9, 7)
            return
        elseif wanted_lua_version > LUA_VERSION then
            add_bent_s(8,8,8,"bent_base_txt", 
                "^xffffffYour LUA files for this mod need updating^!"
            )
            create_rect("XAR_RAINBOW_FLOWER_SOLID", 7, 7, 7, 9, 9, 7)
            return
        end

        local ring_msg =  "You can fly up to the top to find ^xff00ffPink Rings^!, " .. 
            "which will take you to the world exit"
            --"The ^x0000ffBlue Rings^! won't take you anywhere useful right now"
            --"The ^x0000ffBlue Rings^! will teleport you to spawn"
        
        local base_to_load_msg = "If you want to visit this chunk, you first need to travel to " ..
                "^xffffff" .. tostring(msg_x) .. ",^! " ..
                "^xffffff" .. tostring(msg_y) .. ",^! " ..
                "^xffffff" .. tostring(msg_z) .. "^!"
        
        local to_load_msg = base_to_load_msg .. " in Minecraft to load it.\n\n"

        if e == 0 then
            add_bent_s(8,8,8,"bent_base_txt", "^xff0000FBWMine isn't connected :(^!\n\n" ..
                ring_msg
            )
            create_rect("XAR_SOLID_BORING_CONCRETE_RED_X", 7, 7, 7, 9, 9, 7)
        elseif e == 1 then
            add_bent_s(8,8,8,"bent_base_txt", "^xffff00This region doesn't exist^!\n\n" ..
                to_load_msg ..
                ring_msg
            )
            create_rect("XAR_SOLID_BORING_CONCRETE_ORANGE_X", 7, 7, 7, 9, 9, 7)
        elseif e == 2 then
            add_bent_s(8,8,8,"bent_base_txt", 
                "^xffff00The region containing this chunk didn't load in time^!\n\n" ..
                "If you wait a second (literally) and then ^xffffffsave^! and ^xffffffload^!, " ..
                "this chunk should spawn in. Remember, for safety, unless you know where you are, " ..
                "you should move out of this chunk to avoid ^xff0000getting stuck^! in a block!\n\n" ..
                ring_msg
            )
            create_rect("XAR_SOLID_BORING_GATO_LIGHT_BLUE", 7, 7, 7, 9, 9, 7)
        elseif e == 3 then
            add_bent_s(8,8,8,"bent_base_txt", "^xffff00This region is open in Minecraft^!\n\n" ..
                "Well probably, at least. Another program is blocking access to the region file. " ..
                "For now, if you have Minecraft open, chunks near the player won't load. " ..
                "In the future I may add ^x00ff00share mode injection^!, which will fix this.\n\n" ..
                ring_msg
            )
            create_rect("XAR_SOLID_BORING_GATO_PURPLE", 7, 7, 7, 9, 9, 7)
        elseif e == 4 then
            add_bent_s(8,8,8,"bent_base_txt", "^xffff00This chunk doesn't exist^!\n\n" ..
                to_load_msg ..
                ring_msg
            )
            create_rect("XAR_SOLID_BORING_GATO_GREEN", 7, 7, 7, 9, 9, 7)
        elseif e == 5 then
            add_bent_s(8,8,8,"bent_base_txt", "^xffff00This chunk hasn't fully generated^!\n\n" ..
                to_load_msg ..
                ring_msg
            )
            create_rect("XAR_SOLID_BORING_GATO_GREEN", 7, 7, 7, 9, 9, 7)
            set_pos(8, 7, 7, "XAR_SOLID_BORING_GATO_YELLOW")
            set_pos(8, 9, 7, "XAR_SOLID_BORING_GATO_YELLOW")
            set_pos(7, 8, 7, "XAR_SOLID_BORING_GATO_YELLOW")
            set_pos(9, 8, 7, "XAR_SOLID_BORING_GATO_YELLOW")
        elseif e == 6 then
            add_bent_s(8,8,8,"bent_base_txt", "^xffff00This region is hollow^!\n\n" ..
                to_load_msg ..
                ring_msg
            )
            create_rect("XAR_SOLID_BORING_CONCRETE_ORANGE_X", 7, 7, 7, 9, 9, 7)
            set_pos(8, 7, 7, "XAR_SOLID_BORING_GATO_YELLOW")
            set_pos(8, 9, 7, "XAR_SOLID_BORING_GATO_YELLOW")
            set_pos(7, 8, 7, "XAR_SOLID_BORING_GATO_YELLOW")
            set_pos(9, 8, 7, "XAR_SOLID_BORING_GATO_YELLOW")
        elseif e == 7 then
            add_bent_s(8,8,8,"bent_base_txt", 
                "^xffff00This chunk was generated in an older version of Minecraft^!\n\n" ..
                base_to_load_msg ..
                " ^x00ffffin a newer version of Minecraft (1.13+)^! to load it.\n\n" ..
                ring_msg
            )
            create_rect("XAR_DAN_HOUSE_BLOCK_THICK_WOOD_FLOOR", 7, 7, 7, 9, 9, 7)
        
        -- errors you will basically never get
        elseif e == 8 then
            add_bent_s(8,8,8,"bent_base_txt", 
                "^xff0000This chunk is in an invalid format^!\n\n" ..
                ring_msg
            )
            create_rect("XAR_SOLID_BORING_DECO_1", 7, 7, 7, 9, 9, 7)
        elseif e == 9 then
            add_bent_s(8,8,8,"bent_base_txt", 
                "^xff0000This chunk is too big^!\n\n" ..
                ring_msg
            )
            create_rect("XAR_SOLID_BORING_DECO_1", 7, 7, 7, 9, 9, 7)
            set_pos(7, 7, 7, "XAR_SOLID_BORING_MUSHROOM_RED")
            set_pos(9, 9, 7, "XAR_SOLID_BORING_MUSHROOM_RED")
        elseif e == 10 then
            add_bent_s(8,8,8,"bent_base_txt", 
                "^xff0000This chunk is weirdly compressed^!\n\n" ..
                ring_msg
            )
            create_rect("XAR_SOLID_BORING_DECO_1", 7, 7, 7, 9, 9, 7)
            set_pos(9, 7, 7, "XAR_SOLID_BORING_MUSHROOM_YELLOW")
            set_pos(7, 9, 7, "XAR_SOLID_BORING_MUSHROOM_YELLOW")
        end

        return
    end

    -- normal generation
    for x = 0,15 do
    for y = 0,15 do
    for z = 0,15 do
        
        local v = string.byte(t, z*256+x*16+y+1)
        
        if v == 0 then -- should never happen
            set_pos(x,y,z, "XAR_SOLID_BORING_CONCRETE_RED_X")
        else
            if block_ids_set[v] == 0 then
                block_ids[v] = bt_str_to_code(fbwmine_data.blocks[v])
                block_ids_set[v] = 1
            end
            set_pos_c(x,y,z, block_ids[v])
        end

    end end end

end

-- has to be at the end
for i = 1,255 do
    block_ids[i] = 0
    block_ids_set[i] = 0
end