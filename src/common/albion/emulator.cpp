#include <albion/emulator.h>

// for now we will just be lazy and detect by file ext
emu_type get_emulator_type(std::string filename)
{
	size_t ext_idx = filename.find_last_of("."); 
	if(ext_idx != std::string::npos)
	{
		std::string ext = filename.substr(ext_idx+1);

        for(auto &x: ext)
        {
            x = tolower(x);
        }

        // isx needs a format conversion but its fine
        if(ext == "gbc" || ext == "gb" || ext == "sgb" || ext == "isx")
        {
            return emu_type::gameboy;
        }

        else if(ext == "gba")
        {
            return emu_type::gba;
        }

        else if(ext == "z64" || ext == "n64")
        {
            return emu_type::n64;
        }

        else
        {
            return emu_type::none;
        }

	}

    return emu_type::none;
}

std::string get_emulator_name(emu_type type) {
    switch(type) {
        case emu_type::gameboy: return "Gameboy";
        case emu_type::gba: return "Gameboy Advance";
        case emu_type::n64: return "Nintendo 64";
        case emu_type::none: return "Invalid";
    }

    assert(false);
    return "";
}