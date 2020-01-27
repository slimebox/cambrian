#pragma once
#include "forward_def.h"
#include <destoer-emu/lib.h>


enum class ppu_mode
{
    oam_search = 2,
    pixel_transfer = 3,
    hblank = 0,
    vblank = 1
};


enum class dmg_colors
{
    white = 1,
    light_gray =  2,
    dark_gray =  3,
    black =  4
};


struct Obj // struct for holding sprites on a scanline
{
    int index = 0;
    int x_pos = 0;
};

class Ppu
{
public:

    void init(Cpu *c,Memory *m);

    std::vector<uint32_t> screen; // 160 by 144

    static constexpr uint32_t X = 160;
    static constexpr uint32_t Y = 144;

    ppu_mode mode = ppu_mode::oam_search;

    int current_line = 0;
    bool new_vblank = false;

    void update_graphics(int cycles);

    void set_scanline_counter(int v)
    {
        scanline_counter = v;
    }

    // cgb 
    void set_bg_pal_idx(uint8_t v);
    void set_sp_pal_idx(uint8_t v);
    void write_sppd(uint8_t v);
    void write_bgpd(uint8_t v);


    uint8_t get_sppd() const;
    uint8_t get_bgpd() const;


    // save states
    void save_state(std::ofstream &fp);
    void load_state(std::ifstream &fp);

private:

    Cpu *cpu;
    Memory *mem;

    bool push_pixel();
    void tick_fetcher(int cycles);
    void draw_scanline(int cycles);
    void tile_fetch();
    dmg_colors get_colour(uint8_t colour_num, uint16_t address);
    void read_sprites();
    bool sprite_fetch();  


    void reset_fetcher();

    // main ppu state
	bool signal = false;
    uint32_t scanline_counter = 0;



    enum class pixel_source
    {
        tile = 0,
        tile_cgbd = 3, // priority over everything when set in tile attr
        sprite_zero = 1,
        sprite_one = 2
    };

    struct Pixel_Obj // pod type to hold pixels in the fifo
    {
        int colour_num = 0;
        pixel_source source = pixel_source::tile;
        int cgb_pal = 0;
        bool scx_a = false;       
    };

	// fetcher
	bool hblank = false;
	int x_cord = 0; // current x cord of the ppu
	Pixel_Obj ppu_fifo[168];
	int pixel_idx = 0;

	uint8_t ppu_cyc = 0; // how far for a tile fetch is
	uint8_t ppu_scyc = 0; // how far along a sprite fetch is
	int pixel_count = 0; // how many pixels are in the fifo
	Pixel_Obj fetcher_tile[8];
	int tile_cord = 0;
	bool tile_ready = false; // is the tile fetch ready to go into the fio 
	Obj objects_priority[10]; // sprites for the current scanline
	int no_sprites = 0; // how many sprites
	bool sprite_drawn = false;
	bool window_start = false;
	bool x_scroll_tick = false;
	int scx_cnt = 0;

    // cgb pal
	uint8_t bg_pal[0x40] = {0xff}; // bg palette data
	uint8_t sp_pal[0x40] = {0xff}; // sprite pallete data 
	int sp_pal_idx = 0;
	int bg_pal_idx = 0; // index into the bg pal (entry takes two bytes)

};

