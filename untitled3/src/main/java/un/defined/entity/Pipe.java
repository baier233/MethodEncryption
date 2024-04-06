package un.defined.entity;

import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.tree.ClassNode;
import un.defined.Breakpoint;

import java.io.IOException;
import java.util.zip.ZipOutputStream;

public interface Pipe {

    void runObfuscate(Breakpoint blastObfuscate);

    byte[] writeClassBytes(Breakpoint blastObfuscate, ClassWriter classWriter);

    void init(Breakpoint blastObfuscate, ClassNode classNode);

    void process(Breakpoint blastObfuscate, Tuple<ClassNode, ClassReader> classNode) throws Exception;

    void finish(Breakpoint blastObfuscate, ZipOutputStream outputStream) throws IOException;

}
