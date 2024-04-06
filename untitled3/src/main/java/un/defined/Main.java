package un.defined;

import me.baier.NativeHandler;
import un.defined.components.DeleteLineOpcodes;
import un.defined.components.LinkMethodConstantPool;
import un.defined.config.Config;
import un.defined.utility.ReflectUtil;

import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;

public class Main {
    public static void main(String[] args) {
        if (args.length < 3) {
            System.out.println("Usage: java -jar <jar file> <input file> <output file> <config file>");
            return;
        }
        File inputFile = new File(args[0]);
        if (!Files.exists(inputFile.toPath())) {
            System.out.println("Input file does not exist");
            return;
        }
        File outputFile = new File(args[1]);
        if (!outputFile.exists()) {
            try {
                if (outputFile.createNewFile()) {
                    System.out.println("Output file created");
                }
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        }
        File configFile = new File(args[2]);
        if (!configFile.exists()) {
            try {
                if (configFile.createNewFile()) {
                    Files.write(configFile.toPath(), Config.generateConfig().getBytes(StandardCharsets.UTF_8));
                    System.out.println("Config file created");
                }
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        }

        try {
            Breakpoint.loadJar(inputFile.getAbsolutePath());
            new Breakpoint(new Config(configFile).parseConfig())
                    .addPipeLine(new DeleteLineOpcodes("DeleteLineOpcodes"))
                    .addPipeLine(new LinkMethodConstantPool("LinkMethodConstantPool"))
                    .setInputFile(inputFile)
                    .setOutputFile(outputFile)
                    .runLink();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }
}