package un.defined.entity;

import lombok.AllArgsConstructor;
import lombok.Getter;
import org.objectweb.asm.ClassWriter;
import un.defined.Breakpoint;

@Getter
@AllArgsConstructor
public abstract class NamePipe implements Pipe {

    private String preName;

    @Override
    public void runObfuscate(Breakpoint blastObfuscate) {
    }

    @Override
    public byte[] writeClassBytes(Breakpoint blastObfuscate, ClassWriter classWriter) {
        return classWriter.toByteArray();
    }
}
