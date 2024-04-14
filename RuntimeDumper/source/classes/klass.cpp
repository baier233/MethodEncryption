//
// Created by Administrator on 2024/3/15.
//

#include "klass.h"

#include "java_interop.h"
#include "jvm_internal.h"

auto java_hotspot::instance_klass::get_name() -> symbol * {
    static VMStructEntry *_name_entry = JVMWrappers::find_type_fields("Klass").value().get()["_name"];
    if (!_name_entry)
        return nullptr;
    return *reinterpret_cast<symbol **>(reinterpret_cast<uint8_t *>(this) + _name_entry->offset);
}

auto java_hotspot::instance_klass::get_constants() -> const_pool * {
    static VMStructEntry *_constants_entry = JVMWrappers::find_type_fields("InstanceKlass").value().get()["_constants"];
    if (!_constants_entry) return nullptr;
    return *reinterpret_cast<const_pool **>(reinterpret_cast<uint8_t *>(this) + _constants_entry->offset);
}

auto java_hotspot::instance_klass::get_methods() -> array<method *> * {
    static VMStructEntry *_methods_entry = JVMWrappers::find_type_fields("InstanceKlass").value().get()["_methods"];
    if (!_methods_entry) return nullptr;
    return *reinterpret_cast<array<method *> **>(reinterpret_cast<uint8_t *>(this) + _methods_entry->offset);
}

auto java_hotspot::instance_klass::get_fields() -> array<uint16_t> * {
    static VMStructEntry *_fields_entry = JVMWrappers::find_type_fields("InstanceKlass").value().get()["_fields"];
    if (!_fields_entry) return nullptr;
    return *reinterpret_cast<array<uint16_t> **>(reinterpret_cast<uint8_t *>(this) + _fields_entry->offset);
}

auto java_hotspot::instance_klass::set_breakpoints(jvm_internal::breakpoint_info *breakpoints) -> void {
    static VMStructEntry *_breakpoints_entry = JVMWrappers::find_type_fields("InstanceKlass").value().get()[
        "_breakpoints"];
    if (!_breakpoints_entry) return;
    *reinterpret_cast<jvm_internal::breakpoint_info **>(reinterpret_cast<uint8_t *>(this) + _breakpoints_entry->offset)
            = breakpoints;
}

auto java_hotspot::instance_klass::get_breakpoints() -> jvm_internal::breakpoint_info * {
    static VMStructEntry *_breakpoints_entry = JVMWrappers::find_type_fields("InstanceKlass").value().get()[
        "_breakpoints"];
    if (!_breakpoints_entry) return nullptr;
    return *reinterpret_cast<jvm_internal::breakpoint_info **>(
        reinterpret_cast<uint8_t *>(this) + _breakpoints_entry->offset);
}

auto java_hotspot::instance_klass::get_super_klass() -> instance_klass * {
    static VMStructEntry *_super_entry = JVMWrappers::find_type_fields("Klass").value().get()["_super"];
    if (!_super_entry) return nullptr;
    return *reinterpret_cast<instance_klass **>(reinterpret_cast<uint8_t *>(this) + _super_entry->offset);
}

auto java_hotspot::instance_klass::get_java_mirror() -> oop_desc * {
    static VMStructEntry *_java_mirror_entry = JVMWrappers::find_type_fields("Klass").value().get()["_java_mirror"];
    if (!_java_mirror_entry) return nullptr;
    return *reinterpret_cast<oop_desc **>(reinterpret_cast<uint8_t *>(this) + _java_mirror_entry->offset);
}

auto java_hotspot::instance_klass::find_field_info(
    const std::string &field_name,
    const std::string &field_signature
) -> std::tuple<field_info *, instance_klass *> {
    auto current_klass = this;
    while (current_klass) {
        const auto fields = current_klass->get_fields();
        const auto fields_length = fields->get_length() / field_slots;
        const auto fields_data = fields->get_data();
        const auto name_and_signature_param = field_name + field_signature;
        const auto constants = current_klass->get_constants();
        for (auto i = 0; i < fields_length; i++) {
            const auto field = field_info::from_field_array(fields_data, i);
            if (!field) {
                continue;
            }
            if (
                const auto name_and_signature_field = field->get_name(constants)->to_string() +
                                                      field->get_signature(constants)->to_string();
                !name_and_signature_param._Equal(name_and_signature_field)
            ) {
                continue;
            }
            return {field, current_klass};
        }
        current_klass = current_klass->get_super_klass();
    }
    throw std::runtime_error("find_field_error");
}

auto java_hotspot::get_static_object_field(
    field_info *field_info,
    instance_klass *current_klass
) -> jobject {
    const auto java_mirror = current_klass->get_java_mirror();
    const auto address = java_mirror->obj_field(static_cast<int>(field_info->get_offset()));
    java_interop::attach_temp_thread();
    /* this is a local reference, maybe can delete it */
    const auto make_local = jni_handles_internal::make_local(address);
    java_interop::detach_temp_thread();
    return make_local;
}

auto java_hotspot::get_objet_field(
    field_info *field_info,
    jobject obj
) -> jobject {
    const auto result = jni_handles::resolve_non_null(obj);
    java_interop::attach_temp_thread();
    const auto ret = jni_handles_internal::make_local(result->obj_field(static_cast<int>(field_info->get_offset())));
    java_interop::detach_temp_thread();
    std::cout << result->obj_field(static_cast<int>(field_info->get_offset())) << std::endl;
    return ret;
}

auto java_hotspot::set_objet_field(field_info *field_info, jobject obj, jobject value) -> void {
    const auto result = jni_handles::resolve_non_null(obj);
    jni_handles_internal::oop_store(
        result->obj_field_addr<narrowOop>(static_cast<int>(field_info->get_offset())),
        jni_handles::resolve(value)
    );
}

auto java_hotspot::set_static_object_field(
    field_info *field_info,
    instance_klass *current_klass,
    jobject obj
) -> void {
    const auto java_mirror = current_klass->get_java_mirror();
    jni_handles_internal::oop_store(
        java_mirror->obj_field_addr<narrowOop>(static_cast<int>(field_info->get_offset())),
        jni_handles::resolve(obj)
    );
}

auto java_hotspot::get_objet_field_detach(field_info *field_info, jobject obj) -> jobject {
    const auto result = jni_handles::resolve_non_null(obj);
    const auto ret = result->obj_field(static_cast<int>(field_info->get_offset()));
    /* Maybe need to delete it properly */
    const auto oop_pointer = new oop(ret);
    return reinterpret_cast<jobject>(oop_pointer);
}
