#include "controller.h"
#include <destoer-emu/destoer-emu.h>
using namespace gameboy;

#ifdef CONTROLLER_SDL

void GbControllerInput::init()
{
    // pick first valid controller
	for(int i = 0; i < SDL_NumJoysticks(); i++)
	{
		if(SDL_IsGameController(i))
		{
			controller = SDL_GameControllerOpen(i);
		}

		if(controller == NULL)
		{
			throw std::runtime_error("could not open controller!");
		}

        else
        {
            controller_connected = true;
            break;  
        }
	}  
}

void GbControllerInput::update(gameboy::GB &gb)
{
    // no controller we dont care
    if(!controller_connected)
    {
        return;
    }

    // get game controller update
    SDL_GameControllerUpdate();

    static constexpr SDL_GameControllerButton controller_buttons[] = 
    {
        SDL_CONTROLLER_BUTTON_A,
        SDL_CONTROLLER_BUTTON_X,
        SDL_CONTROLLER_BUTTON_START,
        SDL_CONTROLLER_BUTTON_BACK,
        SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
	    SDL_CONTROLLER_BUTTON_RIGHTSHOULDER 
    };

    static constexpr emu_key gb_key[] = 
    {
        emu_key::a,
        emu_key::s,
        emu_key::enter,
        emu_key::space,
        emu_key::minus,
        emu_key::plus
    };
    static_assert(sizeof(controller_buttons) == sizeof(gb_key));


    static constexpr int CONTROLLER_BUTTONS_SIZE = sizeof(controller_buttons) / sizeof(controller_buttons[0]);

    // cache the old state so we know when a new key press has occured
    static bool buttons_prev[CONTROLLER_BUTTONS_SIZE] = {false};

    // check the controller button state
    for(int i = 0; i < CONTROLLER_BUTTONS_SIZE; i++)
    {
        // now we handle controller inputs
        auto b = SDL_GameControllerGetButton(controller,controller_buttons[i]);
        if(b && !buttons_prev[i])
        {
            gb.key_input(static_cast<int>(gb_key[i]),true);
        }

        else if(!b && buttons_prev[i])
        {
            gb.key_input(static_cast<int>(gb_key[i]),false);
        }
        buttons_prev[i] = b;
    }


    // handle the joystick
    const auto x = SDL_GameControllerGetAxis(controller,SDL_CONTROLLER_AXIS_LEFTX);
    const auto y = SDL_GameControllerGetAxis(controller,SDL_CONTROLLER_AXIS_LEFTY);

    // input of more than half in either direction is enough to make
    // to cause an input
    constexpr int16_t threshold = std::numeric_limits<int16_t>::max() / 2;



    // in x axis deadzone deset both
    if(x == threshold)
    {
        gb.key_input(static_cast<int>(emu_key::down),false);
        gb.key_input(static_cast<int>(emu_key::up),false);
    }

    // in y axis deadzone deset both
    if(y == threshold)
    {
        gb.key_input(static_cast<int>(emu_key::left),false);
        gb.key_input(static_cast<int>(emu_key::right),false);
    }


    // right
    gb.key_input(static_cast<int>(emu_key::right),x > threshold);

    // left
    gb.key_input(static_cast<int>(emu_key::left),x < -threshold);

    // up
    gb.key_input(static_cast<int>(emu_key::up),y < -threshold);    

    // down
    gb.key_input(static_cast<int>(emu_key::down),y > threshold);    

}

GbControllerInput::~GbControllerInput()
{
    if(controller_connected && controller != NULL)
    {
        // this is apparently buggy...
        //SDL_GameControllerClose(controller);
        controller = NULL;
    }
}

#endif