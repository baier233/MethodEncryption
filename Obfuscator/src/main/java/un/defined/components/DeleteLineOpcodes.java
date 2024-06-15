package un.defined.components;

import org.objectweb.asm.ClassReader;
import org.objectweb.asm.tree.ClassNode;
import org.objectweb.asm.tree.LineNumberNode;
import un.defined.Breakpoint;
import un.defined.entity.NamePipe;
import un.defined.entity.Tuple;

import java.io.IOException;
import java.util.zip.ZipOutputStream;

public class DeleteLineOpcodes extends NamePipe {
    public DeleteLineOpcodes(String preName) {
        super(preName);
    }

    @Override
    public void init(Breakpoint blastObfuscate, ClassNode classNode) {

    }

    @Override
    public void process(Breakpoint blastObfuscate, Tuple<ClassNode, ClassReader> tuple) throws Exception {
        ClassNode classNode = tuple.getFirst();
        classNode.methods.forEach(methodNode -> {
            methodNode.instructions.forEach(instructionNode -> {
                if (instructionNode instanceof LineNumberNode) {
                    methodNode.instructions.remove(instructionNode);
                }
            });
        });
    }

    @Override
    public void finish(Breakpoint blastObfuscate, ZipOutputStream outputStream) throws IOException {

    }
}
