//
// Created by Administrator on 2024/3/14.
//

#include "jnif/jnif.hpp"

#include "main.h"

#include <filesystem>
#include <fstream>
#include <dbghelp.h>

#include "byte_code_info.h"
#include "field_info.h"

#include "jvm_thread.h"
#include "jvm_internal.h"
#include "java_interop.h"
#include "jni_internal.h"
#include "magic_enum.hpp"
#include "jvm_break_points.h"
#include "vm_helper.h"

#define  ENABLELOG

#ifdef ENABLELOG

#define BEGIN_LOG(stuff) std::cout<< stuff

#define END_LOG << std::endl;

#define FLUSH std::cout.flush();

#else

#define BEGIN_LOG(stuff)

#define END_LOG

#define FLUSH
#endif

#include <sstream>
#include <fstream>
std::unique_ptr<java_interop::debug_accessor> debug_accessor;



/* JVM exports some symbols that we need to use */
extern "C" JNIIMPORT VMStructEntry *gHotSpotVMStructs;
extern "C" JNIIMPORT VMTypeEntry *gHotSpotVMTypes;
extern "C" JNIIMPORT VMIntConstantEntry *gHotSpotVMIntConstants;
extern "C" JNIIMPORT VMLongConstantEntry *gHotSpotVMLongConstants;

/* Find needed offsets */
auto InitGlobalOffsets() -> void {
    /* .\hotspot\src\share\vm\classfile\javaClasses.hpp -> class java_lang_Class : AllStatic */
    const auto java_lang_Class = JVMWrappers::find_type_fields("java_lang_Class");
    if (!java_lang_Class.has_value()) {
        BEGIN_LOG("Failed to find java_lang_Class") END_LOG
    }

    /* java_lang_Class -> _klass_offset */
    global_offsets::klass_offset = *static_cast<jint *>(java_lang_Class.value().get()["_klass_offset"]->address);
    std::cout << "Klass Offset:" << global_offsets::klass_offset << std::endl;
    java_hotspot::bytecode_start_offset = java_hotspot::const_method::get_const_method_length();
}

std::string replace(const std::string& str, const std::string& from, const std::string& to) {
    std::string copy = str;
    size_t start_pos = 0;
    while((start_pos = copy.find(from, start_pos)) != std::string::npos) {
        copy.replace(start_pos, from.length(), to);
        start_pos += to.length(); // handles case where 'to' is a substring of 'from'
    }
    return copy;
}

std::string replaceMultiple(std::string str, const std::vector<std::pair<std::string, std::string>>& replacements) {
    for (const auto& pair : replacements) {
        size_t start_pos = 0;
        while((start_pos = str.find(pair.first, start_pos)) != std::string::npos) {
            str.replace(start_pos, pair.first.length(), pair.second);
            start_pos += pair.second.length();
        }
    }
    return str;
}


#include "header.hpp"
/* Initialize the JVM acquirer */
auto InitJVMAcquirer() -> void {
    JVMWrappers::init(gHotSpotVMStructs, gHotSpotVMTypes, gHotSpotVMIntConstants, gHotSpotVMLongConstants);
    debug_accessor = std::make_unique<java_interop::debug_accessor>();
    //InitJVMThread();
    InitGlobalOffsets();
    static std::ofstream file("header.hpp");
    auto dump = [&](const std::string &class_name,
                    const std::string &method_name)
    {
        auto klass = debug_accessor->get_env()->FindClass(class_name.c_str());
        auto instance_klass = java_interop::get_instance_class(klass);
        if (!instance_klass) {
            return;
        }
        const auto methods = instance_klass->get_methods();
        const auto data = methods->get_data();
        const auto length = methods->get_length();
        for (auto i = 0; i < length; i++) {
            const auto method = data[i];
            const auto const_method = method->get_const_method();
            auto bytecodes = const_method->get_bytecode();
            const size_t bytecodes_length = bytecodes.size();
            if (size_t bytecodes_index = 0;(method->get_name() + method->get_signature())._Equal(method_name)) {
                static const std::vector<std::pair<std::string, std::string>> replacements = {
                        {"/", "_"},
                        {";", "_"},
                        {"(", "_"},
                        {")", "_"},
                        {"[", "_"}
                };
                file<<"const std::vector "<< replace(class_name,"/","_")<< "_" << replaceMultiple(method_name, replacements) << " = {\n" ;
                while (bytecodes_index < bytecodes_length) {
                    const auto bytecode = static_cast<java_runtime::bytecodes>(bytecodes[bytecodes_index]);
                    file << "    static_cast<uint8_t>(0x" << std::uppercase << std::setfill('0') << std::setw(2) <<
                         std::hex << static_cast<unsigned int>(bytecode) <<
                         "), // " << std::dec << bytecodes_index << "\n";
                    bytecodes_index++;
                }
                file << "};\n" << std::endl;
                file.flush();
            }
        }
    };
    InitMethods(dump);
}

BOOL WINAPI DllMain(HINSTANCE hinst_dll, const DWORD ul_reason_for_call, LPVOID lpv_reserved) {
    if (ul_reason_for_call != DLL_PROCESS_ATTACH)
        return TRUE;
    FreeConsole();
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    /* Start the JVM acquirer thread */
    CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(InitJVMAcquirer), nullptr, 0, nullptr);

    return TRUE;
}