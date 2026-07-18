function p.__get_is_solid()
    return true
end

function p.__get_tex()
    return "block_info"
end

function p.__main()
    set_default_block("XAR_EMPTY_BORING")

    -- info shell
    create_rect("XAR_SOLID_BORING_INFO", 0, 0, 0, 15, 15, 15)
    create_rect("XAR_EMPTY_BORING", 1, 1, 1, 14, 14, 14)
    
    -- hole
    create_rect("XAR_EMPTY_BORING", 6, 15, 6, 9, 15, 9)

    -- meme welcome
    add_bent(7, 1, 7, "bent_minecraft_meme")

    -- green column
    add_bent_s(14, 11, 11, "bent_base_txt", 
        "^x00ff00HOW TO SET UP THE MOD^! (read this first)"
    )

    create_rect("XAR_SOLID_BORING_DARK_GREEN_X", 14, 12, 3, 14, 12, 9)
    create_rect("XAR_SOLID_BORING_DARK_GREEN_X", 14, 10, 3, 14, 10, 9)

    add_bent_s(14, 11, 9, "bent_base_txt", 
        "Make sure you have downloaded the latest release here: " ..
        "^xffffffhttps://github.com/Henry-Hart/FBWMine/releases^!.\n\n" ..
        "Unzip it and remember where your unzipped folder is.\n\n" ..
        "If you want you can move the folder to a more convenient location."
    )

    add_bent_s(14, 11, 7, "bent_base_txt", 
        "Inside the main folder, open ^xffffffconfig^!.\n\n" ..
        "Choose one of ^xffff00settings_mansion.txt^!, " .. 
        "^x00ffffsettings_monument.txt^! or ^xff00ffsettings_fortress.txt^!.\n\n" ..
        "Rename your chosen file to ^xffffffsettings.txt^!."
    )

    add_bent_s(14, 11, 5, "bent_base_txt", 
        "Double click ^xffffffFBWMine^! (the copy) to start running it.\n\n" ..
        "^xffffffFBWMine^! will now appear in a terminal.\n\n" ..
        "If the last message in the terminal is " ..
        "^x00ff00Press ENTER to exit...^! then something went wrong. " ..
        "In this case try to fix it by reading the error message, " .. 
        "near the end of the output. If the error is confusing, try rerunning " ..
        "^xffffffFBWMine^! as administrator. If this fails, look on discord for help.\n\n" ..
        "If the last message is ^x00ff00Added inline partner hook!^! then it is working!"
    )

    add_bent_s(14, 11, 3, "bent_base_txt", 
        "Assuming ^xffffffFBWMine^! is working, you can now make your way to the " ..
        "^xffff00World Globe^! on the second-highest pillar.\n\n"
    )

    -- yellow column
    add_bent_s(14, 7, 11, "bent_base_txt", 
        "^xffff00HOW TO CUSTOMISE THE MOD^! (read this after doing ^x00ff00green^!)"
    )

    create_rect("XAR_SOLID_BORING_CONCRETE_YELLOW_X", 14, 6, 3, 14, 6, 9)
    create_rect("XAR_SOLID_BORING_CONCRETE_YELLOW_X", 14, 8, 3, 14, 8, 9)

    add_bent_s(14, 7, 9, "bent_base_txt", 
        "This assumes you have already tested the ^xffff00World Globe^! works once. " ..
        "If you haven't already, follow the ^x00ff00green^! instructions.\n\n" ..
        "To customise the mod, there are 2 files you can change: " ..
        "^xffffffblocks.txt^! and ^xffffffsettings.txt^!.\n\n" ..
        "Begin by renaming ^xffffffsettings.txt^! to ^xffffffsettings_old.txt^! " ..
        "(or, for example, to ^xffff00settings_mansion.txt^! if you chose that previously)."
    )

    add_bent_s(14, 7, 7, "bent_base_txt", 
        "Next, close and reopen ^xffffffFBW^! and come back to this message.\n\n" ..
        "Reopen ^xffffffFBWMine^!. It should now ask you to select your " ..
        "Minecraft world's ^xffffffregion^! folder. This is a folder which " ..
        "contains an entire dimension in a Minecraft world. You can lookup where " ..
        "it is in a save file, but for the most recent version it is at\n" ..
        "^xffffffdimensions\\minecraft\\<dimension name>\\region^!.\n\n" ..
        "^xff0000The world should be 1.13+.^!\n\n" ..
        "After this you will enter spawn coordinates."
    )

    add_bent_s(14, 7, 5, "bent_base_txt", 
        "By the way, in general you should not start ^xffffffFBWMine^! while you are playing " ..
        "on an ^xffffffFBW^! save which uses this mod. Instead, wait for the main " ..
        "menu to appear THEN open ^xffffffFBWMine^! THEN start playing your save " ..
        "(what you did in this tutorial was a special exception).\n\n" ..
        "^xff0000Also, always remember to close ^xffffffFBW^xff0000 before " ..
        "closing ^xffffffFBWMine, " ..
        "^xff0000which should close automatically. If you get this the wrong way round, " ..
        "^xffffffFBW^xff0000 will ^x00fffffreeze^xff0000.^!"
    )

    add_bent_s(14, 7, 3, "bent_base_txt", 
        "Your world will now be inside the ^xffff00World Globe^!, go check it out!\n\n" ..
        "If you want to change any of the blocks, you can open ^xffffffblocks.txt^! " ..
        "which explains how to do this. There is a list of Minecraft block ids and " ..
        "XAR block ids in ^xffffffid_lists^!.\n\n" ..
        "If you are a modder, I will be making a nice way for this to be used in mods " ..
        "in the future. To try this right now, you can rename ^xffffffANStar_fbwmine^! " ..
        "in ^xffffffsettings.txt^! to your mod name and copy the relevant files across."
    )


    -- red box
    create_rect("XAR_SOLID_BORING_CONCRETE_RED_X", 14, 2, 9, 14, 4, 11)
    set_pos(14, 3, 10, "XAR_EMPTY_BORING")

    add_bent_s(14, 3, 10, "bent_base_txt", 
        "Please let me (^xff0000AntiNeutronicStar^!) know about any bugs / feedback " ..
        "on discord.\n\n\n\n" ..
        string.rep("^xffff00~^!^x00ffff~^!^xff00ff~^!", 16) .. "\n" ..
        "Thanks to ^xff0000thatgujo^! for helping test extensively.\n" ..
        string.rep("^xffff00~^!^x00ffff~^!^xff00ff~^!", 16)
    )

end