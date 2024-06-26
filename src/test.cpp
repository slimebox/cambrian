#include <destoer.h>
#include <albion/lib.h>
#ifndef FRONTEND_HEADLESS 

#ifdef GB_ENABLED
#include <gb/gb.h>

// gameboy test running
void gb_run_test_helper(const std::vector<std::string> &tests, int seconds)
{

    int fail = 0;
    int pass = 0;
    int aborted = 0;
    int timeout = 0;

    gameboy::GB gb;

    for(const auto &x: tests)
    {
        
        try
        {
            gb.reset(x);
            gb.apu.playback.stop();
            gb.throttle_emu = false;


            auto start = std::chrono::system_clock::now();

            // add a timer timeout if required
            for(;;)
            {
                gb.run();


                if(gb.mem.test_result == emu_test::fail)
                {
                    std::cout << fmt::format("{}: fail\n",x);
                    fail++;
                    break;
                }

                else if(gb.mem.test_result == emu_test::pass)
                {
                    // we are passing so many compared to fails at this point
                    // it doesnt make sense to print them
                    //std::cout << fmt::format("{}: pass\n",x);
                    pass++;
                    break;
                }

                auto current = std::chrono::system_clock::now();

                // if the test takes longer than 5 seconds time it out
                if(std::chrono::duration_cast<std::chrono::seconds>(current - start).count() > seconds)
                {
                    std::cout << fmt::format("{}: timeout\n",x);
                    timeout++;
                    break;
                }
            }
        }

        catch(std::exception &ex)
        {
            std::cout << fmt::format("{}: aborted {}\n",x,ex.what());
            aborted++;
        }
    }

    printf("total: %zd\n",tests.size());
    printf("pass: %d\n",pass);
    printf("fail: %d\n",fail);
    printf("abort: %d\n",aborted);
    printf("timeout: %d\n",timeout);    
}

void gb_run_tests()
{
    puts("gekkio_tests:");
    auto start = std::chrono::system_clock::now();
    const auto [tree,error] = read_dir_tree("mooneye-gb_hwtests");
    gb_run_test_helper(filter_ext(tree,"gb"),10);
    auto current = std::chrono::system_clock::now();
    auto count = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(current - start).count()) / 1000.0;
    printf("total time taken %f\n",count);
}
#endif

#ifdef N64_ENABLED
#include <n64/n64.h>


#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

b32 read_test_image(const std::string& filename, std::vector<u32>& buf)
{
    int x, y, n;
    unsigned char *data = stbi_load(filename.c_str(), &x, &y, &n, 4);

    if(!data)
    {
        return true;
    }

    buf.resize(x * y);
    memcpy(buf.data(),data,buf.size() * sizeof(u32));
    free(data);

    //printf("read image: %s (%d,%d) %zd\n",filename.c_str(),x,y,buf.size());

    return false;
}


#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

bool write_test_image(const std::string &filename, std::vector<u32> &buf,u32 x, u32 y)
{
    bool error = !stbi_write_png(filename.c_str(),x,y,4,buf.data(),x * sizeof(u32));
    printf("wrote fail image: %s: %d\n",filename.c_str(),error);
    return error;
}

void n64_run_tests()
{
    struct Test
    {
        const char* rom_path;
        const char* image_name;
        const char* name;
        int frames;
    };

    Test TESTS[] = 
    {
        // {"N64/CPUTest/CPU//CPU.N64","N64/CPUTest/CPU//CPU.png","KROM_CPU_",5},
        {"N64/CPUTest/CPU/ADD/CPUADD.N64","N64/CPUTest/CPU/ADD/CPUADD.png","KROM_CPU_ADD",5},
        {"N64/CPUTest/CPU/ADDU/CPUADDU.N64","N64/CPUTest/CPU/ADDU/CPUADDU.png","KROM_CPU_ADDU",5},
        {"N64/CPUTest/CPU/AND/CPUAND.N64","N64/CPUTest/CPU/AND/CPUAND.png","KROM_CPU_AND",5},
        {"N64/CPUTest/CPU/DADDU/CPUDADDU.N64","N64/CPUTest/CPU/DADDU/CPUDADDU.png","KROM_CPU_DADDU",5},
        //{"N64/CPUTest/CPU/DDIV/CPUDDIV.N64","N64/CPUTest/CPU/DDIV/CPUDDIV.png","KROM_CPU_DDIV",5},
        //{"N64/CPUTest/CPU/DDIVU/CPUDDIVU.N64","N64/CPUTest/CPU/DDIVU/CPUDDIVU.png","KROM_CPU_DDIVU",5},
        {"N64/CPUTest/CPU/DIV/CPUDIV.N64","N64/CPUTest/CPU/DIV/CPUDIV.png","KROM_CPU_DIV",5},
        {"N64/CPUTest/CPU/DIVU/CPUDIVU.N64","N64/CPUTest/CPU/DIVU/CPUDIVU.png","KROM_CPU_DIVU",5},
        //{"N64/CPUTest/CPU/DMULT/CPUDMULT.N64","N64/CPUTest/CPU/DMULT/CPUDMULT.png","KROM_CPU_DMULT",5},
        //{"N64/CPUTest/CPU/DMULTU/CPUDMULTU.N64","N64/CPUTest/CPU/DMULTU/CPUDMULTU.png","KROM_CPU_DMULTU",5},
        {"N64/CPUTest/CPU/DSUB/CPUDSUB.N64","N64/CPUTest/CPU/DSUB/CPUDSUB.png","KROM_CPU_DSUB",5},
        //{"N64/CPUTest/CPU/DSUBU/CPUDSUBU.N64","N64/CPUTest/CPU/DSUBU/CPUDSUBU.png","KROM_CPU_DSUBU",5},
    };

    int TEST_SIZE = sizeof(TESTS) / sizeof(Test);


    puts("n64 tests:");
    
    try
    {
        for(int t = 0; t < TEST_SIZE; t++)
        {
            auto& test = TESTS[t];
            printf("start test: %s\n",test.name);

            nintendo64::N64 n64;
            nintendo64::reset(n64,test.rom_path);

            for(int f = 0; f < test.frames; f++) 
            {
                nintendo64::run(n64);
            }

            std::vector<u32> screen_check;

            // Attempt to open the comparison
            const b32 error = read_test_image(test.image_name,screen_check);
            
            // Cannot find file -> auto set the image
            if(error)
            {
                printf("cannot find reference image\n");
                return;
            }

            // Can find file -> compare the images
            else 
            {
                b32 pass = true;

                // compare image and ignore alpha channel
                if(screen_check.size() == n64.rdp.screen.size())
                {
                    for(u32 i = 0; i < screen_check.size(); i++)
                    {
                        const u32 v1 = (screen_check[i] & 0x00ff'ffff);
                        const u32 v2 = (n64.rdp.screen[i] & 0x00ff'ffff);
                        if(v1 != v2)
                        {
                            printf("images differ at: %d, %x != %x\n",i,v1,v2);
                            pass = false;
                            break;
                        }
                    }
                }

                else
                {
                    printf("images differ in size: %zd : %zd\n",screen_check.size(),n64.rdp.screen.size());
                    pass = false;
                }

                printf("%s: %s\n",test.rom_path,pass? "PASS" : "FAIL");

                if(!pass)
                {
                    write_test_image("fail.png",n64.rdp.screen,n64.rdp.screen_x,n64.rdp.screen_y);
                }
            }
        }
    }

    catch(std::exception &ex)
    {
        std::cout << fmt::format("{}: \n",ex.what());
    }
}
#endif

void run_tests()
{
#ifdef GB_ENABLED
    gb_run_tests();    
#endif

#ifdef N64_ENABLED 
    n64_run_tests();
#endif

}

#endif