#include <n64/cpu.h>
namespace nintendo64
{

    // A compile-time replacement for beyond-all-repair's table_gen class.
    // Uses an XMACRO on the mips_table.inl to fill the two tables with the correct function pointers.

    // We need to know what types are memory/class access so that we can add the debug flag to their template.
    // We use templates to determine which types are accesses.
    // False by default.
    template <instr_type type>
    struct is_memory_access : std::false_type { };
    // A little helper so that we don't just copy these two lines over and over.
    #define MEMORY(a) template <> \
    struct is_memory_access<instr_type::a> : std::true_type { };

    MEMORY(store)
    MEMORY(store_float)
    MEMORY(store_cop2)
    MEMORY(load)
    MEMORY(load_float)
    MEMORY(load_cop2)
    // mips_class is defined to be a memory access in table_gen.cpp, so it is here.
    MEMORY(mips_class)

    template <typename instr, typename is_memory_access>
    auto dispatch_instr(...);

    #define INSTR(instr,type,c,d,e,f) \
    template<> auto dispatch_instr<instr, std::true_type>(...) { &instr_##a<true>(...); } \
    template<> auto dispatch_instr<instr, std::false_type>(...) { &instr_##a(...); }

    const INSTR_FUNC INSTR_TABLE_DEBUG[] = {
    #include "mips_table.inl"
    };

    #undef INSTR

    #define INSTR(a,b,c,d,e,f) \
    template<> auto dispatch_instr<a, std::true_type>(...) { &instr_##a(...); } \
    template<> auto dispatch_instr<a, std::false_type>(...) { &instr_##a<false>(...); }

    const INSTR_FUNC INSTR_TABLE_NO_DEBUG[] = {
    #include "mips_table.inl"
    };

    #undef INSTR

}