#pragma once
#include <gb/forward_def.h>
#include <destoer-emu/lib.h>
#include <destoer-emu/debug.h>
#include <gb/scheduler.h>

namespace gameboy
{

// did the last instr have a side effect
// that happens after the instr has executed
enum class instr_state
{
    normal,
    halt,
    ei, // enable interrupt
    di  // disable interrupt
};


class Cpu
{
public:
    Cpu(GB &gb);

    bool get_double() const;

    void insert_new_timer_event() noexcept;
    int get_next_timer_event() const noexcept;

    void update_intr_req() noexcept;
    void update_intr_fire() noexcept;


    // timer
    void update_timers(uint32_t cycles) noexcept;

    void init(bool use_bios = false);
    void step();
    void tick_pending_cycles() noexcept;
    void cycle_tick(uint32_t cycles) noexcept; 
    void cycle_tick_t(uint32_t cycles) noexcept;
    void cycle_delay(uint32_t cycles) noexcept;
    void tima_inc() noexcept;
    bool internal_tima_bit_set() const noexcept;
    bool tima_enabled() const noexcept;
    void tima_reload() noexcept;

    void request_interrupt(int interrupt) noexcept;
    bool get_cgb() const noexcept {return is_cgb;}


    // oam bug
    void oam_bug_write(uint16_t v);
    void oam_bug_read(uint16_t v);
    void oam_bug_read_increment(uint16_t v);


    uint16_t internal_timer = 0;
    
    uint8_t joypad_state = 0xff;

    // freq bits for internal timer
    static constexpr int freq_arr[4] = {9,3,5,7};


    // serial
    int serial_cyc;
    int serial_cnt;

    void tick_serial(int cycles) noexcept;
    void insert_new_serial_event() noexcept;

    /* register reads for the debugger */

    uint16_t get_pc() const noexcept { return pc; }
    uint16_t get_sp() const noexcept { return sp; }
    uint16_t get_hl() const noexcept { return read_hl(); }
    uint16_t get_af() const noexcept { return read_af(); }
    uint16_t get_bc() const noexcept { return read_bc(); }
    uint16_t get_de() const noexcept { return read_de(); }
    uint8_t get_h() const noexcept { return h; }
    uint8_t get_l() const noexcept { return l; }
    uint8_t get_a() const noexcept { return a; }
    uint8_t get_f() const noexcept 
    { 
        return carry << C | half << H
		| zero << Z | negative << N; 
    }
    uint8_t get_b() const noexcept { return b; }
    uint8_t get_c() const noexcept { return c; }
    uint8_t get_d() const noexcept { return d; }
    uint8_t get_e() const noexcept { return e ;}

    // save states
    void save_state(std::ofstream &fp);
    void load_state(std::ifstream &fp);

#ifdef DEBUG
    void change_breakpoint_enable(bool enabled) noexcept
    {
        if(enabled)
        {
            exec_instr_fptr = &Cpu::exec_instr_debug;         
        }

        else
        {
            exec_instr_fptr = &Cpu::exec_instr_no_debug; 
        }
    }
#endif

private:

    Memory &mem;
    Apu &apu;
    Ppu &ppu;
    GameboyScheduler &scheduler;
    Debug &debug;
    Disass &disass;

    // registers
    uint8_t a; /*uint8_t f*/; //af
    uint8_t b; uint8_t c; //bc
    uint8_t d; uint8_t e; //de
    uint8_t h; uint8_t l; //hl
    uint16_t sp;
    uint16_t pc;


    static constexpr uint32_t Z = 7; // zero flag
    static constexpr uint32_t N = 6; // negative flag
    static constexpr uint32_t H = 5; // half carry flag
    static constexpr uint32_t C = 4; // carry flag


    bool zero;
    bool negative;
    bool half;
    bool carry;


    // interrupts
    instr_state instr_side_effect = instr_state::normal;
    bool interrupt_enable = false;
    bool interrupt_req = false;
    bool interrupt_fire = false;
    bool halt_bug = false;

    uint32_t pending_cycles = 0;

    // next opcode
    uint8_t opcode = 0;

    // cgb
    bool is_cgb = false;
    bool is_double = false;

    void write_af(uint16_t data) noexcept;
    void write_bc(uint16_t data) noexcept;
    void write_de(uint16_t data) noexcept;
    void write_hl(uint16_t data) noexcept;

    uint16_t read_af() const noexcept;
    uint16_t read_de() const noexcept;
    uint16_t read_bc() const noexcept;
    uint16_t read_hl() const noexcept;

#ifdef DEBUG
    using EXEC_INSTR_FPTR = void (Cpu::*)(void);

    EXEC_INSTR_FPTR exec_instr_fptr;

    inline void exec_instr()
    {
        std::invoke(exec_instr_fptr,this);
    }

    void exec_instr_debug();

#else 

    inline void exec_instr()
    {
        exec_instr_no_debug();
    }

#endif

    void exec_instr_no_debug();
    

    void exec_cb(uint8_t cbop);
    void handle_halt();

    void switch_double_speed() noexcept;

    uint8_t fetch_opcode() noexcept;

    // interrupts
    void do_interrupts() noexcept;

    // instr helpers
    void set_zero(uint8_t reg) noexcept;
    void instr_dec(uint8_t reg) noexcept;
    void instr_inc(uint8_t reg) noexcept;
    void instr_bit(uint8_t reg, uint8_t bit) noexcept;
    void instr_and(uint8_t num) noexcept;
    uint8_t instr_rl(uint8_t reg) noexcept;
    void instr_sub(uint8_t num) noexcept;
    void instr_cp(uint8_t num) noexcept;
    void instr_sbc(uint8_t num) noexcept;
    void instr_add(uint8_t num) noexcept;
    uint16_t instr_addi(uint16_t reg, int8_t num) noexcept;
    void instr_adc(uint8_t num) noexcept;
    uint16_t instr_addw(uint16_t reg, uint16_t num) noexcept;
    void instr_or(uint8_t val) noexcept;
    uint8_t instr_swap(uint8_t num) noexcept;
    void instr_xor(uint8_t num) noexcept;
    uint8_t instr_sla(uint8_t reg) noexcept;
    uint8_t instr_sra(uint8_t reg) noexcept;
    uint8_t instr_srl(uint8_t reg) noexcept;
    uint8_t instr_rr(uint8_t reg) noexcept;
    uint8_t instr_rrc(uint8_t reg) noexcept;
    uint8_t instr_rlc(uint8_t reg) noexcept;
    void instr_jr() noexcept;
    void instr_jr_cond(bool cond, bool flag) noexcept;
    void instr_jp_cond(bool cond, bool flag) noexcept;
    void call_cond(bool cond, bool flag) noexcept;
    void ret_cond(bool cond, bool falg) noexcept;
    uint16_t instr_incw(uint16_t v) noexcept;
    uint16_t instr_decw(uint16_t v) noexcept;

    // stack helpers
    uint8_t read_stackt() noexcept;
    void write_stackt(uint8_t v) noexcept;
    uint16_t read_stackwt() noexcept;
    void write_stackwt(uint16_t v) noexcept;
    void write_stackw(uint16_t v) noexcept;
    void write_stack(uint8_t v) noexcept;

    void check_rst_loop(uint16_t addr, uint8_t op);


    // oam bug
    uint32_t get_cur_oam_row() const;
    bool oam_should_corrupt(uint16_t v) const noexcept;
};
}