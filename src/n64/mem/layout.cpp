

namespace nintendo64
{

    std::string get_memory_range_name(u32 addr) {
        if (addr < 0x00800000) return "RDRAM";
        if (addr < 0x03F00000) return "UNUSED";
        if (addr < 0x04000000) return "RDRAM Registers";
        if (addr < 0x04001000) return "Signal Processor DMEM";
        if (addr < 0x04002000) return "Signal Processor IMEM";
        if (addr < 0x04040000) return "UNUSED";
        if (addr < 0x05000000) {
            switch ((addr >> 20) & 0xF) {
                case 0: return "Signal Processor Register";
                case 1: case 2: return "Display Processor Register";
                case 3: return "MIPS Interface Register";
                case 4: return "Video Interface Register";
                case 5: return "Audio Interface Register";
                case 6: return "Peripheral Interface Register";
                case 7: return "RDRAM Interface Register";
                case 8: return "Serial Interface Register";
            }
        }
        if (addr < 0x08000000) return "N64 Disk Drive Register";
        if (addr < 0x10000000) return "Static RAM";
        if (addr < 0x1FC00000) return "Cartridge ROM";
        if (addr < 0x1FC007C0) return "PIF ROM";
        if (addr < 0x1FC00800) return "PIF RAM";
        if (addr >= 0x1FD00000 && addr <= 0x7FFFFFFF) return "Cartridge Domain3";
        else return "UNUSED";

    }

template<typename access_type>
access_type read_physical(N64 &n64, u32 addr)
{
    if (!nintendo64::quiet_skip)
        spdlog::trace("MR 0x{:x} at {} memory", addr, get_memory_range_name(addr));

    // just do something naive for now so we can get roms running
    if(addr < 0x00800000)
    {
        return handle_read_n64<access_type>(n64.mem.rd_ram,addr);
    }

    // UNUSED
    else if(addr < 0x03F00000)
    {
        return 0;
    }

    else if(addr < 0x04000000)
    {
        return read_rdram_regs(n64,addr);
    }

    else if(addr < 0x04001000)
    {
        return handle_read_n64<access_type>(n64.mem.sp_dmem,addr & 0xfff);
    }

    else if(addr < 0x04002000)
    {
        return handle_read_n64<access_type>(n64.mem.sp_imem,addr & 0xfff);
    }

    // UNUSED
    else if(addr < 0x04040000)
    {
        return 0;
    }


    else if(addr < 0x0500'0000)
    {
        const u32 idx = (addr >> 20) & 0xf;
        
        switch(idx)
        {
            case 0: return read_sp_regs(n64,addr);
            case 1:
            case 2: return read_dp_regs(n64, addr);
            case 3: return read_mi(n64,addr); 
            case 4: return read_vi(n64,addr); 
            case 5: return read_ai(n64,addr);
            case 6: return read_pi(n64,addr); 
            case 7: return read_ri(n64,addr);
            case 8: return read_si(n64,addr);

            default: return 0;
        }
    }

    // n64dd
    else if(addr < 0x08000000)
    {
        // return as not present for now
        return access_type(0xffffffff);
    }
  
    // sram
    else if(addr < 0x10000000)
    {
        unimplemented("sram");
        return 0;
    }

    // rom
    else if(addr < 0x1FC00000)
    {
        const auto rom_addr = addr & (n64.mem.rom.size() - 1);
        return handle_read_n64<access_type>(n64.mem.rom, rom_addr);
    }

    // currently we dont emulate the pif rom
    // and it cant be read after bootup anyways
    else if(addr < 0x1FC0'07C0)
    {
        //unimplemented("pif boot rom read");
        return 0;
    }

    // pif ram (ignore for now but need to emulate later)
    else if(addr < 0x1FC0'0800)
    {
        return read_pif<access_type>(n64,addr);
    }

    else if(addr >= 0x1FD00000 && addr <= 0x7FFFFFFF)
    {
        unimplemented("cart domain 3");
        return 0;
    }

    else
    {
        printf("warning unused read at %08x\n",addr);
        return access_type(0xffff'ffff);
    }  
}


template<typename access_type>
void write_physical(N64 &n64, u32 addr, access_type v)
{
    if (!nintendo64::quiet_skip)
        spdlog::trace("MW 0x{:x}->0x{:x} at {} memory", v, addr, get_memory_range_name(addr));

    // just do something naive for now so we can get roms running
    if(addr < 0x0080'0000)
    {
        handle_write_n64<access_type>(n64.mem.rd_ram,addr,v);
    }

    // UNUSED
    else if(addr < 0x03F0'0000)
    {
    }

    else if(addr < 0x0400'0000)
    {
        write_rdram_regs(n64,addr,v);
    }

    else if(addr < 0x0400'1000)
    {
        handle_write_n64<access_type>(n64.mem.sp_dmem,addr & 0xfff,v);
    }

    else if(addr < 0x0400'2000)
    {
        handle_write_n64<access_type>(n64.mem.sp_imem,addr & 0xfff,v);
    }

    // UNUSED
    else if(addr < 0x0404'0000)
    {

    }

    else if(addr < 0x0500'0000)
    {
        const u32 idx = (addr >> 20) & 0xf;
        
        switch(idx)
        {
            case 0: write_sp_regs(n64,addr,v); break;
            case 1:
            case 2: write_dp_regs(n64,addr,v); break;
            case 3: write_mi(n64,addr,v); break;
            case 4: write_vi(n64,addr,v); break;
            case 5: write_ai(n64,addr,v); break;
            case 6: write_pi(n64,addr,v); break;
            case 7: write_ri(n64,addr,v); break;
            case 8: write_si(n64,addr,v); break;

            default: break;
        }
    }


    // n64dd
    else if(addr < 0x08000000)
    {
       // not present for now
    }
  
    // sram
    else if(addr < 0x10000000)
    {
        unimplemented("sram");
    }

    // rom
    else if(addr < 0x1FC00000)
    {
        // does this do anything?
        unimplemented("rom write");
    }

    // should this be ignored?
    else if(addr < 0x1FC007C0)
    {
        //unimplemented("pif boot rom write");
    }

    // pif ram (ignore for now but need to emulate later)
    else if(addr < 0x1FC00800)
    {
        write_pif<access_type>(n64,addr,v);
    }

    else if(addr >= 0x1FD00000 && addr <= 0x7FFFFFFF)
    {
        unimplemented("cart domain 3");
    }

    else
    {
        spdlog::warn("Unused write at %08x\n",addr);
    }   
}

}