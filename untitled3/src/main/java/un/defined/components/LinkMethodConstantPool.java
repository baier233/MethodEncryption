package un.defined.components;

import org.apache.commons.io.IOUtils;
import org.objectweb.asm.*;
import org.objectweb.asm.commons.CodeSizeEvaluator;
import org.objectweb.asm.tree.*;
import un.defined.Breakpoint;
import un.defined.config.ClassMap;
import un.defined.config.Settings;
import un.defined.entity.NamePipe;
import un.defined.entity.Tuple;
import un.defined.utility.ASMUtil;
import un.defined.utility.ClassMethodFilter;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Objects;
import java.util.zip.ZipOutputStream;

public class LinkMethodConstantPool extends NamePipe {

    public LinkMethodConstantPool(String preName) {
        super(preName);
    }

    private final HashMap<String, HashMap<Tuple<String,String>, List<String>>> opcodeMap = new HashMap<>();

    //<KlassName,List<Tuple<MethodName,MethodDesc>>>
    private final HashMap<String, List<Tuple<String,String>>> methodsToDumpMap = new HashMap<>();
    private ClassReader lastClassReader = null;

    private ClassNode lastClassNode = null;

    @Override
    public void init(Breakpoint blastObfuscate, ClassNode classNode) {

    }


    @Override
    public byte[] writeClassBytes(Breakpoint blastObfuscate, ClassWriter classWriter) {
        if (lastClassReader == null || lastClassNode == null) {
            return classWriter.toByteArray();
        }
        ClassWriter newClassWriter = new ClassWriter(lastClassReader, 0);
        lastClassNode.accept(newClassWriter);
        return newClassWriter.toByteArray();
    }

    @Override
    public void process(Breakpoint blastObfuscate, Tuple<ClassNode, ClassReader> tuple) throws Exception {
        if (Breakpoint.INSTANCE.getSettings().isDUMP_TYPE()){

            //List<Tuple<MethodName,MethodDesc>>
            List<Tuple<String,String>> list = methodsToDumpMap.computeIfAbsent(tuple.getFirst().name,k -> new ArrayList<>());
            ClassNode classNode = tuple.getFirst();
            lastClassReader = tuple.getSecond();
            lastClassNode = classNode;

            for (MethodNode method : classNode.methods) {
                if (!ClassMethodFilter.shouldProcess(classNode, method)) {
                    continue;
                }

                System.out.println("Resolving Method: " + method.name + " Desc: " + method.desc + " ....");
                list.add(new Tuple<>(method.name,method.desc));
            }

            blastObfuscate.setWriterFlag(0);
            return;
        }

        HashMap<Tuple<String,String>, List<String>> classOpcodeMap = this.opcodeMap.computeIfAbsent(tuple.getFirst().name, k -> new HashMap<>());
        ClassNode classNode = tuple.getFirst();
        lastClassReader = tuple.getSecond();
        lastClassNode = classNode;
        System.out.println("Defined classNode.name = " + classNode.name);
        boolean shouldProcessclinit = false;
        InsnList instructions = new InsnList();
        instructions.add(new LdcInsnNode(Type.getObjectType(classNode.name)));
        instructions.add(new MethodInsnNode(Opcodes.INVOKESTATIC,"me/baier/NativeHandler", "register", "(Ljava/lang/Class;)V", false));
        for (MethodNode method : classNode.methods) {


            if (!ClassMethodFilter.shouldProcess(classNode, method)) {
                continue;
            }

            if (Breakpoint.INSTANCE.getSettings().isREMOVE_ANNOTATION()) {

                if (method.invisibleAnnotations != null)
                    method.invisibleAnnotations.removeIf(annotationNode -> annotationNode.desc.equals(Breakpoint.INSTANCE.getSettings().getANNOTATION_DESC()));
            }
            System.out.println("Encrypting Method: " + method.name + " Desc: " + method.desc + " ....");


            byte[] methodOpcodes = null;
            if (!Breakpoint.INSTANCE.getSettings().isDONT_LOAD_FOR_DUMP()){
                methodOpcodes = Breakpoint.getMethodBytecode(classNode.name,method.name,method.desc);
            }else{
                methodOpcodes = new byte[ClassMap.getCodeSize(classNode.name, method.name + method.desc)];
            }
            System.out.println("Generate Opcodes List for :" +method.name + method.desc);

            //when opcodes.length == 0 ,means that the target class cant be loaded into jvm
            if (methodOpcodes.length == 0){
                continue;
            }

            List<String> opcodes = classOpcodeMap.computeIfAbsent(new Tuple<>(method.name,method.desc), k -> new ArrayList<>());
            for (byte opcode : methodOpcodes) {
                opcodes.add(String.format("0x%02X", opcode));
            }
            method.tryCatchBlocks.clear();
            method.instructions.clear();
            method.localVariables.clear();
            for (int i = 0; i < methodOpcodes.length; i++) {
                method.instructions.add(new InsnNode(Opcodes.NOP));
            }
            AbstractInsnNode lastAbstractInsnNode = method.instructions.getLast();
            AbstractInsnNode lastPreAbstractInsnNode = method.instructions.get(method.instructions.size() - 2);
            Type returnType = Type.getReturnType(method.desc);
            switch (returnType.getSort()) {
                case Type.VOID:
                    method.instructions.set(lastAbstractInsnNode, new InsnNode(Opcodes.RETURN));
                    break;
                case Type.BOOLEAN:
                case Type.CHAR:
                case Type.BYTE:
                case Type.SHORT:
                case Type.INT:
                    method.instructions.set(lastPreAbstractInsnNode, new InsnNode(Opcodes.ICONST_0));
                    method.instructions.set(lastAbstractInsnNode, new InsnNode(Opcodes.IRETURN));
                    break;
                case Type.FLOAT:
                    method.instructions.set(lastPreAbstractInsnNode, new InsnNode(Opcodes.FCONST_0));
                    method.instructions.set(lastAbstractInsnNode, new InsnNode(Opcodes.FRETURN));
                    break;
                case Type.LONG:
                    method.instructions.set(lastPreAbstractInsnNode, new InsnNode(Opcodes.LCONST_0));
                    method.instructions.set(lastAbstractInsnNode, new InsnNode(Opcodes.LRETURN));
                    break;
                case Type.DOUBLE:
                    method.instructions.set(lastPreAbstractInsnNode, new InsnNode(Opcodes.DCONST_0));
                    method.instructions.set(lastAbstractInsnNode, new InsnNode(Opcodes.DRETURN));
                    break;
                default:
                    method.instructions.set(lastPreAbstractInsnNode, new InsnNode(Opcodes.ACONST_NULL));
                    method.instructions.set(lastAbstractInsnNode, new InsnNode(Opcodes.ARETURN));
                    break;
            }


            shouldProcessclinit = true;
        }

        if(Breakpoint.INSTANCE.getSettings().isREMOVE_ANNOTATION()){
            if (classNode.invisibleAnnotations != null)
                classNode.invisibleAnnotations.removeIf(annotationNode -> annotationNode.desc.equals(Breakpoint.INSTANCE.getSettings().getANNOTATION_DESC()));

        }

        if (shouldProcessclinit){
            MethodNode clinit = classNode.methods.stream().filter(methodNode -> "<clinit>".equals(methodNode.name)
                    && "()V".equals(methodNode.desc)).findAny().orElse(null);

            if (clinit == null) {
                clinit = new MethodNode(Opcodes.ACC_STATIC, "<clinit>", "()V", null, null);
                clinit.instructions.add(instructions);
                clinit.instructions.add(new InsnNode(Opcodes.RETURN));
                classNode.methods.add(clinit);
                clinit.maxStack = 1;
            }else{
                clinit.instructions.insertBefore(clinit.instructions.getFirst(),instructions);
                if (clinit.maxStack < 1){
                    clinit.maxStack = 1;
                }
            }

        }


        blastObfuscate.setWriterFlag(0);
    }


    @Override
    public void finish(Breakpoint blastObfuscate, ZipOutputStream outputStream) throws IOException {
        StringBuilder header = new StringBuilder();
        StringBuilder initMethods = new StringBuilder();
        header.append("#ifndef CLASSES_H\n");
        header.append("#define CLASSES_H\n");
        header.append("#include <functional>\n");
        header.append("#include <vector>\n");
        header.append("#include <string>\n");




        if (Breakpoint.INSTANCE.getSettings().isDUMP_TYPE()){
            initMethods.append("inline auto InitMethods(const std::function<void(const std::string& class_name,const std::string& method_name)> dump)\n{\n");
            for (String className : methodsToDumpMap.keySet()){
                List<Tuple<String,String>> methodList = methodsToDumpMap.get(className);
                methodList.forEach(tuple ->
                        initMethods.append("    dump(\"").append(className).
                                append("\",\"").
                                append(tuple.getFirst()).append(tuple.getSecond()).
                                append("\");\n"));


            }
        }else {
            initMethods.append("inline auto InitMethods(const std::function<void(const std::string& class_name,const std::string& method_name,const std::vector<uint8_t>& opcode)> inject)\n{\n");
            for (String className : opcodeMap.keySet()) {
                HashMap<Tuple<String,String>, List<String>> methodOpcodeMap = opcodeMap.get(className);
                for (Tuple<String,String> methodNameAndSig : methodOpcodeMap.keySet()) {

                    String methodName = methodNameAndSig.getFirst() + methodNameAndSig.getSecond();
                    String methodNameReplace = methodName.replaceAll("/", "_").replaceAll(";", "_")
                            .replace("(", "_").replace(")", "_")
                            .replace("[", "_");

                    String opcodeVarName = className.replaceAll("/", "_") + "_" + methodNameReplace ;
                    List<String> opcodes = methodOpcodeMap.get(methodNameAndSig);

                    StringBuilder stringBuilder = new StringBuilder();
                    stringBuilder.append("const std::vector ").append(opcodeVarName).append(" = {\n");
                    int i = 0;
                    for (String opcode : opcodes) {
                        stringBuilder.append("    static_cast<uint8_t>(").append(opcode).append("),").append("// ").append(i).append("\n");
                        i++;
                    }
                    stringBuilder.deleteCharAt(stringBuilder.length() - 1);
                    stringBuilder.append("\n};");
                    stringBuilder.append("\n");
                    header.append(stringBuilder);
                    initMethods.append("    inject(\"").append(className).append("\",\"").append(methodName).append("\",").append(opcodeVarName).append(");\n");

                }
            }
        }

        initMethods.append("}\n");
        header.append(initMethods);
        header.append("#endif //CLASSES_H");

        Files.write(Paths.get(Breakpoint.INSTANCE.getSettings().getCLASS_HEADER_PATH()), header.toString().getBytes(StandardCharsets.UTF_8));


    }
}
