namespace nintendo64
{
    void write_dp_regs(N64& n64, u64 addr ,u32 v)
    {
        auto& dp = n64.mem.dp_regs;

        switch(addr)
        {
            case DP_START: dp.rdp_start = v & 0xFFF; break;
            case DP_END: dp.rdp_end = v & 0xFFF; break;
            case DP_STATUS: {
                if (is_set(v, 0)) dp.status.dma = 0;
                if (is_set(v, 1)) dp.status.dma = 1;

                if (is_set(v, 2)) dp.status.freeze = 0;
                if (is_set(v, 3)) dp.status.freeze = 1;

                if (is_set(v, 4)) dp.status.flush = 0;
                if (is_set(v, 5)) dp.status.flush = 1;

                if (is_set(v, 6)) dp.tmem = 0;
                if (is_set(v, 7)) dp.pipebusy = 0;
                if (is_set(v, 8)) dp.bufbusy = 0;
                if (is_set(v, 9)) dp.clock = 0;
                break;
            }

            case DP_SPAN_TBIST: {
                dp.tbist.check = is_set(v, 0);
                dp.tbist.go = is_set(v, 1);
                dp.tbist.done = is_set(v, 2);
            }

            case DP_SPAN_TEST_MODE: dp.test_mode = is_set(v, 0); break;
            case DP_SPAN_TEST_ADDR: dp.test_addr = v & 0b111111; break;
            case DP_SPAN_TEST_DATA: dp.test_data = v; break;

            case DP_CURRENT:
            case DP_CLOCK:
            case DP_BUFBUSY:
            case DP_PIPEBUSY:
            case DP_TMEM:
                unimplemented("Write to read-only DP registers");

        }

    }

    u32 read_dp_regs(N64& n64, u64 addr)
    {
        auto& dp = n64.mem.dp_regs;

        switch(addr)
        {
            case DP_START: return dp.rdp_start;
            case DP_END: return dp.rdp_end;
            case DP_CURRENT: return dp.rdp_current;
            case DP_STATUS: return dp.status.pack();
            case DP_CLOCK: return dp.clock;
            case DP_BUFBUSY: return dp.bufbusy;
            case DP_PIPEBUSY: return dp.pipebusy;
            case DP_TMEM: return dp.tmem;

            case DP_SPAN_TBIST: return dp.tbist.pack();
            case DP_SPAN_TEST_MODE: return dp.test_mode;
            case DP_SPAN_TEST_ADDR: return dp.test_addr;
            case DP_SPAN_TEST_DATA: return dp.test_data;

            default: unimplemented("DP register read %x", addr);
        }
    }
}