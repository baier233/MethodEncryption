package un.defined.utility;

import org.objectweb.asm.tree.AnnotationNode;
import org.objectweb.asm.tree.ClassNode;
import org.objectweb.asm.tree.MethodNode;
import un.defined.Breakpoint;

public class ClassMethodFilter {
    public static boolean shouldProcess(ClassNode classNode,MethodNode methodNode) {

        if (methodNode.name.equals("<clinit>") ||methodNode.name.equals("<init>") ) return false;

        if ((classNode.invisibleAnnotations != null &&
                classNode.invisibleAnnotations.stream().anyMatch(annotationNode ->
                        annotationNode.desc.equals(Breakpoint.INSTANCE.getSettings().getANNOTATION_DESC()))) || (classNode.visibleTypeAnnotations != null &&
                classNode.visibleTypeAnnotations.stream().anyMatch(annotationNode ->
                        annotationNode.desc.equals(Breakpoint.INSTANCE.getSettings().getANNOTATION_DESC()))) )  {
            return true;
        }
        return (methodNode.invisibleAnnotations != null &&
                methodNode.invisibleAnnotations.stream().anyMatch(annotationNode ->
                        annotationNode.desc.equals(Breakpoint.INSTANCE.getSettings().getANNOTATION_DESC())) )|| (methodNode.visibleAnnotations != null &&
                methodNode.visibleAnnotations.stream().anyMatch(annotationNode ->
                        annotationNode.desc.equals(Breakpoint.INSTANCE.getSettings().getANNOTATION_DESC())));
    }
}
