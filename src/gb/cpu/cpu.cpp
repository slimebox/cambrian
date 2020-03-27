#include <gb/cpu.h>
#include <gb/memory.h>
#include <gb/ppu.h>
#include <gb/apu.h>
#include <destoer-emu/debug.h>

namespace gameboy
{

void Cpu::init(Memory *m, Ppu *p,Apu *ap, Disass *dis, Debug *debugger, bool use_bios)
{
    mem = m;
    ppu = p;
    disass = dis;
	debug = debugger;
	apu = ap;

	write_log("[INFO] new instance started!");


	is_cgb = mem->rom_cgb_enabled();

	// setup regs to skip the bios
	if(!use_bios)
	{
		// set the cgb initial registers 
		if(is_cgb)
		{
			a = 0x11; f = 0x80; // af = 0x1180;
			b = 0x00; c = 0x00; // bc = 0x0000;
			d = 0xff; e = 0x56; // de = 0xff56;
			h = 0x00; l = 0x0d; // hl = 0x000d;
			sp = 0xfffe;
			pc = 0x0100;
		}

		// dmg
		else 
		{
			// set our initial register state
			a = 0x01; f = 0xb0; // af = 0x01b0
			b = 0x00; c = 0x13; // bc = 0x0013
			d = 0x00; e = 0xd8; // de = 0x00d8
			h = 0x01; l = 0x4d; // hl = 0x014d
			sp = 0xfffe;
			pc = 0x0100;
		}
	}

	// bios all set to zero
	else
	{
		a = 0x00; f = 0x00; // af = 0x0000
		b = 0x00; c = 0x00; // bc = 0x0000
		d = 0x00; e = 0x00; // de = 0x0000
		h = 0x00; l = 0x00; // hl = 0x0000
		sp = 0x0000;
		pc = 0x0000;		
	}


	is_double = false;
    internal_timer = 0;
    
    joypad_state = 0xff;

    // interrupts
	instr_side_effect = instr_state::normal;
    interrupt_enable = false;
    halt_bug = false;

}


void Cpu::step()
{
    exec_instr();

	// handle interrupts
	do_interrupts();

	// now we need to test if an ei or di instruction
	// has just occured if it has step a cpu instr and then 
	// perform the requested operation and set the ime flag	
	handle_instr_effects();
}


// m cycle tick
void Cpu::cycle_tick(int cycles) noexcept
{
	//  convert to t cycles and tick
	cycle_tick_t(cycles * 4);
}





/* 	
so inside t cycle tick i need to impl a scheduler 
probably using a std::list of sorted events (as they have to happen in order) which each has a cycle
counter till the event and we subtract it from each... if it hits zero we tick the event
with the pixel fifo and possibly others we need to constantly push the event
we could do something like constantly resetting the tick period to zero
but that seems kidna jank



	something like  
	for(auto &x: event_list)
	{
		x.cycle_limit -= cycles;
		if(x.cycle_limit <= 0)
		{
			// will re dump the event into the list
			// in a way we cant iter over it
			// possibly by dumping it at the front or probably
			// just in a seperate list that we merge at the end
			x.service_event();
		}

		// regardless of how we reque for effiency while things are off we wont requeue them
		// and we will have to call a manual inserter for the event when they are turned back on
		// this will need to insert the event in a sorted place as for how we will insert several events sorted at a single time
		// .....
		// likely we will just need store to dump them to external storage before we merge them sorted with the main list
		// if its empty or cycle is zero we will just dump the thing at the front

		//event_list.requeue_events();
	}

	// dont know if we should sort the list and just use one counter..
	// we also cant just have a service_event for each event in realitly it would be something more like
	// maybye we can get away with the above if we used std::function
	service_event(x.type);
	where type is some event_enum type and we just switch on the event or something like that


	
	// to start we will only implement it for the timer event
	// as it is probably the most straight forward to schdule for


	

*/



// t cycle tick
void Cpu::cycle_tick_t(int cycles) noexcept
{
	// should operate at double speed
	const int factor = is_double ? 2 : 1;

	// timers act at constant speed
	update_timers(cycles); 

	// handler will check if its enabled
	mem->tick_dma(cycles * factor);
	
	// in double speed mode gfx and apu should operate at half
	ppu->update_graphics(cycles / factor); // handle the lcd emulation
	apu->tick(cycles / factor); // advance the apu state	
}

void Cpu::tima_inc() noexcept
{
	// timer about to overflow
	if(mem->io[IO_TIMA] == 255)
	{	
		mem->io[IO_TIMA] = mem->io[IO_TMA]; // reset to value in tma
		request_interrupt(2); // timer overflow interrupt			
	}
			
	else
	{
		mem->io[IO_TIMA]++;
	}	
}

bool Cpu::internal_tima_bit_set() const noexcept
{
	const uint8_t freq = mem->io[IO_TMC] & 0x3;

	const int bit = freq_arr[freq];

	return is_set(internal_timer,bit);
}

bool Cpu::tima_enabled() const noexcept
{
	return is_set(mem->io[IO_TMC],2);	
}

void Cpu::update_timers(int cycles) noexcept
{
	
	const int sound_bit = is_double? 13 : 12;

	const bool sound_bit_set = is_set(internal_timer,sound_bit);
	

	// if our bit is deset and it was before (falling edge)
	// and the timer is enabled of course
	if(tima_enabled())
	{
		bool bit_set = internal_tima_bit_set();

		internal_timer += cycles;

		if(!internal_tima_bit_set() && bit_set)
		{
			tima_inc();
		}
        
		// we repeat this here because we have to add the cycles
		// at the proper time for the falling edge det to work
		// and we dont want to waste time handling the falling edge
		// for the timer when its off
		if(!is_set(internal_timer,sound_bit) && sound_bit_set)
		{
			apu->advance_sequencer(); // advance the sequencer
		}
        
	}

    
	// falling edge for sound clk which allways ticks
	// done when timer is off 
	// (cycles should only ever be added once per function call)
	else 
	{
		internal_timer += cycles;
		if(!is_set(internal_timer,sound_bit) && sound_bit_set)
		{
			apu->advance_sequencer(); // advance the sequencer
		}
	} 
       
}



// handle the side affects of instructions
void Cpu::handle_instr_effects()
{
	switch(instr_side_effect)
	{
		// no instr side effects
		case instr_state::normal:
		{
			break;
		}

		case instr_state::ei: // ei
		{
			instr_side_effect = instr_state::normal;
			exec_instr(); 
			// we have done an instruction now set ime
			// needs to be just after the instruction service
			// but before we service interrupts
			
			if(instr_side_effect != instr_state::di) // if we have just executed di do not renable interrupts
			{	
				interrupt_enable = true;
			}
					
			do_interrupts(); // handle interrupts 
			break;
		}
				
		case instr_state::di:  // di
		{
			instr_side_effect = instr_state::normal;
			interrupt_enable = false; // di should disable immediately unlike ei!
			break;
		}
			
		// this will make the cpu stop executing instr
		// until an interrupt occurs and wakes it up 			
		case instr_state::halt: // halt occured in prev instruction
		{		
			instr_side_effect = instr_state::normal;
			uint8_t req = mem->io[IO_IF]; // requested ints 
			uint8_t enabled = mem->io[IO_IE]; // enabled interrutps
			
			// halt bug
			// halt state not entered and the pc fails to increment for
			// one instruction read 			
			if( (interrupt_enable == false) &&  (req & enabled & 0x1f) != 0)
			{
				halt_bug = true;
				return;
			}
			

			// sanity check to check if this thing will actually fire
			const uint8_t stat = mem->io[IO_STAT];
			if(enabled == 0 || ((((stat >> 3) & 0x7) == 0) && enabled == val_bit(enabled,1)))
			{
				write_log("[ERROR] halt infinite loop");
				throw std::runtime_error("halt infinite loop");
			}

			
			while( ( req & enabled & 0x1f) == 0) 
			{
				// just tick it
				cycle_tick(1);
					
				req = mem->io[IO_IF];
			}


			/*
			// ideally we should just figure out how many cycles to the next interrupt

			// halt is immediatly over we are done
			if(req & enabled & 0x1f)
			{
				do_interrupts();
				return;
			}

			int cycles_to_event;

			// check if timer interrupt enabled (and the timer is enabled) if it is
			// determine when it will fire
			if(is_set(mem->io[IO_TMC],2) && is_set(enabled,2))
			{

			}

			// determine when next stat inerrupt will fire
			// because of the irq stat glitches if its on we have to figure out when it first goes off
			// and then re run the check additonally if our target ends in hblank we need to step it manually
			// as pixel transfer -> hblank takes a variable number of cycles
			// (allthough try to come up with a method to actually calculate it based on number of sprites scx etc)


			// whichever interrupt hits first tick until it :)
			*/

			
			do_interrupts(); // handle interrupts
			break;
		}
	}
}


// interrupts
// needs accuracy improvement with the precise interrupt 
// timings to pass ie_push

void Cpu::request_interrupt(int interrupt) noexcept
{
	// set the interrupt flag to signal
	// an interrupt request
	mem->io[IO_IF] = set_bit( mem->io[IO_IF],interrupt);
}


void Cpu::do_interrupts() noexcept
{

	// if not interrupts fired or not enabled return
	if(!(mem->io[IO_IF] & mem->io[IO_IE] & 0x1f) || !interrupt_enable)
	{
		return;
	}


	// interrupt has fired disable ime
	interrupt_enable = false;

	// some internal work is done in each of these cycles
	cycle_tick(2);
	

	write_stackwt(pc); // save pc

	cycle_tick(1);


	// priority for servicing starts at interrupt 0
	// figure out what interrupt has fired
	// if any at this point
	for(int i = 0; i < 5; i++)
	{
		// if requested & is enabled
		if(is_set(mem->io[IO_IF],i) && is_set(mem->io[IO_IE],i))
		{
			mem->io[IO_IF] = deset_bit(mem->io[IO_IF],i); // mark interrupt as serviced
			pc = 0x40 + (i * 8); // set pc to interrupt vector
			return;
		}
	}
}





// util for opcodes

// register getters and setters
void Cpu::write_bc(uint16_t data) noexcept
{
    b = (data & 0xff00) >> 8;
    c = data & 0x00ff;
}


uint16_t Cpu::read_bc(void) const noexcept
{
    return (b << 8) | c;
}


uint16_t Cpu::read_af(void) const noexcept
{
    return (a << 8) | f;
}



void Cpu::write_af(uint16_t v) noexcept 
{
    a = (v & 0xff00) >> 8;
    f = v & 0x00f0; // only top four bits of flag is active
}

uint16_t Cpu::read_de(void) const noexcept 
{
    return (d << 8) | e;
}


void Cpu::write_de(uint16_t v) noexcept
{
    d = (v & 0xff00) >> 8;
    e = v & 0x00ff;
}


uint16_t Cpu::read_hl(void) const noexcept
{
    return (h << 8) | l;
}


void Cpu::write_hl(uint16_t v) noexcept
{
    h = (v & 0xff00) >> 8;
    l = v & 0x00ff;
}


void Cpu::write_stackt(uint8_t v) noexcept
{
	mem->write_memt(--sp,v); // write to stack
}

void Cpu::write_stackwt(uint16_t v) noexcept
{
	write_stackt((v & 0xff00) >> 8);
	write_stackt((v & 0x00ff));
}

// unticked only used by interrupts
void Cpu::write_stack(uint8_t v) noexcept
{
	mem->write_mem(--sp,v); // write to stack
}

void Cpu::write_stackw(uint16_t v) noexcept
{
	write_stack((v & 0xff00) >> 8);
	write_stack((v & 0x00ff));
}

uint8_t Cpu::read_stackt() noexcept
{	
	return mem->read_memt(sp++);
}

uint16_t Cpu::read_stackwt() noexcept
{
	return read_stackt() | (read_stackt() << 8);
}

}