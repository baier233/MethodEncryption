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

//#define  ENABLELOG

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

/* Initialize the JVM internal */
auto InitJVMInternal(const java_interop::debug_accessor *debug_accessor) -> void {

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
}



/* Initialize the JVM acquirer */
auto InitJVMAcquirer() -> void {
    JVMWrappers::init(gHotSpotVMStructs, gHotSpotVMTypes, gHotSpotVMIntConstants, gHotSpotVMLongConstants);
    debug_accessor = std::make_unique<java_interop::debug_accessor>();
    //InitJVMThread();
    InitGlobalOffsets();
    InitJVMInternal(debug_accessor.get());


    {
        const auto klass = debug_accessor->find_class("Main");
        const auto instance_klass = java_interop::get_instance_class(klass);


        {
            const auto methods = instance_klass->get_methods();
            const auto data = methods->get_data();
            const auto length = methods->get_length();
            for (auto i = 0; i < length; i++) {
                const auto method = data[i];
                const auto const_method = method->get_const_method();


                auto bytecodes = const_method->get_bytecode();
                const size_t bytecodes_length = bytecodes.size();

                if (method->get_name()._Equal("Test")) {
                    method->set_dont_inline(true);
                    const auto access_flags = method->get_access_flags();
                    access_flags->set_not_c1_compilable();
                    access_flags->set_not_c2_compilable();
                    access_flags->set_not_c2_osr_compilable();
                    access_flags->set_queued_for_compilation();

                    const auto constants_pool = method->get_const_method()->get_constants();
                    auto *holder_klass = static_cast<java_hotspot::instance_klass *>(constants_pool->get_pool_holder());


                    int bytecodes_index = 0;
                    while (bytecodes_index < bytecodes_length) {
                        const auto bytecode = static_cast<java_runtime::bytecodes>(bytecodes[bytecodes_index]);

                        if (bytecode == java_runtime::bytecodes::_invokevirtual){

                            //spoofing original bytecode
                            {
                                std::cout << "setting breakpoint for " << bytecodes_index << std::endl;
                                auto *info = jvm_internal::breakpoint_info::create(method, bytecodes_index);
                                info->set_orig_bytecode(java_runtime::bytecodes::_invokevirtual);
                                info->set_next(holder_klass->get_breakpoints());
                                holder_klass->set_breakpoints(info);
                            }

                            //setting bytecode and callback
                            jvm_break_points::set_breakpoint_with_original_code(method,bytecodes_index,bytecodes[bytecodes_index], [ ]( break_point_info* bp ) -> void
                            {
                                //value should be the parameter eg : System.out.Println(parameter)
                                auto value = bp->get_operand(0);
                                auto str = debug_accessor->get_env()->GetStringUTFChars((jstring)value,0);
                                *value = *(uintptr_t*)debug_accessor->get_env()->NewStringUTF("Replaced");
                                std::cout << "original str :" << str << std::endl;
                                return;
                            });
                        }
                        bytecodes_index+= static_cast<int>(jvm_byte_code::bytecode_lengths[static_cast<unsigned int>(bytecode)]);
                    }




                }
            }
        }
    }



}

jint JNICALL
JNI_OnLoad(JavaVM *javaVm, void *reserved){
    java_interop::javaVM = javaVm;
    InitJVMAcquirer();
    return JNI_VERSION_1_8;
};

