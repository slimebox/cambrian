
namespace nintendo64
{

// NOTE: all intr handling goes here

static constexpr u32 COUNT_BIT = 7;
static constexpr u32 MI_BIT = 2;

void check_interrupts(N64 &n64)
{
    auto& cop0 = n64.cpu.cop0;
    auto& status = cop0.status;
    auto& cause = cop0.cause;

    // enable off or execption being serviced
    // no interrupts on
    if(!status.ie || cop0.status.erl || cop0.status.exl)
    {
        return;
    }

    if(status.im & cause.pending)
    {
        n64.cpu.interrupt = true;
        assert(false);
    }
}


void insert_count_event(N64 &n64)
{
    u64 cycles;

    auto& cop0 = n64.cpu.cop0;

    const u32 count = cop0.count;
    const u32 compare = cop0.compare;

    
    if(count < compare)
    {
        cycles = (compare - count) * 2;
    }

    else
    {
        cycles = ((compare - count) + 0xffffffff) * 2; 
    }

    const auto event = n64.scheduler.create_event(cycles,n64_event::count);
    n64.scheduler.insert(event); 
}


void set_intr_cop0(N64& n64, u32 bit)
{
    auto& cause = n64.cpu.cop0.cause;

    cause.pending = set_bit(cause.pending,bit);
    check_interrupts(n64);
}

void deset_intr_cop0(N64& n64, u32 bit)
{
    auto& cause = n64.cpu.cop0.cause;

    cause.pending = deset_bit(cause.pending,bit);
}

// NOTE: do this relative to your storage,
// start tomorrow you are too tired
void count_intr(N64 &n64)
{
    set_intr_cop0(n64,COUNT_BIT);
    insert_count_event(n64);
}

void mi_intr(N64& n64)
{
    set_intr_cop0(n64,MI_BIT);
}


void write_cop0(N64 &n64, u64 v, u32 reg)
{
    auto &cpu = n64.cpu;
    auto &cop0 = cpu.cop0;

    switch(reg)
    {
        // cycle counter (incremented every other cycle)
        case COUNT:
        {
            cop0.count = v;
            insert_count_event(n64);
            break;
        }

        // when this is == count trigger in interrupt
        case COMPARE:
        {
            cop0.compare = v;
            insert_count_event(n64);

            // ip7 in cause is reset when this is written
            deset_intr_cop0(n64,COUNT_BIT);
            break;
        }
        
        case TAGLO: // for cache, ignore for now
        {
            cop0.tag_lo = v;
            break;
        }

        case TAGHI: // for cache, ignore for now
        {
            cop0.tag_hi = v;
            break;
        }

        // various cpu settings
        case STATUS:
        {
            auto& status = cop0.status;

            status.ie = is_set(v,0);
            status.exl = is_set(v,1);
            status.erl = is_set(v,2);
            status.ksu = (v >> 3) & 0b11;

            status.ux = is_set(v,5);
            status.sx = is_set(v,6);
            status.kx = is_set(v,7);

            status.im = (v >> 8) & 0xff;

            status.ds = (v >> 16) & 0x1ff;

            status.re = is_set(v,25);
            status.fr = is_set(v,26);
            status.rp = is_set(v,27);

            status.cu1 = is_set(v,29);


            if(status.rp)
            {
                unimplemented("low power mode");
            }

            if(status.re)
            {
                unimplemented("little endian");
            }

            if((status.ux && status.ksu == 0b10) || (status.sx && status.ksu == 0b01) || (status.kx && status.ksu == 0b00))
            {
                unimplemented("64 bit addressing");
            }

            break;
        }

        // interrupt / exception info
        case CAUSE:
        {
            auto& cause = cop0.cause;

            const auto PENDING_MASK = 0b11 << 8;

            // write can only modify lower 2 bits of interrupt pending
            cause.pending = (cause.pending & ~PENDING_MASK)  | (v & PENDING_MASK);
            break;
        }

        // TODO: what is this used for?
        case PRID:
        {
            cop0.prid = v;
            break;
        }

        case CONFIG:
        {
            cop0.config = v;
            break;
        }

        // read only
        case RANDOM: break;

        default:
        {
            printf("unimplemented cop0 write: %s\n",COP0_NAMES[reg]);
            exit(1);
        }
    }
}

u64 read_cop0(N64& n64, u32 reg)
{
    UNUSED(n64);

    auto& cpu = n64.cpu;
    auto& cop0 = cpu.cop0;

    switch(reg)
    {
        case STATUS:
        {
            auto& status = cop0.status;

            return status.ie | (status.exl << 1) | (status.erl << 2) |
                (status.ksu << 3) | (status.ux << 5) | (status.sx << 6) |
                (status.kx << 7) | (status.im << 8) | (status.ds << 8) |
                (status.re << 25) | (status.fr << 26) | (status.rp << 27) |
                (status.cu1 << 29);
        }

        default:
        {
            printf("unimplemented cop0 read: %s\n",COP0_NAMES[reg]);
            exit(1);
        }        
    }
}

}