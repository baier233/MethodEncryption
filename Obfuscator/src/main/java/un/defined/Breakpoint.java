package un.defined;

import com.google.gson.Gson;
import lombok.Getter;
import lombok.Setter;
import org.apache.commons.io.IOUtils;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.tree.ClassNode;
import un.defined.config.ClassMap;
import un.defined.config.Settings;
import un.defined.entity.Pipe;
import un.defined.entity.Tuple;
import un.defined.utility.ZipUtil;

import java.io.*;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.*;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import java.util.zip.ZipOutputStream;

@Getter
public class Breakpoint {

	public static Breakpoint INSTANCE;

	private Settings settings;

	private File inputFile;

	private File outputFile;

	private final List<Pipe> pipeLines = new ArrayList<>();

	private final List<String> filterClasses = new ArrayList<>();

	private final List<Tuple<ClassNode, ClassReader>> classNodes = new ArrayList<>();


	@Getter
	private final Map<ClassNode, byte[]> classNodeOpcodesMap = new HashMap<>();

	@Setter
	private int writerFlag = ClassWriter.COMPUTE_MAXS | ClassWriter.COMPUTE_FRAMES;

	static {
		//DEBUG
		if (false) {

			System.load("E:\\Dev\\MethodEncryptionObject\\JvmAcquirer\\out\\build\\x64-Debug\\JvmAcquirer.dll");
		} else {
			try {
				URL url = Breakpoint.class.getResource("JvmAcquirer.dll");
				File temp = File.createTempFile("native", ".dll");
				temp.deleteOnExit();

				try (InputStream in = url.openStream();
				     OutputStream out = new FileOutputStream(temp)) {

					byte[] buffer = new byte[1024];
					int length;
					while ((length = in.read(buffer)) != -1) {
						out.write(buffer, 0, length);
					}
				}

				System.load(temp.getAbsolutePath());

			} catch (IOException e) {
				throw new RuntimeException("Failed to load native library", e);
			}
		}

	}

	public Breakpoint(Settings settings) throws IOException {
		INSTANCE = this;
		this.settings = settings;
		System.out.println("this.settings.getANNOTATION_DESC() = " + this.settings.getANNOTATION_DESC());
		System.out.println("this.settings.isREMOVE_ANNOTATION() = " + this.settings.isREMOVE_ANNOTATION());
		System.out.println("this.settings.getCLASS_HEADER_PATH() = " + this.settings.getCLASS_HEADER_PATH());
		System.out.println("this.settings.isDUMP_TYPE() = " + this.settings.isDUMP_TYPE());
		if (!this.settings.isDUMP_TYPE() && this.settings.isDONT_LOAD_FOR_DUMP()) {
			File classMapFile = new File("classMap.json");
			String jsonData = new String(Files.readAllBytes(classMapFile.toPath()), StandardCharsets.UTF_8);
			ClassMap.setClassMap(new Gson().fromJson(jsonData, Map.class));
		}
	}

	public Breakpoint addPipeLine(Pipe line) {
		pipeLines.add(line);
		return this;
	}

	public Breakpoint addFilterClass(String className) {
		filterClasses.add(className);
		return this;
	}

	public Breakpoint setOutputFile(File outputFile) {
		this.outputFile = outputFile;
		return this;
	}

	public Breakpoint setInputFile(File inputFile) {
		this.inputFile = inputFile;
		return this;
	}

	public void runLink() throws IOException {
		classNodes.clear();
		for (Pipe pipeLine : pipeLines) {
			pipeLine.runObfuscate(this);
		}
		ZipOutputStream outputZipFile = new ZipOutputStream(Files.newOutputStream(outputFile.toPath()));
		try (ZipFile jarFile = new ZipFile(inputFile)) {
			Enumeration<? extends ZipEntry> entries = jarFile.entries();
			while (entries.hasMoreElements()) {
				ZipEntry entry = entries.nextElement();
				try (InputStream inputStream = jarFile.getInputStream(entry)) {
					if (!entry.getName().endsWith(".class")) {
						ZipUtil.PutFile(outputZipFile, entry, inputStream);
						continue;
					} else if (filterClasses.stream().anyMatch(entry.getName()::startsWith)) {
						ZipUtil.PutFile(outputZipFile, entry, inputStream);
						continue;
					}
					System.out.println("Processing: " + entry.getName());
					byte[] opcodes = ZipUtil.readStream(inputStream);
					ClassReader classReader = new ClassReader(opcodes);
					ClassNode classNode = new ClassNode();
					classNodeOpcodesMap.put(classNode, opcodes);
					classReader.accept(classNode, 0);
					classNodes.add(new Tuple<>(classNode, classReader));
					for (Pipe pipeLine : pipeLines) {
						pipeLine.init(this, classNode);
					}
				}
			}
			for (Tuple<ClassNode, ClassReader> tuple : classNodes) {
				if (Breakpoint.INSTANCE.getSettings().isREMOVE_ANNOTATION() && Breakpoint.INSTANCE.getSettings().getANNOTATION_DESC().equals("L" + tuple.getFirst().name + ";")) {
					continue;
				}
				writerFlag = ClassWriter.COMPUTE_MAXS | ClassWriter.COMPUTE_FRAMES;
				for (Pipe pipeLine : pipeLines) {
					Objects.requireNonNull(pipeLine).process(this, tuple);
				}
				ClassWriter classWriter = new ClassWriter(writerFlag);
				ClassNode classNode = tuple.getFirst();
				classNode.accept(classWriter);
				byte[] writeContent = !pipeLines.isEmpty() ? null : classWriter.toByteArray();
				for (Pipe pipeLine : pipeLines) {
					writeContent = pipeLine.writeClassBytes(this, classWriter);
				}
				ZipUtil.PutFile(outputZipFile, new ZipEntry(classNode.name + ".class"), writeContent);
			}
			for (Pipe pipeLine : pipeLines) {
				pipeLine.finish(this, outputZipFile);
			}
			InputStream inputStream = Objects.requireNonNull(Breakpoint.class.getResource("NativeHandler.class")).openStream();
			InputStream inputStream2 = Objects.requireNonNull(Breakpoint.class.getResource("NativeHandler.dll")).openStream();
			byte[] nativeHandlerClass = IOUtils.toByteArray(inputStream);
			byte[] nativeHandlerDll = IOUtils.toByteArray(inputStream2);
			ClassReader cr = new ClassReader(nativeHandlerClass);
			ClassWriter cw = new ClassWriter(writerFlag);
			cr.accept(cw, ClassReader.SKIP_DEBUG);
			ZipUtil.PutFile(outputZipFile, new ZipEntry(cr.getClassName() + ".class"), cw.toByteArray());
			ZipUtil.PutFile(outputZipFile, new ZipEntry(cr.getClassName() + ".dll"), nativeHandlerDll);
		} catch (Exception e) {
			throw new RuntimeException(e);
		}
		outputZipFile.close();
	}

	public static native byte[] getMethodBytecode(String className, String methodName, String methodDesc);

	public static native void loadJar(String jarPath);

}
