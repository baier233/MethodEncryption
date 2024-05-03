#ifndef CLASSES_H
#define CLASSES_H
#include <functional>
#include <vector>
#include <string>
inline auto InitMethods(const std::function<void(const std::string& class_name, const std::string& method_name)> dump)
{
    dump("net/minecraft/client/Pi", "e()V");
    dump("net/minecraft/client/Pi", "s()V");
    dump("assets/minecraft/models/item/y", "u(Lassets/minecraft/models/item/QX;)V");
    dump("assets/minecraft/models/item/v", "s(J)V");
    dump("assets/minecraft/models/item/e", "x(SCI)V");
    dump("assets/minecraft/models/item/a", "A(J)F");
    dump("assets/minecraft/models/item/L", "y(J)V");
    dump("assets/minecraft/models/item/L", "l(J)V");
    dump("assets/minecraft/models/item/Qx", "C(Ljava/io/DataOutputStream;J)V");
    dump("assets/minecraft/models/item/WG", "i(Lassets/minecraft/models/item/QX;)V");
    dump("assets/minecraft/models/item/WG", "b(J)V");
    dump("assets/minecraft/models/item/WG", "z(Lnet/minecraft/qH;J)Lassets/minecraft/models/item/i;");
    dump("assets/minecraft/models/item/WG", "T(J)V");
    dump("assets/minecraft/models/item/WM", "n(Lassets/minecraft/models/item/QX;)V");
    dump("assets/minecraft/models/item/WR", "S(J)V");
    dump("assets/minecraft/models/item/We", "Z(J)V");
    dump("assets/minecraft/models/item/We", "u(Lassets/minecraft/models/item/QX;)V");
    dump("assets/minecraft/models/item/Wj", "E(Lassets/minecraft/models/item/Qf;)V");
    dump("assets/minecraft/models/item/Ww", "c(ISLjava/util/function/Consumer;S)V");
    dump("assets/minecraft/models/item/Zb", "P(J)V");
    dump("assets/minecraft/models/item/Za", "F(Lassets/minecraft/models/item/Qq;J)V");
    dump("net/minecraft/client/x_", "r()V");
}
#endif //CLASSES_H