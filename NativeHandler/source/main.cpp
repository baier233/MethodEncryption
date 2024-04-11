//
// Created by Administrator on 2024/3/14.
//

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

//#define NativeLib

#define NativeHandler

std::unique_ptr<java_interop::debug_accessor> debug_accessor;


#ifdef NativeLib
extern "C" {
JNIEXPORT jbyteArray JNICALL Java_un_defined_Breakpoint_getMethodBytecode
        (JNIEnv *, jclass, jclass, jstring, jstring);

JNIEXPORT void JNICALL  Java_un_defined_Breakpoint_loadJar
        (JNIEnv * env, jclass, jstring jarPath);
}

jbyteArray Java_un_defined_Breakpoint_getMethodBytecode(JNIEnv * env, jclass, jclass clz, jstring methodName, jstring methodSign) {

    auto name = env->GetStringUTFChars(methodName,0);
    auto sign = env->GetStringUTFChars(methodSign,0);
    std::string comp(std::string (name) + std::string (sign));
    auto klass = java_interop::get_instance_class(clz);
    auto methodArray = klass->get_methods();
    auto methods = methodArray->get_data();
    for (int index = 0;index < methodArray->get_length();index ++){
        auto method =  methods[index];
        if ((method->get_name() + method->get_signature()) != comp) continue;
        std::vector<uint8_t> code;
        auto size = method->get_const_method()->get_code_size();
        auto start = method->get_const_method()->get_bytecode_start();
        for (int i = 0; i < size; ++i) {
            code.push_back(start[i]);
        }
        auto result = env->NewByteArray(size);
        env->SetByteArrayRegion(result, 0, size, reinterpret_cast<const jbyte *>(code.data()));
        return result;
    }
    return nullptr;
}



void Java_un_defined_Breakpoint_loadJar(JNIEnv * env, jclass, jstring jarPath) {

    auto nameUTF = env->GetStringUTFChars(jarPath, 0);
    debug_accessor->get_jvmti()->A ddToSystemClassLoaderSearch(nameUTF);
}
#endif

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
    java_hotspot::bytecode_start_offset = java_hotspot::const_method::get_const_method_length();
}

inline std::string make_local_pattern = R"(
    48 83 7C 24 38 00
    )";

inline std::string oop_store_pattern = R"(
    48 83 7C 24 48 00
    )";

inline std::string thread_index_pattern = R"(
    B8 70 00 00 00 48 8B 5C 24 30 48 8B 74 24 38 48 83 C4 20 5F C3
    )";


/* Initialize the JVM internal */
auto InitJVMInternal(const java_interop::debug_accessor *debug_accessor) -> void {
#ifndef NativeLib
    /* Get JNIHandles::make_local(oop obj) address */
#ifndef NativeHandler
    const auto steal_env = debug_accessor->get_env();
    auto start_address = reinterpret_cast<uintptr_t>(steal_env->functions->GetStaticObjectField);
    const auto make_local_address = scan(make_local_pattern.c_str(), start_address, start_address + 0x1024) - 5;
    if (!make_local_address) {

        throw std::runtime_error("Failed to find JNIHandles::make_local(oop obj) address");

    }
    {
        java_hotspot::jni_handles_internal::make_local = reinterpret_cast<make_local_t>(get_absolute_address(
                make_local_address));
        BEGIN_LOG("JNIHandles::make_local(oop obj) address: " << reinterpret_cast<void *>(
                java_hotspot::jni_handles_internal::make_local)) END_LOG
    }

    /* Get oop_store address  */
    start_address = reinterpret_cast<uintptr_t>(steal_env->functions->SetStaticObjectField);
    const auto call_address = scan(oop_store_pattern.c_str(), start_address, start_address + 0x512) - 12;
    if (!call_address) {
        throw std::runtime_error("Failed to find oop_store address");
    }
    java_hotspot::jni_handles_internal::oop_store = reinterpret_cast<oop_store_t>(get_absolute_address(call_address));
    BEGIN_LOG("oop_store(int, oop) address: " << reinterpret_cast<void *>(
            java_hotspot::jni_handles_internal::oop_store)) END_LOG
#endif

    /* Any class */
    const auto integer_klass = debug_accessor->find_class("java/lang/Integer");

    if (!integer_klass) {
        BEGIN_LOG("Failed to find java/lang/Integer") END_LOG
    }

    /* Any method */
    const auto integer_hash_code = debug_accessor->find_method(integer_klass, "shortValue", "()S");
    if (!integer_hash_code) {
        BEGIN_LOG("Failed to find java/lang/Integer::shortValue") END_LOG
    }

    /* Get method */
    const auto hash_method = *reinterpret_cast<java_hotspot::method **>(integer_hash_code);
    if (!hash_method) {
        BEGIN_LOG("Failed to find java/lang/Integer::shortValue") END_LOG
    }
    BEGIN_LOG("Bytecode start offset: " << java_hotspot::bytecode_start_offset) END_LOG

    /* Interception address */
    java_hotspot::interpreter_entry *interpreter_entry = hash_method->get_i2i_entry();
    BEGIN_LOG("Interpreter entry: " << interpreter_entry) END_LOG
    const auto interception_address = interpreter_entry->get_interception_address();
    if (!interception_address) {
        BEGIN_LOG("Failed to find interception address") END_LOG
    }
    BEGIN_LOG("Interception address: " << reinterpret_cast<void *>(interception_address)) END_LOG

    /* Get dispatch table */
    const uintptr_t dispatch_table = *reinterpret_cast<uintptr_t *>(interception_address + 2);
    if (!dispatch_table) {
        BEGIN_LOG("Failed to find dispatch table") END_LOG
    }
    BEGIN_LOG("Dispatch table: " << reinterpret_cast<void *>(dispatch_table)) END_LOG

    /* Get breakpoint method */
    const uintptr_t breakpoint_method = *reinterpret_cast<uintptr_t *>(
            dispatch_table + static_cast<uint8_t>(java_runtime::bytecodes::_breakpoint) * 8);
    if (!breakpoint_method) {
        BEGIN_LOG("Failed to find breakpoint method") END_LOG
    }
    BEGIN_LOG("Breakpoint method: " << reinterpret_cast<void *>(breakpoint_method)) END_LOG

    /* Get VM calls */
    const std::vector<void *> vm_calls = vm_helper::find_vm_calls(reinterpret_cast<PVOID>(breakpoint_method));
    if (vm_calls.size() < 2) {
        BEGIN_LOG("Failed to find VM calls") END_LOG
    }
    BEGIN_LOG("VM calls: " << vm_calls.size()) END_LOG

    // Hook the runtime methods
    void *runtime_get_original_bytecode = vm_calls[0];
    void *runtime_breakpoint_method = vm_calls[1];

    BEGIN_LOG("Runtime get original bytecode: " << runtime_get_original_bytecode) END_LOG
    BEGIN_LOG("Runtime breakpoint method: " << runtime_breakpoint_method) END_LOG
    jvm_break_points::breakpoint_hook.init_hook(runtime_breakpoint_method, jvm_break_points::breakpoint_callback);
    jvm_break_points::original_bytecode_hook.init_hook(runtime_get_original_bytecode,
                                                       jvm_break_points::original_bytecode_handler);
    jvm_break_points::breakpoint_hook.start_hook();
    jvm_break_points::original_bytecode_hook.start_hook();
#ifndef NativeHandler
    /* Resolve_ldc */
    {
        const uintptr_t fast_aldc_method = *reinterpret_cast<uintptr_t *>(
                dispatch_table + static_cast<uint8_t>(java_runtime::bytecodes::_fast_aldc_w) * 8);
        if (!fast_aldc_method) {
            BEGIN_LOG("Failed to find fast_aldc method") END_LOG
        }
        BEGIN_LOG("Fast aldc method: " << reinterpret_cast<void *>(fast_aldc_method)) END_LOG

        const std::vector<void *> fast_aldc_vm_calls = vm_helper::find_vm_calls(
                reinterpret_cast<PVOID>(fast_aldc_method));
        if (fast_aldc_vm_calls.size() < 1) {
            BEGIN_LOG("Failed to find fast_aldc VM calls") END_LOG
        }
        BEGIN_LOG("VM calls: " << fast_aldc_vm_calls.size()) END_LOG

        BEGIN_LOG("Resolve_ldc address: " << fast_aldc_vm_calls[0]) END_LOG
    }
#endif
#endif
}


/* Initialize the JVM thread */
auto InitJVMThread() -> void {
#ifndef NativeLib
    /* Get ThreadLocalStorage -> _thread_index(address) */
    const auto module = GetModuleHandleA("jvm.dll");
    const auto _thread_index_address = pattern_scan(
            module,
            {
                    0xB8, 0x70, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x5C, 0x24, 0x30,
                    0x48, 0x8B, 0x74, 0x24, 0x38, 0x48,
                    0x83, 0xC4, 0x20, 0x5F, 0xC3
            }, "xxxxxxxxxxxxxxxxxxxxx", 0
    );
    const auto mov_address = _thread_index_address + 21;
    /* 00000000740F7ABD         | 8B0D A5D49000     | mov ecx,dword ptr ds:[<private: static int ThreadLocalStorage::_thread_index>]                   */
    /* 00000000740F7AC3         | FF15 07B95600     | call qword ptr ds:[<&TlsGetValue>]                                                               */
    const auto mov_content_address = mov_address + 2;
    const uint32_t offset = *reinterpret_cast<uint32_t *>(mov_content_address);
    const auto absolute_address = mov_content_address + offset + 4;
    java_hotspot::thread_local_storage_internal::_thread_index_address_ = reinterpret_cast<uint8_t *>(
            absolute_address);
    BEGIN_LOG("Thread index address: " << reinterpret_cast<void *>(
            java_hotspot::thread_local_storage_internal::_thread_index_address_)) END_LOG

#endif
}

/* Initialize the JVM acquirer */
auto InitJVMAcquirer() -> void {
    JVMWrappers::init(gHotSpotVMStructs, gHotSpotVMTypes, gHotSpotVMIntConstants, gHotSpotVMLongConstants);
    debug_accessor = std::make_unique<java_interop::debug_accessor>();
    //InitJVMThread();
    InitGlobalOffsets();
    InitJVMInternal(debug_accessor.get());

#ifndef NativeHandler
    const auto klass = debug_accessor->find_class("Main");
    const auto instance_klass = java_interop::get_instance_class(klass);
    BEGIN_LOG("Instance klass: " << instance_klass << "\n");
    const auto const_pool = instance_klass->get_constants();
    BEGIN_LOG("Constant pool: " << const_pool << "\n");
    BEGIN_LOG("Constant pool tags: " << const_pool->get_tags()->get_length() << "\n");
    BEGIN_LOG("Constant pool size: " << const_pool->get_length() << "\n");
    const auto name = instance_klass->get_name();
    BEGIN_LOG("Class name symbol: " << name << "\n");
    BEGIN_LOG("Class name: " << name->to_string() << "\n");

    const auto methods = instance_klass->get_methods();
    const auto data = methods->get_data();
    const auto length = methods->get_length();
    BEGIN_LOG("Methods count: " << length << "\n");
    BEGIN_LOG("\n");
    for (auto i = 0; i < length; i++) {
        const auto method = data[i];
        BEGIN_LOG("Method address: " << method << "\n");
        BEGIN_LOG("Method i2i entry: " << method->get_i2i_entry() << "\n");
        BEGIN_LOG("Method name: " << method->get_name() << " " << method->get_signature() << "\n");
        const auto const_method = method->get_const_method();
        BEGIN_LOG("Method max stack: " << const_method->get_max_stack() << "\n");
        BEGIN_LOG("Method max locals: " << const_method->get_max_locals() << "\n");
        BEGIN_LOG("Method code size: " << const_method->get_code_size() << "\n");
        BEGIN_LOG("Method name index: " << const_method->get_name_index() << "\n");
        BEGIN_LOG("Method signature index: " << const_method->get_signature_index() << "\n");
        BEGIN_LOG("Method size of parameters: " << const_method->get_size_of_parameters() << "\n");
        BEGIN_LOG("Method method idnum: " << const_method->get_method_idnum() << "\n");
        auto bytecodes = const_method->get_bytecode();
        const size_t bytecodes_length = bytecodes.size();
        size_t bytecodes_index = 0;
        BEGIN_LOG("const std::vector opcode = {\n");
        while (bytecodes_index < bytecodes_length) {
            const auto bytecode = static_cast<java_runtime::bytecodes>(bytecodes[bytecodes_index]);
            BEGIN_LOG("    static_cast<uint8_t>(0x" << std::uppercase << std::setfill('0') << std::setw(2) <<
                                                    std::hex << static_cast<unsigned int>(bytecode) <<
                                                    "), // " << std::dec << bytecodes_index << "\n");
            bytecodes_index++;
        }
        BEGIN_LOG("};\n") END_LOG
        FLUSH
        if (method->get_name()._Equal("Test")) {
            /* Anti c1_compiler and c2_compiler , and set queued for compilation */
            method->set_dont_inline(true);
            const auto access_flags = method->get_access_flags();
            access_flags->set_not_c1_compilable();
            access_flags->set_not_c2_compilable();
            access_flags->set_not_c2_osr_compilable();
            access_flags->set_queued_for_compilation();

            static const std::vector opcode = {
                    static_cast<uint8_t>(0x03), // 0
                    static_cast<uint8_t>(0x3B), // 1
                    static_cast<uint8_t>(0x03), // 2
                    static_cast<uint8_t>(0x3C), // 3
                    static_cast<uint8_t>(0x1B), // 4
                    static_cast<uint8_t>(0x11), // 5
                    static_cast<uint8_t>(0x03), // 6
                    static_cast<uint8_t>(0xE8), // 7
                    static_cast<uint8_t>(0xA2), // 8
                    static_cast<uint8_t>(0x00), // 9
                    static_cast<uint8_t>(0x57), // 10
                    static_cast<uint8_t>(0x1A), // 11
                    static_cast<uint8_t>(0x12), // 12
                    static_cast<uint8_t>(0x03), // 13
                    static_cast<uint8_t>(0xA1), // 14
                    static_cast<uint8_t>(0x00), // 15
                    static_cast<uint8_t>(0x06), // 16
                    static_cast<uint8_t>(0xB8), // 17
                    static_cast<uint8_t>(0x01), // 18
                    static_cast<uint8_t>(0x00), // 19
                    static_cast<uint8_t>(0xB2), // 20
                    static_cast<uint8_t>(0x02), // 21
                    static_cast<uint8_t>(0x00), // 22
                    static_cast<uint8_t>(0xBB), // 23
                    static_cast<uint8_t>(0x00), // 24
                    static_cast<uint8_t>(0x06), // 25
                    static_cast<uint8_t>(0x59), // 26
                    static_cast<uint8_t>(0xB7), // 27
                    static_cast<uint8_t>(0x03), // 28
                    static_cast<uint8_t>(0x00), // 29
                    static_cast<uint8_t>(0xE6), // 30
                    static_cast<uint8_t>(0x00), // 31
                    static_cast<uint8_t>(0xB6), // 32
                    static_cast<uint8_t>(0x04), // 33
                    static_cast<uint8_t>(0x00), // 34
                    static_cast<uint8_t>(0x1A), // 35
                    static_cast<uint8_t>(0xB6), // 36
                    static_cast<uint8_t>(0x05), // 37
                    static_cast<uint8_t>(0x00), // 38
                    static_cast<uint8_t>(0xB6), // 39
                    static_cast<uint8_t>(0x06), // 40
                    static_cast<uint8_t>(0x00), // 41
                    static_cast<uint8_t>(0xB6), // 42
                    static_cast<uint8_t>(0x07), // 43
                    static_cast<uint8_t>(0x00), // 44
                    static_cast<uint8_t>(0x03), // 45
                    static_cast<uint8_t>(0x3D), // 46
                    static_cast<uint8_t>(0x1C), // 47
                    static_cast<uint8_t>(0x11), // 48
                    static_cast<uint8_t>(0x03), // 49
                    static_cast<uint8_t>(0xE8), // 50
                    static_cast<uint8_t>(0xA2), // 51
                    static_cast<uint8_t>(0x00), // 52
                    static_cast<uint8_t>(0x23), // 53
                    static_cast<uint8_t>(0xB2), // 54
                    static_cast<uint8_t>(0x08), // 55
                    static_cast<uint8_t>(0x00), // 56
                    static_cast<uint8_t>(0x1B), // 57
                    static_cast<uint8_t>(0x32), // 58
                    static_cast<uint8_t>(0x1C), // 59
                    static_cast<uint8_t>(0xB8), // 60
                    static_cast<uint8_t>(0x09), // 61
                    static_cast<uint8_t>(0x00), // 62
                    static_cast<uint8_t>(0x14), // 63
                    static_cast<uint8_t>(0x00), // 64
                    static_cast<uint8_t>(0x0F), // 65
                    static_cast<uint8_t>(0x6B), // 66
                    static_cast<uint8_t>(0x14), // 67
                    static_cast<uint8_t>(0x00), // 68
                    static_cast<uint8_t>(0x11), // 69
                    static_cast<uint8_t>(0x73), // 70
                    static_cast<uint8_t>(0x14), // 71
                    static_cast<uint8_t>(0x00), // 72
                    static_cast<uint8_t>(0x13), // 73
                    static_cast<uint8_t>(0x63), // 74
                    static_cast<uint8_t>(0x8E), // 75
                    static_cast<uint8_t>(0x4F), // 76
                    static_cast<uint8_t>(0x84), // 77
                    static_cast<uint8_t>(0x00), // 78
                    static_cast<uint8_t>(0x01), // 79
                    static_cast<uint8_t>(0x84), // 80
                    static_cast<uint8_t>(0x02), // 81
                    static_cast<uint8_t>(0x01), // 82
                    static_cast<uint8_t>(0xA7), // 83
                    static_cast<uint8_t>(0xFF), // 84
                    static_cast<uint8_t>(0xDC), // 85
                    static_cast<uint8_t>(0x84), // 86
                    static_cast<uint8_t>(0x00), // 87
                    static_cast<uint8_t>(0x01), // 88
                    static_cast<uint8_t>(0x84), // 89
                    static_cast<uint8_t>(0x01), // 90
                    static_cast<uint8_t>(0x01), // 91
                    static_cast<uint8_t>(0xA7), // 92
                    static_cast<uint8_t>(0xFF), // 93
                    static_cast<uint8_t>(0xA8), // 94
                    static_cast<uint8_t>(0xB1), // 95
            };

            /*
            const std::vector<uint8_t> old_opcodes = method->get_const_method()->get_bytecode();
            method->get_const_method()->set_bytecode(org_example_Main_test_multiply_opcode);*/

            std::cout <<  "Bytecode Start :" << (void*)method->get_const_method()->get_bytecode_start() << std::endl;
            const auto constants_pool = method->get_const_method()->get_constants();
            auto *holder_klass = static_cast<java_hotspot::instance_klass *>(constants_pool->get_pool_holder());
            auto *info = jvm_internal::breakpoint_info::create(method, 0);
            info->set_orig_bytecode(java_runtime::bytecodes::_nop);
            info->set_next(holder_klass->get_breakpoints());
            holder_klass->set_breakpoints(info);

            jvm_break_points::set_breakpoint_with_original_code(method,0,opcode[0],[](break_point_info * info) -> void {
                jhook_set_r13_address((void*)opcode.data());
                std::cout << "Addr :" << (void*)info->get_bytecode_address() << " r13 :" << (void*)jhook_get_r13_address()  << std::endl;

            });

            //method->hide_byte_codes(opcode);
        }


        BEGIN_LOG("ok\n");
        FLUSH
    }
#endif
    // Dump the class
    /*const auto klass = debug_accessor->find_class("org/example/Test");
    jvmtiCapabilities capabilities = {};
    capabilities.can_retransform_any_class = 1;
    capabilities.can_retransform_classes = 1;
    capabilities.can_redefine_any_class = 1;
    capabilities.can_redefine_classes = 1;
    jvmtiEventCallbacks callbacks = {};
    callbacks.ClassFileLoadHook = ClassFileLoadHook;
    debug_accessor->get_jvmti()->SetEventCallbacks(&callbacks, sizeof(callbacks));
    debug_accessor->get_jvmti()->AddCapabilities(&capabilities);
    debug_accessor->get_jvmti()->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, nullptr);
    debug_accessor->get_jvmti()->RetransformClasses(1, &klass);
    debug_accessor->get_jvmti()->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, nullptr);*/
}

#ifdef NativeLib
jint JNICALL
JNI_OnLoad(JavaVM *vm, void *reserved){
    JVMWrappers::init(gHotSpotVMStructs, gHotSpotVMTypes, gHotSpotVMIntConstants, gHotSpotVMLongConstants);
    debug_accessor = std::make_unique<java_interop::debug_accessor>();
    InitGlobalOffsets();
    return JNI_VERSION_1_8;
};
#endif

#ifdef NativeHandler
#include "header.hpp"
#endif

#ifdef NativeHandler

static void JNICALL register_method(JNIEnv* env,jclass,jclass klass){
    BEGIN_LOG("Enter Register") END_LOG
    auto inject = [&](const std::string& class_name,
                     const std::string& method_name,
                     const std::vector<uint8_t>& opcode)
    {
        auto instance_klass = java_interop::get_instance_class(klass);
        if (instance_klass->get_name()->to_string() != class_name){
            return;
        }
        BEGIN_LOG("Resolving Class For :" << class_name) END_LOG
        const auto methods = instance_klass->get_methods();
        const auto data = methods->get_data();
        const auto length = methods->get_length();
        for (auto i = 0; i < length; i++) {
            const auto method = data[i];
            const auto const_method = method->get_const_method();
            auto bytecodes = const_method->get_bytecode();
            const size_t bytecodes_length = bytecodes.size();
            if ((method->get_name() + method->get_signature())._Equal(method_name)) {
                method->set_dont_inline(true);
                const auto access_flags = method->get_access_flags();
                access_flags->set_not_c1_compilable();
                access_flags->set_not_c2_compilable();
                access_flags->set_not_c2_osr_compilable();
                access_flags->set_queued_for_compilation();

                const auto constants_pool = method->get_const_method()->get_constants();
                auto *holder_klass = static_cast<java_hotspot::instance_klass *>(constants_pool->get_pool_holder());
                auto *info = jvm_internal::breakpoint_info::create(method, 0);
                info->set_orig_bytecode(java_runtime::bytecodes::_nop);
                info->set_next(holder_klass->get_breakpoints());
                holder_klass->set_breakpoints(info);

                jvm_break_points::set_breakpoint_with_original_code(method, 0, opcode[0], [&opcode](break_point_info* info) -> void {
                    jhook_set_r13_address((void*)(opcode.data()));
                    BEGIN_LOG (info->method->get_name()) END_LOG
                    MessageBox(0, 0, 0, 0);
                    return;
                });

                //method->hide_byte_codes(opcode);
            }


            
        
        }
        return;
    };
    InitMethods(inject);
}


static void InitNativeHandler(){
    auto env = debug_accessor->get_env();
    auto klass = env->FindClass("me/baier/NativeHandler");
    if(!klass) throw std::runtime_error("Unable to find nativeHandler");

    JNINativeMethod table[] = {
            {(char*)"register", (char*)"(Ljava/lang/Class;)V", (void *) &register_method},
    };

    auto err = env->RegisterNatives(klass,table,sizeof(table) / sizeof(JNINativeMethod));
    BEGIN_LOG("Err :" << err)  END_LOG;

}

jint JNICALL
JNI_OnLoad(JavaVM *javaVm, void *reserved){
    java_interop::javaVM = javaVm;
    InitJVMAcquirer();
    InitNativeHandler();
    return JNI_VERSION_1_8;
};
#endif


#ifdef DEBUG

BOOL WINAPI DllMain(HINSTANCE hinst_dll, const DWORD ul_reason_for_call, LPVOID lpv_reserved) {
    if (ul_reason_for_call != DLL_PROCESS_ATTACH)
        return TRUE;

    /* Start the JVM acquirer thread */
    //CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(InitJVMAcquirer), nullptr, 0, nullptr);

    return TRUE;
}
#endif