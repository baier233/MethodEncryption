#ifndef CLASSES_H
#define CLASSES_H
#include <functional>
#include <vector>
#include <string>
inline auto InitMethods(const std::function<void(const std::string& class_name,const std::string& method_name)> dump)
{
    dump("net/ay","c()V");
    dump("net/c2","x()V");
    dump("net/c2","y()V");
    dump("net/c2","a(Lnet/P;)V");
    dump("net/aZ","a(Ljava/io/DataInputStream;)V");
    dump("net/aZ","a(Ljava/io/DataOutputStream;)V");
    dump("net/aP","a()[B");
    dump("net/aP","a([B)Z");
    dump("net/aO","a(Lnet/aQ;)V");
    dump("net/aO","a()V");
    dump("net/aS","a(Ljava/io/DataOutputStream;)V");
    dump("net/aV","a(Ljava/io/DataInputStream;)V");
    dump("net/aU","a(Ljava/io/DataInputStream;)V");
    dump("net/cR$c","a(Lnet/aQ;)V");
    dump("net/dd","s()V");
    dump("net/dd","a(Lnet/P;)V");
    dump("net/dK","a(Lnet/al;)V");
    dump("net/dK","a(Lnet/P;)V");
    dump("net/dE","a(Lnet/aa;)V");
    dump("net/cR","a(Lnet/P;)V");
    dump("net/E","m()V");
    dump("net/minecraft/client/iM","dq()V");
    dump("net/minecraft/client/k","V()V");
}
#endif //CLASSES_H