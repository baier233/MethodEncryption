#ifndef CLASSES_H
#define CLASSES_H
#include <functional>
#include <vector>
#include <string>
inline auto InitMethods(const std::function<void(const std::string& class_name,const std::string& method_name)> dump)
{
    dump("me/baier/AES","Encrypt(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
    dump("me/baier/AES","Decrypt(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
    dump("me/baier/AES","hex2byte(Ljava/lang/String;)[B");
    dump("me/baier/AES","byte2hex([B)Ljava/lang/String;");
}
#endif //CLASSES_H