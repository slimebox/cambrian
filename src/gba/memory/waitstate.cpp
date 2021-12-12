#include <gba/gba.h>

namespace gameboyadvance
{

template u32 Mem::get_waitstates<u32>(u32 addr,bool seq) const;
template u32 Mem::get_waitstates<u16>(u32 addr,bool seq) const;
template u32 Mem::get_waitstates<u8>(u32 addr,bool seq) const;

// TODO: this is an approximation i think the real hardware
// relies on what instrs were executed
void Mem::update_seq(u32 addr)
{
    sequential = addr <= last_addr + sizeof(u32);
    last_addr = addr;
}


u32 Mem::get_rom_wait(u32 region, u32 size, bool seq) const
{
    // for now fudge the numbers
    // TODO: impl the prefetch buffer properly
    if(mem_io.wait_cnt.prefetch)
    {
        return 1;
    }

    else
    {
        return rom_wait_states[(region - 8) / 2][seq][size >> 1];
    }
}

// we also need to a test refactor using one lib, constants and compiling in one dir
// and impl a basic timing test
// this is completly botched...


void set_wait_seq(int *buf, int wait)
{
    // waitstate + N OR S
    buf[0] =  wait + 1;
    buf[1] =  wait + 1;

    // waitstate times two as there are two u16 reads
    buf[2] =  (wait * 2) + 1;
}

void set_wait_nseq(int *buf, int wait, int wait_seq)
{
    buf[0] =  wait + 1;
    buf[1] =  wait + 1;    

    // 2nd of the 16 bit fetches is allways seq
    buf[2] = wait + wait_seq + 1;
}



void Mem::update_wait_states()
{
    static constexpr u32 wait_first_table[] = {4,3,2,8};
    const auto &wait_cnt = mem_io.wait_cnt;

    const auto wait_first0 = wait_first_table[wait_cnt.wait01];
    const auto wait_second0 = !wait_cnt.wait02? 2 : 1;
    set_wait_nseq(&rom_wait_states[0][0][0],wait_first0,wait_second0);
    set_wait_seq(&rom_wait_states[0][1][0],wait_second0);

    const auto wait_first1 = !wait_first_table[wait_cnt.wait11];
    const auto wait_second1 = wait_cnt.wait12? 1 : 4;
    set_wait_nseq(&rom_wait_states[1][0][0],wait_first1,wait_second1);
    set_wait_seq(&rom_wait_states[1][1][0],wait_second1);


    const auto wait_first2 = wait_first_table[wait_cnt.wait21];
    const auto wait_second2 = wait_cnt.wait22? 1 : 8;
    set_wait_nseq(&rom_wait_states[2][0][0],wait_first2,wait_second2);
    set_wait_seq(&rom_wait_states[2][1][0],wait_second2);
    
    // sram is just normal state can be seen as S
    const auto sram_wait = wait_first_table[wait_cnt.sram_cnt];
    set_wait_seq(&wait_states[static_cast<size_t>(memory_region::cart_backup)][0],sram_wait);

#ifdef FETCH_SPEEDHACK

    // settings have changed recache waitstates
    cache_wait_states(cpu.pc_actual);
#endif
}



template<typename access_type>
u32 Mem::get_waitstates(u32 addr, bool seq) const
{
    static_assert(sizeof(access_type) <= 4);

    // need to re pull the region incase dma triggered reads
    const int region =  (addr >> 24) & 0xf; 
    const auto mem_region = memory_region_table[region];
    
    switch(mem_region)
    {
        case memory_region::undefined:
        {
            // how long should this take?
            return 1;
        }

        // TODO: emulate N and S cycles
        case memory_region::rom:
        {
            // hardcode to sequential access!
            return get_rom_wait(region,sizeof(access_type),seq);
        }

        // should unmapped addresses still tick a cycle?
        default:
        {
            // access type >> 1 to get the value
            // 4 -> 2 (word)
            // 2 -> 1 (half)
            // 1 -> 0 (byte)
            return wait_states[region][sizeof(access_type) >> 1];
        }
    }
}


void Mem::cache_wait_states(u32 new_pc)
{
    wait_seq_16 = get_waitstates<u16>(new_pc,true);
    wait_seq_32 = get_waitstates<u32>(new_pc,true);

    wait_nseq_16 = get_waitstates<u16>(new_pc,false);
    wait_nseq_32 = get_waitstates<u32>(new_pc,false);
}



}