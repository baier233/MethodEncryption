//
// Created by Administrator on 2024/3/15.
//

#include "method.h"

#include <iomanip>
#include <ranges>

#include "symbol.h"
#include "bytecode.h"
#include "jvm_breakpoint_info.h"
#include "jvm_break_points.h"
#include "klass.h"

auto java_hotspot::const_method::get_constants() -> const_pool * {
    static VMStructEntry *_constants_entry = JVMWrappers::find_type_fields("ConstMethod").value().get()["_constants"];
    if (!_constants_entry) return nullptr;
    return *reinterpret_cast<const_pool **>(reinterpret_cast<uint8_t *>(this) + _constants_entry->offset);
}

unsigned short java_hotspot::const_method::get_code_size() {
    static VMStructEntry *_code_size_entry = JVMWrappers::find_type_fields("ConstMethod").value().get()["_code_size"];
    if (!_code_size_entry) return 0;
    return *reinterpret_cast<unsigned short *>(reinterpret_cast<uint8_t *>(this) + _code_size_entry->offset);
}

auto java_hotspot::const_method::set_code_size(const unsigned short code_size) -> void {
    static VMStructEntry *_code_size_entry = JVMWrappers::find_type_fields("ConstMethod").value().get()["_code_size"];
    if (!_code_size_entry) return;
    *reinterpret_cast<unsigned short *>(reinterpret_cast<uint8_t *>(this) + _code_size_entry->offset) = code_size;
}

unsigned short java_hotspot::const_method::get_name_index() {
    static VMStructEntry *_name_index_entry = JVMWrappers::find_type_fields("ConstMethod").value().get()["_name_index"];
    if (!_name_index_entry) return 0;
    return *reinterpret_cast<unsigned short *>(reinterpret_cast<uint8_t *>(this) + _name_index_entry->offset);
}

unsigned short java_hotspot::const_method::get_signature_index() {
    static VMStructEntry *_signature_index_entry = JVMWrappers::find_type_fields("ConstMethod").value().get()[
            "_signature_index"];
    if (!_signature_index_entry) return 0;
    return *reinterpret_cast<unsigned short *>(reinterpret_cast<uint8_t *>(this) + _signature_index_entry->offset);
}

auto java_hotspot::const_method::get_const_method_size() -> unsigned short {
    static VMStructEntry *_const_method_size_entry = JVMWrappers::find_type_fields("ConstMethod").value().get()[
            "_const_method_size"];
    if (!_const_method_size_entry) return 0;
    return *reinterpret_cast<unsigned short *>(reinterpret_cast<uint8_t *>(this) + _const_method_size_entry->offset);
}

auto java_hotspot::const_method::get_max_stack() -> unsigned short {
    static VMStructEntry *_max_stack_entry = JVMWrappers::find_type_fields("ConstMethod").value().get()["_max_stack"];
    if (!_max_stack_entry) return 0;
    return *reinterpret_cast<unsigned short *>(reinterpret_cast<uint8_t *>(this) + _max_stack_entry->offset);
}

auto java_hotspot::const_method::set_max_stack(const unsigned short max_stack) -> void {
    static VMStructEntry *_max_stack_entry = JVMWrappers::find_type_fields("ConstMethod").value().get()["_max_stack"];
    if (!_max_stack_entry) return;
    *reinterpret_cast<unsigned short *>(reinterpret_cast<uint8_t *>(this) + _max_stack_entry->offset) = max_stack;
}

auto java_hotspot::const_method::get_max_locals() -> unsigned short {
    static VMStructEntry *_max_locals_entry = JVMWrappers::find_type_fields("ConstMethod").value().get()["_max_locals"];
    if (!_max_locals_entry) return 0;
    return *reinterpret_cast<unsigned short *>(reinterpret_cast<uint8_t *>(this) + _max_locals_entry->offset);
}

auto java_hotspot::const_method::set_max_locals(const unsigned short max_locals) -> void {
    static VMStructEntry *_max_locals_entry = JVMWrappers::find_type_fields("ConstMethod").value().get()["_max_locals"];
    if (!_max_locals_entry) return;
    *reinterpret_cast<unsigned short *>(reinterpret_cast<uint8_t *>(this) + _max_locals_entry->offset) = max_locals;
}

auto java_hotspot::const_method::get_size_of_parameters() -> unsigned short {
    static VMStructEntry *_size_of_parameters_entry = JVMWrappers::find_type_fields("ConstMethod").value().get()[
            "_size_of_parameters"];
    if (!_size_of_parameters_entry) return 0;
    return *reinterpret_cast<unsigned short *>(reinterpret_cast<uint8_t *>(this) + _size_of_parameters_entry->offset);
}

auto java_hotspot::const_method::get_method_idnum() -> unsigned short {
    static VMStructEntry *_method_idnum_entry = JVMWrappers::find_type_fields("ConstMethod").value().get()[
            "_method_idnum"];
    if (!_method_idnum_entry) return 0;
    return *reinterpret_cast<unsigned short *>(reinterpret_cast<uint8_t *>(this) + _method_idnum_entry->offset);
}

auto java_hotspot::const_method::get_bytecode_start() -> uint8_t * {
    return reinterpret_cast<uint8_t *>(reinterpret_cast<uintptr_t>(this) + bytecode_start_offset);
}

auto java_hotspot::const_method::get_bytecode_size() -> int {
    /* This is a more efficient way to get the bytecode size */
    return get_code_size();
    /*uint8_t *start = get_bytecode_start();
    auto bytecode = std::make_unique<java_runtime::bytecode>(start);
    while (bytecode->get_opcode() != java_runtime::bytecodes::invalid) {
        const int length = bytecode->get_length();
        start += length;
        bytecode = std::make_unique<java_runtime::bytecode>(start);
    }
    return static_cast<int>(start - get_bytecode_start());*/
}

auto java_hotspot::const_method::set_bytecode(const std::vector<uint8_t> &bytecode) -> void {
    const int bytecode_size = get_bytecode_size();
    if (bytecode_size < bytecode.size()) {
        std::cerr << "New bytecode is larger than the original" << std::endl;
        return;
    }
    memcpy(get_bytecode_start(), bytecode.data(), bytecode.size());
    /* if the new bytecode is smaller than the original, fill the rest with nops */
    memset(get_bytecode_start() + bytecode.size(), static_cast<uint8_t>(java_runtime::bytecodes::_nop),
           bytecode_size - bytecode.size());
}

auto java_hotspot::const_method::get_bytecode() -> std::vector<uint8_t> {
    const int bytecode_size = get_bytecode_size();
    std::vector<uint8_t> bytecode(bytecode_size);
    memcpy(bytecode.data(), get_bytecode_start(), bytecode_size);
    return bytecode;
}

auto java_hotspot::const_method::get_class_loader_data() -> void * {
    static VMStructEntry *_class_loader_data = JVMWrappers::find_type_fields("ConstMethod").value().get()[
            "_class_loader_data"];
    if (!_class_loader_data) return nullptr;
    return *reinterpret_cast<void **>(reinterpret_cast<uint8_t *>(this) + _class_loader_data->offset);
}

auto java_hotspot::const_method::get_const_method_length() -> size_t {
    static VMTypeEntry *_constMethod_entry = JVMWrappers::find_type("ConstMethod").value();
    if (!_constMethod_entry)return 0;
    return _constMethod_entry->size;
}

auto java_hotspot::method::get_const_method() -> const_method * {
    static VMStructEntry *_constMethod_entry = JVMWrappers::find_type_fields("Method").value().get()["_constMethod"];
    if (!_constMethod_entry) return nullptr;
    return *reinterpret_cast<const_method **>(reinterpret_cast<uint8_t *>(this) + _constMethod_entry->offset);
}

auto java_hotspot::method::get_signature() -> std::string {
    const auto const_method = this->get_const_method();
    const auto signature_index = const_method->get_signature_index();
    const auto const_pool = const_method->get_constants();
    const auto base = reinterpret_cast<symbol **>(const_pool->get_base());
    return base[signature_index]->to_string();
}

auto java_hotspot::method::get_name() -> std::string {
    const auto const_method = this->get_const_method();
    const auto name_index = const_method->get_name_index();
    const auto const_pool = const_method->get_constants();
    const auto base = reinterpret_cast<symbol **>(const_pool->get_base());
    return base[name_index]->to_string();
}

auto java_hotspot::method::get_i2i_entry() -> interpreter_entry * {
    static VMStructEntry *_i2i_entry = JVMWrappers::find_type_fields("Method").value().get()["_i2i_entry"];
    if (!_i2i_entry) return nullptr;
    return *reinterpret_cast<interpreter_entry **>(reinterpret_cast<uint8_t *>(this) + _i2i_entry->offset);
}

auto java_hotspot::method::set_break_point(const uintptr_t offset, const native_callback_t &callback) -> void {
    jvm_break_points::set_breakpoint(this, offset, callback);
}

auto java_hotspot::method::remove_break_point(const uintptr_t offset) -> void {
    jvm_break_points::remove_breakpoint(this, offset);
}

auto java_hotspot::method::remove_all_break_points() -> void {
    jvm_break_points::remove_all_breakpoints(this);
}
#include "magic_enum.hpp"
auto java_hotspot::method::hide_byte_codes(std::vector<uint8_t> fake_opcodes) -> void {
    const size_t bytecode_size = get_const_method()->get_bytecode_size();
    if (const size_t fake_opcodes_size = fake_opcodes.size(); fake_opcodes_size != bytecode_size) {
        return;
    }
    int hide_size = 0;
    const auto constants_pool = get_const_method()->get_constants();
    auto *holder_klass = static_cast<instance_klass *>(constants_pool->get_pool_holder());
    std::cout << "bytecode_size :" << bytecode_size << std::endl;
    while (hide_size < bytecode_size) {
        /* Fake breakpoint */
        std::cout << "Hide Size :" << hide_size << std::endl;
        const auto bytecode = std::make_unique<java_runtime::bytecode>(
                get_const_method()->get_bytecode_start() + hide_size);
        using namespace java_runtime;
        
        
        const auto fake_opcode = static_cast<java_runtime::bytecodes>(fake_opcodes[hide_size]);
        if (const auto opcode = bytecode->get_opcode();opcode == java_runtime::bytecodes::_fast_aldc_w ||
        opcode == java_runtime::bytecodes::_fast_aldc || opcode == java_runtime::bytecodes::_invokedynamic ||opcode == java_runtime::bytecodes::_lxor || opcode == java_runtime::bytecodes::_getstatic || opcode == java_runtime::bytecodes::_iload_1 || opcode == bytecodes::_ldc2_w || opcode == bytecodes::_lload_1 || opcode == bytecodes::_dup2 || opcode == bytecodes::_lstore_1 || opcode == bytecodes::_lstore_3 
                ) {
            hide_size += bytecode->get_length();
            continue;
        }
        auto *info = jvm_internal::breakpoint_info::create(this, hide_size);
        info->set_orig_bytecode(java_runtime::bytecodes::_nop);
        info->set_next(holder_klass->get_breakpoints());
        holder_klass->set_breakpoints(info);
        if (bytecode.get()->get_opcode() == java_runtime::bytecodes::_breakpoint)
        {
            MessageBox(0, "There is an error.", 0, 0);
        }
        std::cout << "Bytecode :" << static_cast<int>(bytecode.get()->get_opcode()) << std::endl;
        /* True breakpoint */
        const auto original_length = bytecode->get_length();
        //std::cout << "0x" << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(bytecode->get_opcode())
          //        << " -> " << std::dec << bytecode->get_length() << std::endl;
        jvm_break_points::set_breakpoint(this, hide_size, [](break_point_info * info) -> void {
            
            std::cout << "Dispatching " << static_cast<int>(jvm_break_points::original_bytecodes[info->get_bytecode_address()]) << " :" <<  magic_enum::enum_name(static_cast<java_runtime::bytecodes>(jvm_break_points::original_bytecodes[info->get_bytecode_address()])) << std::endl;;
        });
        hide_size += original_length ;
    }
}

auto java_hotspot::method::get_flags() -> unsigned short*
{
    if (!this) return nullptr;
    static VMStructEntry* vm_entry = JVMWrappers::find_type_fields("Method").value().get()["_flags"];
    if (!vm_entry)
        return nullptr;
    return (unsigned short*)((uint8_t*)this + vm_entry->offset);
}


auto java_hotspot::method::set_dont_inline(bool enabled) -> void
{
    unsigned short* _flags = get_flags();
    if (!_flags)
    {
        static VMStructEntry* vm_entry = JVMWrappers::find_type_fields("Method").value().get()["_intrinsic_id"];
        if (!vm_entry) return;
        constexpr uintptr_t relative_offset_from_intrinsic_id = 1;
        unsigned char* flags = ((uint8_t*)this + vm_entry->offset + relative_offset_from_intrinsic_id);
        if (enabled)
            *flags |= (1 << 3);
        else
            *flags &= ~(1 << 3);
        return;
    }

    if (enabled)
        *_flags |= _dont_inline;
    else
        *_flags &= ~_dont_inline;
}


auto java_hotspot::method::get_access_flags() -> jvm_internal::access_flags * {
    static VMStructEntry *_access_flags_entry = JVMWrappers::find_type_fields("Method").value().get()["_access_flags"];
    if (!_access_flags_entry) {
        std::cerr << "Access flags not found" << std::endl;
        return {};
    }
    return reinterpret_cast<jvm_internal::access_flags *>(reinterpret_cast<uint8_t *>(this) +
                                                          _access_flags_entry->offset);
}
