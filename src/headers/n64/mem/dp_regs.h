namespace nintendo64 {

    struct DpStatus {
        bool dma = false; // write to set RDP commands in RDRAM
        bool freeze = false; // write to set RDP commands in RSP DMEM
        bool flush = false; // Write to clear freeze
        bool startgclk = false; // write to set freeze
        bool tmem_busy = false;// write to clear flush
        bool pipe_busy = false; // write to set flush
        bool command_busy = false; // write to set DpRegs::tmem to 0
        bool command_buffer_busy = false; // writ to set DpRegs::pipebusy to 0
        bool dma_busy = false; // write to set DpRegs::bufusy to 0
        bool end_valid = false; // write to set DpRegs::clock to 0
        bool start_valid = false; // invalid write

        inline u16 pack() {
            return start_valid << 10 | end_valid << 9 | dma_busy << 8 | command_buffer_busy << 7 |
                    command_busy << 6 | pipe_busy << 5 | tmem_busy << 4 | startgclk << 3 |
                    flush << 2 | freeze << 1 | dma;
        }
    };

    struct DpSpanTbist {
        bool check = false; //
        bool go = false;
        bool done = false;

        u8 fail = 0;

        inline u16 pack() {
            return fail << 3 | done << 2 | go << 1 | check;
        }
    };

    struct DpRegs {
        // Command registers
        u64 rdp_start = 0;
        u64 rdp_end = 0;
        u64 rdp_current = 0;

        DpStatus status;

        u64 clock = 0;
        u64 bufbusy = 0;
        u64 pipebusy = 0;
        u64 tmem = 0;

        // Span registers
        DpSpanTbist tbist;
        u64 test_mode = 0;
        u8 test_addr = 0;
        u32 test_data = 0;
    };

}