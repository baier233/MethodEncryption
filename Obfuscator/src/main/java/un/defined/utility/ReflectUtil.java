package un.defined.utility;

import java.io.File;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.net.URL;
import java.net.URLClassLoader;
import java.security.ProtectionDomain;

public class ReflectUtil {

    private static Method defineClass;

    static {
        try {
            defineClass = ClassLoader.class.getDeclaredMethod("defineClass", String.class, byte[].class, Integer.TYPE, Integer.TYPE, ProtectionDomain.class);
        } catch (NoSuchMethodException e) {
            throw new RuntimeException(e);
        }
    }

    public static Class<?> defineClassByReflect(ClassLoader classLoader,String className,byte[] array,int length){
        defineClass.setAccessible(true);
        try {
            return (Class<?>) defineClass.invoke(classLoader, className.replaceAll("/","."), array, 0, length, classLoader.getClass().getProtectionDomain());
        } catch (Throwable e) {
            e.printStackTrace();
        }

        return null;
    }

    public static void loadJar(String jarPath) {

        try {
            File jarFile = new File(jarPath);
            Method method = URLClassLoader.class.getDeclaredMethod("addURL", URL.class);
            method.setAccessible(true);
            URLClassLoader classLoader = (URLClassLoader) ClassLoader.getSystemClassLoader();
            URL url = jarFile.toURI().toURL();
            method.invoke(classLoader, url);
        } catch (Throwable e) {
            e.printStackTrace();
        }
    }
}
